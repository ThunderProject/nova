module;

#include "result.h"

#include <NRI.h>
#include <NRIDescs.h>
#include <assert.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <coroutine>

export module nova.render.renderer;

import cpu_scheduler;
import coro_task;
import nova.di.singleton;
import nova.platform.window;
import nova.render.frame;
import nova.render.graph.resource;
import nova.render.graph.pass;
import nova.render.graph.render_graph;
import nova.render.graph.compiler;
import nova.render.passes.clear;
import nova.render.runtime.pass_context;
import nova.render.rhi.renderer;
import nova.render.rhi.swapchain;

export namespace nova::render {
    enum class frame_status : std::uint8_t {
        rendered,
        out_of_date
    };

    struct renderer_desc {
        rhi::swapchain_desc swapchain{};
        std::uint32_t recording_lane_count{0};
        passes::clear_color clear_color{};
    };

    class renderer final {
    public:
        renderer(const renderer&) = delete;
        renderer& operator=(const renderer&) = delete;

        renderer(renderer&&) noexcept = default;
        renderer& operator=(renderer&&) = delete;

        [[nodiscard]] static nova::result<renderer> create(
            const platform::presentation_handle presentation,
            const platform::extent2d extent,
            const renderer_desc& desc = {}
        ) {
            const rhi::renderer_desc rhi_desc {
                .swapchain = desc.swapchain,
                .recording_lane_count = desc.recording_lane_count
            };

            auto rhi_renderer = rhi::renderer::create(presentation, extent, rhi_desc);

            if(!rhi_renderer) {
                return nova::err(std::move(rhi_renderer.error()));
            }

            renderer result {
                std::move(*rhi_renderer),
                desc.swapchain,
                desc.clear_color
            };

            auto graph_res = result.rebuild_graph();
            if(!graph_res) {
                return nova::err(std::move(graph_res.error()));
            }

            return result;
        }

        [[nodiscard]] frame begin_frame() const {
            return frame{};
        }

        [[nodiscard]] nova::coro::task<nova::result<frame_status>> render(frame submitted_frame) {
            if(!submitted_frame.empty()) {
                co_return nova::err(std::string{
                    "rendering is not connected to the GPU pipeline yet"
                });
            }

            auto& scheduler = nova::ioc::ioc().resolve<nova::cpu_scheduler<>>();

            auto res = co_await m_rhi.render_frame(
                scheduler,
                [this](rhi::frame_recording_context& recording) {
                    return record_graph(recording);
                }
            );

            if(!res) {
                co_return nova::err(std::move(res.error()));
            }

            switch(*res) {
                case rhi::frame_status::rendered: co_return frame_status::rendered;
                case rhi::frame_status::out_of_date: co_return frame_status::out_of_date;
            }
            std::unreachable();
        }

        [[nodiscard]] nova::result<nova::ok> recreate_swapchain(
            const platform::presentation_handle presentation,
            const platform::extent2d extent
        ) {
            auto res = m_rhi.recreate_swapchain(presentation, extent, m_swapchain_desc);

            if(!res) {
                return nova::err(std::move(res.error()));
            }

            if(extent.empty()) {
                return nova::ok{};
            }

            return rebuild_graph();
        }

        void set_clear_color(const passes::clear_color color) noexcept {
            m_clear_color = color;
            m_clear.color(color);
        }

        [[nodiscard]] passes::clear_color clear_color() const noexcept {
            return m_clear_color;
        }

        [[nodiscard]] platform::extent2d extent() const noexcept {
            return m_rhi.presentation_swapchain().extent();
        }

    private:
        renderer(rhi::renderer rhi_renderer, const rhi::swapchain_desc swapchain_desc, const passes::clear_color clear_color)
            :
            m_rhi(std::move(rhi_renderer)),
            m_swapchain_desc(swapchain_desc),
            m_clear_color(clear_color)
        {}

        [[nodiscard]] static nova::result<graph::format> graph_format(const nri::Format format) {
            switch(format) {
                case nri::Format::BGRA8_UNORM: return graph::format::bgra8_unorm;
                case nri::Format::BGRA8_SRGB: return graph::format::bgra8_srgb;
                case nri::Format::RGBA8_UNORM: return graph::format::rgba8_unorm;
                case nri::Format::RGBA8_SRGB: return graph::format::rgba8_srgb;
                case nri::Format::R10_G10_B10_A2_UNORM: return graph::format::rgb10a2_unorm;
                case nri::Format::RGBA16_SFLOAT: return graph::format::rgba16_sfloat;
                default: return nova::err(
                    std::format(
                        "Unsupported swapchain format {}",
                        static_cast<std::uint32_t>(format)
                    )
                );
            }
        }

        [[nodiscard]] nova::result<nova::ok> rebuild_graph() {
            const auto& swapchain = m_rhi.presentation_swapchain();
            const auto extent = swapchain.extent();

            auto format = graph_format(swapchain.format());
            if(!format) {
                return nova::err(std::move(format.error()));
            }

            m_graph = graph::render_graph{};
            m_compiled = graph::compiled_graph{};
            m_clear = passes::clear_pass{};
            m_backbuffer = {};
            m_present = {};

            const graph::texture_desc backbuffer_desc{
                .dimension = graph::texture_dimension::texture_2d,
                .format = *format,
                .extent = {
                    .width = extent.width,
                    .height = extent.height,
                    .depth = 1
                },
                .mip_count = 1,
                .layer_count = 1,
                .sample_count = 1
            };

            m_backbuffer = m_graph.import_texture("backbuffer", backbuffer_desc);

            m_clear = passes::clear_pass { m_backbuffer, m_clear_color };

            auto _ = m_clear.add_to(m_graph);

            const auto backbuffer = m_backbuffer;

            m_present = m_graph.add_pass(
                {
                    .name = "present",
                    .queue = graph::pass_queue::graphics
                },
                [backbuffer](graph::pass_builder& builder) {
                    builder.read(
                        backbuffer,
                        graph::resource_usage::present
                    );
                }
            );

            auto compiled = graph::compiled_graph::compile(m_graph);
            if(!compiled) {
                return nova::err(std::move(compiled.error()));
            }

            m_compiled = std::move(*compiled);

            m_texture_bindings.assign(
                m_graph.texture_count(),
                runtime::texture_binding{}
            );

            m_buffer_bindings.assign(
                m_graph.buffer_count(),
                runtime::buffer_binding{}
            );

            return nova::ok{};
        }

        [[nodiscard]] nova::result<nova::ok> record_graph(rhi::frame_recording_context& recording) {
            DEBUG_ASSERT(m_backbuffer.valid());
            DEBUG_ASSERT(m_backbuffer.index() < m_texture_bindings.size());

            auto& backbuffer_binding = m_texture_bindings[m_backbuffer.index()];

            backbuffer_binding = {};
            backbuffer_binding.texture = recording.image.texture;
            backbuffer_binding.color_attachment = &recording.color_attachment;

            runtime::pass_context context{
                recording.core,
                recording.commands,
                std::span<const runtime::texture_binding>{m_texture_bindings},
                std::span<const runtime::buffer_binding>{m_buffer_bindings}
            };

            for(const auto pass : m_compiled.execution_order()) {
                if(pass == m_clear.handle()) {
                    transition_to_color_attachment(recording);

                    auto res = m_clear.record(context);
                    if(!res) {
                        return nova::err(std::move(res.error()));
                    }

                    continue;
                }

                if(pass == m_present) {
                    transition_to_present(recording);
                    continue;
                }

                return nova::err(std::string{"Compiled render graph contains an unsupported pass"});
            }

            return nova::ok{};
        }

        static void transition_to_color_attachment(rhi::frame_recording_context& recording) noexcept {
            DEBUG_ASSERT(recording.image.texture != nullptr);

            nri::TextureBarrierDesc texture{};
            texture.texture = recording.image.texture;

            texture.before.access = nri::AccessBits::NONE;
            texture.before.layout = nri::Layout::UNDEFINED;
            texture.before.stages = nri::StageBits::NONE;

            texture.after.access = nri::AccessBits::COLOR_ATTACHMENT_WRITE;
            texture.after.layout = nri::Layout::COLOR_ATTACHMENT;
            texture.after.stages = nri::StageBits::COLOR_ATTACHMENT;

            texture.mipNum = 1;
            texture.layerNum = 1;
            texture.planes = nri::PlaneBits::COLOR;

            nri::BarrierDesc barrier{};
            barrier.textures = &texture;
            barrier.textureNum = 1;

            recording.core.CmdBarrier(recording.commands.native(), barrier);
        }

        static void transition_to_present(rhi::frame_recording_context& recording) noexcept {
            DEBUG_ASSERT(recording.image.texture != nullptr);

            nri::TextureBarrierDesc texture{};
            texture.texture = recording.image.texture;

            texture.before.access = nri::AccessBits::COLOR_ATTACHMENT_WRITE;
            texture.before.layout = nri::Layout::COLOR_ATTACHMENT;
            texture.before.stages = nri::StageBits::COLOR_ATTACHMENT;

            texture.after.access = nri::AccessBits::NONE;
            texture.after.layout = nri::Layout::PRESENT;
            texture.after.stages = nri::StageBits::NONE;

            texture.mipNum = 1;
            texture.layerNum = 1;
            texture.planes = nri::PlaneBits::COLOR;

            nri::BarrierDesc barrier{};
            barrier.textures = &texture;
            barrier.textureNum = 1;

            recording.core.CmdBarrier(recording.commands.native(), barrier);
        }

        rhi::renderer m_rhi;
        rhi::swapchain_desc m_swapchain_desc;
        passes::clear_color m_clear_color;
        graph::render_graph m_graph;
        graph::compiled_graph m_compiled;
        graph::texture_handle m_backbuffer;
        graph::pass_handle m_present;
        passes::clear_pass m_clear;
        std::vector<runtime::texture_binding> m_texture_bindings;
        std::vector<runtime::buffer_binding> m_buffer_bindings;
    };
}
