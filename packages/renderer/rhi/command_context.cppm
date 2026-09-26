module;

#include "result.h"

#include <NRI.h>
#include <NRIDescs.h>
#include <assert.hpp>
#include <magic_enum/magic_enum.hpp>
#include <utility>

export module nova.render.rhi.command_context;

import nova.render.rhi.core;
import nova.render.rhi.device;

export namespace nova::render::rhi {
    class command_context final {
    public:
        command_context(const command_context&) = delete;
        command_context& operator=(const command_context&) = delete;

        command_context(command_context&& other) noexcept
            :
            m_device(std::exchange(other.m_device, nullptr)),
            m_allocator(std::exchange(other.m_allocator, nullptr)),
            m_command_buffer(std::exchange(other.m_command_buffer, nullptr)),
            m_recording(std::exchange(other.m_recording, false)) 
        {}

        command_context& operator=(command_context&& other) noexcept {
            if(this == &other) {
                return *this;
            }

            DEBUG_ASSERT(!m_recording);

            destroy();

            m_device = std::exchange(other.m_device, nullptr);
            m_allocator = std::exchange(other.m_allocator, nullptr);
            m_command_buffer = std::exchange(other.m_command_buffer, nullptr);
            m_recording = std::exchange(other.m_recording, false);

            return *this;
        }

        ~command_context() noexcept {
            DEBUG_ASSERT(!m_recording);
            destroy();
        }

        
        [[nodiscard]] static nova::result<command_context> create(device& device, nri::Queue& queue) {
            command_context context(device);

            auto res = check(device.core().CreateCommandAllocator(queue, context.m_allocator));

            if(!res) {
                return nova::err(std::move(res.error()));
            }

            res = check(device.core().CreateCommandBuffer(*context.m_allocator, context.m_command_buffer));

            if(!res) {
                device.core().DestroyCommandAllocator(context.m_allocator);
                context.m_allocator = nullptr;
                return nova::err(std::move(res.error()));
            }
            return context;
        }

        [[nodiscard]] nova::result<nova::ok> begin() {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(m_allocator != nullptr);
            DEBUG_ASSERT(m_command_buffer != nullptr);
            DEBUG_ASSERT(!m_recording);

            auto& core = m_device->core();

            core.ResetCommandAllocator(*m_allocator);

            auto res = check(core.BeginCommandBuffer(*m_command_buffer, nullptr));
            if(!res) {
                return nova::err(std::move(res.error()));
            }

            m_recording = true;

            return nova::ok{};
        }

        [[nodiscard]] nova::result<nova::ok> end() {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(m_command_buffer != nullptr);
            DEBUG_ASSERT(m_recording);

            const auto res = check(m_device->core().EndCommandBuffer(*m_command_buffer));

            m_recording = false;

            if(!res) {
                return nova::err(res.error());
            }

            return nova::ok{};
        }

        [[nodiscard]] nri::CommandBuffer& native() noexcept {
            DEBUG_ASSERT(m_command_buffer != nullptr);
            return *m_command_buffer;
        }

        [[nodiscard]] const nri::CommandBuffer& native() const noexcept {
            DEBUG_ASSERT(m_command_buffer != nullptr);
            return *m_command_buffer;
        }

        [[nodiscard]] bool recording() const noexcept {
            return m_recording;
        }
    private:
        explicit command_context(device& device) noexcept
            :
            m_device(&device)
        {}

        void destroy() noexcept {
            if(m_device == nullptr) {
                return;
            }

            auto& core = m_device->core();

            if(m_command_buffer != nullptr) {
                core.DestroyCommandBuffer(m_command_buffer);
                m_command_buffer = nullptr;
            }

            if(m_allocator != nullptr) {
                core.DestroyCommandAllocator(m_allocator);
                m_allocator = nullptr;
            }

            m_device = nullptr;
            m_recording = false;
        }

        device* m_device{nullptr};
        nri::CommandAllocator* m_allocator{nullptr};
        nri::CommandBuffer* m_command_buffer{nullptr};
        bool m_recording{false};
    };
}
