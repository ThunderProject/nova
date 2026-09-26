module;

#include "result.h"

#include <NRI.h>
#include <NRIDescs.h>
#include <assert.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <coroutine>

export module nova.render.rhi.renderer;

import cpu_scheduler;
import coro_task;
import nova.platform.window;
import nova.render.rhi.core;
import nova.render.rhi.device;
import nova.render.rhi.command_context;
import nova.render.rhi.frame_context;
import nova.render.rhi.queue;
import nova.render.rhi.swapchain;
import nova.di.singleton;

export namespace nova::render::rhi {
    enum class frame_status : std::uint8_t {
        rendered,
        out_of_date
    };

    struct renderer_desc {
        swapchain_desc swapchain{};
        std::uint32_t recording_lane_count{0};
        nri::Color32f clear_color{
            .x= 0.02f,
            .y= 0.02f,
            .z= 0.025f,
            .w= 1.0f
        };
    };

    class renderer final {
    public:
        renderer(const renderer&) = delete;
        renderer& operator=(const renderer&) = delete;
        renderer& operator=(renderer&&) = delete;

        renderer(renderer&& other) noexcept
            :
            m_device(std::move(other.m_device)),
            m_graphics_queue(std::move(other.m_graphics_queue)),
            m_swapchain(std::move(other.m_swapchain)),
            m_frames(std::move(other.m_frames)),
            m_backbuffer_views(std::move(other.m_backbuffer_views)),
            m_clear_color(other.m_clear_color),
            m_recording_lane_count(other.m_recording_lane_count),
            m_frame_index(std::exchange(other.m_frame_index, 0))
        {}

        ~renderer() noexcept {
            if(m_device == nullptr) {
                return;
            }

            auto _ = m_device->wait_idle();
            destroy_backbuffer_views();
        }

        [[nodiscard]] static nova::result<renderer> create(
            const platform::presentation_handle presentation,
            const platform::extent2d extent,
            const renderer_desc& desc = {}
        ) {
            auto& scheduler = nova::ioc::ioc().resolve<nova::cpu_scheduler<>>();

            if(extent.empty()) [[unlikely]] {
                return nova::err(std::string{"Cannot create renderer with zero extent"});
            }

            auto device_res = device::create();
            if(!device_res) {
                return nova::err(std::move(device_res.error()));
            }

            auto device_ptr = std::make_unique<device>(std::move(*device_res));

            const auto recording_lane_count = desc.recording_lane_count == 0
                ? scheduler.worker_count()
                : desc.recording_lane_count;

            if(recording_lane_count == 0) [[unlikely]] {
                return nova::err(std::string{"Renderer requires at least one recording lane"});
            }

            renderer result(std::move(device_ptr), recording_lane_count, desc.clear_color);

            auto res = result.create_swapchain(presentation, extent, desc.swapchain);
            if(!res) {
                return nova::err(std::move(res.error()));
            }

            const auto frame_count = desc.swapchain.queued_frame_count == 0
                ? std::uint32_t{2}
                : static_cast<std::uint32_t>(desc.swapchain.queued_frame_count);

            result.m_frames.reserve(frame_count);

            for(std::uint32_t i = 0; i < frame_count; ++i) {
                auto frame = frame_context::create(*result.m_device, recording_lane_count);
                if(!frame) {
                    return nova::err(std::move(frame.error()));
                }

                result.m_frames.emplace_back(std::move(*frame));
            }

            return result;
        }

        [[nodiscard]] nova::coro::task<nova::result<frame_status>> render_frame(nova::cpu_scheduler<>& scheduler) {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(m_swapchain.has_value());
            DEBUG_ASSERT(!m_frames.empty());

            co_await scheduler.schedule();

            auto& frame = m_frames[m_frame_index];

            while(!frame.complete()) {
                co_await scheduler.yield();
            }

            auto acquired = m_swapchain->acquire();

            if(!acquired) {
                if(acquired.error() == nri::Result::OUT_OF_DATE) {
                    co_return frame_status::out_of_date;
                }

                co_return nova::err(nri_result_error("AcquireNextTexture", acquired.error()));
            }

            DEBUG_ASSERT(acquired->index < m_backbuffer_views.size());

            auto& context = frame.graphics_context(0);

            auto record_res = record_clear(context, *acquired);
            if(!record_res) {
                co_return nova::err(std::move(record_res.error()));
            }

            std::array<command_context*, 1> contexts{ &context };

            auto submit_res = m_graphics_queue.submit(frame, *acquired, std::span{contexts});
            if(!submit_res) {
                co_return nova::err(std::move(submit_res.error()));
            }

            advance_frame();

            auto present_res = m_swapchain->present(*acquired);

            if(!present_res) {
                if(present_res.error() == nri::Result::OUT_OF_DATE) {
                    co_return frame_status::out_of_date;
                }

                co_return nova::err(nri_result_error("QueuePresent", present_res.error()));
            }

            co_return frame_status::rendered;
        }

        [[nodiscard]] nova::result<nova::ok> recreate_swapchain(
            const platform::presentation_handle presentation,
            const platform::extent2d extent,
            const swapchain_desc& desc = {}
        ) {
            DEBUG_ASSERT(m_device != nullptr);

            if(extent.empty()) {
                return nova::ok{};
            }

            auto res = m_device->wait_idle();
            if(!res) {
                return nova::err(std::move(res.error()));
            }

            destroy_backbuffer_views();
            m_swapchain.reset();

            return create_swapchain(presentation, extent, desc);
        }

        [[nodiscard]] const swapchain& presentation_swapchain() const noexcept {
            DEBUG_ASSERT(m_swapchain.has_value());
            return *m_swapchain;
        }

        [[nodiscard]] swapchain& presentation_swapchain() noexcept {
            DEBUG_ASSERT(m_swapchain.has_value());
            return *m_swapchain;
        }

        [[nodiscard]] std::uint32_t recording_lane_count() const noexcept {
            return m_recording_lane_count;
        }
    private:
        renderer(std::unique_ptr<device> device, const std::uint32_t recording_lane_count, const nri::Color32f clear_color)
            :
            m_device(std::move(device)),
            m_graphics_queue(*m_device, m_device->graphics_queue(), recording_lane_count),
            m_clear_color(clear_color),
            m_recording_lane_count(recording_lane_count)
        {}

        [[nodiscard]] static std::string nri_result_error(const char* operation, const nri::Result result) {
            return std::format("{} failed with NRI result {}", operation, magic_enum::enum_name(result));
        }

        [[nodiscard]] nova::result<nova::ok> create_swapchain(
            const platform::presentation_handle presentation,
            const platform::extent2d extent,
            const swapchain_desc& desc
        ) {
            auto swapchain_res = swapchain::create(*m_device, presentation, extent, desc);
            if(!swapchain_res) {
                return nova::err(std::move(swapchain_res.error()));
            }

            m_swapchain.emplace(std::move(*swapchain_res));

            auto res = create_backbuffer_views();
            if(!res) {
                m_swapchain.reset();
                return nova::err(std::move(res.error()));
            }

            return nova::ok{};
        }

        [[nodiscard]] nova::result<nova::ok> create_backbuffer_views() {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(m_swapchain.has_value());
            DEBUG_ASSERT(m_backbuffer_views.empty());

            auto& core = m_device->core();

            m_backbuffer_views.reserve(m_swapchain->texture_count());

            for(std::size_t i = 0; i < m_swapchain->texture_count(); ++i) {
                nri::TextureViewDesc view_desc{};

                view_desc.texture = &m_swapchain->texture(i);
                view_desc.type = nri::TextureView::COLOR_ATTACHMENT;
                view_desc.format = m_swapchain->format();
                view_desc.mipNum = 1;
                view_desc.layerNum = 1;
                view_desc.sliceNum = 1;
                view_desc.planes = nri::PlaneBits::COLOR;

                nri::Descriptor* view = nullptr;

                auto res = check(core.CreateTextureView(view_desc, view));
                if(!res) {
                    destroy_backbuffer_views();
                    return nova::err(std::move(res.error()));
                }

                m_backbuffer_views.emplace_back(view);
            }

            return nova::ok{};
        }

        [[nodiscard]] nova::result<nova::ok> record_clear(command_context& context, const acquired_image& image) {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(image.texture != nullptr);
            DEBUG_ASSERT(image.index < m_backbuffer_views.size());
            DEBUG_ASSERT(m_backbuffer_views[image.index] != nullptr);

            auto res = context.begin();
            if(!res) {
                return nova::err(std::move(res.error()));
            }

            auto& core = m_device->core();
            auto& command_buffer = context.native();

            nri::TextureBarrierDesc attachment{};
            attachment.texture = image.texture;
            attachment.before.access = nri::AccessBits::NONE;
            attachment.before.layout = nri::Layout::UNDEFINED;
            attachment.before.stages = nri::StageBits::NONE;
            attachment.after.access = nri::AccessBits::COLOR_ATTACHMENT_WRITE;
            attachment.after.layout = nri::Layout::COLOR_ATTACHMENT;
            attachment.after.stages = nri::StageBits::COLOR_ATTACHMENT;
            attachment.mipNum = 1;
            attachment.layerNum = 1;
            attachment.planes = nri::PlaneBits::COLOR;

            nri::BarrierDesc attachment_barrier{};
            attachment_barrier.textures = &attachment;
            attachment_barrier.textureNum = 1;

            core.CmdBarrier(command_buffer, attachment_barrier);

            nri::AttachmentDesc color_attachment{};
            color_attachment.descriptor = m_backbuffer_views[image.index];
            color_attachment.clearValue.color.f = m_clear_color;
            color_attachment.loadOp = nri::LoadOp::CLEAR;
            color_attachment.storeOp = nri::StoreOp::STORE;

            nri::RenderingDesc rendering{};
            rendering.colors = &color_attachment;
            rendering.colorNum = 1;

            core.CmdBeginRendering(command_buffer, rendering);
            core.CmdEndRendering(command_buffer);

            nri::TextureBarrierDesc present{};
            present.texture = image.texture;
            present.before.access = nri::AccessBits::COLOR_ATTACHMENT_WRITE;
            present.before.layout = nri::Layout::COLOR_ATTACHMENT;
            present.before.stages = nri::StageBits::COLOR_ATTACHMENT;
            present.after.access = nri::AccessBits::NONE;
            present.after.layout = nri::Layout::PRESENT;
            present.after.stages = nri::StageBits::NONE;
            present.mipNum = 1;
            present.layerNum = 1;
            present.planes = nri::PlaneBits::COLOR;

            nri::BarrierDesc present_barrier{};
            present_barrier.textures = &present;
            present_barrier.textureNum = 1;

            core.CmdBarrier(command_buffer, present_barrier);

            res = context.end();
            if(!res) {
                return nova::err(std::move(res.error()));
            }

            return nova::ok{};
        }

        void advance_frame() noexcept {
            DEBUG_ASSERT(!m_frames.empty());

            ++m_frame_index;

            if(m_frame_index == m_frames.size()) {
                m_frame_index = 0;
            }
        }

        void destroy_backbuffer_views() noexcept {
            if(m_device == nullptr) {
                return;
            }

            for(auto*& view : m_backbuffer_views) {
                if(view != nullptr) {
                    m_device->core().DestroyDescriptor(view);
                    view = nullptr;
                }
            }

            m_backbuffer_views.clear();
        }

        std::unique_ptr<device> m_device;
        queue m_graphics_queue;

        std::optional<swapchain> m_swapchain;
        std::vector<frame_context> m_frames;
        std::vector<nri::Descriptor*> m_backbuffer_views;

        nri::Color32f m_clear_color{};

        std::uint32_t m_recording_lane_count{0};
        std::size_t m_frame_index{0};
    };
}
