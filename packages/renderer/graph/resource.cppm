module;
#include <cstdint>
#include <limits>
#include <variant>

export module nova.render.graph.resource;

namespace nova::render::graph {
    export inline constexpr std::uint32_t invalid_resource_index = std::numeric_limits<std::uint32_t>::max();

    export enum class resource_kind : std::uint8_t {
        texture,
        buffer
    };

    export enum class resource_lifetime : std::uint8_t {
        external,
        transient,
        persistent
    };

    export enum class texture_dimension : std::uint8_t {
        texture_1d,
        texture_2d,
        texture_3d
    };

    export enum class format : std::uint8_t {
        unknown,

        r8_unorm,
        r8_snorm,
        r8_uint,
        r8_sint,

        r16_unorm,
        r16_snorm,
        r16_uint,
        r16_sint,
        r16_sfloat,

        r32_uint,
        r32_sint,
        r32_sfloat,

        rg16_sfloat,
        rg32_sfloat,

        rgba8_unorm,
        rgba8_srgb,
        bgra8_unorm,
        bgra8_srgb,

        rgb10a2_unorm,

        rgba16_sfloat,
        rgba32_sfloat,

        d16_unorm,
        d32_sfloat,
        d24_unorm_s8_uint,
        d32_sfloat_s8_uint
    };

    export struct resource_id {
        resource_kind kind{};
        std::uint32_t index{invalid_resource_index};

        [[nodiscard]] constexpr bool valid() const noexcept {
            return index != invalid_resource_index;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept {
            return valid();
        }

        auto operator<=>(const resource_id&) const = default;
    };

    export struct texture_resource_tag {
        static constexpr resource_kind kind = resource_kind::texture;
    };

    export struct buffer_resource_tag {
        static constexpr resource_kind kind = resource_kind::buffer;
    };

    export template<class Tag>
    concept resource_tag =
        requires { Tag::kind; } &&
        std::same_as<std::remove_cv_t<decltype(Tag::kind)>, resource_kind> &&
        requires { typename std::integral_constant<resource_kind, Tag::kind>; };

    export template<resource_tag Tag>
    class resource_handle final {
    public:
        constexpr resource_handle() noexcept = default;

        [[nodiscard]] static constexpr resource_handle from_index(const std::uint32_t index) noexcept {
            return resource_handle{index};
        }
        
        [[nodiscard]] constexpr bool valid() const noexcept {
            return m_index != invalid_resource_index;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept {
            return valid();
        }

        [[nodiscard]] constexpr std::uint32_t index() const noexcept {
            return m_index;
        }

        [[nodiscard]] constexpr resource_id id() const noexcept {
            return {
                .kind = Tag::kind,
                .index = m_index
            };
        }

        auto operator<=>(const resource_handle&) const = default;

    private:
        explicit constexpr resource_handle(const std::uint32_t index) noexcept
            :
            m_index(index)
        {}

        std::uint32_t m_index{invalid_resource_index};
    };

    export using texture_handle = resource_handle<texture_resource_tag>;
    export using buffer_handle = resource_handle<buffer_resource_tag>;

    export struct texture_extent {
        std::uint32_t width{1};
        std::uint32_t height{1};
        std::uint32_t depth{1};

        [[nodiscard]] constexpr bool empty() const noexcept {
            return width == 0 || height == 0 || depth == 0;
        }

        auto operator<=>(const texture_extent&) const = default;
    };

    export struct texture_desc {
        texture_dimension dimension{texture_dimension::texture_2d};
        format format{format::unknown};

        texture_extent extent{};

        std::uint16_t mip_count{1};
        std::uint16_t layer_count{1};
        std::uint8_t sample_count{1};

        auto operator<=>(const texture_desc&) const = default;
    };

    export struct buffer_desc {
        std::uint64_t size{0};
        std::uint32_t stride{0};

        auto operator<=>(const buffer_desc&) const = default;
    };

    export using resource_desc = std::variant<texture_desc, buffer_desc>;

    export struct resource_info {
        resource_lifetime lifetime{resource_lifetime::transient};
        resource_desc description;
    };
}
