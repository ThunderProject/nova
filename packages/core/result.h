#pragma once
#include <expected>
#include <string>
#include <type_traits>

namespace nova {
    struct ok{};

    template <class T, class Err = std::string>
    using result = std::expected<T, Err>;

    template <class Err = std::string>
    [[nodiscard]] constexpr std::unexpected<std::decay_t<Err>> err(Err&& error = Err()) {
        return std::unexpected<std::decay_t<Err>>(std::forward<Err>(error));
    }
}
