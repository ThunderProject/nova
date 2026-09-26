module;

#include "result.h"

#include <algorithm>
#include <assert.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

export module nova.render.graph.compiler;

import nova.render.graph.resource;
import nova.render.graph.pass;
import nova.render.graph.render_graph;

namespace nova::render::graph {
    struct resolved_texture_range {
        std::uint32_t mip_begin;
        std::uint32_t mip_end;
        std::uint32_t layer_begin;
        std::uint32_t layer_end;
    };

    struct resolved_buffer_range {
        std::uint64_t begin;
        std::uint64_t end;
    };

    [[nodiscard]] bool valid_access_mode(const resource_access& access) noexcept {
        switch(access.usage) {
            case resource_usage::shader_resource:
            case resource_usage::copy_source:
            case resource_usage::constant_buffer:
            case resource_usage::vertex_buffer:
            case resource_usage::index_buffer:
            case resource_usage::indirect_buffer:
            case resource_usage::present:
                return access.mode == access_mode::read;
            case resource_usage::copy_destination:
                return access.mode == access_mode::write;
            case resource_usage::color_attachment:
            case resource_usage::depth_stencil_attachment:
                return access.mode == access_mode::write || access.mode == access_mode::read_write;
            case resource_usage::shader_storage:
                return true;
        }
        return false;
    }

    [[nodiscard]] bool usage_supports_resource(const resource_usage usage, const resource_kind kind) noexcept {
        switch(usage) {
            case resource_usage::shader_resource:
            case resource_usage::shader_storage:
            case resource_usage::copy_source:
            case resource_usage::copy_destination:
                return true;
            case resource_usage::color_attachment:
            case resource_usage::depth_stencil_attachment:
            case resource_usage::present:
                return kind == resource_kind::texture;
            case resource_usage::constant_buffer:
            case resource_usage::vertex_buffer:
            case resource_usage::index_buffer:
            case resource_usage::indirect_buffer:
                return kind == resource_kind::buffer;
        }
        return false;
    }

    [[nodiscard]] bool usage_requires_shader_stage(const resource_usage usage) noexcept {
        return usage == resource_usage::shader_resource ||
            usage == resource_usage::shader_storage ||
            usage == resource_usage::constant_buffer;
    }

    [[nodiscard]] bool valid_queue_usage(const pass_queue queue, const resource_usage usage) noexcept {
        switch(queue) {
            case pass_queue::graphics:
                return true;
            case pass_queue::compute:
                return usage != resource_usage::color_attachment &&
                    usage != resource_usage::depth_stencil_attachment &&
                    usage != resource_usage::vertex_buffer &&
                    usage != resource_usage::index_buffer &&
                    usage != resource_usage::present;
            case pass_queue::copy:
                return usage == resource_usage::copy_source || usage == resource_usage::copy_destination;
        }
        return false;
    }

    [[nodiscard]] nova::result<resolved_texture_range> resolve_texture_range(
        const texture_desc& desc,
        const resource_range& range
    ) {
        if(std::holds_alternative<std::monostate>(range)) {
            return resolved_texture_range {
                .mip_begin = 0,
                .mip_end = desc.mip_count,
                .layer_begin = 0,
                .layer_end = desc.layer_count
            };
        }

        const auto* texture_range = std::get_if<texture_subresource_range>(&range);
        if(texture_range == nullptr) {
            return nova::err(std::string{"Buffer range used for a texture resource"});
        }

        const auto mip_offset = static_cast<std::uint32_t>(texture_range->mip_offset);
        const auto layer_offset = static_cast<std::uint32_t>(texture_range->layer_offset);

        if(mip_offset >= desc.mip_count) {
            return nova::err(std::string{"Texture mip offset is outside the resource"});
        }

        if(layer_offset >= desc.layer_count) {
            return nova::err(std::string{"Texture layer offset is outside the resource"});
        }

        const auto mip_count = texture_range->mip_count == remaining_subresources
            ? static_cast<std::uint32_t>(desc.mip_count) - mip_offset
            : static_cast<std::uint32_t>(texture_range->mip_count);

        const auto layer_count = texture_range->layer_count == remaining_subresources
            ? static_cast<std::uint32_t>(desc.layer_count) - layer_offset
            : static_cast<std::uint32_t>(texture_range->layer_count);

        if(mip_count == 0 || mip_count > static_cast<std::uint32_t>(desc.mip_count) - mip_offset) {
            return nova::err(std::string{"Texture mip range is outside the resource"});
        }

        if(layer_count == 0 || layer_count > static_cast<std::uint32_t>(desc.layer_count) - layer_offset) {
            return nova::err(std::string{"Texture layer range is outside the resource"});
        }

        return resolved_texture_range {
            .mip_begin = mip_offset,
            .mip_end = mip_offset + mip_count,
            .layer_begin = layer_offset,
            .layer_end = layer_offset + layer_count
        };
    }

    [[nodiscard]] nova::result<resolved_buffer_range> resolve_buffer_range(
        const buffer_desc& desc,
        const resource_range& range
    ) {
        if(std::holds_alternative<std::monostate>(range)) {
            return resolved_buffer_range{
                .begin = 0,
                .end = desc.size
            };
        }

        const auto* buffer = std::get_if<buffer_range>(&range);
        if(buffer == nullptr) {
            return nova::err(std::string{"Texture range used for a buffer resource"});
        }

        if(buffer->offset >= desc.size) {
            return nova::err(std::string{"Buffer offset is outside the resource"});
        }

        const auto size = buffer->size == whole_buffer
            ? desc.size - buffer->offset
            : buffer->size;

        if(size == 0 || size > desc.size - buffer->offset) {
            return nova::err(std::string{"Buffer range is outside the resource"});
        }

        return resolved_buffer_range{
            .begin = buffer->offset,
            .end = buffer->offset + size
        };
    }

    [[nodiscard]] bool overlaps(const resolved_texture_range& lhs, const resolved_texture_range& rhs) noexcept {
        const auto mip_overlap = lhs.mip_begin < rhs.mip_end && rhs.mip_begin < lhs.mip_end;
        const auto layer_overlap = lhs.layer_begin < rhs.layer_end && rhs.layer_begin < lhs.layer_end;

        return mip_overlap && layer_overlap;
    }

    [[nodiscard]] bool overlaps(const resolved_buffer_range& lhs, const resolved_buffer_range& rhs) noexcept {
        return lhs.begin < rhs.end && rhs.begin < lhs.end;
    }

    [[nodiscard]] nova::result<bool> accesses_overlap(
        const render_graph& graph,
        const resource_access& lhs,
        const resource_access& rhs
    ) {
        if(lhs.resource != rhs.resource) {
            return false;
        }

        const auto& info = graph.resource(lhs.resource);

        switch(lhs.resource.kind) {
            case resource_kind::texture: {
                const auto* desc = std::get_if<texture_desc>(&info.description);
                if(desc == nullptr) {
                    return nova::err(std::string{"Texture resource contains a non-texture description"});
                }

                auto lhs_range = resolve_texture_range(*desc, lhs.range);
                if(!lhs_range) {
                    return nova::err(std::move(lhs_range.error()));
                }

                auto rhs_range = resolve_texture_range(*desc, rhs.range);
                if(!rhs_range) {
                    return nova::err(std::move(rhs_range.error()));
                }

                return overlaps(*lhs_range, *rhs_range);
            }

            case resource_kind::buffer: {
                const auto* desc = std::get_if<buffer_desc>(&info.description);
                if(desc == nullptr) {
                    return nova::err(std::string{"Buffer resource contains a non-buffer description"});
                }

                auto lhs_range = resolve_buffer_range(*desc, lhs.range);
                if(!lhs_range) {
                    return nova::err(std::move(lhs_range.error()));
                }

                auto rhs_range = resolve_buffer_range(*desc, rhs.range);
                if(!rhs_range) {
                    return nova::err(std::move(rhs_range.error()));
                }

                return overlaps(*lhs_range, *rhs_range);
            }
        }

        std::unreachable();
    }

    [[nodiscard]] nova::result<nova::ok> validate_resource(const render_graph& graph, const resource_id resource) {
        const auto& info = graph.resource(resource);

        switch(resource.kind) {
            case resource_kind::texture: {
                const auto* desc = std::get_if<texture_desc>(&info.description);
                if(desc == nullptr) {
                    return nova::err(std::format(
                        "Texture resource '{}' contains a non-texture description",
                        graph.resource_name(resource)
                    ));
                }

                if(desc->extent.empty()) {
                    return nova::err(std::format(
                        "Texture resource '{}' has an empty extent",
                        graph.resource_name(resource)
                    ));
                }

                if(desc->format == format::unknown) {
                    return nova::err(std::format(
                        "Texture resource '{}' has an unknown format",
                        graph.resource_name(resource)
                    ));
                }

                if(desc->mip_count == 0 || desc->layer_count == 0 || desc->sample_count == 0) {
                    return nova::err(std::format(
                        "Texture resource '{}' has an invalid mip, layer, or sample count",
                        graph.resource_name(resource)
                    ));
                }

                return nova::ok{};
            }

            case resource_kind::buffer: {
                const auto* desc = std::get_if<buffer_desc>(&info.description);
                if(desc == nullptr) {
                    return nova::err(std::format(
                        "Buffer resource '{}' contains a non-buffer description",
                        graph.resource_name(resource)
                    ));
                }

                if(desc->size == 0) {
                    return nova::err(std::format(
                        "Buffer resource '{}' has zero size",
                        graph.resource_name(resource)
                    ));
                }

                return nova::ok{};
            }
        }

        std::unreachable();
    }

    [[nodiscard]] nova::result<nova::ok> validate_access(
        const render_graph& graph,
        const pass_handle pass,
        const resource_access& access
    ) {
        const auto pass_info = graph.pass(pass);
        const auto resource_name = graph.resource_name(access.resource);

        if(!usage_supports_resource(access.usage, access.resource.kind)) {
            return nova::err(std::format(
                "Pass '{}' uses resource '{}' with an incompatible resource usage",
                pass_info.name,
                resource_name
            ));
        }

        if(!valid_access_mode(access)) {
            return nova::err(std::format(
                "Pass '{}' uses resource '{}' with an invalid access mode",
                pass_info.name,
                resource_name
            ));
        }

        if(!valid_queue_usage(pass_info.queue, access.usage)) {
            return nova::err(std::format(
                "Pass '{}' uses resource '{}' with a usage unsupported by its queue",
                pass_info.name,
                resource_name
            ));
        }

        if(usage_requires_shader_stage(access.usage)) {
            if(access.stages == shader_stage::none) {
                return nova::err(std::format(
                    "Pass '{}' uses shader resource '{}' without specifying shader stages",
                    pass_info.name,
                    resource_name
                ));
            }
        }
        else if(access.stages != shader_stage::none) {
            return nova::err(std::format(
                "Pass '{}' specifies shader stages for non-shader resource '{}'",
                pass_info.name,
                resource_name
            ));
        }

        const auto& info = graph.resource(access.resource);

        switch(access.resource.kind) {
            case resource_kind::texture: {
                const auto* desc = std::get_if<texture_desc>(&info.description);
                DEBUG_ASSERT(desc != nullptr);

                auto range = resolve_texture_range(*desc, access.range);
                if(!range) {
                    return nova::err(std::format(
                        "Pass '{}' has an invalid range for texture '{}': {}",
                        pass_info.name,
                        resource_name,
                        range.error()
                    ));
                }

                break;
            }

            case resource_kind::buffer: {
                const auto* desc = std::get_if<buffer_desc>(&info.description);
                DEBUG_ASSERT(desc != nullptr);

                auto range = resolve_buffer_range(*desc, access.range);
                if(!range) {
                    return nova::err(std::format(
                        "Pass '{}' has an invalid range for buffer '{}': {}",
                        pass_info.name,
                        resource_name,
                        range.error()
                    ));
                }

                break;
            }
        }

        return nova::ok{};
    }

    [[nodiscard]] nova::result<nova::ok> validate_graph(const render_graph& graph) {
        for(std::size_t i = 0; i < graph.texture_count(); ++i) {
            const auto resource = texture_handle::from_index(static_cast<std::uint32_t>(i)).id();

            auto res = validate_resource(graph, resource);
            if(!res) {
                return nova::err(std::move(res.error()));
            }
        }

        for(std::size_t i = 0; i < graph.buffer_count(); ++i) {
            const auto resource = buffer_handle::from_index(static_cast<std::uint32_t>(i)).id();

            auto res = validate_resource(graph, resource);
            if(!res) {
                return nova::err(std::move(res.error()));
            }
        }

        for(std::size_t i = 0; i < graph.pass_count(); ++i) {
            const auto pass = graph.pass(i);
            const auto pass_info = graph.pass(pass);

            if(pass_info.name.empty()) {
                return nova::err(std::format("Render graph pass {} has an empty name", i));
            }

            const auto accesses = graph.accesses(pass);

            for(const auto& access : accesses) {
                auto res = validate_access(graph, pass, access);
                if(!res) {
                    return nova::err(std::move(res.error()));
                }
            }

            for(std::size_t lhs_index = 0; lhs_index < accesses.size(); ++lhs_index) {
                for(std::size_t rhs_index = lhs_index + 1; rhs_index < accesses.size(); ++rhs_index) {
                    const auto& lhs = accesses[lhs_index];
                    const auto& rhs = accesses[rhs_index];

                    if(lhs.resource != rhs.resource) {
                        continue;
                    }

                    auto overlap = accesses_overlap(graph, lhs, rhs);
                    if(!overlap) {
                        return nova::err(std::move(overlap.error()));
                    }

                    if(*overlap) {
                        return nova::err(std::format(
                            "Pass '{}' declares overlapping accesses to resource '{}' more than once",
                            pass_info.name,
                            graph.resource_name(lhs.resource)
                        ));
                    }
                }
            }
        }

        return nova::ok{};
    }

    [[nodiscard]] nova::result<bool> passes_conflict(const render_graph& graph, const pass_handle lhs, const pass_handle rhs) {
        for(const auto& lhs_access : graph.accesses(lhs)) {
            for(const auto& rhs_access : graph.accesses(rhs)) {
                if(lhs_access.resource != rhs_access.resource) {
                    continue;
                }

                if(!lhs_access.writes() && !rhs_access.writes()) {
                    continue;
                }

                auto overlap = accesses_overlap(graph, lhs_access, rhs_access);
                if(!overlap) {
                    return nova::err(std::move(overlap.error()));
                }

                if(*overlap) {
                    return true;
                }
            }
        }

        return false;
    }

    export class compiled_graph final {
    public:
        [[nodiscard]] static nova::result<compiled_graph> compile(const render_graph& graph) {
            auto validation = validate_graph(graph);
            if(!validation) {
                return nova::err(std::move(validation.error()));
            }

            const auto pass_count = graph.pass_count();

            compiled_graph result;

            result.m_dependencies.resize(pass_count);
            result.m_wave_index.resize(pass_count);

            std::vector<std::vector<std::size_t>> successors(pass_count);
            std::vector<std::size_t> indegree(pass_count, 0);

            for(std::size_t current_index = 0; current_index < pass_count; ++current_index) {
                const auto current = graph.pass(current_index);

                for(std::size_t previous_index = 0; previous_index < current_index; ++previous_index) {
                    const auto previous = graph.pass(previous_index);

                    auto conflict = passes_conflict(graph, previous, current);
                    if(!conflict) {
                        return nova::err(std::move(conflict.error()));
                    }

                    if(!*conflict) {
                        continue;
                    }

                    successors[previous_index].emplace_back(current_index);
                    ++indegree[current_index];

                    result.m_dependencies[current_index].emplace_back(previous);
                }
            }

            std::vector<std::size_t> current_wave;
            std::vector<std::size_t> next_wave;

            current_wave.reserve(pass_count);
            next_wave.reserve(pass_count);
            result.m_execution_order.reserve(pass_count);
            result.m_wave_offsets.reserve(pass_count + 1);

            for(std::size_t i = 0; i < pass_count; ++i) {
                if(indegree[i] == 0) {
                    current_wave.emplace_back(i);
                }
            }

            result.m_wave_offsets.emplace_back(0);

            std::size_t wave_index = 0;

            while(!current_wave.empty()) {
                std::ranges::sort(current_wave);

                for(const auto pass_index : current_wave) {
                    result.m_execution_order.emplace_back(pass_handle::from_index(static_cast<std::uint32_t>(pass_index)));
                    result.m_wave_index[pass_index] = wave_index;
                }

                result.m_wave_offsets.emplace_back(result.m_execution_order.size());

                next_wave.clear();

                for(const auto pass_index : current_wave) {
                    for(const auto successor : successors[pass_index]) {
                        DEBUG_ASSERT(indegree[successor] != 0);

                        --indegree[successor];

                        if(indegree[successor] == 0) {
                            next_wave.emplace_back(successor);
                        }
                    }
                }

                current_wave.swap(next_wave);
                ++wave_index;
            }

            if(result.m_execution_order.size() != pass_count) [[unlikely]] {
                return nova::err(std::string{"Render graph contains a dependency cycle"});
            }

            return result;
        }

        [[nodiscard]] std::size_t pass_count() const noexcept {
            return m_execution_order.size();
        }

        [[nodiscard]] std::size_t wave_count() const noexcept {
            DEBUG_ASSERT(!m_wave_offsets.empty());
            return m_wave_offsets.size() - 1;
        }

        [[nodiscard]] std::span<const pass_handle> execution_order() const noexcept {
            return m_execution_order;
        }

        [[nodiscard]] std::span<const pass_handle> wave(const std::size_t index) const noexcept {
            DEBUG_ASSERT(index < wave_count());

            const auto begin = m_wave_offsets[index];
            const auto end = m_wave_offsets[index + 1];

            return std::span{
                m_execution_order.data() + begin,
                end - begin
            };
        }

        [[nodiscard]] std::size_t wave_index(const pass_handle pass) const noexcept {
            DEBUG_ASSERT(pass.valid());
            DEBUG_ASSERT(pass.index() < m_wave_index.size());

            return m_wave_index[pass.index()];
        }

        [[nodiscard]] std::span<const pass_handle> dependencies(const pass_handle pass) const noexcept {
            DEBUG_ASSERT(pass.valid());
            DEBUG_ASSERT(pass.index() < m_dependencies.size());

            return m_dependencies[pass.index()];
        }

    private:
        std::vector<pass_handle> m_execution_order;
        std::vector<std::size_t> m_wave_offsets;
        std::vector<std::size_t> m_wave_index;
        std::vector<std::vector<pass_handle>> m_dependencies;
    };
}
