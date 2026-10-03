#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

import nova.math.vector;

namespace {
    template<std::size_t Dim>
    concept vector_supported = requires {
        typename nova::math::vector<Dim>;
    };

    template<std::size_t Dim>
    void require_approx(
        const nova::math::vector<Dim>& actual,
        const std::array<float, Dim>& expected,
        const float tolerance = 1.0e-5f
    ) {
        const auto values = actual.to_array();
        for(std::size_t i = 0; i < Dim; ++i) {
            CAPTURE(i);

            REQUIRE(values[i] == Catch::Approx(expected[i]).epsilon(tolerance).margin(tolerance));
        }
    }
}

TEST_CASE("vector") {
    SECTION("type constraints and properties") {
        STATIC_REQUIRE_FALSE(vector_supported<0>);
        STATIC_REQUIRE_FALSE(vector_supported<1>);
        STATIC_REQUIRE(vector_supported<2>);
        STATIC_REQUIRE(vector_supported<3>);
        STATIC_REQUIRE(vector_supported<4>);
        STATIC_REQUIRE_FALSE(vector_supported<5>);

        STATIC_REQUIRE(std::same_as<nova::math::vec2, nova::math::vector<2>>);
        STATIC_REQUIRE(std::same_as<nova::math::vec3, nova::math::vector<3>>);
        STATIC_REQUIRE(std::same_as<nova::math::vec4, nova::math::vector<4>>);

        STATIC_REQUIRE(sizeof(nova::math::vec2) == 8);
        STATIC_REQUIRE(sizeof(nova::math::vec3) == 16);
        STATIC_REQUIRE(sizeof(nova::math::vec4) == 16);

        STATIC_REQUIRE(alignof(nova::math::vec2) == 8);
        STATIC_REQUIRE(alignof(nova::math::vec3) == 16);
        STATIC_REQUIRE(alignof(nova::math::vec4) == 16);

        STATIC_REQUIRE(std::is_trivially_copyable_v<nova::math::vec2>);
        STATIC_REQUIRE(std::is_trivially_copyable_v<nova::math::vec3>);
        STATIC_REQUIRE(std::is_trivially_copyable_v<nova::math::vec4>);

        STATIC_REQUIRE(std::is_standard_layout_v<nova::math::vec2>);
        STATIC_REQUIRE(std::is_standard_layout_v<nova::math::vec3>);
        STATIC_REQUIRE(std::is_standard_layout_v<nova::math::vec4>);

        STATIC_REQUIRE(nova::math::vec2::dimension == 2);
        STATIC_REQUIRE(nova::math::vec3::dimension == 3);
        STATIC_REQUIRE(nova::math::vec4::dimension == 4);

        STATIC_REQUIRE(nova::math::vec2::size() == 2);
        STATIC_REQUIRE(nova::math::vec3::size() == 3);
        STATIC_REQUIRE(nova::math::vec4::size() == 4);
    }

    SECTION("constexpr operations") {
        constexpr nova::math::vec3 lhs{1.0f, 2.0f, 3.0f};
        constexpr nova::math::vec3 rhs{4.0f, 5.0f, 6.0f};

        STATIC_REQUIRE(lhs.x() == 1.0f);
        STATIC_REQUIRE(lhs.y() == 2.0f);
        STATIC_REQUIRE(lhs.z() == 3.0f);

        constexpr auto add = lhs + rhs;
        constexpr auto sub = rhs - lhs;
        constexpr auto mul = lhs * rhs;
        constexpr auto div = rhs / lhs;

        STATIC_REQUIRE((add.to_array() == std::array<float, 3>{5.0f, 7.0f, 9.0f}));
        STATIC_REQUIRE((sub.to_array() == std::array<float, 3>{3.0f, 3.0f, 3.0f}));
        STATIC_REQUIRE((mul.to_array() == std::array<float, 3>{4.0f, 10.0f, 18.0f}));
        STATIC_REQUIRE((div.to_array() == std::array<float, 3>{4.0f, 2.5f, 2.0f}));

        constexpr auto scalar_add = lhs + 2.0f;
        constexpr auto scalar_sub = lhs - 1.0f;
        constexpr auto scalar_mul = lhs * 2.0f;
        constexpr auto scalar_div = lhs / 2.0f;

        STATIC_REQUIRE((scalar_add.to_array() == std::array<float, 3>{3.0f, 4.0f, 5.0f}));
        STATIC_REQUIRE((scalar_sub.to_array() == std::array<float, 3>{0.0f, 1.0f, 2.0f}));
        STATIC_REQUIRE((scalar_mul.to_array() == std::array<float, 3>{2.0f, 4.0f, 6.0f}));
        STATIC_REQUIRE((scalar_div.to_array() == std::array<float, 3>{0.5f, 1.0f, 1.5f}));

        STATIC_REQUIRE(dot(lhs, rhs) == 32.0f);

        constexpr auto cross_result = cross(nova::math::vec3{1.0f, 0.0f, 0.0f}, nova::math::vec3{0.0f, 1.0f, 0.0f});

        STATIC_REQUIRE((cross_result.to_array() == std::array<float, 3>{0.0f, 0.0f, 1.0f}));

        constexpr auto zero = nova::math::vec3::zero();
        constexpr auto one = nova::math::vec3::one();

        STATIC_REQUIRE((zero.to_array() == std::array<float, 3>{0.0f, 0.0f, 0.0f}));
        STATIC_REQUIRE((one.to_array() == std::array<float, 3>{1.0f, 1.0f, 1.0f}));
    }

    SECTION("construction") {
        const nova::math::vec2 v2;
        const nova::math::vec3 v3;
        const nova::math::vec4 v4;

        REQUIRE(v2.to_array() == std::array<float, 2>{0.0f, 0.0f});
        REQUIRE(v3.to_array() == std::array<float, 3>{0.0f, 0.0f, 0.0f});
        REQUIRE(v4.to_array() == std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});

        REQUIRE(nova::math::vec2{2.0f}.to_array() == std::array<float, 2>{2.0f, 2.0f});
        REQUIRE(nova::math::vec3{2.0f}.to_array() == std::array<float, 3>{2.0f, 2.0f, 2.0f});
        REQUIRE(nova::math::vec4{2.0f}.to_array() == std::array<float, 4>{2.0f, 2.0f, 2.0f, 2.0f});

        REQUIRE(nova::math::vec2{1.0f, 2.0f}.to_array() == std::array<float, 2>{1.0f, 2.0f});
        REQUIRE(nova::math::vec3{1.0f, 2.0f, 3.0f}.to_array() == std::array<float, 3>{1.0f, 2.0f, 3.0f});

        REQUIRE(nova::math::vec4{1.0f, 2.0f, 3.0f, 4.0f}.to_array() == std::array<float, 4>{1.0f, 2.0f, 3.0f, 4.0f});

        const std::array<float, 3> values{4.0f, 5.0f, 6.0f};
        const nova::math::vec3 from_array{values};

        REQUIRE(from_array.to_array() == values);
    }

    SECTION("factories") {
        REQUIRE(nova::math::vec2::zero() == nova::math::vec2{0.0f, 0.0f});
        REQUIRE(nova::math::vec3::zero() == nova::math::vec3{0.0f, 0.0f, 0.0f});
        REQUIRE(nova::math::vec4::zero() == nova::math::vec4{0.0f, 0.0f, 0.0f, 0.0f});

        REQUIRE(nova::math::vec2::one() == nova::math::vec2{1.0f, 1.0f});
        REQUIRE(nova::math::vec3::one() == nova::math::vec3{1.0f, 1.0f, 1.0f});
        REQUIRE(nova::math::vec4::one() == nova::math::vec4{1.0f, 1.0f, 1.0f, 1.0f});

        REQUIRE(nova::math::vec3::unit_x() == nova::math::vec3{1.0f, 0.0f, 0.0f});
        REQUIRE(nova::math::vec3::unit_y() == nova::math::vec3{0.0f, 1.0f, 0.0f});
        REQUIRE(nova::math::vec3::unit_z() == nova::math::vec3{0.0f, 0.0f, 1.0f});

        REQUIRE(nova::math::vec4::unit_x() == nova::math::vec4{1.0f, 0.0f, 0.0f, 0.0f});
        REQUIRE(nova::math::vec4::unit_y() == nova::math::vec4{0.0f, 1.0f, 0.0f, 0.0f});
        REQUIRE(nova::math::vec4::unit_z() == nova::math::vec4{0.0f, 0.0f, 1.0f, 0.0f});
        REQUIRE(nova::math::vec4::unit_w() == nova::math::vec4{0.0f, 0.0f, 0.0f, 1.0f});
    }

    SECTION("component access") {
        nova::math::vec4 value{1.0f, 2.0f, 3.0f, 4.0f};

        REQUIRE(value.get<0>() == 1.0f);
        REQUIRE(value.get<1>() == 2.0f);
        REQUIRE(value.get<2>() == 3.0f);
        REQUIRE(value.get<3>() == 4.0f);

        REQUIRE(value.x() == 1.0f);
        REQUIRE(value.y() == 2.0f);
        REQUIRE(value.z() == 3.0f);
        REQUIRE(value.w() == 4.0f);

        for(std::size_t i = 0; i < value.size(); ++i) {
            REQUIRE(value[i] == static_cast<float>(i + 1));
        }
    }

    SECTION("component mutation") {
        nova::math::vec4 value;

        value.set<0>(1.0f);
        value.set<1>(2.0f);
        value.set<2>(3.0f);
        value.set<3>(4.0f);

        REQUIRE(value == nova::math::vec4{1.0f, 2.0f, 3.0f, 4.0f});

        value.set_x(5.0f);
        value.set_y(6.0f);
        value.set_z(7.0f);
        value.set_w(8.0f);

        REQUIRE(value == nova::math::vec4{5.0f, 6.0f, 7.0f, 8.0f});

        value.set(0, 9.0f);
        value.set(1, 10.0f);
        value.set(2, 11.0f);
        value.set(3, 12.0f);

        REQUIRE(value == nova::math::vec4{9.0f, 10.0f, 11.0f, 12.0f});

        nova::math::vec3 v3{1.0f, 2.0f, 3.0f};

        v3.set<0>(4.0f);
        v3.set<1>(5.0f);
        v3.set<2>(6.0f);

        REQUIRE(v3 == nova::math::vec3{4.0f, 5.0f, 6.0f});

        nova::math::vec2 v2{1.0f, 2.0f};

        v2.set(0, 3.0f);
        v2.set(1, 4.0f);

        REQUIRE(v2 == nova::math::vec2{3.0f, 4.0f});
    }

    SECTION("store writes exactly active components") {
        {
            std::array<float, 5> output{-1.0f, -1.0f, -1.0f, -1.0f, -1.0f};
            nova::math::vec2{1.0f, 2.0f}.store(output.data() + 1);
            REQUIRE(output == std::array<float, 5>{-1.0f, 1.0f, 2.0f, -1.0f, -1.0f});
        }

        {
            std::array<float, 5> output{-1.0f, -1.0f, -1.0f, -1.0f, -1.0f};
            nova::math::vec3{1.0f, 2.0f, 3.0f}.store(output.data() + 1);
            REQUIRE(output == std::array<float, 5>{-1.0f, 1.0f, 2.0f, 3.0f, -1.0f});
        }

        {
            std::array<float, 6> output{-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f};
            nova::math::vec4{1.0f, 2.0f, 3.0f, 4.0f}.store(output.data() + 1);
            REQUIRE(output == std::array<float, 6>{-1.0f, 1.0f, 2.0f, 3.0f, 4.0f, -1.0f});
        }
    }

    SECTION("vector arithmetic") {
        const nova::math::vec4 lhs{8.0f, 12.0f, 18.0f, 24.0f};
        const nova::math::vec4 rhs{2.0f, 3.0f, 6.0f, 8.0f};

        REQUIRE(lhs + rhs == nova::math::vec4{10.0f, 15.0f, 24.0f, 32.0f});
        REQUIRE(lhs - rhs == nova::math::vec4{6.0f, 9.0f, 12.0f, 16.0f});
        REQUIRE(lhs * rhs == nova::math::vec4{16.0f, 36.0f, 108.0f, 192.0f});
        REQUIRE(lhs / rhs == nova::math::vec4{4.0f, 4.0f, 3.0f, 3.0f});
    }

    SECTION("scalar arithmetic") {
        const nova::math::vec3 value{1.0f, 2.0f, 4.0f};

        REQUIRE(value + 2.0f == nova::math::vec3{3.0f, 4.0f, 6.0f});
        REQUIRE(value - 2.0f == nova::math::vec3{-1.0f, 0.0f, 2.0f});
        REQUIRE(value * 2.0f == nova::math::vec3{2.0f, 4.0f, 8.0f});
        REQUIRE(value / 2.0f == nova::math::vec3{0.5f, 1.0f, 2.0f});

        REQUIRE(2.0f + value == nova::math::vec3{3.0f, 4.0f, 6.0f});
        REQUIRE(2.0f * value == nova::math::vec3{2.0f, 4.0f, 8.0f});
        REQUIRE(10.0f - value == nova::math::vec3{9.0f, 8.0f, 6.0f});
        REQUIRE(8.0f / value == nova::math::vec3{8.0f, 4.0f, 2.0f});
    }

    SECTION("compound assignment") {
        nova::math::vec3 value{8.0f, 12.0f, 16.0f};

        value += nova::math::vec3{1.0f, 2.0f, 4.0f};
        REQUIRE(value == nova::math::vec3{9.0f, 14.0f, 20.0f});

        value -= nova::math::vec3{1.0f, 4.0f, 5.0f};
        REQUIRE(value == nova::math::vec3{8.0f, 10.0f, 15.0f});

        value *= nova::math::vec3{2.0f, 3.0f, 4.0f};
        REQUIRE(value == nova::math::vec3{16.0f, 30.0f, 60.0f});

        value /= nova::math::vec3{4.0f, 5.0f, 6.0f};
        REQUIRE(value == nova::math::vec3{4.0f, 6.0f, 10.0f});

        value += 2.0f;
        REQUIRE(value == nova::math::vec3{6.0f, 8.0f, 12.0f});

        value -= 1.0f;
        REQUIRE(value == nova::math::vec3{5.0f, 7.0f, 11.0f});

        value *= 2.0f;
        REQUIRE(value == nova::math::vec3{10.0f, 14.0f, 22.0f});

        value /= 2.0f;
        REQUIRE(value == nova::math::vec3{5.0f, 7.0f, 11.0f});
    }

    SECTION("unary operators and equality") {
        const nova::math::vec3 value{1.0f, -2.0f, 3.0f};

        REQUIRE(+value == value);
        REQUIRE(-value == nova::math::vec3{-1.0f, 2.0f, -3.0f});

        REQUIRE(value == nova::math::vec3{1.0f, -2.0f, 3.0f});
        REQUIRE_FALSE(value == nova::math::vec3{1.0f, -2.0f, 4.0f});
    }

    SECTION("dot product") {
        REQUIRE(dot(nova::math::vec2{2.0f, 3.0f}, nova::math::vec2{4.0f, 5.0f}) == 23.0f);
        REQUIRE(dot(nova::math::vec3{1.0f, 2.0f, 3.0f}, nova::math::vec3{4.0f, -5.0f, 6.0f}) == 12.0f);
        REQUIRE(dot(nova::math::vec4{1.0f, 2.0f, 3.0f, 4.0f}, nova::math::vec4{5.0f, 6.0f, 7.0f, 8.0f}) == 70.0f);
    }

    SECTION("cross product") {
        const auto x = nova::math::vec3::unit_x();
        const auto y = nova::math::vec3::unit_y();
        const auto z = nova::math::vec3::unit_z();

        REQUIRE(cross(x, y) == z);
        REQUIRE(cross(y, z) == x);
        REQUIRE(cross(z, x) == y);

        REQUIRE(cross(y, x) == -z);
        REQUIRE(cross(x, x) == nova::math::vec3::zero());

        REQUIRE(
            cross(
                nova::math::vec3{1.0f, 2.0f, 3.0f},
                nova::math::vec3{4.0f, 5.0f, 6.0f}
            ) == nova::math::vec3{-3.0f, 6.0f, -3.0f}
        );
    }

    SECTION("length") {
        const nova::math::vec2 v2{3.0f, 4.0f};
        const nova::math::vec3 v3{2.0f, 3.0f, 6.0f};
        const nova::math::vec4 v4{1.0f, 2.0f, 2.0f, 4.0f};

        REQUIRE(v2.length_squared() == 25.0f);
        REQUIRE(v2.length() == Catch::Approx(5.0f));
        REQUIRE(v3.length_squared() == 49.0f);
        REQUIRE(v3.length() == Catch::Approx(7.0f));
        REQUIRE(v4.length_squared() == 25.0f);
        REQUIRE(v4.length() == Catch::Approx(5.0f));
    }

    SECTION("normalization") {
        const nova::math::vec3 value{3.0f, 4.0f, 0.0f};
        const auto normalized = value.normalized();

        require_approx(normalized, std::array<float, 3>{0.6f, 0.8f, 0.0f});
        REQUIRE(normalized.length() == Catch::Approx(1.0f).epsilon(1.0e-6f));

        auto mutable_value = value;
        auto& reference = mutable_value.normalize();

        REQUIRE(&reference == &mutable_value);
        require_approx(mutable_value, std::array<float, 3>{0.6f, 0.8f, 0.0f});
    }

    SECTION("fast normalization") {
        const nova::math::vec3 value{3.0f, 4.0f, 0.0f};
        const auto normalized = value.normalized_fast();

        require_approx(normalized, std::array<float, 3>{0.6f, 0.8f, 0.0f}, 2.0e-6f);
        REQUIRE(normalized.length() == Catch::Approx(1.0f).epsilon(2.0e-6f).margin(2.0e-6f));
    }

    SECTION("normalized or zero") {
        REQUIRE(nova::math::vec3::zero().normalized_or_zero() == nova::math::vec3::zero());
        REQUIRE(nova::math::vec3{1.0e-8f, 0.0f, 0.0f}.normalized_or_zero(1.0e-6f) == nova::math::vec3::zero());

        require_approx(
            nova::math::vec3{3.0f, 4.0f, 0.0f}.normalized_or_zero(),
            std::array<float, 3>{0.6f, 0.8f, 0.0f}
        );

        const float infinity = std::numeric_limits<float>::infinity();
        const float nan = std::numeric_limits<float>::quiet_NaN();

        REQUIRE(nova::math::vec3{infinity, 0.0f, 0.0f}.normalized_or_zero() == nova::math::vec3::zero());
        REQUIRE(nova::math::vec3{nan, 0.0f, 0.0f}.normalized_or_zero() == nova::math::vec3::zero());
    }

    SECTION("distance") {
        const nova::math::vec3 lhs{1.0f, 2.0f, 3.0f};
        const nova::math::vec3 rhs{4.0f, 6.0f, 3.0f};

        REQUIRE(distance_squared(lhs, rhs) == 25.0f);
        REQUIRE(distance(lhs, rhs) == Catch::Approx(5.0f));

        REQUIRE(distance_squared(lhs, lhs) == 0.0f);
        REQUIRE(distance(lhs, lhs) == 0.0f);
    }

    SECTION("lerp") {
        const nova::math::vec3 lhs{0.0f, 10.0f, 20.0f};
        const nova::math::vec3 rhs{10.0f, 20.0f, 40.0f};

        REQUIRE(lerp(lhs, rhs, 0.0f) == lhs);
        REQUIRE(lerp(lhs, rhs, 1.0f) == rhs);

        require_approx(lerp(lhs, rhs, 0.25f), std::array<float, 3>{2.5f, 12.5f, 25.0f});
    }

    SECTION("reflection") {
        const nova::math::vec3 incident{1.0f, -1.0f, 0.0f};
        const nova::math::vec3 normal{0.0f, 1.0f, 0.0f};
        REQUIRE(reflect(incident, normal) == nova::math::vec3{1.0f, 1.0f, 0.0f});
    }

    SECTION("projection and rejection") {
        const nova::math::vec3 value{3.0f, 4.0f, 0.0f};
        const nova::math::vec3 onto{2.0f, 0.0f, 0.0f};

        require_approx(project(value, onto), std::array<float, 3>{3.0f, 0.0f, 0.0f});
        require_approx(reject(value, onto), std::array<float, 3>{0.0f, 4.0f, 0.0f});
        require_approx(project(value, onto) + reject(value, onto), value.to_array());
    }

    SECTION("finite classification") {
        const float infinity = std::numeric_limits<float>::infinity();
        const float nan = std::numeric_limits<float>::quiet_NaN();

        REQUIRE(nova::math::vec2{1.0f, 2.0f}.is_finite());
        REQUIRE(nova::math::vec3{1.0f, 2.0f, 3.0f}.is_finite());
        REQUIRE(nova::math::vec4{1.0f, 2.0f, 3.0f, 4.0f}.is_finite());

        REQUIRE_FALSE(nova::math::vec2{infinity, 0.0f}.is_finite());
        REQUIRE_FALSE(nova::math::vec3{0.0f, nan, 0.0f}.is_finite());
        REQUIRE_FALSE(nova::math::vec4{0.0f, 0.0f, -infinity, 0.0f}.is_finite());
    }
}
