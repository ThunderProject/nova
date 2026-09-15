#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>

import nova.di.singleton;

namespace {
    struct eager_service {
        int value;
    };

    struct eager_identity_service {
        int value;
    };

    struct lazy_service {
        int value;
    };

    struct lazy_identity_service {
        int value;
    };

    struct inferred_service {
        int value;
    };

    struct owned_service {
        int value;
    };

    struct duplicate_service {
        int value;
    };

    struct missing_service {
        int value;
    };
}

TEST_CASE("Singleton") {
    SECTION("Eager registration") {
        auto& service = nova::ioc::ioc().register_service<eager_service>(42);

        REQUIRE(service.value == 42);
        REQUIRE(nova::ioc::ioc().resolve<eager_service>().value == 42);
    }

    SECTION("Eager identity") {
        auto& registered = nova::ioc::ioc().register_service<eager_identity_service>(42);
        auto& resolved = nova::ioc::ioc().resolve<eager_identity_service>();

        REQUIRE(std::addressof(registered) == std::addressof(resolved));
    }

    SECTION("Lazy registration") {
        int calls = 0;

        nova::ioc::ioc().register_service<lazy_service>([&] {
            ++calls;
            return lazy_service{42};
        });

        REQUIRE(calls == 0);

        auto& service = nova::ioc::ioc().resolve<lazy_service>();

        REQUIRE(service.value == 42);
        REQUIRE(calls == 1);
    }

    SECTION("Lazy identity") {
        int calls = 0;

        nova::ioc::ioc().register_service<lazy_identity_service>([&] {
            ++calls;
            return lazy_identity_service{42};
        });

        auto& first = nova::ioc::ioc().resolve<lazy_identity_service>();
        auto& second = nova::ioc::ioc().resolve<lazy_identity_service>();

        REQUIRE(std::addressof(first) == std::addressof(second));
        REQUIRE(calls == 1);
    }

    SECTION("Inferred registration") {
        nova::ioc::ioc().register_service([] { return inferred_service{42}; });

        REQUIRE(nova::ioc::ioc().resolve<inferred_service>().value == 42);
    }

    SECTION("Unique ownership") {
        nova::ioc::ioc().register_service<owned_service>([] {
            return std::make_unique<owned_service>(
                owned_service{42}
            );
        });

        REQUIRE(nova::ioc::ioc().resolve<owned_service>().value == 42);
    }

    SECTION("Duplicate registration") {
        nova::ioc::ioc().register_service<duplicate_service>(42);

        REQUIRE_THROWS_AS(nova::ioc::ioc().register_service<duplicate_service>(1337),std::logic_error);
    }

    SECTION("Missing service") {
        REQUIRE_THROWS_AS(nova::ioc::ioc().resolve<missing_service>(), std::logic_error);
    }
}
