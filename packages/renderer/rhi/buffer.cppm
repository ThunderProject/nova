module;
#include "result.h"
#include <NRI.h>
#include <NRIDescs.h>
#include <assert.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <nlohmann/detail/value_t.hpp>
#include <span>
#include <type_traits>
#include <utility>
export module nova.render.rhi.buffer;


import nova.render.rhi.core;
import nova.render.rhi.device;

export namespace nova::render::rhi {
    class buffer final {
    public:
        buffer(const buffer&) = delete;
        buffer& operator=(const buffer&) = delete;
        buffer(buffer&& rhs) noexcept
            :
            m_device(std::exchange(rhs.m_device, nullptr)),
            m_buffer(std::exchange(rhs.m_buffer, nullptr)),
            m_shader_resource(std::exchange(rhs.m_shader_resource, nullptr)),
            m_size(std::exchange(rhs.m_size, 0)),
            m_stride(std::exchange(rhs.m_stride, 0))
        {}
        buffer& operator=(buffer&& rhs) noexcept {
            if(this == &rhs) {
                return *this;
            }

            destroy();
            m_device = std::exchange(rhs.m_device, nullptr);
            m_buffer = std::exchange(rhs.m_buffer, nullptr);
            m_shader_resource = std::exchange(rhs.m_shader_resource, nullptr);
            m_size = std::exchange(rhs.m_size, 0);
            m_stride = std::exchange(rhs.m_stride, 0);
            return *this;
        }
        ~buffer() noexcept {
            destroy();
        }

        [[nodiscard]] static nova::result<buffer> create_upload_buffer(
            device& device, 
            const std::uint64_t size, 
            const std::uint32_t stride
        ) {
            if(size == 0) [[unlikely]] {
                return nova::err(std::string("Buffer size must be greater than zero"));
            }

            if(stride == 0) [[unlikely]] {
                return nova::err(std::string("Buffer stride must be greater than zero"));
            }

            buffer buf{device};

            nri::BufferDesc desc{};
            desc.size = size;
            desc.structureStride = stride;
            desc.usage = nri::BufferUsageBits::SHADER_RESOURCE;

            auto result = check(device.core().CreateCommittedBuffer(
                device.native(),
                nri::MemoryLocation::DEVICE_UPLOAD,
                0.0f,
                desc,
                buf.m_buffer
                )
            );

            if(!result) {
                return nova::err(std::move(result.error()));
            }

            nri::BufferViewDesc view_desc{};
            view_desc.buffer = buf.m_buffer;
            view_desc.type = nri::BufferView::STRUCTURED_BUFFER;
            view_desc.offset = 0;
            view_desc.size = size;
            view_desc.structureStride = stride;

            result = check(device.core().CreateBufferView(view_desc, buf.m_shader_resource));
            if(!result) {
                return nova::err(std::move(result.error()));
            }

            buf.m_size = size;
            buf.m_stride = stride;

            return buf;
        }

        [[nodiscard]] nova::result<nova::ok> upload(const std::span<const std::byte> data) {
            DEBUG_ASSERT(m_device != nullptr);
            DEBUG_ASSERT(m_buffer != nullptr);

            if(data.size_bytes() > m_size) [[unlikely]] {
                return nova::err(std::string("upload exceeds buffer capacity"));
            }

            if(data.empty()) {
                return nova::ok{};
            }

            void* const mapped_buffer = m_device->core().MapBuffer(*m_buffer, 0, data.size_bytes());

            if(mapped_buffer == nullptr) [[unlikely]] {
                return nova::err(std::string("Failed to map upload buffer"));
            }

            std::memcpy(mapped_buffer, data.data(), data.size_bytes());

            m_device->core().UnmapBuffer(*m_buffer);

            return nova::ok{};
        }

        template<class T>
        requires std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>
        [[nodiscard]] nova::result<nova::ok> upload(const std::span<const T> data) {
            return upload(std::as_bytes(data));
        }

        [[nodiscard]] nri::Buffer& native() noexcept {
            DEBUG_ASSERT(m_buffer != nullptr);
            return *m_buffer;
        }

         [[nodiscard]] const nri::Buffer& native() const noexcept {
            DEBUG_ASSERT(m_buffer != nullptr);
            return *m_buffer;
        }

        [[nodiscard]] nri::Descriptor& shader_resource() noexcept {
            DEBUG_ASSERT(m_shader_resource != nullptr);
            return *m_shader_resource;
        }

        [[nodiscard]] std::uint64_t size() const noexcept {
            return m_size;
        }

        [[nodiscard]] std::uint32_t stride() const noexcept {
            return m_stride;
        }
    private:
        explicit buffer(device& device) noexcept
            :
            m_device(&device)
        {}

        void destroy() noexcept {
            if(m_device == nullptr) {
                return;
            }

            auto& core = m_device->core();

            if(m_shader_resource != nullptr) {
                core.DestroyDescriptor(m_shader_resource);
                m_shader_resource = nullptr;
            }

            if(m_buffer != nullptr) {
                core.DestroyBuffer(m_buffer);
                m_buffer = nullptr;
            }

            m_device = nullptr;
            m_size = 0;
            m_stride = 0;
        }

        device* m_device{nullptr};
        nri::Buffer* m_buffer{nullptr};
        nri::Descriptor* m_shader_resource{nullptr};
        std::uint64_t m_size{0};
        std::uint32_t m_stride{0};
    };
}
