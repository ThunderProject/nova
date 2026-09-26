module;

#include "result.h"

#include <NRI.h>
#include <NRIDescs.h>
#include <assert.hpp>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

export module nova.render.rhi.queue;

import nova.render.rhi.core;
import nova.render.rhi.device;
import nova.render.rhi.command_context;
import nova.render.rhi.frame_context;
import nova.render.rhi.swapchain;

export namespace nova::render::rhi {
    class queue final {
    public:
        explicit queue(device& device, nri::Queue& queue, const std::size_t max_command_buffers)
            :
            m_device(&device),
            m_queue(&queue)
        {
            m_command_buffers.reserve(max_command_buffers);
        }

        queue(const queue&) = delete;
        queue& operator=(const queue&) = delete;

        queue(queue&& other) noexcept
            :
            m_device(std::exchange(other.m_device, nullptr)),
            m_queue(std::exchange(other.m_queue, nullptr)),
            m_command_buffers(std::move(other.m_command_buffers))
        {}

        queue& operator=(queue&& other) noexcept {
            if(this == &other) {
                return *this;
            }

            m_device = std::exchange(other.m_device, nullptr);
            m_queue = std::exchange(other.m_queue, nullptr);
            m_command_buffers = std::move(other.m_command_buffers);

            return *this;
        }

        [[nodiscard]] nova::result<nova::ok> submit(
            frame_context& frame, 
            const acquired_image& image,
            const std::span<command_context* const> contexts
        ) {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(m_queue != nullptr);
            DEBUG_ASSERT(image.acquire_semaphore != nullptr);
            DEBUG_ASSERT(image.release_semaphore != nullptr);
            DEBUG_ASSERT(!contexts.empty());

            m_command_buffers.clear();

            for(auto* context : contexts) {
                DEBUG_ASSERT(context != nullptr);
                DEBUG_ASSERT(!context->recording());
                m_command_buffers.emplace_back(&context->native());
            }

            nri::FenceSubmitDesc acquire_fence{};
            acquire_fence.fence = image.acquire_semaphore;
            acquire_fence.stages = nri::StageBits::COLOR_ATTACHMENT;

            const auto frame_fence_value = frame.next_fence_value();

            nri::FenceSubmitDesc signal_fences[2]{};

            signal_fences[0].fence = image.release_semaphore;
            signal_fences[1].fence = &frame.fence();
            signal_fences[1].value = frame_fence_value;

            nri::QueueSubmitDesc submit_desc{};
            submit_desc.waitFences = &acquire_fence;
            submit_desc.waitFenceNum = 1;
            submit_desc.commandBuffers = m_command_buffers.data();
            submit_desc.commandBufferNum = static_cast<std::uint32_t>(m_command_buffers.size());
            submit_desc.signalFences = signal_fences;
            submit_desc.signalFenceNum = 2;

            auto res = check(m_device->core().QueueSubmit(*m_queue, submit_desc));
            if(!res) {
                return nova::err(std::move(res.error()));
            }

            frame.commit_submission();

            return nova::ok{};
        }

        [[nodiscard]] nova::result<nova::ok> wait_idle() {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(m_queue != nullptr);
            return check(m_device->core().QueueWaitIdle(m_queue));
        }

        [[nodiscard]] nri::Queue& native() noexcept {
            DEBUG_ASSERT(m_queue != nullptr);
            return *m_queue;
        }

        [[nodiscard]] const nri::Queue& native() const noexcept {
            DEBUG_ASSERT(m_queue != nullptr);
            return *m_queue;
        }
    private:
        device* m_device{nullptr};
        nri::Queue* m_queue{nullptr};
        std::vector<nri::CommandBuffer*> m_command_buffers;
    };
}
