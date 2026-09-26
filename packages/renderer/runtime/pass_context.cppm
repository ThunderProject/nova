module;

#include <NRI.h>
#include <assert.hpp>
#include <span>

export module nova.render.runtime.pass_context;

import nova.render.graph.resource;
import nova.render.rhi.command_context;

export namespace nova::render::runtime {
    struct texture_binding {
        nri::Texture* texture{nullptr};
        nri::Descriptor* shader_resource{nullptr};
        nri::Descriptor* storage{nullptr};
        nri::Descriptor* color_attachment{nullptr};
        nri::Descriptor* depth_stencil_attachment{nullptr};
    };

    struct buffer_binding {
        nri::Buffer* buffer{nullptr};
        nri::Descriptor* shader_resource{nullptr};
        nri::Descriptor* storage{nullptr};
        nri::Descriptor* constant_buffer{nullptr};
    };

    class pass_context final {
    public:
        pass_context(
            nri::CoreInterface& core, 
            rhi::command_context& commands,
            const std::span<const texture_binding> textures,
            const std::span<const buffer_binding> buffers
        ) noexcept
            :
            m_core(&core),
            m_commands(&commands),
            m_textures(textures),
            m_buffers(buffers)
        {}

        pass_context(const pass_context&) = delete;
        pass_context& operator=(const pass_context&) = delete;
        pass_context(pass_context&&) = delete;
        pass_context& operator=(pass_context&&) = delete;

        [[nodiscard]] nri::CoreInterface& core() noexcept {
            DEBUG_ASSERT(m_core != nullptr);
            return *m_core;
        }

        [[nodiscard]] rhi::command_context& commands() noexcept {
            DEBUG_ASSERT(m_commands != nullptr);
            return *m_commands;
        }

        [[nodiscard]] nri::CommandBuffer& command_buffer() noexcept {
            DEBUG_ASSERT(m_commands != nullptr);
            return m_commands->native();
        }

        [[nodiscard]] nri::Texture& texture(const graph::texture_handle handle) const noexcept {
            const auto& binding = texture_binding_for(handle);

            DEBUG_ASSERT(binding.texture != nullptr);
            return *binding.texture;
        }

        [[nodiscard]] nri::Buffer& buffer(const graph::buffer_handle handle) const noexcept {
            const auto& binding = buffer_binding_for(handle);

            DEBUG_ASSERT(binding.buffer != nullptr);
            return *binding.buffer;
        }

        [[nodiscard]] nri::Descriptor& shader_resource(const graph::texture_handle handle) const noexcept {
            const auto& binding = texture_binding_for(handle);

            DEBUG_ASSERT(binding.shader_resource != nullptr);
            return *binding.shader_resource;
        }

        [[nodiscard]] nri::Descriptor& shader_resource(const graph::buffer_handle handle) const noexcept {
            const auto& binding = buffer_binding_for(handle);

            DEBUG_ASSERT(binding.shader_resource != nullptr);
            return *binding.shader_resource;
        }

        [[nodiscard]] nri::Descriptor& storage(const graph::texture_handle handle) const noexcept {
            const auto& binding = texture_binding_for(handle);

            DEBUG_ASSERT(binding.storage != nullptr);
            return *binding.storage;
        }

        [[nodiscard]] nri::Descriptor& storage(const graph::buffer_handle handle) const noexcept {
            const auto& binding = buffer_binding_for(handle);

            DEBUG_ASSERT(binding.storage != nullptr);
            return *binding.storage;
        }

        [[nodiscard]] nri::Descriptor& color_attachment(const graph::texture_handle handle) const noexcept {
            const auto& binding = texture_binding_for(handle);

            DEBUG_ASSERT(binding.color_attachment != nullptr);
            return *binding.color_attachment;
        }

        [[nodiscard]] nri::Descriptor& depth_stencil_attachment(const graph::texture_handle handle) const noexcept {
            const auto& binding = texture_binding_for(handle);

            DEBUG_ASSERT(binding.depth_stencil_attachment != nullptr);
            return *binding.depth_stencil_attachment;
        }

        [[nodiscard]] nri::Descriptor& constant_buffer(const graph::buffer_handle handle) const noexcept {
            const auto& binding = buffer_binding_for(handle);

            DEBUG_ASSERT(binding.constant_buffer != nullptr);
            return *binding.constant_buffer;
        }

    private:
        [[nodiscard]] const texture_binding& texture_binding_for(const graph::texture_handle handle) const noexcept {
            DEBUG_ASSERT(handle.valid());
            DEBUG_ASSERT(handle.index() < m_textures.size());
            return m_textures[handle.index()];
        }

        [[nodiscard]] const buffer_binding& buffer_binding_for(const graph::buffer_handle handle) const noexcept {
            DEBUG_ASSERT(handle.valid());
            DEBUG_ASSERT(handle.index() < m_buffers.size());
            return m_buffers[handle.index()];
        }

        nri::CoreInterface* m_core{nullptr};
        rhi::command_context* m_commands{nullptr};

        std::span<const texture_binding> m_textures;
        std::span<const buffer_binding> m_buffers;
    };
}
