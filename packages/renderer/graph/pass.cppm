module;
#include <cstdint>
#include <limits>
#include <string_view>
#include <variant>

export module nova.render.graph.pass;

import nova.render.graph.resource;

namespace nova::render::graph {
    export inline constexpr std::uint32_t invalid_pass_index = std::numeric_limits<std::uint32_t>::max();
    export inline constexpr std::uint16_t remaining_subresources = std::numeric_limits<std::uint16_t>::max();
    export inline constexpr std::uint64_t whole_buffer = std::numeric_limits<std::uint64_t>::max();

    export enum class pass_queue : std::uint8_t {
        graphics,
        compute,
        copy
    };

    export enum class access_mode : std::uint8_t {
        read,
        write,
        read_write
    };

    export enum class resource_usage : std::uint8_t {
        shader_resource,
        shader_storage,

        color_attachment,
        depth_stencil_attachment,

        copy_source,
        copy_destination,

        constant_buffer,
        vertex_buffer,
        index_buffer,
        indirect_buffer,

        present
    };

    export enum class shader_stage : std::uint8_t {
        none = 0,
        task = 1U << 0,
        mesh = 1U << 1,
        fragment = 1U << 2,
        compute = 1U << 3,
        all_graphics = task | mesh | fragment,
        all = all_graphics | compute
    };

    export [[nodiscard]] constexpr shader_stage operator|(const shader_stage lhs, const shader_stage rhs) noexcept {
        return static_cast<shader_stage>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
    }

    export [[nodiscard]] constexpr shader_stage operator&(const shader_stage lhs, const shader_stage rhs) noexcept {
        return static_cast<shader_stage>(static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs));
    }

    export constexpr shader_stage& operator|=(shader_stage& lhs, const shader_stage rhs) noexcept {
        lhs = lhs | rhs;
        return lhs;
    }

    export [[nodiscard]] constexpr bool contains(const shader_stage value, const shader_stage stage) noexcept {
        return (value & stage) == stage;
    }

    export struct texture_subresource_range {
        std::uint16_t mip_offset{0};
        std::uint16_t mip_count{remaining_subresources};

        std::uint16_t layer_offset{0};
        std::uint16_t layer_count{remaining_subresources};

        auto operator<=>(const texture_subresource_range&) const = default;
    };

    export struct buffer_range {
        std::uint64_t offset{0};
        std::uint64_t size{whole_buffer};

        auto operator<=>(const buffer_range&) const = default;
    };

    export using resource_range = std::variant<std::monostate, texture_subresource_range, buffer_range>;

    export struct resource_access {
        resource_id resource{};
        access_mode mode{access_mode::read};
        resource_usage usage{resource_usage::shader_resource};
        shader_stage stages{shader_stage::none};
        resource_range range;

        [[nodiscard]] constexpr bool reads() const noexcept {
            return mode == access_mode::read || mode == access_mode::read_write;
        }

        [[nodiscard]] constexpr bool writes() const noexcept {
            return mode == access_mode::write || mode == access_mode::read_write;
        }
    };

    export class pass_handle final {
    public:
        constexpr pass_handle() noexcept = default;

        [[nodiscard]] static constexpr pass_handle from_index(const std::uint32_t index) noexcept {
            return pass_handle{index};
        }

        [[nodiscard]] constexpr bool valid() const noexcept {
            return m_index != invalid_pass_index;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept {
            return valid();
        }

        [[nodiscard]] constexpr std::uint32_t index() const noexcept {
            return m_index;
        }

        auto operator<=>(const pass_handle&) const = default;

    private:
        explicit constexpr pass_handle(const std::uint32_t index) noexcept
            :
            m_index(index)
        {}

        std::uint32_t m_index{invalid_pass_index};
    };

    export struct pass_desc {
        std::string_view name;
        pass_queue queue{pass_queue::graphics};
    };
}
