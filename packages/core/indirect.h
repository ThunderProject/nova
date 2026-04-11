#pragma once

#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>
#include "libassert/assert.hpp"

namespace nova {
    namespace detail {
        template <class T, class U>
        constexpr auto synth_three_way(const T& t, const U& u)
        requires requires {
            { t < u } -> std::convertible_to<bool>;
            { u < t } -> std::convertible_to<bool>;
        }
        {
            if constexpr (std::three_way_comparable_with<T, U>) {
                return t <=> u;
            }
            else {
                if (t < u) {
                    return std::weak_ordering::less;
                }
                if (u < t) {
                    return std::weak_ordering::greater;
                }
                return std::weak_ordering::equivalent;
            }
        }

        template <class T, class U = T>
        using synth_three_way_result = decltype(synth_three_way(std::declval<const T&>(), std::declval<const U&>()));
    }


    template <class T, class A>
    class indirect;

    template <class>
    inline constexpr bool is_indirect_v = false;

    template <class T, class A>
    inline constexpr bool is_indirect_v<indirect<T, A>> = true;

    template<class T, class A = std::allocator<T>>
    class indirect {
        using allocator_traits = std::allocator_traits<A>;
    public:
        using value_type = T;
        using allocator_type = A;
        using pointer = typename allocator_traits::pointer;
        using const_pointer = typename allocator_traits::const_pointer;

        explicit constexpr indirect() requires std::default_initializable<allocator_type>
            :
            m_allocator()
        {
            static_assert(std::default_initializable<T>);
            m_ptr = construct_from(m_allocator);
        }

        template <class U = T>
        explicit constexpr indirect(U&& u)
        requires(
            !std::same_as<std::remove_cvref_t<U>, indirect> &&
            !std::same_as<std::remove_cvref_t<U>, std::in_place_t> &&
            std::constructible_from<T, U> && std::default_initializable<allocator_type>
        )
            :
            m_allocator()
        {
            m_ptr = construct_from(m_allocator, std::forward<U>(u));
        }

        template <class... Us>
        explicit constexpr indirect(std::in_place_t, Us&&... us)
        requires std::constructible_from<T, Us&&...> && std::default_initializable<A>
            :
            m_allocator()
        {
            m_ptr = construct_from(m_allocator, std::forward<Us>(us)...);
        }


        template<class U = T, class... Us>
        explicit constexpr indirect(std::in_place_t, std::initializer_list<U> init, Us&&... us)
        requires std::constructible_from<T, std::initializer_list<U>, Us...> && std::default_initializable<A>
            :
            m_allocator()
        {
            m_ptr = construct_from(m_allocator, init, std::forward<Us>(us)...);
        }

        constexpr indirect(const indirect& rhs)
            :
            indirect(std::allocator_arg, allocator_traits::select_on_container_copy_construction(rhs.m_allocator), rhs)
        {
            static_assert(std::copy_constructible<T>);
        }

        constexpr indirect(indirect&& rhs) noexcept(allocator_traits::is_always_equal::value)
            :
            indirect(std::allocator_arg, rhs.m_allocator, std::move(rhs)) {}

        explicit constexpr indirect(std::allocator_arg_t, const allocator_type& allocator)
            :
            m_allocator(allocator)
        {
            m_ptr = construct_from(allocator);
        }

        template <class U = T>
        explicit constexpr indirect(std::allocator_arg_t, const allocator_type& allocator, U&& u)
        requires(
            !std::same_as<std::remove_cvref_t<U>, indirect> &&
            !std::same_as<std::remove_cvref_t<U>, std::in_place_t> &&
            std::constructible_from<T, U>
        )
            :
            m_allocator(allocator)
        {
            m_ptr = construct_from(m_allocator, std::forward<U>(u));
        }

        template <class U = T>
        explicit constexpr indirect(std::allocator_arg_t, const allocator_type& allocator, std::in_place_t, U&& u)
        requires(!std::same_as<std::remove_cvref_t<U>, indirect> && std::constructible_from<T, U>)
            :
            m_allocator(allocator)
        {
           m_ptr = construct_from(m_allocator, std::forward<U>(u));
        }

        template <class... Us>
        explicit constexpr indirect(std::allocator_arg_t, const allocator_type& allocator, std::in_place_t, Us&&... us)
        requires std::constructible_from<T, Us&&...>
            :
            m_allocator(allocator)
        {
            m_ptr = construct_from(m_allocator, std::forward<Us>(us)...);
        }

        template <class U = T, class... Us>
        explicit constexpr indirect(std::allocator_arg_t, const A& allocator, std::in_place_t, std::initializer_list<U> init, Us&&... us)
        requires std::constructible_from<T, std::initializer_list<U>&, Us...>
            :
            m_allocator(allocator)
        {
            m_ptr = construct_from(allocator, init, std::forward<Us>(us)...);
        }

        constexpr indirect(std::allocator_arg_t, const allocator_type& allocator, const indirect& rhs)
            :
            m_allocator(allocator)
        {
            static_assert(std::copy_constructible<T>);

            m_ptr = rhs.valueless_after_move()
                ? nullptr
                : construct_from(m_allocator, *rhs);
         }

        constexpr indirect(std::allocator_arg_t, const allocator_type& allocator, indirect&& rhs) noexcept(allocator_traits::is_always_equal::value)
            :
            m_ptr(nullptr),
            m_allocator(allocator)
        {
            static_assert(std::move_constructible<T>);

            if constexpr (allocator_traits::is_always_equal::value) {
                m_ptr = std::exchange(rhs.m_ptr, nullptr);
                return;
            }

            if (m_allocator == rhs.m_allocator) {
                m_ptr = std::exchange(rhs.m_ptr, nullptr);
                return;
            }

            if (!rhs.valueless_after_move()) {
                m_ptr = construct_from(m_allocator, std::move(*rhs));
                rhs.reset();
            }
         }

        constexpr ~indirect() { reset(); }

        constexpr indirect& operator=(const indirect& rhs) {
            static_assert(std::copy_constructible<T>);

            if(this == &rhs) {
                return *this;
            }

            constexpr bool propagate_alloc = allocator_traits::propagate_on_container_copy_assignment::value;
            auto& target_alloc = propagate_alloc
                ? rhs.m_allocator
                : m_allocator;

            if (rhs.valueless_after_move()) {
                reset();
                if constexpr (propagate_alloc) {
                    m_allocator = rhs.m_allocator;
                }
                return *this;
            }

            if constexpr (std::assignable_from<T&, const T&>) {
                if (!valueless_after_move() && m_allocator == rhs.m_allocator) {
                    *m_ptr = *rhs.m_ptr;
                    if constexpr (propagate_alloc) {
                        m_allocator = rhs.m_allocator;
                    }
                    return *this;
                }
            }

            auto* tmp = construct_from(target_alloc, *rhs.m_ptr);
            reset();
            m_ptr = tmp;

           if constexpr (propagate_alloc) {
               m_allocator = rhs.m_allocator;
           }

           return *this;
        }

        constexpr indirect& operator=(indirect&& rhs)
            noexcept(
                allocator_traits::propagate_on_container_move_assignment::value ||
                allocator_traits::is_always_equal::value
            )
        {
            static_assert(std::move_constructible<T>);

            if (this == &rhs) {
                return *this;
            }

            constexpr bool propagate_alloc = allocator_traits::propagate_on_container_move_assignment::value;

            if (rhs.valueless_after_move()) {
                reset();

                if constexpr (propagate_alloc) {
                    m_allocator = std::move(rhs.m_allocator);
                }
                return *this;
            }

            if (m_allocator == rhs.m_allocator) {
                reset();
                m_ptr = std::exchange(rhs.m_ptr, nullptr);

                if constexpr (propagate_alloc) {
                    m_allocator = std::move(rhs.m_allocator);
                }
                return *this;
            }

            auto* tmp = construct_from(propagate_alloc ? rhs.m_allocator : m_allocator, std::move(*rhs.m_ptr));
            reset();
            rhs.reset();
            m_ptr = tmp;

            if constexpr (propagate_alloc) {
                m_allocator = std::move(rhs.m_allocator);
            }
            return *this;
        }

        template <class U>
        constexpr indirect& operator=(U&& u)
        requires(
            !std::same_as<std::remove_cvref_t<U>, indirect> &&
            std::constructible_from<T, U> && std::assignable_from<T&, U>
        )
        {
            if (valueless_after_move()) {
                m_ptr = construct_from(m_allocator, std::forward<U>(u));
                return *this;
            }

            *m_ptr = std::forward<U>(u);
            return *this;
        }

        [[nodiscard]] constexpr const T& operator*() const& noexcept {
            DEBUG_ASSERT(!valueless_after_move());
            return *m_ptr;
          }

        [[nodiscard]] constexpr T& operator*() & noexcept {
            DEBUG_ASSERT(!valueless_after_move());

            return *m_ptr;
        }

        [[nodiscard]] constexpr T&& operator*() && noexcept {
            DEBUG_ASSERT(!valueless_after_move());
            return std::move(*m_ptr);
        }

        [[nodiscard]] constexpr const T&& operator*() const&& noexcept {
            DEBUG_ASSERT(!valueless_after_move());
            return std::move(*m_ptr);
        }

        [[nodiscard]] constexpr const_pointer operator->() const noexcept {
            DEBUG_ASSERT(!valueless_after_move());
            return m_ptr;
        }

        [[nodiscard]] constexpr pointer operator->() noexcept {
            DEBUG_ASSERT(!valueless_after_move());
            return m_ptr;
        }

        [[nodiscard]] constexpr allocator_type get_allocator() const noexcept { return m_allocator; }

        [[nodiscard]] constexpr bool valueless_after_move() const noexcept {
            return m_ptr == nullptr;
        }

        constexpr void swap(indirect& rhs)
            noexcept(
                allocator_traits::propagate_on_container_swap::value ||
                allocator_traits::is_always_equal::value
            )
        {
            if constexpr (allocator_traits::propagate_on_container_swap::value) {
                std::swap(m_allocator, rhs.m_allocator);
                std::swap(m_ptr, rhs.m_ptr);
                return;
            }
            else {
                if (m_allocator == rhs.m_allocator) {
                    std::swap(m_ptr, rhs.m_ptr);
                }
                else {
                    std::unreachable();
                }
            }
          }

        friend constexpr void swap(indirect& lhs, indirect& rhs) noexcept(noexcept(lhs.swap(rhs))) {
            lhs.swap(rhs);
        }


        template <class U, class A2>
        [[nodiscard]] friend constexpr bool operator==(const indirect<T, A>& lhs, const indirect<U, A2>& rhs) noexcept(noexcept(*lhs == *rhs)) {
            if (lhs.valueless_after_move()) {
                return rhs.valueless_after_move();
            }
            if (rhs.valueless_after_move()) {
                return false;
            }
            return *lhs == *rhs;
        }

        template <class U>
        [[nodiscard]] friend constexpr bool operator==(const indirect<T, A>& lhs, const U& rhs) noexcept(noexcept(*lhs == rhs))
            requires(!is_indirect_v<U>)
        {
            if (lhs.valueless_after_move()) {
                return false;
            }
            return *lhs == rhs;
        }

        template <class U, class A2>
        [[nodiscard]] friend constexpr auto operator<=>(const indirect<T, A>& lhs, const indirect<U, A2>& rhs) -> detail::synth_three_way_result<T, U> {
            if (lhs.valueless_after_move() || rhs.valueless_after_move()) {
                return !lhs.valueless_after_move() <=> !rhs.valueless_after_move();
            }
            return detail::synth_three_way(*lhs, *rhs);
        }

        template <class U>
        [[nodiscard]] friend constexpr auto operator<=>(const indirect<T, A>& lhs, const U& rhs) -> auto requires(!is_indirect_v<U>) {
            return [](const auto& lhs2, const auto& rhs2) -> detail::synth_three_way_result<T, U> {
                if (lhs2.valueless_after_move()) {
                    return std::strong_ordering::less;
                }
                return detail::synth_three_way(*lhs2, rhs2);
            }(lhs, rhs);
        }
    private:
        template<class... Args>
        [[nodiscard]] constexpr static pointer construct_from(allocator_type allocator, Args&&... args) {
            pointer memory = allocator_traits::allocate(allocator, 1);

            try {
                allocator_traits::construct(allocator, std::to_address(memory), std::forward<Args>(args)...);
                return memory;
            }
            catch(...) {
                allocator_traits::deallocate(allocator, memory, 1);
                throw;
            }
        }

        constexpr static void destroy_with(allocator_type allocator, pointer ptr) {
            allocator_traits::destroy(allocator, std::to_address(ptr));
            allocator_traits::deallocate(allocator, ptr, 1);
        }

        constexpr void reset() noexcept {
            if (auto ptr = std::exchange(m_ptr, nullptr)) {
                   destroy_with(m_allocator, ptr);
            }
        }

        pointer m_ptr;
        [[no_unique_address]] allocator_type m_allocator;
    };

    template <class T>
    concept is_hashable = requires(T t) { std::hash<T>{}(t); };

    template <typename Value>
    indirect(Value) -> indirect<Value>;

    template <typename Alloc, typename Value>
    indirect(std::allocator_arg_t, Alloc, Value) -> indirect<Value, typename std::allocator_traits<Alloc>::template rebind_alloc<Value>>;
}

template <class T, class Alloc> requires nova::is_hashable<T>
struct std::hash<nova::indirect<T, Alloc>> {
    constexpr std::size_t operator()(const nova::indirect<T, Alloc>& key) const {
        if (key.valueless_after_move()) {
            return static_cast<std::size_t>(-1);
        }
        return std::hash<typename nova::indirect<T, Alloc>::value_type>{}(*key);
    }
};
