#include <assert.hpp>
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <latch>
#include <memory>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

import cpu_scheduler;
import scheduler_work;
import coro_algorithms;

namespace {
    nova::scheduler_config coroutine_config(uint32_t workers) {
        return {
            .worker_count = workers,
            .local_queue_capacity = 2,
            .global_queue_capacity = 2,
            .task_slots_per_worker = 4,
            .global_poll_interval = 3,
            .steal_batch_size = 2
        };
    }

    nova::coro::task<std::uint64_t> fib(nova::cpu_scheduler<>& tp, uint32_t n) {
        if(n < 2) {
            co_return n;
        }

        auto [a,b] = co_await nova::coro::when_all(tp, fib(tp, n - 1), fib(tp, n - 2));
        co_return a + b;
    }
    
    nova::coro::task<int> value_task(int value) {
         co_return value;
    }
    
    nova::coro::task<void> increment_task(nova::cpu_scheduler<>& tp, std::atomic_uint32_t& total, uint32_t count = 1) {
        for(auto i = 0u; i != count; ++i) {
             co_await tp.yield();
             total.fetch_add(1, std::memory_order::relaxed);
        }
    }

    nova::coro::task<int> fail_task() {
         throw std::runtime_error("task failure");
         co_return 0;
    }

    nova::coro::task<std::unique_ptr<int>> owned_task(std::unique_ptr<int> value) {
         co_return value;
    }
    
    nova::coro::task<int> await_old_future(nova::cpu_scheduler<>::future<int> value) {
         co_return co_await std::move(value);
    }
    
    nova::coro::task<int> fork_example(nova::cpu_scheduler<>& tp) {
         auto left = nova::coro::fork(tp, value_task(20));
         auto right = nova::coro::fork(tp, value_task(22));
    
         auto [a,b] = co_await nova::coro::join(tp, std::move(left), std::move(right));
         co_return a + b;
     }
    
    struct move_error final : std::runtime_error {
        move_error() : std::runtime_error{"move failure"} {}
    };

    struct throwing_value {
        throwing_value(std::atomic_int32_t& live_count, std::atomic_bool& fail)
            :
            live(&live_count),
            fail(&fail)
        {
            live->fetch_add(1, std::memory_order::relaxed);
        }

        throwing_value(const throwing_value&) = delete;
        throwing_value(throwing_value&& rhs)
            :
            live(rhs.live),
            fail(rhs.fail)
        {
            DEBUG_ASSERT(fail != nullptr && live != nullptr);

            if(fail->load(std::memory_order::relaxed)) {
                throw move_error();
            }
            live->fetch_add(1, std::memory_order::relaxed);
        }
        ~throwing_value() {
            DEBUG_ASSERT(live != nullptr);
            live->fetch_sub(1, std::memory_order::relaxed);
        }

        std::atomic_int32_t* live{nullptr};
        std::atomic_bool* fail{nullptr};
    };

    struct blocking_copy {
        blocking_copy(std::latch& ready, std::latch& release)
            :
            entered(&ready),
            released(&release)
        {}

        blocking_copy(const blocking_copy& rhs)
            :
            entered(rhs.entered),
            released(rhs.released)
        {
            DEBUG_ASSERT(entered != nullptr && released != nullptr);

            entered->count_down();
            released->wait();
        }

        int operator()() const noexcept { return 42; }

        std::latch* entered{nullptr};
        std::latch* released{nullptr};
    };

    struct external_gate {
        std::atomic<void*> continuation{nullptr};
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> next) noexcept {
            continuation.store(next.address(), std::memory_order::release);
            continuation.notify_one();
        }
        void await_resume() const noexcept {}
        void wait_registered() const noexcept { continuation.wait(nullptr, std::memory_order::acquire); }
        void resume() noexcept {
            std::coroutine_handle<>::from_address(continuation.load(std::memory_order::acquire)).resume();
        }
    };

    nova::coro::task<int> suspended_task(external_gate& gate) {
         co_await gate;
         co_return 42;
     }

    TEST_CASE("CPU coroutines", "[cpu_scheduler][coroutine]") {
        SECTION("Empty and unstarted") {
            nova::coro::task<int> empty;
            CHECK_FALSE(empty.valid());

            auto owner = std::make_shared<int>(42);
            const std::weak_ptr<int> weak_ptr = owner;

            {
                auto unstarted = nova::coro::invoke(
                    [owned = std::move(owner)] -> nova::coro::task<int> {
                        DEBUG_ASSERT(owned != nullptr);
                        co_return *owned;
                    }
                );

                CHECK(unstarted.valid());
                CHECK_FALSE(weak_ptr.expired());
            }

            CHECK(weak_ptr.expired());
        }
        SECTION("Recursive joins") {
            for(auto workers : {1u, 4u}) {
                nova::cpu_scheduler<> tp{coroutine_config(workers)};

                for(auto i = 0u; i != 12; ++i) {
                    CHECK(nova::coro::sync_wait(tp, fib(tp, 12)) == 144);
                }
            }
        }
        SECTION("Lazy ownership") {
            nova::cpu_scheduler<> tp{coroutine_config(2)};
            std::atomic<unsigned> calls{0};

            auto operation = nova::coro::invoke(
                [owned = std::make_unique<int>(42), &calls]() -> nova::coro::task<int> {
                    calls.fetch_add(1, std::memory_order::relaxed);
                    co_return *owned;
                }
            );

            CHECK(calls.load(std::memory_order::relaxed) == 0);
            CHECK(sync_wait(tp, std::move(operation)) == 42);
            CHECK(calls.load(std::memory_order::relaxed) == 1);
            CHECK_FALSE(operation.valid());
        }
        SECTION("move only tasks") {
            nova::cpu_scheduler<> tp{coroutine_config(4)};
            std::atomic_uint32_t total = 0;

            auto[a, _, b] = nova::coro::sync_wait(
                tp, 
                nova::coro::when_all(
                    tp, 
                    owned_task(std::make_unique<int>(20)), 
                    increment_task(tp, total), 
                    value_task(22)
                )
            );

            CHECK(*a + b == 42);
            CHECK(total.load(std::memory_order::relaxed) == 1);
            CHECK(std::tuple_size_v<decltype(sync_wait(tp, nova::coro::when_all(tp)))> == 0);
        }

        SECTION("Vector joins") {
            nova::cpu_scheduler<> tp{coroutine_config(4)};
            std::vector<nova::coro::task<int>> children;

            for(int i = 0; i != 2000; ++i) {
                children.emplace_back(value_task(i));
            }
            auto results = sync_wait(tp, when_all(tp, std::move(children)));
            REQUIRE(results.size() == 2000);

            for(int i = 0; i != 2000; ++i) {
                CHECK(results[static_cast<std::size_t>(i)] == i);
            }
            CHECK(sync_wait(tp, when_all(tp, std::vector<nova::coro::task<int>>{})).empty());
        }

        SECTION("Exception joining") {
            nova::cpu_scheduler<> tp{coroutine_config(4)};
            std::atomic<unsigned> total{0};

            CHECK_THROWS_AS(sync_wait(tp, when_all(tp, fail_task(), increment_task(tp, total, 4000))), std::runtime_error);
            CHECK(total.load(std::memory_order::relaxed) == 4000);
        }

        SECTION("Yield affinity") {
            nova::cpu_scheduler<> tp{coroutine_config(4)};
            std::atomic_uint32_t wrong_thread{0};
            std::vector<nova::coro::task<void>> children;

            for(unsigned i = 0; i != 32; ++i) {
                children.emplace_back(nova::coro::invoke(
                    [&]() -> nova::coro::task<void> {
                        co_await tp.schedule();

                        for(unsigned j = 0; j != 1000; ++j) {
                            co_await tp.yield();

                            if(!tp.is_worker_thread()) {
                                wrong_thread.fetch_add(1, std::memory_order::relaxed);
                            }
                        }
                    }
                ));
            }
        
            static_cast<void>(sync_wait(tp, when_all(tp, std::move(children))));
            CHECK(wrong_thread.load(std::memory_order::relaxed) == 0);
        }

        SECTION("Yield progress") {
            nova::cpu_scheduler<> tp{coroutine_config(1)};
        
            bool ran = false;
            auto setter = nova::coro::invoke([&]() -> nova::coro::task<void> {
                ran = true;
                co_return;
            });

            auto waiter = nova::coro::invoke([&]() -> nova::coro::task<bool> {
                for(unsigned i = 0; i != 100 && !ran; ++i) {
                    co_await tp.yield();
                }
                co_return ran;
            });

            const auto [ignored, observed] = sync_wait(tp, when_all(tp, std::move(setter), std::move(waiter)));
            static_cast<void>(ignored);
            CHECK(observed);
        }

        SECTION("Awaitable futures") {
            nova::cpu_scheduler<> tp{coroutine_config(1)};
    
            CHECK(sync_wait(tp, await_old_future(tp.submit([] { return 42; }))) == 42);
            CHECK(sync_wait(tp, fork_example(tp)) == 42);
            CHECK(sync_wait(tp, tp.schedule(value_task(42))) == 42);
        }

        SECTION("Worker progress") {
            nova::cpu_scheduler<> tp {coroutine_config(1)};
            
            auto result = tp.submit([&] { return sync_wait(tp, fib(tp, 12)); });
            CHECK(result.get() == 144);
        }

        SECTION("Slot overflow") {
            std::vector<nova::cpu_scheduler<>::future<int>> values;
            {
                auto cfg = coroutine_config(1);
                cfg.task_slots_per_worker = 1;
                nova::cpu_scheduler<> tp{cfg};
            
                values.emplace_back(tp.submit([] { return 7; }));
                values.front().wait();

                auto deferred = value_task(123);
                CHECK_FALSE(tp.try_submit(std::move(deferred)).has_value());
                CHECK(deferred.valid());

                for(int i = 0; i != 100; ++i) {
                    values.emplace_back(tp.submit(value_task(i)));
                }
            }
        
            CHECK(values.front().get() == 7);

            for(int i = 0; i != 100; ++i) {
                CHECK(values[static_cast<std::size_t>(i + 1)].get() == i);
            }
        }

        SECTION("Throwing result cleanup") {
            nova::cpu_scheduler<> tp{coroutine_config(2)};
        
            std::atomic_int32_t live{0};
            std::atomic_bool fail{false};

            auto result = tp.submit([&] { return throwing_value{live, fail}; });
            result.wait();
            fail.store(true, std::memory_order::relaxed);

            CHECK_THROWS_AS(result.get(), move_error);
            tp.shutdown();
            CHECK(live.load(std::memory_order::relaxed) == 0);
        }

        SECTION("Throwing return cleanup") {
            nova::cpu_scheduler<> tp{coroutine_config(2)};
            std::atomic_int32_t live{0};
            std::atomic_bool fail{true};

            auto child = nova::coro::invoke([&]() -> nova::coro::task<throwing_value> {
                co_return throwing_value{live, fail};
            });

            CHECK_THROWS_AS(sync_wait(tp, std::move(child)), move_error);
            tp.shutdown();
            CHECK(live.load(std::memory_order::relaxed) == 0);
        }

        SECTION("Shutdown construction") {
            using future = nova::cpu_scheduler<>::future<int>;

            nova::cpu_scheduler<> tp{coroutine_config(2)};
            std::latch entered{1};
            std::latch released{1};
            blocking_copy callable{entered, released};
            future result;

            std::jthread producer{[&] { result = tp.submit(callable); }};
            entered.wait();
            tp.request_stop();
            std::jthread stopper{[&] { tp.shutdown(); }};
            released.count_down();
            
            producer.join();
            stopper.join();

            CHECK(result.get() == 42);
            CHECK(tp.pending() == 0);
        }

        SECTION("Suspended root lifetime") {
            nova::cpu_scheduler<> tp{coroutine_config(2)};
            external_gate gate;
            auto result = tp.submit(suspended_task(gate));

            gate.wait_registered();
            tp.request_stop();

            std::jthread stopper{[&] { tp.shutdown(); }};
            gate.resume();
            stopper.join();
            CHECK(result.get() == 42);
        }

        SECTION("Concurrent shutdown") {
            nova::cpu_scheduler<> tp{coroutine_config(4)};
            std::atomic_uint32_t total{0};
    
            for(unsigned i = 0; i != 50; ++i) {
                nova::coro::spawn(tp, increment_task(tp, total, 100));
            }

            std::jthread first{[&] { tp.shutdown(); }};
            std::jthread second{[&] { tp.shutdown(); }};

            first.join();
            second.join();

            CHECK(total.load(std::memory_order::relaxed) == 5000);
            CHECK(tp.pending() == 0);
        }

        SECTION("For after stop") {
            nova::cpu_scheduler<> tp{coroutine_config(1)};

            auto result = tp.submit(nova::coro::invoke([&]() -> nova::coro::task<int> {
                tp.request_stop();

                co_await tp.yield();
                co_return co_await fork_example(tp);
            }));

            CHECK(result.get() == 42);
            CHECK_THROWS_AS(tp.submit(value_task(1)), nova::scheduler_stopped);
        }
    }
}

