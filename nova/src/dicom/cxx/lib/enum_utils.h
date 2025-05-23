#pragma once
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace nova {
    namespace detail {
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 9)
        inline constexpr auto suffix = sizeof("]") -1;
#define FUNC_SIG __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
        inline constexpr auto suffix = sizeof(">(void) noexcept")-1;
#define FUNC_SIG __FUNCSIG__
#else
#define FUNC_SIG
#endif

        constexpr auto max_range = 256;
        consteval bool is_digit(char c) { return c >= '0' && c <= '9'; }

        template<class T>
        struct limits {
            static constexpr auto Range = sizeof(T) <= 2
                ? static_cast<int32_t>(std::numeric_limits<T>::max()) - static_cast<int32_t>(std::numeric_limits<T>::min() + 1)
                : std::numeric_limits<int32_t>::max();

            static constexpr auto Size = std::min(Range, max_range);
            static constexpr auto Offset = std::is_signed_v<T> ? (Size + 1) / 2 : 0;
        };
    }

    namespace impl {
        template<class Enum, Enum>
        constexpr std::string_view enum_to_string_impl() noexcept {
            constexpr std::string_view fullName{FUNC_SIG};
            constexpr auto index = fullName.find_last_of(", :)-", fullName.size() - detail::suffix) + 1;

            if constexpr (detail::is_digit(fullName[index])) {
                return {};
            }
            return fullName.substr(index, fullName.size() - detail::suffix - index);
        }

        template<class Enum, int32_t Offset, int32_t... I>
        constexpr std::string_view enum_to_string_helper(Enum value, std::integer_sequence<int32_t, I...>) {
            return std::array<decltype(&enum_to_string_impl<Enum, static_cast<Enum>(0)>), sizeof...(I)> {
                { enum_to_string_impl<Enum, static_cast<Enum>(I - Offset)>... }
            }[Offset + static_cast<int>(value)]();
        }

        template<class Enum, int32_t Offset, int32_t... I>
        [[nodiscard]] constexpr std::optional<Enum> enum_from_string(std::string_view name, std::integer_sequence<int32_t, I...>) noexcept {
            std::optional<Enum> returnValue;

            ((impl::enum_to_string_impl<Enum, static_cast<Enum>(I - Offset)>() != name ||
            (returnValue = static_cast<Enum>(I - Offset), false)) && ...);

            return returnValue;
        }
    }

    template<class Enum, class U = std::underlying_type_t<std::decay_t<Enum>>>
    requires std::is_enum_v<std::decay_t<Enum>>
    constexpr std::string_view enum_to_string(Enum enumValue) noexcept {
        using Indices = std::make_integer_sequence<int, detail::limits<U>::Size>;

        if(static_cast<int>(enumValue) >= detail::max_range || static_cast<int>(enumValue) <= -detail::max_range) {
            return {};
        }
        return impl::enum_to_string_helper<std::decay_t<Enum>, detail::limits<U>::Offset>(enumValue, Indices{});
    }

    template<auto EnumValue>
    requires std::is_enum_v<std::decay_t<decltype(EnumValue)>>
    constexpr std::string_view enum_to_string() noexcept {
        return impl::enum_to_string_impl<decltype(EnumValue), EnumValue>();
    }

    template<class Enum, class U = std::underlying_type_t<std::decay_t<Enum>>>
    requires std::is_enum_v<std::decay_t<Enum>>
    std::optional<Enum> string_to_enum(std::string_view name) noexcept {
        return impl::enum_from_string<Enum, detail::limits<U>::Offset>(
           name,
           std::make_integer_sequence<int, detail::limits<U>::Size>{}
       );
    }
}
