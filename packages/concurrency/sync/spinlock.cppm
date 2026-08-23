module;
#include <atomic>
export module spinlock;

import hints;
import backoff;

namespace nova {
    export enum class spinlock_wait_mode : std::uint8_t {
        busy_wait,
        backoff_spin,
    };

    export template<spinlock_wait_mode BackoffPolicy = spinlock_wait_mode::busy_wait>
    class spinlock {
    public:
        /**
        * @brief Acquires the lock.
        *
        * Blocks the calling thread until the lock becomes available.
        *
        */
        void lock() noexcept {
            if (!m_lock.exchange(true, std::memory_order_acquire)) {
                return;
            }

            if constexpr (BackoffPolicy == spinlock_wait_mode::busy_wait) {
                for (;;) {
                    while (m_lock.load(std::memory_order_relaxed)) {
                        hint::spin_loop();
                    }

                    if (!m_lock.exchange(true, std::memory_order_acquire)) {
                        return;
                    }
                }
            }
            else {
                backoff wait{};
                for (;;) {
                    while (m_lock.load(std::memory_order_relaxed)) {
                        wait.spin();
                    }

                    if (!m_lock.exchange(true, std::memory_order_acquire)) {
                        return;
                    }
                }
            }
        }

        /**
        * @brief Attempts to acquire the lock without blocking.
        *
        * @return @c true if the lock was successfully acquired,
        *         @c false if it is already held by another thread.
        *
        */
        [[nodiscard]] bool try_lock() noexcept {
            return !m_lock.load(std::memory_order_relaxed) && !m_lock.exchange(true, std::memory_order_acquire);
        }

        /**
        * @brief Releases the lock.
        *
        * Makes the lock available for other threads to acquire.
       *
        * @exception: This function does not throw any exceptions.
        */
        void unlock() noexcept {
            m_lock.store(false, std::memory_order_release);
        }
    private:
        std::atomic_bool m_lock = {false};
    };
}