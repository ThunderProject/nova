module;

#include "libassert/assert.hpp"
#include <concepts>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

export module coro_task;
export import scheduler_work;

namespace nova::coro {
    export template<class T = void>
    class task;

    export template<class T>
    class result_storage {
        static_assert(std::is_object_v<T> && !std::is_const_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
    public:
        template<class U>
        requires std::constructible_from<T, U&&>
        void return_value(U&& value) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
            m_value.emplace(std::forward<U>(value));
        }

        void unhandled_exception() noexcept {
            m_exception = std::current_exception();
        }

        [[nodiscard]] T result() {
            if(m_exception != nullptr) {
                std::rethrow_exception(m_exception);
            }

            DEBUG_ASSERT(m_value.has_value());
            return std::move(*m_value);
        }
    private:
        std::optional<T> m_value;
        std::exception_ptr m_exception;
    };

    export template<>
    class result_storage<void> {
    public:
        void return_void() noexcept {}

        void unhandled_exception() noexcept {
            m_exception = std::current_exception();
        }

        void result() {
            if(m_exception != nullptr) {
                std::rethrow_exception(m_exception);
            }
        }
    private:
        std::exception_ptr m_exception;
    };

    export template<class T>
    class task_promise final : public schedulable_work, public result_storage<T> {
    public:
        using handle = std::coroutine_handle<task_promise>;
        using completion_function = std::coroutine_handle<>(*)(void*) noexcept;

        task_promise() noexcept
            :
            schedulable_work(&task_promise::resume_work)
        {}

        [[nodiscard]] task<T> get_return_object() noexcept;

        [[nodiscard]] static std::suspend_always initial_suspend() noexcept {
            return {};
        }

        struct final_awaiter {
            [[nodiscard]] static bool await_ready() noexcept {
                return false;
            }

            [[nodiscard]] std::coroutine_handle<> await_suspend(const handle current) const noexcept {
                auto& promise = current.promise();

                if(promise.m_completion != nullptr) {
                    const auto completion = promise.m_completion;
                    auto* const context = promise.m_context;
                    return completion(context);
                }

                return promise.m_continuation;
            }

            static void await_resume() noexcept {}
        };

        [[nodiscard]] static final_awaiter final_suspend() noexcept {
            return {};
        }

        void continuation(const std::coroutine_handle<> next) noexcept {
            m_continuation = next;
        }

        void completion(const completion_function function, void* const context) noexcept {
            m_completion = function;
            m_context = context;
        }
    private:
        static void resume_work(schedulable_work& work) noexcept {
            handle::from_promise(static_cast<task_promise&>(work)).resume();
        }

        std::coroutine_handle<> m_continuation{std::noop_coroutine()};
        completion_function m_completion{};
        void* m_context{};
    };

    export template<class T>
    class [[nodiscard]] task {
    public:
        using promise_type = task_promise<T>;
        using promise = promise_type;
        using handle = std::coroutine_handle<promise_type>;
        using value_type = T;

        task() noexcept = default;
        task(const task&) = delete;
        task& operator=(const task&) = delete;

        task(task&& rhs) noexcept
            :
            m_handle(std::exchange(rhs.m_handle, {}))
        {}

        task& operator=(task&& rhs) noexcept {
            if(this != std::addressof(rhs)) {
                reset();
                m_handle = std::exchange(rhs.m_handle, {});
            }
            return *this;
        }

        ~task() noexcept {
            reset();
        }

        [[nodiscard]] bool valid() const noexcept {
            return static_cast<bool>(m_handle);
        }

        class awaiter {
        public:
            explicit awaiter(const handle current) noexcept
                :
                m_handle(current)
            {}

            awaiter(const awaiter&) = delete;
            awaiter& operator=(const awaiter&) = delete;

            ~awaiter() noexcept {
                if(m_started && !m_handle.done()) [[unlikely]] {
                    std::terminate();
                }
                m_handle.destroy();
            }

            [[nodiscard]] static bool await_ready() noexcept {
                return false;
            }

            [[nodiscard]] std::coroutine_handle<> await_suspend(const std::coroutine_handle<> next) noexcept {
                m_started = true;
                m_handle.promise().continuation(next);
                return m_handle;
            }

            T await_resume() {
                return m_handle.promise().result();
            }
        private:
            handle m_handle;
            bool m_started{false};
        };

        [[nodiscard]] awaiter operator co_await() && noexcept {
            DEBUG_ASSERT(valid());
            return awaiter{std::exchange(m_handle, {})};
        }

        awaiter operator co_await() & = delete;

        [[nodiscard]] handle release() && noexcept {
            return std::exchange(m_handle, {});
        }
    private:
        friend class task_promise<T>;

        explicit task(const handle current) noexcept
            :
            m_handle(current)
        {}

        void reset() noexcept {
            if(m_handle != nullptr) {
                m_handle.destroy();
                m_handle = {};
            }
        }

        handle m_handle{};
    };

    template<class T>
    task<T> task_promise<T>::get_return_object() noexcept {
        return task<T>{handle::from_promise(*this)};
    }

    export struct detached_task {
        struct promise_type final : schedulable_work {
            using handle = std::coroutine_handle<promise_type>;

            promise_type() noexcept
                :
                schedulable_work(&promise_type::resume_work)
            {}

            [[nodiscard]] static detached_task get_return_object() noexcept { return {}; }
            [[nodiscard]] static std::suspend_never initial_suspend() noexcept { return {}; }
            [[nodiscard]] static std::suspend_never final_suspend() noexcept { return {}; }
            static void return_void() noexcept {}
            static void unhandled_exception() noexcept { std::terminate(); }
        private:
            static void resume_work(schedulable_work& work) noexcept {
                handle::from_promise(static_cast<promise_type&>(work)).resume();
            }
        };
    };

    template<class>
    struct task_traits;

    template<class T>
    struct task_traits<task<T>> {
        using value_type = T;
    };

    export template<class T>
    concept task_instance = requires {
        typename task_traits<std::remove_cvref_t<T>>::value_type;
    };

    export template<class T>
    using task_value_t = typename task_traits<std::remove_cvref_t<T>>::value_type;

    export template<class F, class... Args>
    requires task_instance<std::invoke_result_t<F&, Args&&...>>
    auto invoke(F function, Args... args) -> task<task_value_t<std::invoke_result_t<F&, Args&&...>>> {
        co_return co_await std::invoke(function, std::move(args)...);
    }
}
