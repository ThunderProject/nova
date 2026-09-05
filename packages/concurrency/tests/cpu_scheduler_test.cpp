#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

import cpu_scheduler;

namespace {
    using namespace std::chrono_literals;

    using scheduler_type = nova::cpu_scheduler<>;
    using int_future = scheduler_type::future<int>;
    using void_future = scheduler_type::future<void>;

    static_assert(!std::is_copy_constructible_v<scheduler_type>);
    static_assert(!std::is_copy_assignable_v<scheduler_type>);
    static_assert(!std::is_move_constructible_v<scheduler_type>);
    static_assert(!std::is_move_assignable_v<scheduler_type>);
    static_assert(std::is_nothrow_destructible_v<scheduler_type>);

    static_assert(std::default_initializable<int_future>);
    static_assert(!std::is_copy_constructible_v<int_future>);
    static_assert(!std::is_copy_assignable_v<int_future>);
    static_assert(std::is_nothrow_move_constructible_v<int_future>);
    static_assert(std::is_nothrow_move_assignable_v<int_future>);
    static_assert(std::is_nothrow_destructible_v<int_future>);
    static_assert(std::is_nothrow_move_constructible_v<void_future>);
    static_assert(std::is_nothrow_move_assignable_v<void_future>);
    static_assert(!std::is_copy_constructible_v<void_future>);
    static_assert(!std::is_copy_assignable_v<void_future>);
    static_assert(std::same_as<decltype(std::declval<int_future&>().get()), int>);
    static_assert(std::same_as<decltype(std::declval<void_future&>().get()), void>);

    [[nodiscard]] nova::scheduler_config config(std::uint32_t workers = 2) {
        return {
            .worker_count = workers,
            .local_queue_capacity = 64,
            .global_queue_capacity = 128,
            .task_slots_per_worker = 256,
            .global_poll_interval = 7,
            .steal_batch_size = 8
        };
    }

    template<class Predicate>
    [[nodiscard]] bool eventually(Predicate&& predicate) {
        const auto deadline = std::chrono::steady_clock::now() + 5s;

        do {
            if(std::invoke(predicate)) {
                return true;
            }
            std::this_thread::sleep_for(1ms);
        } while(std::chrono::steady_clock::now() < deadline);

        return std::invoke(predicate);
    }

    class event {
    public:
        void set() noexcept {
            m_set.store(true, std::memory_order::release);
            m_set.notify_all();
        }

        void wait() const noexcept {
            m_set.wait(false, std::memory_order::acquire);
        }

        [[nodiscard]] bool is_set() const noexcept {
            return m_set.load(std::memory_order::acquire);
        }
    private:
        std::atomic<bool> m_set{false};
    };

    struct release_on_exit {
        event& gate;

        ~release_on_exit() noexcept {
            gate.set();
        }
    };

    struct counted_delete {
        std::atomic<unsigned>* destroyed;

        void operator()(int* pointer) const noexcept {
            delete pointer;
            destroyed->fetch_add(1, std::memory_order::relaxed);
        }
    };

    using counted_ptr = std::unique_ptr<int, counted_delete>;

    [[nodiscard]] counted_ptr counted(std::atomic<unsigned>& destroyed, int value = 42) {
        return counted_ptr{new int{value}, counted_delete{std::addressof(destroyed)}};
    }

    struct task_error final : std::runtime_error {
        task_error() : std::runtime_error{"task failed"} {}
    };

    struct construction_error final : std::runtime_error {
        construction_error() : std::runtime_error{"copy failed"} {}
    };

    struct throwing_copy {
        throwing_copy() = default;
        throwing_copy(const throwing_copy&) {
            throw construction_error{};
        }
        throwing_copy(throwing_copy&&) noexcept = default;

        int operator()() && noexcept {
            return 42;
        }
    };

    struct move_only_callable {
        std::unique_ptr<int> value;

        int operator()() && noexcept {
            return *value;
        }

        int operator()() & = delete;
    };

    struct accumulator {
        int value = 0;

        int add(int amount) noexcept {
            value += amount;
            return value;
        }
    };

    struct alignas(128) wide_value {
        std::array<std::uint64_t, 32> values{};
    };

    std::atomic<unsigned> handled_exceptions{0};
    std::atomic<unsigned> unexpected_exceptions{0};

    void handle_exception(std::exception_ptr exception) noexcept {
        if(exception == nullptr) {
            unexpected_exceptions.fetch_add(1, std::memory_order::relaxed);
            return;
        }

        try {
            std::rethrow_exception(exception);
        }
        catch(const task_error&) {
            handled_exceptions.fetch_add(1, std::memory_order::relaxed);
        }
        catch(...) {
            unexpected_exceptions.fetch_add(1, std::memory_order::relaxed);
        }
    }
}

TEST_CASE("CPU scheduler", "[cpu_scheduler]") {
    SECTION("Configuration") {
        for(const auto workers : {1u, 2u, 4u}) {
            CAPTURE(workers);
            scheduler_type scheduler{config(workers)};

            CHECK(scheduler.worker_count() == workers);
            CHECK(scheduler.pending() == 0);

            scheduler.shutdown();
            scheduler.shutdown();
            CHECK(scheduler.pending() == 0);
        }
    }

    SECTION("Invalid configuration") {
        const auto invalid = [](auto change) {
            auto cfg = config();
            change(cfg);
            CAPTURE(cfg.worker_count, cfg.local_queue_capacity, cfg.global_queue_capacity, cfg.task_slots_per_worker, cfg.global_poll_interval, cfg.steal_batch_size);
            CHECK_THROWS_AS(scheduler_type{cfg}, std::invalid_argument);
        };

        invalid([](auto& cfg) { cfg.worker_count = 0; });
        invalid([](auto& cfg) { cfg.local_queue_capacity = 0; });
        invalid([](auto& cfg) { cfg.global_queue_capacity = 0; });
        invalid([](auto& cfg) { cfg.global_queue_capacity = 3; });
        invalid([](auto& cfg) { cfg.task_slots_per_worker = 0; });
        invalid([](auto& cfg) { cfg.global_poll_interval = 0; });
        invalid([](auto& cfg) { cfg.steal_batch_size = 0; });
        invalid([](auto& cfg) { cfg.steal_batch_size = 33; });
        invalid([](auto& cfg) { cfg.local_queue_capacity = 4; });
    }

    SECTION("Empty future") {
        int_future value;
        void_future completion;

        CHECK_FALSE(value.valid());
        CHECK_FALSE(value.ready());
        CHECK_FALSE(completion.valid());
        CHECK_FALSE(completion.ready());
    }

    SECTION("Results") {
        scheduler_type scheduler{config()};
        std::vector<int_future> futures;
        futures.reserve(200);

        for(const auto value : std::views::iota(0, 200)) {
            futures.emplace_back(scheduler.submit([value] { return value * value; }));
        }

        for(const auto value : std::views::iota(0, 200)) {
            CAPTURE(value);
            auto& future = futures[static_cast<std::size_t>(value)];
            CHECK(future.get() == value * value);
            CHECK_FALSE(future.valid());
            CHECK_FALSE(future.ready());
        }

        scheduler.shutdown();
        CHECK(scheduler.pending() == 0);
    }

    SECTION("Void result") {
        int value = 0;
        scheduler_type scheduler{config()};
        auto future = scheduler.submit([&] { value = 42; });

        REQUIRE_NOTHROW(future.get());
        CHECK(value == 42);
        CHECK_FALSE(future.valid());
        CHECK_FALSE(future.ready());
    }

    SECTION("Wait visibility") {
        std::array<int, 64> values{};
        scheduler_type scheduler{config()};
        auto future = scheduler.submit([&] {
            for(const auto index : std::views::iota(std::size_t{0}, values.size())) {
                values[index] = static_cast<int>(index + 1);
            }
        });

        future.wait();
        future.wait();
        CHECK(future.valid());
        CHECK(future.ready());

        for(const auto index : std::views::iota(std::size_t{0}, values.size())) {
            CAPTURE(index);
            CHECK(values[index] == static_cast<int>(index + 1));
        }

        future.get();
        CHECK_FALSE(future.valid());
    }

    SECTION("Pending work") {
        event entered;
        event release;
        scheduler_type scheduler{config(1)};
        release_on_exit cleanup{release};

        auto first = scheduler.submit([&] {
            entered.set();
            release.wait();
            return 1;
        });
        REQUIRE(eventually([&] { return entered.is_set(); }));

        auto second = scheduler.submit([] { return 2; });
        auto third = scheduler.submit([] { return 3; });

        CHECK(first.valid());
        CHECK_FALSE(first.ready());
        CHECK_FALSE(second.ready());
        CHECK_FALSE(third.ready());
        CHECK(scheduler.pending() == 3);

        release.set();
        scheduler.shutdown();

        CHECK(scheduler.pending() == 0);
        CHECK(first.get() == 1);
        CHECK(second.get() == 2);
        CHECK(third.get() == 3);
    }

    SECTION("Value arguments") {
        event entered;
        event release;
        scheduler_type scheduler{config(1)};
        release_on_exit cleanup{release};

        auto blocker = scheduler.submit([&] {
            entered.set();
            release.wait();
        });
        REQUIRE(eventually([&] { return entered.is_set(); }));

        int value = 40;
        auto future = scheduler.submit([](int lhs, int rhs) { return lhs + rhs; }, value, 2);
        value = 100;

        release.set();
        blocker.get();
        CHECK(future.get() == 42);
        CHECK(value == 100);
    }

    SECTION("Reference arguments") {
        int value = 40;
        scheduler_type scheduler{config()};
        auto future = scheduler.submit([](int& target, int amount) {
            target += amount;
            return target;
        }, std::ref(value), 2);

        CHECK(future.get() == 42);
        CHECK(value == 42);
    }

    SECTION("Member function") {
        accumulator object;
        scheduler_type scheduler{config()};
        auto future = scheduler.submit(&accumulator::add, std::addressof(object), 42);

        CHECK(future.get() == 42);
        CHECK(object.value == 42);
    }

    SECTION("Move-only callable") {
        scheduler_type scheduler{config()};
        auto future = scheduler.submit(move_only_callable{std::make_unique<int>(42)});

        CHECK(future.get() == 42);
    }

    SECTION("Move-only argument") {
        scheduler_type scheduler{config()};
        auto value = std::make_unique<int>(42);
        auto future = scheduler.submit([](std::unique_ptr<int> owned) {
            return *owned;
        }, std::move(value));

        CHECK(value == nullptr);
        CHECK(future.get() == 42);
    }

    SECTION("Move-only result") {
        scheduler_type scheduler{config()};
        auto future = scheduler.submit([] { return std::make_unique<int>(42); });
        auto result = future.get();

        REQUIRE(result != nullptr);
        CHECK(*result == 42);
        CHECK_FALSE(future.valid());
    }

    SECTION("Custom storage") {
        wide_value input;
        input.values.fill(42);
        nova::cpu_scheduler<512, 128, 8> scheduler{config()};
        auto future = scheduler.submit([input]() mutable {
            input.values.back() = 99;
            return input;
        });
        const auto result = future.get();

        CHECK(result.values.front() == 42);
        CHECK(result.values.back() == 99);
        CHECK(input.values.back() == 42);
    }

    SECTION("Move construction") {
        scheduler_type scheduler{config()};
        auto source = scheduler.submit([] { return 42; });
        auto destination = std::move(source);

        CHECK_FALSE(source.valid());
        CHECK_FALSE(source.ready());
        CHECK(destination.valid());
        CHECK(destination.get() == 42);
    }

    SECTION("Move assignment") {
        std::atomic<unsigned> destroyed{0};
        scheduler_type scheduler{config()};
        auto first = scheduler.submit([&] { return counted(destroyed, 1); });
        auto second = scheduler.submit([&] { return counted(destroyed, 2); });

        first.wait();
        second.wait();
        scheduler.shutdown();
        first = std::move(second);

        CHECK_FALSE(second.valid());
        CHECK_FALSE(second.ready());
        CHECK(destroyed.load(std::memory_order::relaxed) == 1);

        auto result = first.get();
        REQUIRE(result != nullptr);
        CHECK(*result == 2);
        result.reset();
        CHECK(destroyed.load(std::memory_order::relaxed) == 2);
    }

    SECTION("Move empty") {
        std::atomic<unsigned> destroyed{0};
        scheduler_type scheduler{config()};
        auto future = scheduler.submit([&] { return counted(destroyed); });
        future.wait();
        scheduler.shutdown();

        decltype(future) empty;
        future = std::move(empty);

        CHECK_FALSE(future.valid());
        CHECK_FALSE(future.ready());
        CHECK_FALSE(empty.valid());
        CHECK(destroyed.load(std::memory_order::relaxed) == 1);
    }

    SECTION("Discard ready") {
        std::atomic<unsigned> destroyed{0};
        scheduler_type scheduler{config()};
        {
            auto future = scheduler.submit([&] { return counted(destroyed); });
            future.wait();
            scheduler.shutdown();
            CHECK(destroyed.load(std::memory_order::relaxed) == 0);
        }

        CHECK(destroyed.load(std::memory_order::relaxed) == 1);
    }

    SECTION("Discard queued") {
        event entered;
        event release;
        std::atomic<unsigned> executed{0};
        std::atomic<unsigned> destroyed{0};
        scheduler_type scheduler{config(1)};
        release_on_exit cleanup{release};

        auto blocker = scheduler.submit([&] {
            entered.set();
            release.wait();
        });
        REQUIRE(eventually([&] { return entered.is_set(); }));

        {
            auto future = scheduler.submit([&, owned = counted(destroyed)]() mutable {
                executed.fetch_add(1, std::memory_order::relaxed);
                return std::move(owned);
            });
            CHECK_FALSE(future.ready());
        }

        release.set();
        blocker.get();
        scheduler.shutdown();

        CHECK(executed.load(std::memory_order::relaxed) == 1);
        CHECK(destroyed.load(std::memory_order::relaxed) == 1);
        CHECK(scheduler.pending() == 0);
    }

    SECTION("Exceptions") {
        scheduler_type scheduler{config()};
        auto future = scheduler.submit([]() -> int { throw task_error{}; });

        REQUIRE_NOTHROW(future.wait());
        CHECK(future.valid());
        CHECK(future.ready());
        CHECK_THROWS_AS(future.get(), task_error);
        CHECK_FALSE(future.valid());
        CHECK_FALSE(future.ready());

        auto next = scheduler.submit([] { return 42; });
        CHECK(next.get() == 42);
    }

    SECTION("Exception cleanup") {
        std::atomic<unsigned> destroyed{0};
        scheduler_type scheduler{config()};
        auto value = scheduler.submit([owned = counted(destroyed)]() -> int {
            throw task_error{};
        });
        auto completion = scheduler.submit([owned = counted(destroyed)]() -> void {
            throw task_error{};
        });

        CHECK_THROWS_AS(value.get(), task_error);
        CHECK_THROWS_AS(completion.get(), task_error);
        CHECK_FALSE(completion.valid());
        scheduler.shutdown();

        CHECK(destroyed.load(std::memory_order::relaxed) == 2);
    }

    SECTION("Callable cleanup") {
        std::atomic<unsigned> destroyed{0};
        int observed = 0;
        scheduler_type scheduler{config()};
        auto value = scheduler.submit([owned = counted(destroyed)] { return *owned; });
        auto completion = scheduler.submit([&, owned = counted(destroyed)] {
            observed = *owned;
        });

        CHECK(value.get() == 42);
        completion.get();
        scheduler.shutdown();

        CHECK(observed == 42);
        CHECK(destroyed.load(std::memory_order::relaxed) == 2);
    }

    SECTION("Construction failure") {
        auto cfg = config(1);
        cfg.task_slots_per_worker = 1;
        scheduler_type scheduler{cfg};
        throwing_copy callable;

        CHECK_THROWS_AS(scheduler.submit(callable), construction_error);
        CHECK_THROWS_AS(scheduler.try_submit(callable), construction_error);
        CHECK_THROWS_AS(scheduler.submit_detached(callable), construction_error);
        CHECK_THROWS_AS(scheduler.try_submit_detached(callable), construction_error);
        CHECK(scheduler.pending() == 0);

        auto future = scheduler.try_submit([] { return 42; });
        REQUIRE(future.has_value());
        CHECK(future->get() == 42);
    }

    SECTION("Try submit") {
        scheduler_type scheduler{config()};
        auto future = scheduler.try_submit([](int value) { return value + 2; }, 40);

        REQUIRE(future.has_value());
        CHECK(future->valid());
        CHECK(future->get() == 42);
        CHECK_FALSE(future->valid());
    }

    SECTION("Slot exhaustion") {
        auto cfg = config(1);
        cfg.task_slots_per_worker = 1;
        scheduler_type scheduler{cfg};
        auto held = scheduler.submit([] { return 7; });
        held.wait();

        // A ready future still owns its slot until consumed or destroyed.
        auto value = std::make_unique<int>(42);
        auto rejected = scheduler.try_submit([](std::unique_ptr<int> owned) {
            return *owned;
        }, std::move(value));

        CHECK_FALSE(rejected.has_value());
        REQUIRE(value != nullptr);
        CHECK(*value == 42);
        CHECK_FALSE(scheduler.try_submit_detached([](std::unique_ptr<int>) {}, std::move(value)));
        REQUIRE(value != nullptr);
        CHECK(*value == 42);

        CHECK(held.get() == 7);
        std::optional<int_future> next;
        REQUIRE(eventually([&] {
            next = scheduler.try_submit([] { return 42; });
            return next.has_value();
        }));
        CHECK(next->get() == 42);
    }

    SECTION("Discard reuse") {
        auto cfg = config(1);
        cfg.task_slots_per_worker = 1;
        scheduler_type scheduler{cfg};
        {
            auto future = scheduler.submit([] { return 7; });
            future.wait();
        }

        std::optional<int_future> next;
        REQUIRE(eventually([&] {
            next = scheduler.try_submit([] { return 42; });
            return next.has_value();
        }));
        CHECK(next->get() == 42);
    }

    SECTION("Slot reuse") {
        auto cfg = config(1);
        cfg.task_slots_per_worker = 1;
        scheduler_type scheduler{cfg};

        for(const auto index : std::views::iota(0, 128)) {
            CAPTURE(index);
            std::optional<int_future> future;
            REQUIRE(eventually([&] {
                future = scheduler.try_submit([index] {
                    if(index % 2 == 0) {
                        throw task_error{};
                    }
                    return index;
                });
                return future.has_value();
            }));

            if(index % 2 == 0) {
                CHECK_THROWS_AS(future->get(), task_error);
            }
            else {
                CHECK(future->get() == index);
            }
            CHECK_FALSE(future->valid());
        }
    }

    SECTION("Detached work") {
        int value = 0;
        std::atomic<unsigned> destroyed{0};
        scheduler_type scheduler{config()};
        scheduler.submit_detached([](int& target, std::unique_ptr<int> owned) {
            target = *owned;
        }, std::ref(value), std::make_unique<int>(42));

        // Detached submission also supports discarding a non-void result.
        CHECK(scheduler.try_submit_detached([owned = counted(destroyed)]() mutable {
            return std::move(owned);
        }));
        scheduler.shutdown();

        CHECK(value == 42);
        CHECK(destroyed.load(std::memory_order::relaxed) == 1);
        CHECK(scheduler.pending() == 0);
    }

    SECTION("Detached exceptions") {
        handled_exceptions.store(0, std::memory_order::relaxed);
        unexpected_exceptions.store(0, std::memory_order::relaxed);
        std::atomic<unsigned> destroyed{0};
        auto cfg = config();
        cfg.unhandled_exception = &handle_exception;
        scheduler_type scheduler{cfg};

        scheduler.submit_detached([owned = counted(destroyed)] {
            throw task_error{};
        });
        CHECK(scheduler.try_submit_detached([owned = counted(destroyed)] {
            throw task_error{};
        }));
        scheduler.shutdown();

        CHECK(handled_exceptions.load(std::memory_order::relaxed) == 2);
        CHECK(unexpected_exceptions.load(std::memory_order::relaxed) == 0);
        CHECK(destroyed.load(std::memory_order::relaxed) == 2);
        CHECK(scheduler.pending() == 0);
    }

    SECTION("Nested get") {
        scheduler_type scheduler{config(1)};
        auto parent = scheduler.submit([&] {
            auto child = scheduler.submit([] { return 42; });
            return child.get();
        });

        CHECK(parent.get() == 42);
    }

    SECTION("Nested wait") {
        scheduler_type scheduler{config(1)};
        auto parent = scheduler.submit([&] {
            std::vector<int_future> children;
            children.reserve(32);
            for(const auto value : std::views::iota(1, 33)) {
                children.emplace_back(scheduler.submit([value] { return value; }));
            }

            int sum = 0;
            for(auto& child : children) {
                child.wait();
                sum += child.get();
            }
            return sum;
        });

        CHECK(parent.get() == 528);
    }

    SECTION("Recursive work") {
        scheduler_type scheduler{config(4)};
        const auto tree = [&scheduler](auto self, unsigned depth) -> unsigned {
            if(depth == 0) {
                return 1;
            }
            auto left = scheduler.submit(self, self, depth - 1);
            auto right = scheduler.submit(self, self, depth - 1);
            return left.get() + right.get();
        };
        auto root = scheduler.submit(tree, tree, 6u);

        CHECK(root.get() == 64);
    }

    SECTION("Parallel children") {
        constexpr std::size_t count = 32;
        std::array<std::atomic<unsigned>, count> hits{};
        std::atomic<unsigned> other_workers{0};
        event published;
        event release;
        scheduler_type scheduler{config(4)};
        release_on_exit cleanup{release};

        auto parent = scheduler.submit([&] {
            const auto owner = std::this_thread::get_id();
            for(const auto index : std::views::iota(std::size_t{0}, count)) {
                scheduler.submit_detached([&, index, owner] {
                    hits[index].fetch_add(1, std::memory_order::relaxed);
                    if(std::this_thread::get_id() != owner) {
                        other_workers.fetch_add(1, std::memory_order::relaxed);
                    }
                });
            }
            published.set();
            release.wait();
        });

        REQUIRE(eventually([&] { return published.is_set(); }));
        // The parent stays occupied. Independent children must make progress
        // on other workers. Do not assert victim selection, batch size, or order.
        const bool progressed = eventually([&] {
            return other_workers.load(std::memory_order::relaxed) >= count / 2;
        });
        release.set();
        parent.get();
        scheduler.shutdown();

        CHECK(progressed);
        for(const auto index : std::views::iota(std::size_t{0}, count)) {
            CAPTURE(index);
            CHECK(hits[index].load(std::memory_order::relaxed) == 1);
        }
    }

    SECTION("Concurrent producers") {
        constexpr std::size_t producer_count = 4;
        constexpr std::size_t per_producer = 512;
        std::array<std::atomic<unsigned>, producer_count * per_producer> hits{};
        std::array<std::exception_ptr, producer_count> errors{};
        event start;
        auto cfg = config(4);
        cfg.global_queue_capacity = 8;
        cfg.task_slots_per_worker = 64;
        scheduler_type scheduler{cfg};
        std::array<std::jthread, producer_count> producers;
        release_on_exit cleanup{start};

        for(const auto producer : std::views::iota(std::size_t{0}, producer_count)) {
            producers[producer] = std::jthread{[&, producer] {
                start.wait();
                try {
                    for(const auto offset : std::views::iota(std::size_t{0}, per_producer)) {
                        const auto index = producer * per_producer + offset;
                        scheduler.submit_detached([&, index] {
                            hits[index].fetch_add(1, std::memory_order::relaxed);
                        });
                    }
                }
                catch(...) {
                    errors[producer] = std::current_exception();
                }
            }};
        }

        start.set();
        for(auto& producer : producers) {
            producer.join();
        }
        scheduler.shutdown();

        for(const auto& error : errors) {
            CHECK(error == nullptr);
        }
        for(const auto index : std::views::iota(std::size_t{0}, hits.size())) {
            CAPTURE(index);
            CHECK(hits[index].load(std::memory_order::relaxed) == 1);
        }
        CHECK(scheduler.pending() == 0);
    }

    SECTION("Queue pressure") {
        event entered;
        event release;
        std::atomic<unsigned> completed{0};
        auto cfg = config(1);
        cfg.global_queue_capacity = 2;
        scheduler_type scheduler{cfg};
        release_on_exit cleanup{release};

        auto blocker = scheduler.submit([&] {
            entered.set();
            release.wait();
        });
        REQUIRE(eventually([&] { return entered.is_set(); }));

        scheduler.submit_detached([&] { completed.fetch_add(1, std::memory_order::relaxed); });
        scheduler.submit_detached([&] { completed.fetch_add(1, std::memory_order::relaxed); });
        auto overflow = scheduler.try_submit([] { return 42; });
        const bool detached = scheduler.try_submit_detached([&] {
            completed.fetch_add(1, std::memory_order::relaxed);
        });

        release.set();
        blocker.get();
        scheduler.shutdown();

        REQUIRE(overflow.has_value());
        CHECK(overflow->get() == 42);
        CHECK(detached);
        CHECK(completed.load(std::memory_order::relaxed) == 3);
    }

    SECTION("Idle wakeup") {
        scheduler_type scheduler{config(1)};
        for(const auto round : std::views::iota(0, 8)) {
            CAPTURE(round);
            std::this_thread::sleep_for(10ms);
            auto future = scheduler.submit([round] { return round; });
            REQUIRE(eventually([&] { return future.ready(); }));
            CHECK(future.get() == round);
        }
    }

    SECTION("Worker wakeup") {
        constexpr std::uint32_t workers = 65;
        std::atomic<unsigned> entered{0};
        event release;
        auto cfg = config(workers);
        cfg.task_slots_per_worker = 4;
        scheduler_type scheduler{cfg};
        release_on_exit cleanup{release};
        std::array<void_future, workers> futures;

        std::this_thread::sleep_for(10ms);
        for(auto& future : futures) {
            future = scheduler.submit([&] {
                entered.fetch_add(1, std::memory_order::relaxed);
                release.wait();
            });
        }

        const bool all_started = eventually([&] {
            return entered.load(std::memory_order::relaxed) == workers;
        });
        release.set();
        for(auto& future : futures) {
            future.get();
        }
        scheduler.shutdown();

        CHECK(all_started);
        CHECK(scheduler.pending() == 0);
    }

    SECTION("Shutdown children") {
        constexpr std::size_t count = 32;
        std::array<std::atomic<unsigned>, count> hits{};
        scheduler_type scheduler{config(1)};
        scheduler.submit_detached([&] {
            for(const auto index : std::views::iota(std::size_t{0}, count)) {
                scheduler.submit_detached([&, index] {
                    hits[index].fetch_add(1, std::memory_order::relaxed);
                });
            }
        });

        scheduler.shutdown();
        for(const auto index : std::views::iota(std::size_t{0}, count)) {
            CAPTURE(index);
            CHECK(hits[index].load(std::memory_order::relaxed) == 1);
        }
        CHECK(scheduler.pending() == 0);
    }

    SECTION("Destructor drains") {
        constexpr std::size_t count = 128;
        std::array<std::atomic<unsigned>, count> hits{};
        {
            scheduler_type scheduler{config()};
            for(const auto index : std::views::iota(std::size_t{0}, count)) {
                scheduler.submit_detached([&, index] {
                    hits[index].fetch_add(1, std::memory_order::relaxed);
                });
            }
        }

        for(const auto index : std::views::iota(std::size_t{0}, count)) {
            CAPTURE(index);
            CHECK(hits[index].load(std::memory_order::relaxed) == 1);
        }
    }

    SECTION("Stopped submission") {
        scheduler_type scheduler{config()};
        scheduler.shutdown();
        scheduler.shutdown();

        CHECK_THROWS_AS(scheduler.submit([] {}), nova::scheduler_stopped);
        CHECK_THROWS_AS(scheduler.submit_detached([] {}), nova::scheduler_stopped);
        CHECK_FALSE(scheduler.try_submit([] {}).has_value());
        CHECK_FALSE(scheduler.try_submit_detached([] {}));
        CHECK(scheduler.pending() == 0);

        auto value = std::make_unique<int>(42);
        auto future = scheduler.try_submit([](std::unique_ptr<int> owned) {
            return *owned;
        }, std::move(value));
        CHECK_FALSE(future.has_value());
        REQUIRE(value != nullptr);
        CHECK(*value == 42);
    }

    SECTION("Independent schedulers") {
        scheduler_type first{config(1)};
        scheduler_type second{config(1)};
        auto result = first.submit([&] {
            auto other = second.submit([] { return 42; });
            return other.get();
        });

        CHECK(result.get() == 42);
        first.shutdown();
        auto next = second.submit([] { return 7; });
        CHECK(next.get() == 7);
    }
}
