#pragma once
#include <expected>
#include <string>

namespace nova {
    struct ok{};

    template <class T, class Err = std::string>
    using result = std::expected<T, Err>;
}
