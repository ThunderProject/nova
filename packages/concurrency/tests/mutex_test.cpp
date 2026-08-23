#include <array>
#include <atomic>
#include <barrier>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>
#include <catch2/catch_test_macros.hpp>

import mutex;
import hints;

using namespace std::chrono_literals;

namespace {
    template<class Pred>
    [[nodiscard]] bool wait_until(Pred&& pred, const std::chrono::milliseconds timeout = 1s) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;

        while(!pred()) {
            if(std::chrono::steady_clock::now() >= deadline) {
                return false;
            }
            std::this_thread::yield();
        }
        return true;
    }

    template<class T>
    concept BasicLockable = requires(T& mutex) {
        mutex.lock();
        mutex.unlock();
    };

    template<class T>
    concept Lockable = BasicLockable<T> && requires(T& mutex) {
        { mutex.try_lock() } -> std::same_as<bool>;
    };
}

TEST_CASE("mutex") {
    SECTION("type properties") {
        STATIC_REQUIRE(BasicLockable<nova::mutex>);
        STATIC_REQUIRE(Lockable<nova::mutex>);

        STATIC_REQUIRE(sizeof(nova::mutex) == 1);
        STATIC_REQUIRE(alignof(nova::mutex) == 1);

        STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<nova::mutex>);
        STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<nova::mutex>);
        STATIC_REQUIRE_FALSE(std::is_move_constructible_v<nova::mutex>);
        STATIC_REQUIRE_FALSE(std::is_move_assignable_v<nova::mutex>);
    }
    SECTION("lock and unlock") {
        nova::mutex mutex;

        mutex.lock();
        mutex.unlock();

        mutex.lock();
        mutex.unlock();
    }
    SECTION("try_lock success") {
        nova::mutex mutex;

        REQUIRE(mutex.try_lock());
        mutex.unlock();

        REQUIRE(mutex.try_lock());
        mutex.unlock();
    }
    SECTION("try_lock failure") {
        nova::mutex mutex;
        mutex.lock();

        std::atomic result{true};

        std::thread thread([&] {
            result.store(mutex.try_lock(), std::memory_order_relaxed);

            if(result.load(std::memory_order_relaxed)) {
                mutex.unlock();
            }
        });

        thread.join();

        REQUIRE_FALSE(result.load(std::memory_order_relaxed));

        mutex.unlock();

        REQUIRE(mutex.try_lock());
        mutex.unlock();
    }
    SECTION("lock") {
        nova::mutex mutex;
        mutex.lock();

        std::atomic started{false};
        std::atomic acquired{false};

        std::thread thread([&] {
            started.store(true, std::memory_order_release);

            mutex.lock();
            acquired.store(true, std::memory_order_release);
            mutex.unlock();
        });

        REQUIRE(wait_until([&] { return started.load(std::memory_order_acquire); }));

        std::this_thread::sleep_for(20ms);

        REQUIRE_FALSE(acquired.load(std::memory_order_acquire));

        mutex.unlock();

        REQUIRE(wait_until([&] { return acquired.load(std::memory_order_acquire); }));

        thread.join();
    }
    SECTION("std::lock_guard compatibility") {
        nova::mutex mutex;
        std::uint64_t value = 0;

        {
            std::lock_guard lock(mutex);
            value = 42;
        }

        REQUIRE(value == 42);
        REQUIRE(mutex.try_lock());
        mutex.unlock();
    }
    SECTION("std::unique_lock compatibility") {
        nova::mutex mutex;

        {
            std::unique_lock lock(mutex);

            REQUIRE(lock.owns_lock());
            REQUIRE(lock.mutex() == &mutex);

            lock.unlock();
            REQUIRE_FALSE(lock.owns_lock());

            lock.lock();
            REQUIRE(lock.owns_lock());
        }

        REQUIRE(mutex.try_lock());
        mutex.unlock();
    }
    SECTION("std::unique_lock defer_lock compatibility") {
        nova::mutex mutex;

        std::unique_lock lock(mutex, std::defer_lock);

        REQUIRE_FALSE(lock.owns_lock());

        lock.lock();

        REQUIRE(lock.owns_lock());

        lock.unlock();

        REQUIRE_FALSE(lock.owns_lock());
    }
    SECTION("std::unique_lock try_to_lock compatibility") {
        nova::mutex mutex;

        {
            std::unique_lock lock(mutex, std::try_to_lock);
            REQUIRE(lock.owns_lock());
        }

        mutex.lock();

        std::atomic owns{true};

        std::thread thread([&] {
            std::unique_lock lock(mutex, std::try_to_lock);
            owns.store(lock.owns_lock(), std::memory_order_relaxed);
        });

        thread.join();

        REQUIRE_FALSE(owns.load(std::memory_order_relaxed));

        mutex.unlock();
    }
    SECTION("std::unique_lock adopt_lock compatibility") {
        nova::mutex mutex;

        mutex.lock();

        {
            std::unique_lock lock(mutex, std::adopt_lock);
            REQUIRE(lock.owns_lock());
        }

        REQUIRE(mutex.try_lock());
        mutex.unlock();
    }
    SECTION("std::scoped_lock compatibility") {
        nova::mutex first;
        nova::mutex second;

        {
            std::scoped_lock lock(first, second);
        }

        REQUIRE(first.try_lock());
        first.unlock();

        REQUIRE(second.try_lock());
        second.unlock();
    }

    SECTION("std::scoped_lock works with std::mutex") {
        nova::mutex first;
        std::mutex second;

        {
            std::scoped_lock lock(first, second);
        }

        REQUIRE(first.try_lock());
        first.unlock();

        REQUIRE(second.try_lock());
        second.unlock();
    }

    SECTION("std::lock compatibility") {
        nova::mutex first;
        nova::mutex second;

        std::unique_lock first_lock(first, std::defer_lock);
        std::unique_lock second_lock(second, std::defer_lock);

        std::lock(first_lock, second_lock);

        REQUIRE(first_lock.owns_lock());
        REQUIRE(second_lock.owns_lock());
    }

    SECTION("condition_variable_any compatibility") {
        nova::mutex mutex;
        std::condition_variable_any cv;

        bool ready = false;
        bool observed = false;

        std::thread waiter([&] {
            std::unique_lock lock(mutex);

            cv.wait(lock, [&] {
                return ready;
            });

            observed = ready;
        });

        {
            std::lock_guard lock(mutex);
            ready = true;
        }

        cv.notify_one();
        waiter.join();

        REQUIRE(observed);
    }

    SECTION("mutual exclusion heavy contention") {
        constexpr std::size_t threads = 16;
        constexpr std::size_t iterations = 50'000;

        nova::mutex mutex;

        std::uint64_t counter = 0;

        std::atomic<int> inside{0};
        std::atomic<std::uint64_t> violations{0};

        std::barrier start(static_cast<std::ptrdiff_t>(threads + 1));

        std::vector<std::thread> workers;
        workers.reserve(threads);

        for(std::size_t thread = 0; thread != threads; ++thread) {
            workers.emplace_back([&] {
                start.arrive_and_wait();

                for(std::size_t i = 0; i != iterations; ++i) {
                    std::lock_guard lock(mutex);

                    if(inside.fetch_add(1, std::memory_order_relaxed) != 0) {
                        violations.fetch_add(1, std::memory_order_relaxed);
                    }

                    ++counter;

                    if((i & 0xff) == 0) {
                        std::this_thread::yield();
                    }

                    inside.fetch_sub(1, std::memory_order_relaxed);
                }
            });
        }

        start.arrive_and_wait();

        for(auto& worker : workers) {
            worker.join();
        }

        REQUIRE(violations.load(std::memory_order_relaxed) == 0);
        REQUIRE(counter == threads * iterations);
    }

    SECTION("unlock publishes writes") {
        constexpr std::size_t iterations = 250'000;

        struct state {
            std::uint64_t generation{};
            std::uint64_t inverse{};
            std::uint64_t mirror{};
        };

        nova::mutex mutex;
        state value{};

        std::atomic<bool> done{false};
        std::atomic<std::uint64_t> invalid{0};

        std::thread writer([&] {
            for(std::uint64_t generation = 1; generation <= iterations; ++generation) {
                std::lock_guard lock(mutex);

                value.generation = generation;
                value.inverse = ~generation;
                value.mirror = generation;
            }

            done.store(true, std::memory_order_release);
        });

        std::thread reader([&] {
            while(!done.load(std::memory_order_acquire)) {
                std::lock_guard lock(mutex);

                if(value.inverse != ~value.generation || value.mirror != value.generation) {
                    invalid.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });

        writer.join();
        reader.join();

        REQUIRE(invalid.load(std::memory_order_relaxed) == 0);
    }
}