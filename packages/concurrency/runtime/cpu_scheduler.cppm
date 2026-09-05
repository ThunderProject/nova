module;
#include "libassert/assert.hpp"
#include <array>
#include <bit>
#include <cstddef>
#include <exception>
#include <latch>
#include <memory>
#include <new>
#include <stdexcept>
#include <atomic>
#include <functional>
#include <stop_token>
#include <tuple>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
export module cpu_scheduler;

import backoff;
import concurrentdeque;
import mpmc_queue;
import thread_parker;

namespace nova {
    constexpr inline std::size_t cache_line_size = 64;

    enum class completion_state : std::uint8_t {
        empty,
        queued,
        ready
    };

    template<class Function, class... Args>
    class invocation {
    public:
        template<class Fnc, class...A>
        explicit invocation(Fnc&& fnc, A&&... args) noexcept(std::conjunction_v<std::is_nothrow_constructible<Function, Fnc&&>, std::is_nothrow_constructible<std::tuple<Args...>, A&&...>>)
            :
            m_function(std::forward<Fnc>(fnc)),
            m_arguments(std::forward<A>(args)...)
        {}

        decltype(auto) operator()() {
            return std::apply(
                [this](auto&... args) -> decltype(auto) {
                    return std::invoke(std::move(m_function), std::move(args)...);
                },
                m_arguments
            );
        }
    private:
        [[no_unique_address]]
        Function m_function;

        [[no_unique_address]]
        std::tuple<Args...> m_arguments;
    };

    class fast_rng {
    public:
        explicit fast_rng(std::uint64_t seed) noexcept
            :
            m_state(mix(seed))
        {
            if(m_state == 0) {
                m_state = 0x9e3779b97f4a7c15ULL;
            }
        }

        [[nodiscard]] std::uint32_t bounded(const std::uint32_t bound) noexcept {
            DEBUG_ASSERT(bound != 0);

            const auto product = static_cast<std::uint64_t>(next()) * bound;
            return static_cast<std::uint32_t>(product >> 32);
        }

        [[nodiscard]] std::uint32_t next() noexcept {
            auto value = m_state;

            value ^= value >> 12;
            value ^= value << 25;
            value ^= value >> 27;

            m_state = value;

            return static_cast<std::uint32_t>((value * 0x2545f4914f6cdd1dULL) >> 32);
        }
    private:
        [[nodiscard]] static std::uint64_t mix(std::uint64_t value) noexcept {
            value += 0x9e3779b97f4a7c15ULL;
            value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
            value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;

            return value ^ (value >> 31);
        }
        std::uint64_t m_state;
    };

    inline void terminate_on_unhandled_exception(std::exception_ptr) noexcept {
        std::terminate();
    }

    template<std::size_t StorageSize, std::size_t StorageAlignment>
    class task_pool;

    template<std::size_t StorageSize, std::size_t StorageAlignment>
    class alignas(std::max(cache_line_size, StorageAlignment)) task_slot {
        using pool_type = task_pool<StorageSize, StorageAlignment>;

        using run_function = void(*)(task_slot&) noexcept;
        using destroy_function = void(*)(task_slot&) noexcept;
        using exception_handler = void(*)(std::exception_ptr) noexcept;
    public:
        task_slot() noexcept = default;
        task_slot(const task_slot&) = delete;
        task_slot& operator=(const task_slot&) = delete;
        task_slot(task_slot&&) = delete;
        task_slot& operator=(task_slot&&) = delete;

        template<class Result, class Model, class... Args>
        void prepare(Args&&... args) {
            static_assert(sizeof(Model) <= StorageSize);
            static_assert(alignof(Model) <= StorageAlignment);
            static_assert(std::is_nothrow_destructible_v<Model>);

            if constexpr (!std::is_void_v<Result>) {
                static_assert(std::is_object_v<Result>);
                static_assert(!std::is_const_v<Result>);
                static_assert(sizeof(Result) <= StorageSize);
                static_assert(alignof(Result) <= StorageAlignment);
                static_assert(std::is_nothrow_move_constructible_v<Result>);
                static_assert(std::is_nothrow_destructible_v<Result>);
            }

            std::construct_at(payload<Model>(), std::forward<Args>(args)...);
            m_destroy = &destroy_payload<Model>;
            m_run = &run<Result, Model>;

            m_references.store(2, std::memory_order::relaxed);
            m_state.store(completion_state::queued, std::memory_order::relaxed);
        }

        template<class Model, class... Args>
        void prepare_detached(const exception_handler handler, Args&&... args) {
            static_assert(sizeof(Model) <= StorageSize);
            static_assert(alignof(Model) <= StorageAlignment);
            static_assert(std::is_nothrow_destructible_v<Model>);

            std::construct_at(payload<Model>(), std::forward<Args>(args)...);
            m_destroy = &destroy_payload<Model>;
            m_run = &run_detached<Model>;
            m_exception_handler = handler;

            m_references.store(1, std::memory_order::relaxed);
            m_state.store(completion_state::queued, std::memory_order::relaxed);
        }

        void execute() noexcept {
            DEBUG_ASSERT(m_run != nullptr);
            m_run(*this);
        }

        [[nodiscard]] bool ready() const noexcept {
            return m_state.load(std::memory_order::acquire) == completion_state::ready;
        }

        void wait() const noexcept  {
            auto state = m_state.load(std::memory_order::acquire);

            while(state != completion_state::ready) {
                m_state.wait(state, std::memory_order::acquire);
                state = m_state.load(std::memory_order::acquire);
            }
        }


        template<class Result>
        [[nodiscard]] Result take_result() noexcept {
            static_assert(!std::is_void_v<Result>);
            static_assert(std::is_nothrow_move_constructible_v<Result>);

            return std::move(*payload<Result>());
        }

        [[nodiscard]] std::exception_ptr exception() const noexcept {
            return m_exception;
        }

        void release_reference() noexcept;
    private:
        friend pool_type;

        void init(pool_type* const pool, const std::uint32_t index, const std::uint32_t next) noexcept {
            m_pool = pool;
            m_index = index;
            m_next_free.store(next, std::memory_order::relaxed);
        }

        void reset() noexcept {
            DEBUG_ASSERT(m_destroy == nullptr);

            m_exception = {};
            m_exception_handler = nullptr;
            m_run = nullptr;
            m_references.store(1, std::memory_order::relaxed);
            m_state.store(completion_state::empty, std::memory_order::relaxed);
        }

        template<class Payload>
        [[nodiscard]] Payload* payload() noexcept {
            return std::launder(reinterpret_cast<Payload*>(m_storage.data()));
        }

        template<class Payload>
        [[nodiscard]] const Payload* payload() const noexcept {
            return std::launder(reinterpret_cast<const Payload*>(m_storage.data()));
        }

        template<class Payload>
        static void destroy_payload(task_slot& slot) noexcept {
            std::destroy_at(slot.template payload<Payload>());
        }

        template<class Result, class Model>
        static void run(task_slot& slot) noexcept {
            auto* const model = slot.template payload<Model>();
            bool model_alive = true;

            try {
                if constexpr (std::is_void_v<Result>) {
                    (*model)();
                    std::destroy_at(model);
                    model_alive = false;
                    slot.m_destroy = nullptr;
                }
                else {
                    Result result = (*model)();

                    std::destroy_at(model);
                    model_alive = false;

                    std::construct_at(slot.template payload<Result>(), std::move(result));
                    slot.m_destroy = &destroy_payload<Result>;
                }
            }
            catch(...) {
                if(model_alive) {
                    std::destroy_at(model);
                }

                slot.m_destroy = nullptr;
                slot.m_exception = std::current_exception();
            }

            slot.m_state.store(completion_state::ready, std::memory_order::release);
            slot.m_state.notify_one();
        }

        template<class Model>
        static void run_detached(task_slot& slot) noexcept {
            auto* const model = slot.template payload<Model>();

            try {
                static_cast<void>((*model)());
            }
            catch(...) {
                const auto exception = std::current_exception();

                std::destroy_at(model);
                slot.m_destroy = nullptr;
                slot.m_exception_handler(exception);
                return;
            }

            std::destroy_at(model);
            slot.m_destroy = nullptr;
        }

        alignas(StorageAlignment)
        std::array<std::byte, StorageSize> m_storage{};

        std::atomic_uint32_t m_references{0};
        std::atomic<completion_state> m_state{completion_state::empty};
        std::atomic_uint32_t m_next_free{0};

        pool_type* m_pool{nullptr};
        std::uint32_t m_index;

        run_function m_run{};
        destroy_function m_destroy{};
        exception_handler m_exception_handler{};
        std::exception_ptr m_exception;

    };

    template<std::size_t StorageSize, std::size_t StorageAlignment>
    class task_pool {
        using slot_type = task_slot<StorageSize, StorageAlignment>;

        static constexpr std::uint32_t index_bits = 20;
        static constexpr std::uint64_t index_mask = (std::uint64_t{1} << index_bits) - 1;
        static constexpr std::uint32_t empty_index = static_cast<std::uint32_t>(index_mask);
    public:
        explicit task_pool(const std::uint32_t capacity)
            :
            m_capacvity(validate_capacity(capacity)),
            m_slots(std::make_unique<slot_type[]>(m_capacvity))
        {
            for(uint32_t i = 0; i != capacity; ++i) {
                const auto next = i + 1 == m_capacvity
                    ? empty_index
                    : i + 1;
                m_slots[i].init(this, i, next);
            }
            m_head.store(pack(0,0), std::memory_order::relaxed);
        }

        task_pool(const task_pool&) = delete;
        task_pool& operator=(const task_pool&) = delete;
        task_pool& operator=(task_pool&&) = delete;
        task_pool(task_pool&&) = delete;

        ~task_pool() noexcept {
            DEBUG_ASSERT(m_live.load(std::memory_order::relaxed) == 0, "A task future outlived its scheduler");
        }

        [[nodiscard]] slot_type* try_acquire() noexcept {
            auto head = m_head.load(std::memory_order::acquire);

            for(;;) {
                const auto index = unpack_index(head);

                if(index == empty_index) {
                    return nullptr;
                }

                auto& slot = m_slots[index];
                const auto next = slot.m_next_free.load(std::memory_order::relaxed);

                if(m_head.compare_exchange_weak(head, pack(next, unpack_tag(head) + 1), std::memory_order::acquire, std::memory_order::relaxed)) {
                    m_live.fetch_add(1, std::memory_order::relaxed);
                    slot.reset();

                    return std::addressof(slot);
                }
            }
        }

        [[nodiscard]] std::uint64_t live() const noexcept {
            return m_live.load(std::memory_order::acquire);
        }
    private:
        friend slot_type;

        static std::uint32_t validate_capacity(const std::uint32_t capacity) {
            if(capacity == 0 || capacity > empty_index) {
                throw std::invalid_argument("task pool capacity is outside the supported range");
            }
            return capacity;
        }

        void recycle(slot_type& slot) noexcept {
            auto head = m_head.load(std::memory_order::relaxed);

            for(;;) {
                slot.m_next_free.store(unpack_index(head), std::memory_order::relaxed);

                if(m_head.compare_exchange_weak(head, pack(slot.m_index, unpack_tag(head) + 1), std::memory_order::release, std::memory_order::relaxed)) {
                    m_live.fetch_sub(1, std::memory_order::release);
                    return;
                }
            }
        }

        [[nodiscard]] static constexpr std::uint64_t pack(const std::uint32_t index, const std::uint64_t tag) noexcept {
            return (tag << index_bits) | static_cast<uint64_t>(index);
        }

        [[nodiscard]] static constexpr std::uint32_t unpack_index(const std::uint64_t val) noexcept {
            return static_cast<std::uint32_t>(val & index_mask);
        }

        [[nodiscard]] static constexpr std::uint64_t unpack_tag(const std::uint64_t val) noexcept {
            return val >> index_bits;
        }

        const std::uint32_t m_capacvity;
        std::unique_ptr<slot_type[]> m_slots;

        alignas(cache_line_size)
        std::atomic_uint64_t m_head{0};

        alignas(cache_line_size)
        std::atomic_uint64_t m_live{0};
    };

    template<std::size_t StorageSize, std::size_t StorageAlignment>
    void task_slot<StorageSize, StorageAlignment>::release_reference() noexcept {
        const auto prev = m_references.fetch_sub(1, std::memory_order::acq_rel);

        DEBUG_ASSERT(prev != 0);

        if(prev != 1) {
            return;
        }

        if(m_destroy != nullptr) {
            m_destroy(*this);
            m_destroy = nullptr;
        }

        m_exception = {};
        m_run = nullptr;
        m_exception_handler = nullptr;

        m_state.store(completion_state::empty, std::memory_order::relaxed);
        m_pool->recycle(*this);
    }

    export class scheduler_stopped final : std::runtime_error {
    public:
        scheduler_stopped() : std::runtime_error{"CPU scheduler has stopped"} {}
    };

    export struct scheduler_config {
        std::uint32_t worker_count = std::max(1u, std::thread::hardware_concurrency());
        std::size_t local_queue_capacity = 4096;
        std::size_t global_queue_capacity = 16384;
        std::uint32_t task_slots_per_worker = 4096;
        std::uint32_t global_poll_interval = 61;
        std::uint32_t steal_batch_size = 32;

        void(*unhandled_exception)(std::exception_ptr) noexcept = &terminate_on_unhandled_exception;
    };

    export template<
        std::size_t TaskStorageSize = 128,
        std::size_t TaskStorageAlignment = 64,
        std::size_t MaximumStealBatch = 32
    >
    class cpu_scheduler {
        static_assert(TaskStorageSize != 0);
        static_assert(std::has_single_bit(TaskStorageAlignment));
        static_assert(MaximumStealBatch != 0);

        using slot_type = task_slot<TaskStorageSize, TaskStorageAlignment>;
        using pool_type = task_pool<TaskStorageSize, TaskStorageAlignment>;
        using task_pointer = slot_type*;

        struct worker_state {
            worker_state(const std::uint32_t worker_index, const scheduler_config& cfg, const std::uint64_t seed)
                :
                index(worker_index),
                local_queue(cfg.local_queue_capacity),
                task_slots(cfg.task_slots_per_worker),
                rng(seed),
                lifo_slot(nullptr),
                global_budget(cfg.global_poll_interval)
            {}

            const std::uint32_t index;
            concurrent_deque<task_pointer, Flavor::Lifo> local_queue;
            pool_type task_slots;
            thread_parker parker;
            fast_rng rng;
            std::array<task_pointer, MaximumStealBatch> stolen_tasks{};
            task_pointer lifo_slot;
            std::uint32_t global_budget;
        };
    public:
        template<class Result>
        class future {
        public:
            future() noexcept = default;
            future(const future&) = delete;
            future& operator=(const future&) = delete;
            future(future&& rhs) noexcept
                :
                m_scheduler(std::exchange(rhs.m_scheduler, nullptr)),
                m_slot(std::exchange(rhs.m_slot, nullptr))
            {}
            future& operator=(future&& rhs) noexcept {
                if(this == std::addressof(rhs)) {
                    return *this;
                }

                reset();
                m_scheduler = std::exchange(rhs.m_scheduler, nullptr);
                m_slot = std::exchange(rhs.m_slot, nullptr);

                return *this;
            }
            ~future() noexcept {
                reset();
            }

            [[nodiscard]] bool valid() const noexcept {
                return m_slot != nullptr;
            }

            [[nodiscard]] bool ready() const noexcept {
                return m_slot != nullptr && m_slot->ready();
            }

            void wait() const {
                DEBUG_ASSERT(m_slot != nullptr);
                DEBUG_ASSERT(m_scheduler != nullptr);

                m_scheduler->wait_for(*m_slot);
            }

            Result get() {
                DEBUG_ASSERT(m_slot != nullptr);
                DEBUG_ASSERT(m_scheduler != nullptr);

                auto* const slot = std::exchange(m_slot, nullptr);
                m_scheduler->wait_for(*slot);
                m_scheduler = nullptr;

                const auto exception = slot->exception();

                if(exception != nullptr) {
                    slot->release_reference();
                    std::rethrow_exception(exception);
                }

                if constexpr(std::is_void_v<Result>) {
                    slot->release_reference();
                    return;
                }
                else {
                    auto result = slot->template take_result<Result>();
                    slot->release_reference();
                    return result;
                }
            }
        private:
            friend cpu_scheduler;

            future(cpu_scheduler* const scheduler, slot_type* const slot) noexcept
                :
                m_scheduler(scheduler),
                m_slot(slot)
            {}

            void reset() noexcept {
                if(m_slot != nullptr) {
                    m_slot->release_reference();
                    m_slot = nullptr;
                    m_scheduler = nullptr;
                }
            }

            cpu_scheduler* m_scheduler{};
            slot_type* m_slot{};
        };

        explicit cpu_scheduler(scheduler_config config = {})
            :
            m_config(validate_config(config)),
            m_global_queue(m_config.global_queue_capacity),
            m_ready(m_config.worker_count),
            m_idle_block_count((m_config.worker_count + 63U) / 64U),
            m_idle_blocks(std::make_unique<std::atomic<std::uint64_t>[]>(m_idle_block_count))
        {
            for(std::size_t i = 0; i != m_idle_block_count; ++i) {
                m_idle_blocks[i].store(0, std::memory_order::relaxed);
            }

            m_workers.reserve(m_config.worker_count);
            m_threads.reserve(m_config.worker_count);

            const auto scheduler_seed = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(this));

            for(std::uint32_t i = 0; i != m_config.worker_count; ++i) {
                m_workers.emplace_back(
                    std::make_unique<worker_state>(
                        i,
                        m_config,
                        scheduler_seed ^ (0x9e3779b97f4a7c15ULL * (static_cast<std::uint64_t>(i) + 1))
                    )
                );
            }

            for(std::uint32_t i = 0; i != m_config.worker_count; ++i) {
                m_threads.emplace_back([this, i](std::stop_token stop_token) {
                    worker_loop(stop_token, *m_workers[i]);
                });
            }

            m_ready.wait();
        }

        cpu_scheduler(const cpu_scheduler&) = delete;
        cpu_scheduler& operator=(const cpu_scheduler&) = delete;
        cpu_scheduler(cpu_scheduler&&) = delete;
        cpu_scheduler& operator=(cpu_scheduler&&) = delete;

        ~cpu_scheduler() noexcept {
            shutdown();

            for(const auto& worker : m_workers) {
                DEBUG_ASSERT(worker->task_slot.live() == 0, "A task future outlived its scheduler");
            }
        }

        [[nodiscard]] std::uint32_t worker_count() const noexcept {
            return m_config.worker_count;
        }

        [[nodiscard]] std::size_t pending() const noexcept {
            return m_pending.load(std::memory_order::acquire);
        }

        template<class Fnc, class... Args>
        [[nodiscard]] auto submit(Fnc&& fnc, Args&&... args) {
            using fnc_type = std::decay_t<Fnc>;
            using model_type = invocation<fnc_type, std::decay_t<Args>...>;

            using invocation_result = std::invoke_result_t<fnc_type, std::decay_t<Args>...>;
            using result_type = std::remove_cv_t<invocation_result>;

            static_assert(!std::is_reference_v<result_type>);
            static_assert(sizeof(model_type) <= TaskStorageSize);
            static_assert(alignof(model_type) <= TaskStorageAlignment);

            auto* const slot = acquire_slot();

            try {
                slot->template prepare<result_type, model_type>(
                    std::forward<Fnc>(fnc),
                    std::forward<Args>(args)...
                );
            }
            catch(...) {
                slot->release_reference();
                throw;
            }

            future<result_type> result {this, slot};
            publish(slot);
            return result;
        }

        template<class Fnc, class... Args>
        [[nodiscard]] auto try_submit(Fnc&& fnc, Args&&... args) {
            using function_type = std::decay_t<Fnc>;
            using model_type = invocation<function_type, std::decay_t<Args>...>;

            using invocation_result = std::invoke_result_t<function_type, std::decay_t<Args>...>;
            using result_type = std::remove_cv_t<invocation_result>;

            static_assert(!std::is_reference_v<result_type>);
            static_assert(sizeof(model_type) <= TaskStorageSize);
            static_assert(alignof(model_type) <= TaskStorageAlignment);

            using future_type = future<result_type>;

            if(!accepts_submission()) {
                return std::optional<future_type>{};
            }

            auto* const slot = try_acquire_slot();

            if(slot == nullptr) {
                return std::optional<future_type>{};
            }

            try {
                slot->template prepare<result_type, model_type>(
                    std::forward<Fnc>(fnc),
                    std::forward<Args>(args)...
                );
            }
            catch(...) {
                slot->release_reference();
                throw;
            }

            future_type result{this, slot};
            publish(slot);

            return std::optional<future_type>{ std::move(result) };
        }

        template<class Fnc, class... Args>
        void submit_detached(Fnc&& fnc, Args&&... args) {
            using model_type = invocation<std::decay_t<Fnc>, std::decay_t<Args>...>;

            static_assert(sizeof(model_type) <= TaskStorageSize);
            static_assert(alignof(model_type) <= TaskStorageAlignment);

            auto* const slot = acquire_slot();

            try {
                slot->template prepare_detached<model_type>(
                    m_config.unhandled_exception,
                    std::forward<Fnc>(fnc),
                    std::forward<Args>(args)...
                );
            }
            catch(...) {
                slot->release_reference();
                throw;
            }

            publish(slot);
        }

        template<class Fnc, class... Args>
        [[nodiscard]] bool try_submit_detached(Fnc&& fnc, Args&&... args) {
            using model_type = invocation<std::decay_t<Fnc>, std::decay_t<Args>...>;

            static_assert(sizeof(model_type) <= TaskStorageSize);
            static_assert(alignof(model_type) <= TaskStorageAlignment);

            if(!accepts_submission()) {
                return false;
            }

            auto* const slot = try_acquire_slot();

            if(slot == nullptr) {
                return false;
            }

            try {
                slot->template prepare_detached<model_type>(
                    m_config.unhandled_exception,
                    std::forward<Fnc>(fnc),
                    std::forward<Args>(args)...
                );
            }
            catch(...) {
                slot->release_reference();
                throw;
            }

            publish(slot);
            return true;
        }

        void shutdown() noexcept {
            bool expected = false;

            if(!m_shutdown_started.compare_exchange_strong(expected, true, std::memory_order::acq_rel, std::memory_order::acquire)) {
                return;
            }

            m_accepting.store(false, std::memory_order::release);
            m_stopping.store(true, std::memory_order::release);

            wake_all();

            for(auto& thread : m_threads) {
                if(thread.joinable()) {
                    thread.join();
                }
            }
        }
    private:
        [[nodiscard]] static scheduler_config validate_config(scheduler_config cfg) {
            if(cfg.worker_count == 0) {
                throw std::invalid_argument("worker_count must be greater than zero");
            }

            if(cfg.global_queue_capacity == 0 || !std::has_single_bit(cfg.global_queue_capacity)) {
                throw std::invalid_argument("global_queue_capacity must be a power of two");
            }

            if(cfg.task_slots_per_worker == 0) {
                throw std::invalid_argument("task_slots_per_worker must be greater than zero");
            }

            if(cfg.global_poll_interval == 0) {
                throw std::invalid_argument("global_poll_interval must be greater than zero");
            }


            if(cfg.steal_batch_size == 0 || cfg.steal_batch_size > MaximumStealBatch || cfg.steal_batch_size > cfg.local_queue_capacity) {
                throw std::invalid_argument("steal_batch_size is outside the supported range");
            }

            if(cfg.unhandled_exception == nullptr) {
                cfg.unhandled_exception = std::addressof(terminate_on_unhandled_exception);
            }

            return cfg;
        }

        [[nodiscard]] bool accepts_submission() const noexcept {
            return m_accepting.load(std::memory_order::acquire) || tls_scheduler == this;
        }

        [[nodiscard]] slot_type* try_acquire_slot() noexcept {
            if(tls_scheduler == this) {
                if(auto* slot = tls_worker->task_slots.try_acquire()) {
                    return slot;
                }
            }

            const auto count = static_cast<uint32_t>(m_workers.size());
            auto index = m_pool_cursor.fetch_add(1, std::memory_order::relaxed) % count;

            for(std::uint32_t i = 0; i != count; ++i) {
                if(auto* slot = m_workers[index]->task_slots.try_acquire()) {
                    return slot;
                }

                if(++index == count) {
                    index = 0;
                }
            }
            return nullptr;
        }

        [[nodiscard]] slot_type* acquire_slot() {
            if(!accepts_submission()) {
                throw scheduler_stopped{};
            }

            backoff bo;

            for(;;) {
                if(auto* slot = try_acquire_slot()) {
                    return slot;
                }

                if(!accepts_submission()) {
                    throw scheduler_stopped{};
                }

                task_pointer task = nullptr;

                if(tls_scheduler == this && try_get_task(*tls_worker, task)) {
                    execute_task(task);
                    bo.reset();
                    continue;
                }

                if(tls_scheduler != this && m_global_queue.try_pop(task)) {
                    execute_task(task);
                    bo.reset();
                    continue;
                }

                bo.snooze();
            }
        }

        void publish(task_pointer task) noexcept {
            m_pending.fetch_add(1, std::memory_order::release);

            if(tls_scheduler == this) {
                auto& worker = *tls_worker;

                auto* const displaced = std::exchange(worker.lifo_slot, task);

                if(displaced == nullptr) {
                    return;
                }

                if(worker.local_queue.push(displaced)) {
                    wake_one();
                    return;
                }

                if(m_global_queue.try_push(displaced)) {
                    wake_one();
                    return;
                }

                execute_task(displaced);
                return;
            }

            if(m_global_queue.try_push(task)) {
                wake_one();
                return;
            }

            execute_task(task);
        }

        [[nodiscard]] bool try_get_task(worker_state& worker, task_pointer& out) noexcept {
            bool checked_global = false;

            if(--worker.global_budget == 0) {
                worker.global_budget = m_config.global_poll_interval;
                checked_global = true;

                if(m_global_queue.try_pop(out)) {
                    return true;
                }
            }

            if(worker.lifo_slot != nullptr) {
                out = std::exchange(worker.lifo_slot, nullptr);
                return true;
            }

            if(const auto task = worker.local_queue.pop()) {
                out = *task;
                return true;
            }

            if(!checked_global && m_global_queue.try_pop(out)) {
                return true;
            }

            return steal_batch(worker, out);
        }

        [[nodiscard]] bool steal_batch(worker_state& worker, task_pointer& out) noexcept {
            const auto count = static_cast<std::uint32_t>(m_workers.size());

            if(count <= 1) {
                return false;
            }

            auto victim_index = worker.rng.bounded(count);

            for(std::uint32_t i = 0; i != count; ++i) {
                if(victim_index != worker.index) {
                    auto& victim = *m_workers[victim_index];

                    const auto available = victim.local_queue.size();
                    if(available != 0) {
                        const auto desired = std::min(
                            {
                                static_cast<std::size_t>(m_config.steal_batch_size),
                                worker.stolen_tasks.size(),
                                (available + 1) / 2
                            }
                        );

                        auto dst = std::span<task_pointer>(worker.stolen_tasks.data(), desired);
                        const auto stolen = victim.local_queue.steal_batch(dst);

                        if(stolen != 0) {
                            out = worker.stolen_tasks[0];

                            for(std::size_t j = stolen; j-- > 1;) {
                                const auto task = worker.stolen_tasks[j];
                                const auto inserted = worker.local_queue.push(task);

                                if(!inserted) [[unlikely]] {
                                    if(!m_global_queue.try_push(task)) {
                                        execute_task(task);
                                    }
                                }
                            }

                            if(stolen > 1) {
                                wake_one();
                            }
                            return true;
                        }
                    }
                }

                if(++victim_index == count) {
                    victim_index = 0;
                }
            }

            return false;
        }

        void execute_task(task_pointer task) noexcept {
            DEBUG_ASSERT(task != nullptr);

            task->execute();

            const auto remaining = m_pending.fetch_sub(1, std::memory_order::acq_rel) - 1;
            task->release_reference();

            if(remaining == 0) {
                m_pending.notify_all();

                if(m_stopping.load(std::memory_order::acquire)) {
                    wake_all();
                }
            }
        }

        void wait_for(slot_type& awaited) {
            while(!awaited.ready()) {
                if(tls_scheduler == this) {
                    task_pointer task {nullptr};

                    if(try_get_task(*tls_worker, task)) {
                        execute_task(task);
                        continue;
                    }
                }
                awaited.wait();
            }
        }

        [[nodiscard]] bool should_stop() const noexcept {
            return m_stopping.load(std::memory_order::acquire) &&
                   m_pending.load(std::memory_order::acquire) == 0;
        }

        void worker_loop(std::stop_token stop_token, worker_state& worker) noexcept {
            tls_scheduler = this;
            tls_worker = &worker;

            m_ready.count_down();
            backoff bo;

            while(!stop_token.stop_requested()) {
                task_pointer task = nullptr;

                if(try_get_task(worker, task)) {
                    bo.reset();
                    execute_task(task);
                    continue;
                }

                if(should_stop()) {
                    break;
                }

                bo.snooze();

                if(!bo.is_completed()) {
                    continue;
                }

                const auto block = worker.index / 64U;
                const auto bit = std::uint64_t{1} << (worker.index % 64U);

                m_idle_blocks[block].fetch_or(bit, std::memory_order::seq_cst);

                if(try_get_task(worker, task)) {
                    m_idle_blocks[block].fetch_and(~bit, std::memory_order::seq_cst);
                    bo.reset();
                    execute_task(task);
                    continue;
                }

                if(should_stop()) {
                    m_idle_blocks[block].fetch_and(~bit, std::memory_order::seq_cst);
                    break;
                }

                worker.parker.park();

                m_idle_blocks[block].fetch_and(~bit, std::memory_order::seq_cst);

                bo.reset();
            }

            tls_worker = nullptr;
            tls_scheduler = nullptr;
        }

        bool wake_one() noexcept {
            if(m_idle_block_count == 0) {
                return false;
            }
            auto block = m_wake_cursor.load(std::memory_order::relaxed);
            if(block >= m_idle_block_count) {
                block = 0;
            }

            for(std::size_t i = 0; i != m_idle_block_count; ++i) {
                auto bits = m_idle_blocks[block].load(std::memory_order::seq_cst);

                while(bits != 0) {
                    const auto position = std::countr_zero(bits);
                    const auto mask = std::uint64_t{1} << position;

                    if(m_idle_blocks[block].compare_exchange_weak(bits, bits & ~mask, std::memory_order::seq_cst, std::memory_order::relaxed)) {
                        const auto index = static_cast<std::uint32_t>(block * std::size_t{64} + static_cast<std::size_t>(position));

                        if(index < m_workers.size()) {
                            m_wake_cursor.store(block + 1, std::memory_order::relaxed);
                            m_workers[index]->parker.unpark();
                            return true;
                        }
                    }
                }

                if(++block == m_idle_block_count) {
                    block = 0;
                }
            }
            return false;
        }

        void wake_all() noexcept {
            for(auto& worker : m_workers) {
                worker->parker.unpark();
            }
        }

        scheduler_config m_config;
        mpmc::queue<task_pointer, mpmc::wait_mode::backoff_spin> m_global_queue;
        std::vector<std::unique_ptr<worker_state>> m_workers;
        std::vector<std::jthread> m_threads;
        std::latch m_ready;
        const std::size_t m_idle_block_count;
        std::unique_ptr<std::atomic<std::uint64_t>[]> m_idle_blocks;

        alignas(cache_line_size) std::atomic<std::size_t> m_pending{0};
        alignas(cache_line_size) std::atomic<std::uint32_t> m_pool_cursor{0};
        alignas(cache_line_size) std::atomic<std::size_t> m_wake_cursor{0};

        std::atomic<bool> m_accepting{true};
        std::atomic<bool> m_stopping{false};
        std::atomic<bool> m_shutdown_started{false};

        static inline thread_local cpu_scheduler* tls_scheduler = nullptr;
        static inline thread_local worker_state* tls_worker = nullptr;
    };
}
