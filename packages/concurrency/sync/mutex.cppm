module;

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>

export module mutex;

import hints;
import thread_parker;

namespace nova {
    constexpr inline std::uint8_t mutex_locked = 0b01;
    constexpr inline std::uint8_t mutex_parked = 0b10;

    constexpr inline std::size_t parking_bucket_count = 256;
    constexpr inline std::size_t parking_bucket_alignment = 128;
    constexpr inline std::uint64_t golden_ratio = 0x9e3779b97f4a7c15ULL;

    static_assert(std::has_single_bit(parking_bucket_count));

    struct waiter {
        const void* key{};
        waiter* prev{};
        waiter* next{};
        thread_parker parker;
    };

    struct alignas(parking_bucket_alignment) parking_bucket {
        std::atomic_flag lock = ATOMIC_FLAG_INIT;
        waiter* head{};
        waiter* tail{};
    };

    constinit inline std::array<parking_bucket, parking_bucket_count> parking_lot{};

    [[nodiscard]] inline parking_bucket& bucket_for(const void* key) noexcept {
        constexpr auto bits = std::countr_zero(parking_bucket_count);
        const auto address = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(key));
        return parking_lot[(address * golden_ratio) >> (64 - bits)];
    }

    inline void lock_bucket(parking_bucket& bucket) noexcept {
        if(!bucket.lock.test_and_set(std::memory_order_acquire)) [[likely]] {
            return;
        }

        do {
            while(bucket.lock.test(std::memory_order_relaxed)) {
                hint::spin_loop();
            }
        }
        while(bucket.lock.test_and_set(std::memory_order_acquire));
    }

    inline void unlock_bucket(parking_bucket& bucket) noexcept {
        bucket.lock.clear(std::memory_order_release);
    }

    inline void enqueue(parking_bucket& bucket, waiter& waiter) noexcept {
        waiter.prev = bucket.tail;

        if(bucket.tail != nullptr) {
            bucket.tail->next = std::addressof(waiter);
        }
        else {
            bucket.head = std::addressof(waiter);
        }
        bucket.tail = std::addressof(waiter);
    }

    [[nodiscard]] inline waiter* dequeue_one(parking_bucket& bucket, const void* key, bool& more) noexcept {
        auto* waiter = bucket.head;

        while((waiter != nullptr) && waiter->key != key) {
            waiter = waiter->next;
        }

        if(waiter == nullptr) {
            more = false;
            return nullptr;
        }

        if(waiter->prev != nullptr) {
            waiter->prev->next = waiter->next;
        }
        else {
            bucket.head = waiter->next;
        }

        if(waiter->next != nullptr) {
            waiter->next->prev = waiter->prev;
        }
        else {
            bucket.tail = waiter->prev;
        }

        more = false;
        for(auto* next = waiter->next; next != nullptr; next = next->next) {
            if(next->key == key) {
                more = true;
                break;
            }
        }
        return waiter;
    }

    void mutex_lock(std::atomic<std::uint8_t>& mutex, std::uint8_t state) noexcept {
        if(state == mutex_locked) {
            for(std::uint32_t step = 0; step != 7; ++step) {
                const auto spins = std::uint32_t{1} << step;

                for(std::uint32_t i = 0; i != spins; ++i) {
                    hint::spin_loop();
                }

                state = mutex.load(std::memory_order_relaxed);

                if((state & mutex_locked) == 0) {
                    if(mutex.compare_exchange_weak(state, static_cast<std::uint8_t>(state | mutex_locked), std::memory_order_acquire, std::memory_order_relaxed)) {
                        return;
                    }
                }
                if((state & mutex_parked) != 0) {
                    break;
                }
            }
        }

        const auto* key = std::addressof(mutex);

        for(;;) {
            state = mutex.load(std::memory_order_relaxed);

            while((state & mutex_locked) == 0) {
                if(mutex.compare_exchange_weak(state, static_cast<std::uint8_t>(state | mutex_locked), std::memory_order_acquire, std::memory_order_relaxed)) {
                    return;
                }
            }

            waiter waiter{.key = key, .parker = {}};
            auto& bucket = bucket_for(key);

            lock_bucket(bucket);
            state = mutex.load(std::memory_order_relaxed);

            for(;;) {
                if(!(state & mutex_locked)) {
                    unlock_bucket(bucket);
                    break;
                }

                if((state & mutex_parked) != 0) {
                    enqueue(bucket, waiter);
                    unlock_bucket(bucket);
                    waiter.parker.park();
                    break;
                }

                if(mutex.compare_exchange_weak(state, static_cast<std::uint8_t>(state | mutex_parked), std::memory_order_relaxed, std::memory_order_relaxed)) {
                    enqueue(bucket, waiter);
                    unlock_bucket(bucket);
                    waiter.parker.park();
                    break;
                }
            }
        }
    }

    void mutex_unlock(std::atomic<std::uint8_t>& mutex) noexcept {
        const auto* key = std::addressof(mutex);
        auto& bucket = bucket_for(key);

        lock_bucket(bucket);

        bool more = false;
        auto* waiter = dequeue_one(bucket, key, more);

        mutex.store(more ? mutex_parked : std::uint8_t{0}, std::memory_order_release);

        unlock_bucket(bucket);

        if(waiter != nullptr) {
            waiter->parker.unpark();
        }
    }

    export class mutex {
    public:
        constexpr mutex() noexcept = default;
        mutex(const mutex&) = delete;
        mutex& operator=(const mutex&) = delete;
        mutex(mutex&&) = delete;
        mutex& operator=(mutex&&) = delete;
        ~mutex() = default;

        void lock() noexcept {
            auto expected = std::uint8_t{0};

            if(m_state.compare_exchange_weak(expected, mutex_locked, std::memory_order_acquire, std::memory_order_relaxed)) [[likely]] {
                return;
            }
            mutex_lock(m_state, expected);
        }

        [[nodiscard]] bool try_lock() noexcept {
            auto state = m_state.load(std::memory_order_relaxed);

            for(;;) {
                if((state & mutex_locked) != 0) {
                    return false;
                }

                if(m_state.compare_exchange_weak(state, static_cast<std::uint8_t>(state | mutex_locked), std::memory_order_acquire, std::memory_order_relaxed)) {
                    return true;
                }
            }
        }

        void unlock() noexcept {
            auto expected = mutex_locked;

            if(m_state.compare_exchange_strong(expected, std::uint8_t{0}, std::memory_order_release, std::memory_order_relaxed)) [[likely]] {
                return;
            }
            mutex_unlock(m_state);
        }

    private:
        std::atomic<std::uint8_t> m_state{0};
    };

    static_assert(std::atomic<std::uint8_t>::is_always_lock_free);
    static_assert(sizeof(mutex) == 1);
    static_assert(alignof(mutex) == 1);
}