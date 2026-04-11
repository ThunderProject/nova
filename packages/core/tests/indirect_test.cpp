#include "indirect.h"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

template<class T>
class tracking_allocator {
public:
    using value_type = T;

    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap            = std::false_type;
    using is_always_equal                        = std::false_type;

    tracking_allocator() = default;
    tracking_allocator(std::size_t* allocations, std::size_t* deallocations, int32_t tag)
        :
        allocations(allocations),
        deallocations(deallocations),
        tag(tag)
    {}


    template<typename U>
    tracking_allocator(const tracking_allocator<U>& rhs) noexcept
        :
        allocations(rhs.allocations),
        deallocations(rhs.deallocations),
        tag(rhs.tag)
    {}

    [[nodiscard]] T* allocate(std::size_t n) {
        if(allocations != nullptr) {
            *allocations += n;
        }
        return std::allocator<T>{}.allocate(n);
    }

    void deallocate(T* ptr, std::size_t n) {
        if(deallocations != nullptr) {
            *deallocations += n;
        }
        std::allocator<T>{}.deallocate(ptr, n);
    }

    template<class U>
    struct rebind {
        using other = tracking_allocator<U>;
    };

    template<typename U>
    friend class tracking_allocator;

    friend bool operator==(const tracking_allocator& lhs, const tracking_allocator& rhs) noexcept {
        return lhs.tag == rhs.tag;
    }

    friend bool operator!=(const tracking_allocator& lhs, const tracking_allocator& rhs) noexcept {
        return !(lhs == rhs);
    }

    std::size_t* allocations{};
    std::size_t* deallocations{};
    int32_t tag{};
};

struct initializer_list_constructible {
    initializer_list_constructible(std::initializer_list<int> init, int e = 0)
        :
        values(init),
        extra(e)
    {}

    friend bool operator==(const initializer_list_constructible&, const initializer_list_constructible&) = default;

    std::vector<int32_t> values;
    int32_t extra = 0;
};

auto make_valueless_int_indirect() {
    nova::indirect<int> tmp(std::in_place, 123);
    auto moved_to = std::move(tmp);
    (void)moved_to;
    return tmp;
}

struct tracked_value {
    tracked_value() : value(0) {
        ++default_ctor_count;
    }

    explicit tracked_value(int32_t value) : value(value) {
        ++value_ctor_count;
    }

    tracked_value(const tracked_value& rhs) : value(rhs.value) {
        ++copy_ctor_count;
    }

    tracked_value(tracked_value&& rhs) noexcept : value(rhs.value) {
        ++move_ctor_count;
        value = invalid_value;
    }

    tracked_value& operator=(const tracked_value& rhs) {
        ++copy_assign_ctor_count;
        value = rhs.value;
        return *this;
    }

    tracked_value& operator=(tracked_value&& rhs) noexcept {
        ++move_assign_ctor_count;
        value = rhs.value;
        rhs.value = invalid_value;
        return *this;
    }

    ~tracked_value() noexcept {
        ++dtor_count;
    }

    friend bool operator==(const tracked_value&, const tracked_value&) = default;
    friend bool operator<=>(const tracked_value&, const tracked_value&) = default;

    static void reset() {
        default_ctor_count = 0;
        value_ctor_count = 0;
        copy_ctor_count = 0;
        move_ctor_count = 0;
        copy_assign_ctor_count = 0;
        move_assign_ctor_count = 0;
        dtor_count = 0;
    }

    static constexpr int32_t invalid_value = std::numeric_limits<std::int32_t>::max();

    static inline int32_t default_ctor_count = 0;
    static inline int32_t value_ctor_count = 0;
    static inline int32_t copy_ctor_count = 0;
    static inline int32_t move_ctor_count = 0;
    static inline int32_t copy_assign_ctor_count = 0;
    static inline int32_t move_assign_ctor_count = 0;
    static inline int32_t dtor_count = 0;

    int32_t value;
};

TEST_CASE("nova::indirect", "[indirect]") {
    SECTION("Constructors") {
        SECTION("default constructor") {
            nova::indirect<int32_t> x;
            REQUIRE_FALSE(x.valueless_after_move());
            REQUIRE(*x == 0);
        }
        SECTION("single value constructor lvalue") {
            constexpr int32_t value = 42;
            nova::indirect<int32_t> x(value);

            REQUIRE_FALSE(x.valueless_after_move());
            REQUIRE(*x == 42);
        }
        SECTION("single value constructor rvalue") {
            nova::indirect<int32_t> x(99);

            REQUIRE_FALSE(x.valueless_after_move());
            REQUIRE(*x == 99);
        }
        SECTION("in_place constructor forwards args") {
            nova::indirect<std::pair<int, int>> x(std::in_place, 10, 20);

            REQUIRE(x->first == 10);
            REQUIRE(x->second == 20);
        }
        SECTION("in_place initializer_list constructor") {
            nova::indirect<initializer_list_constructible> x(std::in_place, {1,2,3}, 77);

            REQUIRE(x->values == std::vector<int32_t>{1,2,3});
            REQUIRE(x->extra == 77);
        }
        SECTION("allocator_arg default constructor stores allocator and constructs value") {
            std::size_t allocations = 0;
            std::size_t deallocations = 0;

            nova::indirect<int32_t, tracking_allocator<int32_t>> x(std::allocator_arg, {&allocations, &deallocations, 7});

            REQUIRE(*x == 0);
            REQUIRE(x.get_allocator().tag == 7);
            REQUIRE(allocations == 1);
            REQUIRE(deallocations == 0);
        }
        SECTION("allocator_arg value constructor") {
            std::size_t allocations = 0;
            std::size_t deallocations = 0;

            using allocator = tracking_allocator<int32_t>;

            nova::indirect<int32_t, allocator> x(std::allocator_arg, {&allocations, &deallocations, 3}, 55);

            REQUIRE(*x == 55);
            REQUIRE(x.get_allocator().tag == 3);
            REQUIRE(allocations == 1);
            REQUIRE(deallocations == 0);
        }
        SECTION("allocator_arg in_place constructor") {
            std::size_t allocations = 0;
            std::size_t deallocations = 0;

            using allocator = tracking_allocator<std::pair<int32_t, int32_t>>;

            nova::indirect<std::pair<int32_t, int32_t>, allocator> x(
                std::allocator_arg,
                {&allocations, &deallocations, 11},
                std::in_place,
                1,
                2
            );

            REQUIRE(x->first == 1);
            REQUIRE(x->second == 2);
            REQUIRE(x.get_allocator().tag == 11);
        }
        SECTION("allocator_arg in_place initializer_list constructor") {
            std::size_t allocations = 0;
            std::size_t deallocations = 0;

            using allocator = tracking_allocator<initializer_list_constructible>;

            nova::indirect<initializer_list_constructible, allocator> x(
                std::allocator_arg,
                {&allocations, &deallocations, 9},
                std::in_place,
                {4,5,6},
                12
            );

            REQUIRE(x->values == std::vector<int32_t>{4,5,6});
            REQUIRE(x->extra == 12);
            REQUIRE(x.get_allocator().tag == 9);
        }
        SECTION("copy constructor") {
            nova::indirect<int32_t> a(std::in_place, 42);
            nova::indirect<int32_t> b(a);

            REQUIRE(*a == 42);
            REQUIRE(*b == 42);
            REQUIRE(std::addressof(*a) != std::addressof(*b));
        }
        SECTION("move constructor") {
            nova::indirect<int> a(std::in_place, 42);
            auto* old_address = std::addressof(*a);
            nova::indirect<int32_t> b(std::move(a));

            REQUIRE(a.valueless_after_move());
            REQUIRE_FALSE(b.valueless_after_move());
            REQUIRE(*b == 42);
            REQUIRE(std::addressof(*b) == old_address);
        }
        SECTION("allocator extended copy constructor") {
            std::size_t allocations = 0;
            std::size_t deallocations = 0;

            using allocator = tracking_allocator<int32_t>;

            nova::indirect<int32_t, allocator> a(std::allocator_arg, {&allocations, &deallocations, 1}, std::in_place, 42);
            nova::indirect<int32_t, allocator> b(std::allocator_arg, {&allocations, &deallocations, 2}, a);

            REQUIRE(*a == 42);
            REQUIRE(*b == 42);
            REQUIRE(std::addressof(*a) != std::addressof(*b));
            REQUIRE(a.get_allocator().tag == 1);
            REQUIRE(b.get_allocator().tag == 2);
        }
        SECTION("allocator-extended move constructor") {
            std::size_t allocations = 0;
            std::size_t deallocations = 0;

            using allocator = tracking_allocator<int32_t>;

            nova::indirect<int, allocator> a(std::allocator_arg, {&allocations, &deallocations, 7}, std::in_place, 42);
            auto* old_address = std::addressof(*a);

            nova::indirect<int, allocator> b(std::allocator_arg, {&allocations, &deallocations, 7}, std::move(a));

            REQUIRE(a.valueless_after_move());
            REQUIRE(*b == 42);
            REQUIRE(std::addressof(*b) == old_address);
        }

        SECTION("copy construction from valueless") {
            auto valueless = make_valueless_int_indirect();
            REQUIRE(valueless.valueless_after_move());

            nova::indirect<int> copy(valueless);
            REQUIRE(copy.valueless_after_move());
         }
         SECTION("move construction from valueless") {
            auto valueless = make_valueless_int_indirect();
            REQUIRE(valueless.valueless_after_move());

            nova::indirect<int> moved(std::move(valueless));
            REQUIRE(moved.valueless_after_move());
        }
        SECTION("CTAD") {
            nova::indirect x(123);
            static_assert(std::is_same_v<decltype(x), nova::indirect<int>>);
            REQUIRE(*x == 123);
        }
        SECTION("allocator CTAD") {
            std::size_t allocations = 0;
            std::size_t deallocations = 0;

            using allocator = tracking_allocator<int32_t>;

            nova::indirect x(std::allocator_arg, allocator{&allocations, &deallocations, 4}, 321);

            static_assert(std::is_same_v<decltype(x), nova::indirect<int, allocator>>);
            REQUIRE(*x == 321);
        }
    }
    SECTION("Destructor") {
        SECTION("destructor destroys contained object once") {
            tracked_value::reset();
            {
                nova::indirect<tracked_value> x(std::in_place, 42);
                REQUIRE_FALSE(x.valueless_after_move());
            }

            REQUIRE(tracked_value::dtor_count == 1);
        }
        SECTION("destructor deallocates exactly once with tracking allocator") {
            std::size_t allocations = 0;
            std::size_t deallocations = 0;

            {
                using allocator = tracking_allocator<int>;
                nova::indirect<int, allocator> x(std::allocator_arg, {&allocations, &deallocations, 1}, std::in_place, 99);
                REQUIRE(allocations == 1);
                REQUIRE(deallocations == 0);
            }

            REQUIRE(allocations == 1);
            REQUIRE(deallocations == 1);
        }
    }
}
