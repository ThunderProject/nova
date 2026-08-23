#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdint>
#include <latch>
#include <thread>
#include <unistd.h>
#include <array>


import thread_parker; 

using namespace std::chrono_literals;

TEST_CASE("Thread parker") {
    SECTION("Token") {
        nova::thread_parker parker;
        parker.unpark();

        CHECK(parker.park_for(0ns));
        CHECK_FALSE(parker.park_for(0ns));
    }
    SECTION("park") {
        nova::thread_parker parker;
        std::latch ready{1};

        int32_t value = 0;
        int32_t observed = 0;

        std::jthread waiter {
            [&] {
                ready.count_down();
                parker.park();
                observed = value;
            }
        };

        ready.wait();
        value = 42;
        parker.unpark();

        if(waiter.joinable()) {
            waiter.join();
        }
        CHECK(observed == 42);
    }
    SECTION("park zero") {
        nova::thread_parker parker;

        CHECK_FALSE(parker.park_for(0ns));
        CHECK_FALSE(parker.park_for(0ms));
        CHECK_FALSE(parker.park_for(0.0s));
    }
    SECTION("park negative") {
        nova::thread_parker parker;

        CHECK_FALSE(parker.park_for(-1ns));
        CHECK_FALSE(parker.park_for(-1ms));
        CHECK_FALSE(parker.park_for(-1.0s));
    }
    SECTION("park timeout") {
        nova::thread_parker parker;

        constexpr auto timeout = 20ms;

        const auto begin = std::chrono::steady_clock::now();
        const bool acquired = parker.park_for(timeout);
        const auto elapsed = std::chrono::steady_clock::now() - begin;

        CHECK_FALSE(acquired);
        CHECK(elapsed >= timeout);
    }
    SECTION("park wake") {
        nova::thread_parker parker;
        std::latch ready{1};

        bool acquired = false;

        std::jthread waiter {
            [&] {
                ready.count_down();
                acquired = parker.park_for(2s);
            }
        };

        ready.wait();
        parker.unpark();

        if(waiter.joinable()) {
            waiter.join();
        }

        CHECK(acquired);
    }
    SECTION("park sync") {
        nova::thread_parker parker;
        std::latch ready{1};

        int32_t value = 0;
        int32_t observed = 0;
        bool acquired = false;

        std::jthread waiter {
            [&] {
                ready.count_down();

                acquired = parker.park_for(2s);

                if(acquired) {
                    observed = value;
                }
            }
        };

        ready.wait();

        value = 42;
        parker.unpark();

        if(waiter.joinable()) {
            waiter.join();
        }

        REQUIRE(acquired);
        CHECK(observed == 42);
    }
    SECTION("coalesce") {
        nova::thread_parker parker;

        parker.unpark();
        parker.unpark();
        parker.unpark();

        CHECK(parker.park_for(0ns));
        CHECK_FALSE(parker.park_for(0ns));
    }
    SECTION("coalesce multiple threads") {
        nova::thread_parker parker;

        constexpr std::size_t count = 8;

        std::array<std::jthread, count> threads;

        for (auto& thread : threads) {
            thread = std::jthread {
                [&] {
                    parker.unpark();
                }
            };
        }

        CHECK(parker.park_for(0ns));
        CHECK_FALSE(parker.park_for(0ns));
    }
}
