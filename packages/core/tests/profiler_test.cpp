#include <catch2/catch_test_macros.hpp>
#include <thread>

import core.profiler;

using namespace std::chrono_literals;

TEST_CASE("Scoped profiler records regions", "[core][profiler]") {
    nova::profiler::start();

    {
        nova::profiler::scope<"Outer region"> outer;
        std::this_thread::sleep_for(1ms);

        {
            nova::profiler::scope<"First region"> first;
            std::this_thread::sleep_for(2ms);
        }

        {
            nova::profiler::scope<"Second region"> second;
            std::this_thread::sleep_for(1ms);

            {
                nova::profiler::scope<"Nested region"> nested;
                std::this_thread::sleep_for(2ms);

                {
                    nova::profiler::scope<"Deep nested region"> deep;
                    std::this_thread::sleep_for(1ms);
                }
            }
        }
    }

    nova::profiler::stop();

    REQUIRE_FALSE(nova::profiler::is_running());
    REQUIRE(nova::profiler::counter_frequency_hz() > 0.0);

    nova::profiler::report();
}
