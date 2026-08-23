module;

#include <cstdint>
#include <semaphore>
#include <chrono>
#include <atomic>

export module thread_parker;

namespace nova {
    template<class Rep, class Period>
    concept is_duration = requires {
        typename std::chrono::duration<Rep, Period>;
        requires std::is_arithmetic_v<Rep>;
    };

    /**
    * @brief Lightweight utility for blocking and waking threads.
    *
    * A parked thread remains blocked until it is explicitly unparked from
    * another thread, or an optional timeout elapses.
    *
    * Typical usage:
    * - A thread calls `park()` (or one of the timed variants) to wait until
    *   it should resume execution.
    * - Another thread calls `unpark()` to wake the parked thread.
    *
    * This is often used in synchronization primitives or task schedulers to
    * efficiently block idle threads until new work is available.
    */
    export class thread_parker {
    public:
        /**
         * @brief Parks the calling thread until a token is available.
         *
         * Blocks unless or until the token has been made available via `unpark()`.
         *
         * `unpark()` followed by `park()` guarantees that this call returns immediately.
         *
         * Memory ordering:
         *  - `unpark()` synchronizes-with this `park()`, guaranteeing that memory operations
         *     before `unpark()` are visible after `park()`      
        */
        void park() noexcept {
            if(prepare_wait()) {
                return;
            }

            m_token.acquire();
            consume_wake();
        }

        /**
        * @brief Parks the calling thread for up to the specified duration.
        *
        * If a token is available, it is consumed immediately and `true` is returned.
        * If no token is available and timeout is zero or negative, returns `false` without blocking.
        * Otherwise, blocks until `unpark()` is called or the timeout duration has been exceeded.
        *
        * Memory Ordering:
        * - `unpark()` synchronizes-with this `park_for()`,
        * guaranteeing that memory operations before `unpark()` are visible after `park_for()`.
        *
        * @return `true` if a token was acquired, `false` on timeout.
        *
        */
        template<class Rep, class Period>
        requires is_duration<Rep, Period>
        [[nodiscard]] bool park_for(const std::chrono::duration<Rep, Period>& timeout) noexcept {
            if(try_consume()) {
                return true;
            }

            // If the timeout is zero or negative, then there is no need to actually block
            if(timeout <= std::chrono::duration<Rep, Period>::zero()) {
                return false;
            }

            if(prepare_wait()) {
                return true;
            }

            if(m_token.try_acquire_for(timeout)) {
                consume_wake(); 
                return true;
            }

            return cancel_wait();
        }

        /**
        * @brief Parks the calling thread until a token is available, but only up until the specified time point.
        *
        * If a token is available, it is consumed immediately and `true` is returned.
        * Otherwise, blocks until `unpark()` is called or the time point is reached.
        *
        * Memory Ordering:
        * - `unpark()` synchronizes-with this `park_until()`, guaranteeing that memory
        *   operations before `unpark()` are visible after `park_until()` returns `true`.
        *
        * @return `true` if a token was acquired, `false` on timeout.
        *
        */
        template<class Clock, class Duration>
        requires std::chrono::is_clock_v<Clock>
        [[nodiscard]] bool park_until(const std::chrono::time_point<Clock, Duration>& timePoint) noexcept {
            if(try_consume()) {
                return true;
            }

            if(timePoint <= Clock::now()) {
                return false;
            }

            if(prepare_wait()) {
                return true;
            }

            if(m_token.try_acquire_until(timePoint)) {
                consume_wake();
                return true;
            }
            return cancel_wait();
        }

        /**
        * @brief Atomically makes the token available if it is not already
        *
        * This method will wake up the thread blocked on`park` or `park_for`, if there is any
        *
        * This operation strongly happens-before invocations of `acquire()` or `try_acquire()` that observe its effects.
        */
        void unpark() noexcept {
            const auto prev = m_state.exchange(state::notified, std::memory_order_release);

            if(prev == state::waiting) {
                m_token.release();
            }
        }
    private:
        enum class state : uint8_t {
            empty,
            waiting,
            notified
        };

        [[nodiscard]] bool try_consume() noexcept {
            auto expected = state::notified;

            return m_state.compare_exchange_strong(
                expected,
                state::empty,
                std::memory_order_acquire, 
                std::memory_order_relaxed
            );
        }

        void consume_wake() noexcept {
            m_state.exchange(state::empty, std::memory_order_acquire);
        }

        [[nodiscard]] bool prepare_wait() noexcept {
            for(;;) {
                auto expected = state::empty;

                if(m_state.compare_exchange_weak(expected, state::waiting, std::memory_order_relaxed, std::memory_order_relaxed)) {
                    return false;
                }

                if(expected != state::notified) {
                    continue;
                }

                expected = state::notified;

                if(m_state.compare_exchange_weak(expected, state::empty, std::memory_order_acquire, std::memory_order_relaxed)) {
                    return true;
                }
            }
        }

        [[nodiscard]] bool cancel_wait() noexcept {
            auto expected = state::waiting;

            if(m_state.compare_exchange_strong(expected, state::empty, std::memory_order_relaxed, std::memory_order_relaxed)) {
                return false;
            }

            m_token.acquire();
            consume_wake();
            return true;
        }

        std::binary_semaphore m_token{0};
        std::atomic<state> m_state{state::empty};
    };
}
