module;

#include <concepts>
#include <coroutine>
#include <memory>

export module scheduler_work;

namespace nova {
    export class schedulable_work {
    public:
        using dispatch_function = void(*)(schedulable_work&) noexcept;

        schedulable_work(const schedulable_work&) = delete;
        schedulable_work& operator=(const schedulable_work&) = delete;

        void dispatch() noexcept { m_dispatch(*this); }

        schedulable_work* next_work{};

    protected:
        explicit constexpr schedulable_work(const dispatch_function dispatch) noexcept
            :
            m_dispatch(dispatch)
        {}
        ~schedulable_work() = default;
    private:
        dispatch_function m_dispatch;
    };

    export template<class Promise>
    concept schedulable_promise = std::derived_from<Promise, schedulable_work>;

    export template<schedulable_promise Promise>
    [[nodiscard]] schedulable_work* as_work(const std::coroutine_handle<Promise> handle) noexcept {
        return std::addressof(static_cast<schedulable_work&>(handle.promise()));
    }
}