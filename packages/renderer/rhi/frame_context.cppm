module;

#include "result.h"

#include <NRI.h>
#include <assert.hpp>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

export module nova.render.rhi.frame_context;

import nova.render.rhi.core;
import nova.render.rhi.device;
import nova.render.rhi.command_context;

export namespace nova::render::rhi {
    class frame_context final {
    public:
        frame_context(const frame_context&) = delete;
        frame_context& operator=(const frame_context&) = delete;

        frame_context(frame_context&& other) noexcept
            :
            m_device(std::exchange(other.m_device, nullptr)),
            m_fence(std::exchange(other.m_fence, nullptr)),
            m_graphics_contexts(std::move(other.m_graphics_contexts)),
            m_fence_value(std::exchange(other.m_fence_value, 0))
        {}

        frame_context& operator=(frame_context&& other) noexcept {
            if(this == &other) {
                return *this;
            }

            destroy();

            m_device = std::exchange(other.m_device, nullptr);
            m_fence = std::exchange(other.m_fence, nullptr);
            m_graphics_contexts = std::move(other.m_graphics_contexts);
            m_fence_value = std::exchange(other.m_fence_value, 0);

            return *this;
        }

        ~frame_context() noexcept {
            destroy();
        }

        [[nodiscard]] static nova::result<frame_context> create(device& device, const std::uint32_t recording_lane_count) {
            DEBUG_ASSERT(recording_lane_count != 0);

            frame_context frame(device);

            auto res = check(device.core().CreateFence(device.native(), 0, frame.m_fence));
            if(!res) {
                return nova::err(std::move(res.error()));
            }

            frame.m_graphics_contexts.reserve(recording_lane_count);

            for(std::uint32_t lane = 0; lane < recording_lane_count; ++lane) {
                auto context = command_context::create(device, device.graphics_queue());
                if(!context) {
                    return nova::err(std::move(context.error()));
                }

                frame.m_graphics_contexts.emplace_back(std::move(*context));
            }

            return frame;
        }

        void wait() noexcept {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(m_fence != nullptr);

            if(m_fence_value == 0) {
                return;
            }

            auto& core = m_device->core();

            if(core.GetFenceValue(*m_fence) >= m_fence_value) {
                return;
            }

            core.Wait(*m_fence, m_fence_value);
        }

        [[nodiscard]] bool complete() const noexcept {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(m_fence != nullptr);

            if(m_fence_value == 0) {
                return true;
            }

            return m_device->core().GetFenceValue(*m_fence) >= m_fence_value;
        }

        [[nodiscard]] command_context& graphics_context(const std::size_t index) noexcept {
            DEBUG_ASSERT(index < m_graphics_contexts.size());
            return m_graphics_contexts[index];
        }

        [[nodiscard]] const command_context& graphics_context(const std::size_t index) const noexcept {
            DEBUG_ASSERT(index < m_graphics_contexts.size());
            return m_graphics_contexts[index];
        }

        [[nodiscard]] std::size_t graphics_context_count() const noexcept {
            return m_graphics_contexts.size();
        }

        [[nodiscard]] nri::Fence& fence() noexcept {
            DEBUG_ASSERT(m_fence != nullptr);
            return *m_fence;
        }

        [[nodiscard]] const nri::Fence& fence() const noexcept {
            DEBUG_ASSERT(m_fence != nullptr);
            return *m_fence;
        }

        [[nodiscard]] std::uint64_t next_fence_value() const noexcept {
            return m_fence_value + 1;
        }

        void commit_submission() noexcept {
            ++m_fence_value;
        }

        [[nodiscard]] std::uint64_t fence_value() const noexcept {
            return m_fence_value;
        }
    private:
        explicit frame_context(device& device) noexcept
            :
            m_device(&device)
        {}

        void destroy() noexcept {
            if(m_device == nullptr) {
                return;
            }

            if(m_fence != nullptr && m_fence_value != 0) {
                wait();
            }

            m_graphics_contexts.clear();

            if(m_fence != nullptr) {
                m_device->core().DestroyFence(m_fence);
                m_fence = nullptr;
            }

            m_device = nullptr;
            m_fence_value = 0;
        }

        device* m_device{nullptr};
        nri::Fence* m_fence{nullptr};
        std::vector<command_context> m_graphics_contexts;
        std::uint64_t m_fence_value{0};
    };
}
