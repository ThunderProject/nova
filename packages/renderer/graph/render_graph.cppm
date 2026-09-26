module;

#include <assert.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module nova.render.graph.render_graph;

import nova.render.graph.resource;
import nova.render.graph.pass;

namespace nova::render::graph {
    export class render_graph;

    export class pass_builder final {
    public:
        pass_builder(const pass_builder&) = delete;
        pass_builder& operator=(const pass_builder&) = delete;
        pass_builder(pass_builder&&) = delete;
        pass_builder& operator=(pass_builder&&) = delete;

        pass_builder& read(
            const texture_handle resource,
            const resource_usage usage,
            const shader_stage stages = shader_stage::none
        ) {
            return add(resource.id(), access_mode::read, usage, stages, {});
        }

        pass_builder& read(
            const texture_handle resource,
            const resource_usage usage,
            const shader_stage stages,
            const texture_subresource_range range
        ) {
            return add(resource.id(), access_mode::read, usage, stages, range);
        }

        pass_builder& read(
            const buffer_handle resource,
            const resource_usage usage,
            const shader_stage stages = shader_stage::none
        ) {
            return add(resource.id(), access_mode::read, usage, stages, {});
        }

        pass_builder& read(
            const buffer_handle resource,
            const resource_usage usage,
            const shader_stage stages,
            const buffer_range range
        ) {
            return add(resource.id(), access_mode::read, usage, stages, range);
        }

        pass_builder& write(
            const texture_handle resource,
            const resource_usage usage,
            const shader_stage stages = shader_stage::none
        ) {
            return add(resource.id(), access_mode::write, usage, stages, {});
        }

        pass_builder& write(
            const texture_handle resource,
            const resource_usage usage,
            const shader_stage stages,
            const texture_subresource_range range
        ) {
            return add(resource.id(), access_mode::write, usage, stages, range);
        }

        pass_builder& write(
            const buffer_handle resource,
            const resource_usage usage,
            const shader_stage stages = shader_stage::none
        ) {
            return add(resource.id(), access_mode::write, usage, stages, {});
        }

        pass_builder& write(
            const buffer_handle resource,
            const resource_usage usage,
            const shader_stage stages,
            const buffer_range range
        ) {
            return add(resource.id(), access_mode::write, usage, stages, range);
        }

        pass_builder& read_write(
            const texture_handle resource,
            const resource_usage usage,
            const shader_stage stages = shader_stage::none
        ) {
            return add(resource.id(), access_mode::read_write, usage, stages, {});
        }

        pass_builder& read_write(
            const texture_handle resource,
            const resource_usage usage,
            const shader_stage stages,
            const texture_subresource_range range
        ) {
            return add(resource.id(), access_mode::read_write, usage, stages, range);
        }

        pass_builder& read_write(
            const buffer_handle resource,
            const resource_usage usage,
            const shader_stage stages = shader_stage::none
        ) {
            return add(resource.id(), access_mode::read_write, usage, stages, {});
        }

        pass_builder& read_write(
            const buffer_handle resource,
            const resource_usage usage,
            const shader_stage stages,
            const buffer_range range
        ) {
            return add(resource.id(), access_mode::read_write, usage, stages, range);
        }

    private:
        friend class render_graph;

        explicit pass_builder(render_graph& graph, const pass_handle pass) noexcept
            :
            m_graph(&graph),
            m_pass(pass)
        {}

        pass_builder& add(
            resource_id resource,
            access_mode mode,
            resource_usage usage,
            shader_stage stages,
            resource_range range
        );

        render_graph* m_graph;
        pass_handle m_pass;
    };

    class render_graph final {
    public:
        [[nodiscard]] texture_handle create_texture(
            const std::string_view name,
            const texture_desc& desc,
            const resource_lifetime lifetime = resource_lifetime::transient
        ) {
            DEBUG_ASSERT(lifetime != resource_lifetime::external);
            return add_texture(name, desc, lifetime);
        }

        [[nodiscard]] buffer_handle create_buffer(
            const std::string_view name,
            const buffer_desc& desc,
            const resource_lifetime lifetime = resource_lifetime::transient
        ) {
            DEBUG_ASSERT(lifetime != resource_lifetime::external);
            return add_buffer(name, desc, lifetime);
        }

        [[nodiscard]] texture_handle import_texture(const std::string_view name, const texture_desc& desc) {
            return add_texture(name, desc, resource_lifetime::external);
        }

        [[nodiscard]] buffer_handle import_buffer(const std::string_view name, const buffer_desc& desc) {
            return add_buffer(name, desc, resource_lifetime::external);
        }

        template<class Setup>
        requires std::invocable<Setup&, pass_builder&>
        [[nodiscard]] pass_handle add_pass(const pass_desc& desc, Setup&& setup) {
            const auto index = static_cast<std::uint32_t>(m_passes.size());

            m_passes.emplace_back(
                pass_record {
                    .name = std::string{desc.name},
                    .queue = desc.queue,
                    .accesses = {}
                }
            );

            const auto handle = pass_handle::from_index(index);
            pass_builder builder{*this, handle};
            std::invoke(setup, builder);
            return handle;
        }

        [[nodiscard]] std::size_t texture_count() const noexcept {
            return m_textures.size();
        }

        [[nodiscard]] std::size_t buffer_count() const noexcept {
            return m_buffers.size();
        }

        [[nodiscard]] std::size_t pass_count() const noexcept {
            return m_passes.size();
        }

        [[nodiscard]] const resource_info& resource(const texture_handle handle) const noexcept {
            DEBUG_ASSERT(handle.valid());
            DEBUG_ASSERT(handle.index() < m_textures.size());
            return m_textures[handle.index()].info;
        }

        [[nodiscard]] const resource_info& resource(const buffer_handle handle) const noexcept {
            DEBUG_ASSERT(handle.valid());
            DEBUG_ASSERT(handle.index() < m_buffers.size());
            return m_buffers[handle.index()].info;
        }

        [[nodiscard]] const resource_info& resource(const resource_id id) const noexcept {
            DEBUG_ASSERT(id.valid());

            switch(id.kind) {
                case resource_kind::texture:
                    DEBUG_ASSERT(id.index < m_textures.size());
                    return m_textures[id.index].info;
                case resource_kind::buffer:
                    DEBUG_ASSERT(id.index < m_buffers.size());
                    return m_buffers[id.index].info;
            }
            std::unreachable();
        }

        [[nodiscard]] std::string_view resource_name(const resource_id id) const noexcept {
            DEBUG_ASSERT(id.valid());

            switch(id.kind) {
                case resource_kind::texture:
                    DEBUG_ASSERT(id.index < m_textures.size());
                    return m_textures[id.index].name;

                case resource_kind::buffer:
                    DEBUG_ASSERT(id.index < m_buffers.size());
                    return m_buffers[id.index].name;
            }
            std::unreachable();
        }

        [[nodiscard]] pass_desc pass(const pass_handle handle) const noexcept {
            DEBUG_ASSERT(handle.valid());
            DEBUG_ASSERT(handle.index() < m_passes.size());

            const auto& pass = m_passes[handle.index()];

            return {
                .name = pass.name,
                .queue = pass.queue
            };
        }

        [[nodiscard]] std::span<const resource_access> accesses(const pass_handle handle) const noexcept {
            DEBUG_ASSERT(handle.valid());
            DEBUG_ASSERT(handle.index() < m_passes.size());

            return m_passes[handle.index()].accesses;
        }

        [[nodiscard]] pass_handle pass(const std::size_t index) const noexcept {
            DEBUG_ASSERT(index < m_passes.size());

            return pass_handle::from_index(static_cast<std::uint32_t>(index));
        }

    private:
        friend class pass_builder;

        struct resource_record {
            std::string name;
            resource_info info;
        };

        struct pass_record {
            std::string name;
            pass_queue queue{pass_queue::graphics};
            std::vector<resource_access> accesses;
        };

        [[nodiscard]] texture_handle add_texture(
            const std::string_view name,
            const texture_desc& desc,
            const resource_lifetime lifetime
        ) {
            const auto index = static_cast<std::uint32_t>(m_textures.size());

            m_textures.emplace_back(
                resource_record{
                    .name = std::string{name},
                    .info = {
                        .lifetime = lifetime,
                        .description = desc
                    }
                }
            );

            return texture_handle::from_index(index);
        }

        [[nodiscard]] buffer_handle add_buffer(
            const std::string_view name,
            const buffer_desc& desc,
            const resource_lifetime lifetime
        ) {
            const auto index = static_cast<std::uint32_t>(m_buffers.size());

            m_buffers.emplace_back(
                resource_record{
                    .name = std::string{name},
                    .info = {
                        .lifetime = lifetime,
                        .description = desc
                    }
                }
            );

            return buffer_handle::from_index(index);
        }

        void add_access(const pass_handle pass, resource_access access) {
            DEBUG_ASSERT(pass.valid());
            DEBUG_ASSERT(pass.index() < m_passes.size());
            DEBUG_ASSERT(access.resource.valid());

            switch(access.resource.kind) {
                case resource_kind::texture:
                    DEBUG_ASSERT(access.resource.index < m_textures.size());
                    break;

                case resource_kind::buffer:
                    DEBUG_ASSERT(access.resource.index < m_buffers.size());
                    break;
            }

            m_passes[pass.index()].accesses.emplace_back(std::move(access));
        }

        std::vector<resource_record> m_textures;
        std::vector<resource_record> m_buffers;
        std::vector<pass_record> m_passes;
    };

    pass_builder& pass_builder::add(
        const resource_id resource,
        const access_mode mode,
        const resource_usage usage,
        const shader_stage stages,
        resource_range range
    ) {
        DEBUG_ASSERT(m_graph != nullptr);
        DEBUG_ASSERT(m_pass.valid());
        DEBUG_ASSERT(resource.valid());

        m_graph->add_access(
            m_pass,
            {
                .resource = resource,
                .mode = mode,
                .usage = usage,
                .stages = stages,
                .range = std::move(range)
            }
        );

        return *this;
    }
}
