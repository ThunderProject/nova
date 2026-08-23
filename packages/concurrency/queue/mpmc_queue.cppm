module;

#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <cstdint>
#include <sys/types.h>
#include <type_traits>
#include <utility>
#include <bit>
#include "libassert/assert.hpp"

export module mpmc_queue;

import backoff;
import hints;

namespace nova::mpmc {
    export enum wait_mode : std::uint8_t {
        busy_wait, // Pure spin-waiting, lowest latency but can waste CPU cycles.
        backoff_spin, // spinning with exponential backoff to reduce contention.
    };

    constexpr inline std::size_t cell_alignment = 128;

    template<class T>
    class alignas(cell_alignment) cell {
        static constexpr bool nothrow_move_assignable = std::is_nothrow_move_assignable_v<T>;
        static constexpr bool nothrow_copy_assignable = std::is_nothrow_copy_assignable_v<T>;
    public:
        template<class... Args>
        static constexpr bool can_set_value = std::conjunction_v<
            std::is_nothrow_constructible<T, Args...>,
            std::disjunction<
                std::bool_constant<nothrow_move_assignable>,
                std::bool_constant<nothrow_copy_assignable>
            >
        >;

        cell() = default;
        cell(const cell&) = delete;
        cell& operator=(const cell&) = delete;
        cell(cell&&) = delete;
        cell& operator=(cell&&) = delete;
        ~cell() = default;

        template<class... Args>
        requires can_set_value<Args...>
        void set_value(Args&&... args) noexcept {
            T value(std::forward<Args>(args)...);

            if constexpr (nothrow_move_assignable) {
                m_data = std::move(value);
            }
            else {
                m_data = value;
            }
        }

        [[nodiscard]] T get_value() noexcept requires(std::is_nothrow_move_constructible_v<T>) {
            return std::move(m_data);
        }

        void get_value(T& out) noexcept requires (nothrow_move_assignable || nothrow_copy_assignable) {
            if constexpr (nothrow_move_assignable) {
                out = std::move(m_data);
            }
            else {
                out = m_data;
            }
        }

        [[nodiscard]] std::size_t load_sequence(const std::memory_order order) noexcept {
            return m_sequence.load(order);
        }

        void store_sequence(const std::size_t value, const std::memory_order order) noexcept {
            m_sequence.store(value, order);
        }
     private:
        T m_data;
        std::atomic<std::size_t> m_sequence{0};
    };

    struct ticket {
        std::size_t index; // position within the ring buffer [0, m_capacity)
        std::size_t cycle; // how many full passes of the ring buffer have been completed (monotonic counter of wraparounds)
    };

    class ticker_dispenser {
    public:
        explicit ticker_dispenser(std::size_t capacity) noexcept
            :
            m_mask(capacity -1),
            m_shift(static_cast<std::size_t>(std::countr_zero(capacity)))
        {}

        [[nodiscard]] ticket next_producer() noexcept {
            // Relaxed order here is fine: head is just a ticket counter.
            // Ordering is enforced by the cell class
            return compute_ticket(m_head.fetch_add(1, std::memory_order_relaxed));
        }

        [[nodiscard]] ticket next_consumer() noexcept {
            // Relaxed order here is fine: head is just a ticket counter.
            // Ordering is enforced by the cell class
            return compute_ticket(m_tail.fetch_add(1, std::memory_order_relaxed));
        }

        [[nodiscard]] ticket compute_ticket(const std::size_t position) const noexcept {
            return {
                .index = position & m_mask,
                .cycle = position >> m_shift
            };
        }

        [[nodiscard]]std::atomic<std::size_t>& head() noexcept { return m_head; }
        [[nodiscard]]std::atomic<std::size_t>& tail() noexcept { return m_tail; }

        [[nodiscard]] std::size_t load_head() const noexcept { return m_head.load(std::memory_order_relaxed); }
        [[nodiscard]] std::size_t load_tail() const noexcept { return m_tail.load(std::memory_order_relaxed); }
    private:
        const std::size_t m_mask;
        const std::size_t m_shift;

        alignas(128) std::atomic_size_t m_head{0};
        alignas(128) std::atomic_size_t m_tail{0};
    };

    /**
    * @brief multi-producer, multi-consumer lock-free queue.
    *
    * This queue supports multiple producer threads (calling \c push / \c try_push / \c emplace / \c try_emplace)
    * and multiple consumer threads (calling \c pop / \c try_pop).
    * A thread may also freely switch between being a producer and consumer
    * @tparam T         Item type.
    * @tparam WaitMode  The WaitMode to use
    * @tparam Allocator Allocator type for storage.
    */
    export template<
        class T,
        wait_mode WaitMode = wait_mode::backoff_spin,
        class Allocator = std::allocator<cell<T>>
    >
    class queue {
        static_assert(std::atomic_size_t::is_always_lock_free);
        static_assert(std::is_nothrow_destructible_v<T>);

        using allocator_traits = std::allocator_traits<Allocator>;

        static_assert(std::same_as<typename allocator_traits::value_type, cell<T>>);
        static_assert(std::default_initializable<T>);
        static_assert(std::atomic<std::size_t>::is_always_lock_free);
    public:
        /**
        * @brief Construct a queue with a given logical capacity.
        * @param capacity Maximum number of elements that can be stored concurrently. Must be a power of two
        * @param allocator  Allocator instance for the underlying storage.
        */
        explicit queue(const std::size_t capacity, Allocator allocator = {})
            :
            m_capacity(capacity),
            m_allocator(std::move(allocator)),
            m_ticket_dispenser(capacity)
        {
            DEBUG_ASSERT(std::has_single_bit(m_capacity), "Capacity must be a non-zero power of two");
            m_buffer = allocator_traits::allocate(m_allocator, m_capacity);

            try {
                std::ranges::uninitialized_default_construct(m_buffer, m_buffer + m_capacity);
            }
            catch(...) {
                allocator_traits::deallocate(m_allocator, m_buffer, m_capacity);
                throw;
            }
        }

        queue(const queue&) = delete;
        queue& operator=(const queue&) = delete;
        queue(queue&&) = delete;
        queue& operator=(queue&&) = delete;

        ~queue() noexcept {
            std::ranges::destroy(m_buffer, m_buffer + m_capacity);
            allocator_traits::deallocate(m_allocator, m_buffer, m_capacity);
        }
        
         /**
        * @brief Push an item into the queue.
        * @param item The item to push into the queue.
        * @note Blocks if the queue is full until space is available.
        */
        template<class U>
        requires cell<T>::template can_set_value<U&&>
        void push(U&& item) noexcept {
            emplace(std::forward<U>(item));
        }

        /**
        * @brief Tries to push an item into the queue without blocking.
        * @param item The item to push into the queue.
        * @return \c true if the item was enqueued, otherwise \c false.
        */
        template<class U>
        requires cell<T>::template can_set_value<U&&>
        [[nodiscard]] bool try_push(U&& item) noexcept {
            return try_emplace(std::forward<U>(item));
        }

         /**
        * @brief In-place construct and push the item into the queue.
        * @tparam Args Constructor argument types for \c T.
        * @param args  Arguments forwarded to \c T's constructor.
        * @note Blocks if the queue is full until space is available.
        */
        template<class... Args>
        requires cell<T>::template can_set_value<Args&&...>
        void emplace(Args&&... args) noexcept {
            const auto[index, cycle] = m_ticket_dispenser.next_producer();

            auto& cell = m_buffer[index];
            const auto sequence = cycle * 2;

            // Wait until the cell is free to write too.
            // wait_for_sequence uses std::memory_order_acquire under the hood, which prevents cell.set_value(...)
            // from being hoisted/reordered before this load
            wait_for_sequence(cell, sequence);

            cell.set_value(std::forward<Args>(args)...);

            // publish the value. std::memory_order_release prevents cell.set_value(...) from being reordered below this store
            cell.store_sequence(sequence + 1, std::memory_order_release);
        }

        /**
         * @brief In-place construct and tries to push the item into the queue.
         * @tparam Args Constructor argument types for \c T.
         * @param args  Arguments forwarded to \c T's constructor.
         * @return \c true if the item was enqueued, otherwise \c false.
         */
        template<class... Args>
        requires cell<T>::template can_set_value<Args&&...>
        [[nodiscard]] bool try_emplace(Args&&... args) noexcept {
            // Relaxed order here is fine: head is just a ticket counter. Ordering is enforced by the cell class
            auto& head = m_ticket_dispenser.head();
            auto expected = head.load(std::memory_order_relaxed);

            for(;;) {
                const auto[index, cycle] = m_ticket_dispenser.compute_ticket(expected);
                auto& cell = m_buffer[index];
                const auto sequence = cycle * 2;

                // std::memory_order_acquire prevents the write from being reordered above this load, and
                // it establishes a happens-before relationship with any previous pops, so we know that the consumer is
                // done reading at this index we will write too.
                if(cell.load_sequence(std::memory_order_acquire) == sequence) [[likely]] {
                    // Relaxed order here is fine: head is just a ticket counter. Ordering is enforced by the cell class
                    if(head.compare_exchange_weak(expected, expected + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
                        cell.set_value(std::forward<Args>(args)...);

                        // std::memory_order_release prevents the write to be reordered below this store, and
                        // it establishes a happens-before relationship with later pop's/try_pop's
                        cell.store_sequence(sequence + 1, std::memory_order_release);
                        return true;
                    }
                }
                else {
                    // Relaxed order here is fine: head is just a ticket counter. Ordering is enforced by the cell class
                    const auto prev = std::exchange(expected, head.load(std::memory_order_relaxed));
                    if(expected == prev) {
                        return false;
                    }
                }
            }
        }

        /**
        * @brief Pop and return the next item from the queue.
        * @return The next available item.
        * @note Blocks until an item becomes available.
        */
        [[nodiscard]] T pop() noexcept requires std::is_nothrow_move_constructible_v<T> {
            const auto [index, cycle] = m_ticket_dispenser.next_consumer();
            auto& cell = m_buffer[index];

            const auto sequence = (cycle * 2) + 1;

            // Wait until the slot has been filled.
            // wait_for_sequence uses std::memory_order_acquire under the hood, which establishes
            // a happens-before relationship with the producer's release (publish → consume)
            wait_for_sequence(cell, sequence);

            auto value = cell.get_value();

            // mark the slot as free. std::memory_order_release prevents cell.get_value() from being reordered below this store.
            cell.store_sequence(sequence + 1, std::memory_order_release);

            return value;
        }

        /**
        * @brief Pop and write the next item from the queue to the provided reference.
        * @return @param out Reference to receive the next available item.
        * @note Blocks until an item becomes available.
        */
        void pop(T& out) noexcept 
        requires(std::is_nothrow_move_assignable_v<T> || std::is_nothrow_copy_assignable_v<T>) {
            const auto [index, cycle] = m_ticket_dispenser.next_consumer();

            auto& cell = m_buffer[index];

            const auto sequence = (cycle * 2) + 1;

            // Wait until the slot has been filled.
            // wait_for_sequence uses std::memory_order_acquire under the hood, which establishes
            // a happens-before relationship with the producer's release (publish → consume)
            wait_for_sequence(cell, sequence);

            cell.get_value(out);

            // mark the slot as free. std::memory_order_release prevents cell.get_value() from being reordered below this store.
            cell.store_sequence(sequence + 1, std::memory_order_release);
        }

        /**
        * @brief Try to pop an item from the queue without blocking.
        * @return The item if available, otherwise \c std::nullopt.
        */
        [[nodiscard]] std::optional<T> try_pop() noexcept requires std::is_nothrow_move_constructible_v<T> {
            // Relaxed order here is fine: tail is just a ticket counter.
            // Ordering is enforced by the cell object
            auto& tail = m_ticket_dispenser.tail();
            auto expected = tail.load(std::memory_order_relaxed);

            for(;;) {
                const auto [index, cycle] = m_ticket_dispenser.compute_ticket(expected);

                auto& cell = m_buffer[index];
                const auto sequence = (cycle * 2) + 1;

                // std::memory_order_acquire prevents the read from being reordered above this load, and
                // it establishes a happens-before relationship with any previous push, so we know that the producer is
                // done writing to this index.
                if(cell.load_sequence(std::memory_order_acquire) == sequence) [[likely]] {
                    // Relaxed order here is fine: head is just a ticket counter. Ordering is enforced by the cell class
                    if(tail.compare_exchange_weak(expected, expected + 1, std::memory_order::relaxed, std::memory_order::relaxed)) {
                        std::optional<T> value{std::in_place, cell.get_value()};

                        // std::memory_order_release prevents the read to be reordered below this store, and
                        // it establishes a happens-before relationship with later push/try_push
                        cell.store_sequence(sequence + 1, std::memory_order::release);
                        return value;
                    }
                }
                else {
                    // Relaxed order here is fine: head is just a ticket counter. Ordering is enforced by the cell class
                    const auto prev = std::exchange(expected, tail.load(std::memory_order::relaxed));

                    if(expected == prev) {
                        return std::nullopt;
                    }
                }
            }
        }

        /**
        * @brief Try to pop an item from the queue without blocking.
        * @param out Reference to receive the item if available
        * @return true if an item was available and written into the output reference, otherwise false.
        * @note the item reference is only written too if the function returns true.
        */
        [[nodiscard]] bool try_pop(T& out) noexcept
        requires(std::is_nothrow_move_assignable_v<T> || std::is_nothrow_copy_assignable_v<T>) {
            // Relaxed order here is fine: tail is just a ticket counter.
            // Ordering is enforced by the cell object
            auto& tail = m_ticket_dispenser.tail();
            auto expected = tail.load(std::memory_order::relaxed);

            for(;;) {
                const auto [index, cycle] = m_ticket_dispenser.compute_ticket(expected);

                auto& cell = m_buffer[index];

                const auto sequence = (cycle * 2) + 1;

                // std::memory_order_acquire prevents the read from being reordered above this load, and
                // it establishes a happens-before relationship with any previous push, so we know that the producer is
                // done writing to this index.
                if(cell.load_sequence(std::memory_order::acquire) == sequence) [[likely]] {
                    // Relaxed order here is fine: head is just a ticket counter. Ordering is enforced by the cell class
                    if(tail.compare_exchange_weak(expected, expected + 1, std::memory_order::relaxed, std::memory_order::relaxed)) {
                        cell.get_value(out);

                        // std::memory_order_release prevents the read to be reordered below this store, and
                        // it establishes a happens-before relationship with later push/try_push
                        cell.store_sequence(sequence + 1, std::memory_order::release);
                        return true;
                    }
                }
                else {
                    // Relaxed order here is fine: head is just a ticket counter. Ordering is enforced by the cell class
                    const auto prev = std::exchange(expected, tail.load(std::memory_order::relaxed));
                    if(expected == prev) {
                        return false;
                    }
                }
            }
        }

         /**
        * @brief Returns current size (approximate, due to concurrency).
        *
        * @return The number of elements logically in the deque
        */
        [[nodiscard]] std::size_t size() const noexcept {
            const auto head = m_ticket_dispenser.load_head();
            const auto tail = m_ticket_dispenser.load_tail();

            if(head <= tail) {
                return 0;
            }

            return std::min(head - tail, m_capacity);
        }

        /**
        * @brief Checks if the queue is empty (approximate, due to concurrency).
        * @return true if the queue appears empty, otherwise false.
        */
        [[nodiscard]] bool empty() const noexcept {
            return size() == 0;
        }

        /**
        * @brief Returns the current capacity of the queue.
        *
        * @return The capacity of the queue.
        */
        [[nodiscard]] std::size_t capacity() const noexcept { 
            return m_capacity; 
        }
    private:
        static void wait_for_sequence(cell<T>& cell, std::size_t expected) noexcept {
            if(cell.load_sequence(std::memory_order_acquire) == expected) {
                return;
            }

            if constexpr (WaitMode == wait_mode::busy_wait) {
                do {
                    hint::spin_loop();
                } while(cell.load_sequence(std::memory_order_acquire) != expected);
            }
            else {
                backoff bo;

                do {
                    bo.spin();
                } while(cell.load_sequence(std::memory_order_acquire) != expected);
            }
        }
        const std::size_t m_capacity;

        [[no_unique_address]] Allocator m_allocator;

        cell<T>* m_buffer = nullptr;
        ticker_dispenser m_ticket_dispenser;
    };
}

