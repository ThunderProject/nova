#include "indirect.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <utility>

TEST_CASE("nova::indirect", "[indirect]") {
    SECTION("Constructors") {
        SECTION("Default constructor") {
            nova::indirect<int32_t> x;
            REQUIRE_FALSE(x.valueless_after_move());
            REQUIRE(*x == 0);
        }
        SECTION("single value constructor lvalue") {
            constexpr int32_t value = 42;
            nova::indirect<int32_t> x(value);

            REQUIRE_FALSE(x.valueless_after_move());
            REQUIRE(*x == 42);
        }
        SECTION("single value constructor rvalue") {
            nova::indirect<int32_t> x(99);

            REQUIRE_FALSE(x.valueless_after_move());
            REQUIRE(*x == 99);
        }
        SECTION("in_place constructor forwards args") {
            nova::indirect<std::pair<int, int>> x(std::in_place, 10, 20);

            REQUIRE(x->first == 10);
            REQUIRE(x->second == 20);
        }
    }
}
