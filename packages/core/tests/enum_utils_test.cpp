#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <optional>
#include "enum_utils.h"

enum class color : std::uint8_t {
    red,
    green,
    blue
};

enum class signs : std::int8_t {
    negative = -1,
    zero = 0,
    positive = 1
};

enum class sparse : std::int16_t {
    a = 0,
    b = -300,
    c = 300
};

TEST_CASE("enum_to_string") {
    CHECK(nova::enum_to_string(color::red) == "red");
    CHECK(nova::enum_to_string(color::green) == "green");
    CHECK(nova::enum_to_string(color::blue) == "blue");
}

TEST_CASE("enum_to_string compile time") {
    STATIC_CHECK(nova::enum_to_string(color::red) == "red");
    STATIC_CHECK(nova::enum_to_string(color::green) == "green");
    STATIC_CHECK(nova::enum_to_string(color::blue) == "blue");
}

TEST_CASE("string_to_enum") {
    CHECK(nova::string_to_enum<color>("red") == color::red);
    CHECK(nova::string_to_enum<color>("green") == color::green);
    CHECK(nova::string_to_enum<color>("blue") == color::blue);
}

TEST_CASE("string_to_enum compile time") {
    STATIC_CHECK(nova::string_to_enum<color>("red") == color::red);
    STATIC_CHECK(nova::string_to_enum<color>("green") == color::green);
    STATIC_CHECK(nova::string_to_enum<color>("blue") == color::blue);
}

TEST_CASE("enum_to_string->string_to_enum") {
    for (auto value : { color::red, color::green, color::blue }) {
        const auto name = nova::enum_to_string(value);
        REQUIRE(!name.empty());
        CHECK(nova::string_to_enum<color>(name) == value);
    }
}

TEST_CASE("signed") {
    CHECK(nova::enum_to_string(signs::negative) == "negative");
    CHECK(nova::enum_to_string(signs::zero) == "zero");
    CHECK(nova::enum_to_string(signs::positive) == "positive");

    CHECK(nova::string_to_enum<signs>("negative") == signs::negative);
    CHECK(nova::string_to_enum<signs>("zero") == signs::zero);
    CHECK(nova::string_to_enum<signs>("positive") == signs::positive);
}

TEST_CASE("Out of range") {
    CHECK(nova::enum_to_string(sparse::a) == "a");

    CHECK(nova::enum_to_string(sparse::b).empty());
    CHECK(nova::enum_to_string(sparse::c).empty());

    CHECK(nova::string_to_enum<sparse>("a") == sparse::a);
    CHECK(nova::string_to_enum<sparse>("b") == std::nullopt);
    CHECK(nova::string_to_enum<sparse>("c") == std::nullopt);
}
