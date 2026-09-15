#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace nova {
    template<class F>
    requires std::is_nothrow_invocable_v<F&>
    class scope_exit final {
    public:
        explicit scope_exit(F fn) noexcept(std::is_nothrow_move_constructible_v<F>)
            : 
            m_fn(std::move(fn)) 
        {}

        scope_exit(const scope_exit&) = delete;
        scope_exit& operator=(const scope_exit&) = delete;

        scope_exit(scope_exit&& rhs) noexcept(std::is_nothrow_move_constructible_v<F>)
            : 
            m_fn(std::move(rhs.m_fn)),
            m_active(std::exchange(rhs.m_active, false))
        {}

        scope_exit& operator=(scope_exit&&) = delete;

        ~scope_exit() {
            if(m_active) {
                std::invoke(m_fn);
            }
        }

        void release() noexcept {
            m_active = false;
        }

    private:
        [[no_unique_address]] F m_fn;
        bool m_active{true};
    };

    template<class F>
    scope_exit(F) -> scope_exit<std::decay_t<F>>;
}
