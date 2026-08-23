#include <catch2/catch_test_macros.hpp>
#include <concepts>
#include <type_traits>
#include <utility>
#include <ranges>

import backoff;

static_assert(std::default_initializable<nova::backoff>);
static_assert(std::is_nothrow_default_constructible_v<nova::backoff>);
static_assert(noexcept(std::declval<nova::backoff&>().reset()));
static_assert(noexcept(std::declval<nova::backoff&>().spin()));
static_assert(noexcept(std::declval<nova::backoff&>().snooze()));
static_assert(noexcept(std::declval<const nova::backoff&>().is_completed()));

TEST_CASE("Backoff") {
    SECTION("Initial state") {
        nova::backoff bo;
        REQUIRE_FALSE(bo.is_completed());
    }
    SECTION("Spin progression") {
        nova::backoff bo;

        for(const auto _ : std::views::iota(0u, 64u)) {
            bo.spin();
            REQUIRE_FALSE(bo.is_completed());
        }
    }
    SECTION("Snooze completion") {
        nova::backoff bo;

        for(const auto _ : std::views::iota(0u, 64u)) {
            if(bo.is_completed()) {
                break;
            }

            bo.snooze();
        }

        REQUIRE(bo.is_completed());
    }
    SECTION("Completion stability") {
        nova::backoff bo;

        while(!bo.is_completed()) {
            bo.snooze();
        }

        bo.snooze();
        REQUIRE(bo.is_completed());
    }
}