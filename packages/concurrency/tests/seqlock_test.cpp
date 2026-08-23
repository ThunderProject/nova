#include <array>
#include <atomic>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <catch2/catch_test_macros.hpp>

import seqlock;

namespace {
    template<std::size_t Size>
    struct blob {
        friend constexpr bool operator==(const blob&, const blob&) noexcept = default;
        std::array<std::uint8_t, Size> data{};
    };

    template<std::size_t Size>
    [[nodiscard]] constexpr blob<Size> make_blob(const std::uint8_t seed) noexcept {
        blob<Size> result;
        for (std::size_t i = 0; i < Size; ++i) {
            result.data[i] = static_cast<std::uint8_t>(seed + static_cast<std::uint8_t>(i * 37));
        }
        return result;
    }

    template<std::size_t Size>
    void test_size_path() {
        static_assert(sizeof(blob<Size>) == Size);
        static_assert(std::is_trivially_copyable_v<blob<Size>>);

        const auto initial = make_blob<Size>(0x11);
        const auto second  = make_blob<Size>(0x57);
        const auto third   = make_blob<Size>(0xa3);

        nova::seqlock<blob<Size>> lock(initial);

        REQUIRE(lock.load() == initial);

        lock.store(second);
        REQUIRE(lock.load() == second);

        lock.store(third);
        REQUIRE(lock.load() == third);

        // Make sure repeatedly replacing the same storage doesn't leave
        // any stale bytes behind.
        lock.store(initial);
        REQUIRE(lock.load() == initial);
    }


    struct snapshot24 {
        friend constexpr bool operator==(const snapshot24&, const snapshot24&) noexcept = default;
        std::uint64_t generation;
        std::uint64_t inverse;
        std::uint64_t check;
    };

    static_assert(sizeof(snapshot24) == 24);
    static_assert(std::is_trivially_copyable_v<snapshot24>);

    [[nodiscard]] constexpr snapshot24 make_snapshot24(const std::uint64_t generation) noexcept {
        constexpr std::uint64_t salt = 0x9e3779b97f4a7c15ULL;

        return {
            .generation = generation,
            .inverse = ~generation,
            .check = generation ^ salt,
        };
    }


    [[nodiscard]] constexpr bool valid(const snapshot24& value) noexcept {
        constexpr std::uint64_t salt = 0x9e3779b97f4a7c15ULL;
        return value.inverse == ~value.generation && value.check == (value.generation ^ salt);
    }

    //
    // Deliberately 40 bytes.
    //
    // There is no specialized 40-byte assembler routine, so this exercises
    // nova_seqlock_{load,store}_generic
    //
    struct snapshot40 {
        friend constexpr bool operator==(const snapshot40&, const snapshot40&) noexcept = default;

        std::uint64_t generation;
        std::uint64_t inverse;
        std::uint64_t xor_value;
        std::uint64_t add_value;
        std::uint64_t mirror;
    };

    static_assert(sizeof(snapshot40) == 40);
    static_assert(std::is_trivially_copyable_v<snapshot40>);

    [[nodiscard]] constexpr snapshot40 make_snapshot40(const std::uint64_t generation) noexcept {
        constexpr std::uint64_t xor_salt = 0xd6e8feb86659fd93ULL;
        constexpr std::uint64_t add_salt = 0xa0761d6478bd642fULL;

        return {
            .generation = generation,
            .inverse = ~generation,
            .xor_value = generation ^ xor_salt,
            .add_value = generation + add_salt,
            .mirror = generation,
        };
    }


    [[nodiscard]] constexpr bool valid(const snapshot40& value) noexcept {
        constexpr std::uint64_t xor_salt = 0xd6e8feb86659fd93ULL;
        constexpr std::uint64_t add_salt = 0xa0761d6478bd642fULL;
        return  value.inverse == ~value.generation &&
                value.xor_value == (value.generation ^ xor_salt) &&
                value.add_value == (value.generation + add_salt) &&
                value.mirror == value.generation;
    }

    template<class Snapshot, class MakeSnapshot>
    void run_concurrent_stress(MakeSnapshot&& make_snapshot) {
        constexpr std::size_t reader_count = 8;
        constexpr std::uint64_t iterations = 250'000;

        nova::seqlock<Snapshot> lock(make_snapshot(0));

        std::atomic<bool> done{false};

        std::atomic<std::uint64_t> reads{0};
        std::atomic<std::uint64_t> invalid_snapshots{0};

        std::barrier start{ static_cast<std::ptrdiff_t>(reader_count + 2) };

        std::vector<std::thread> readers;
        readers.reserve(reader_count);

        for (std::size_t reader = 0; reader < reader_count; reader++) {
            readers.emplace_back([&] {
                start.arrive_and_wait();

                std::uint64_t local_reads = 0;
                std::uint64_t local_invalid = 0;

                while (!done.load(std::memory_order_acquire)) {
                    const auto snapshot = lock.load();
                    if (!valid(snapshot)) {
                        ++local_invalid;
                    }
                    ++local_reads;
                }

                //
                // Also validate a snapshot after publication has stopped.
                //
                const auto final_snapshot = lock.load();

                if (!valid(final_snapshot)) {
                    ++local_invalid;
                }

                ++local_reads;

                reads.fetch_add(local_reads, std::memory_order_relaxed);

                invalid_snapshots.fetch_add(local_invalid, std::memory_order_relaxed);
            });
        }


        std::thread writer([&] {
            start.arrive_and_wait();

            for (std::uint64_t generation = 1; generation <= iterations; generation++) {
                lock.store(make_snapshot(generation));

                if ((generation & 0x3ff) == 0) {
                    std::this_thread::yield();
                }
            }
            done.store(true, std::memory_order_release);
        });


        start.arrive_and_wait();
        writer.join();

        for (auto& reader : readers) {
            reader.join();
        }

        REQUIRE(invalid_snapshots.load(std::memory_order_relaxed) == 0);
        REQUIRE(reads.load(std::memory_order_relaxed) >= reader_count);
        REQUIRE(lock.load() == make_snapshot(iterations));
    }

    template<class T>
    concept seqlock_supported = requires { typename nova::seqlock<T>; }; }

    TEST_CASE("seqlock") {
        SECTION("type constraints and public type properties") {
            STATIC_REQUIRE(seqlock_supported<std::uint64_t>);
            STATIC_REQUIRE(seqlock_supported<snapshot24>);
            STATIC_REQUIRE_FALSE(seqlock_supported<const std::uint64_t>);
            STATIC_REQUIRE_FALSE(seqlock_supported<volatile std::uint64_t>);
            STATIC_REQUIRE_FALSE(seqlock_supported<std::uint64_t[4]>);
            STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<nova::seqlock<std::uint64_t>>);
            STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<nova::seqlock<std::uint64_t>>);
            STATIC_REQUIRE_FALSE(std::is_move_constructible_v<nova::seqlock<std::uint64_t>>);
            STATIC_REQUIRE_FALSE(std::is_move_assignable_v<nova::seqlock<std::uint64_t>>);
            STATIC_REQUIRE(noexcept(std::declval<const nova::seqlock<std::uint64_t>&>().load()));
            STATIC_REQUIRE(noexcept(std::declval<nova::seqlock<std::uint64_t>&>().store(std::declval<const std::uint64_t&>())));
        }
        SECTION("default construction payload") {
            nova::seqlock<std::uint64_t> lock;
            REQUIRE(lock.load() == 0);
        }
        SECTION("explicit construction") {
            constexpr std::uint64_t value = 0x0123456789abcdefULL;
            const nova::seqlock<std::uint64_t> lock(value);
            REQUIRE(lock.load() == value);
        }
        SECTION("store") {
            nova::seqlock<std::uint64_t> lock(1);
            REQUIRE(lock.load() == 1);

            lock.store(2);
            REQUIRE(lock.load() == 2);

            lock.store(0xffffffffffffffffULL);
            REQUIRE(lock.load() == 0xffffffffffffffffULL);

            lock.store(0);
            REQUIRE(lock.load() == 0);
        }
        SECTION("specialized path") {
            test_size_path<1>();
            test_size_path<2>();
            test_size_path<4>();
            test_size_path<8>();
            test_size_path<16>();
            test_size_path<24>();
            test_size_path<32>();
            test_size_path<48>();
            test_size_path<64>();
            test_size_path<128>();
        }
        SECTION("generic path") {
            test_size_path<3>();
            test_size_path<5>();
            test_size_path<7>();
            test_size_path<15>();
            test_size_path<40>();
            test_size_path<56>();
            test_size_path<96>();
            test_size_path<127>();
            test_size_path<129>();
            test_size_path<256>();
        }
        SECTION("many sequential publications") {
            nova::seqlock<snapshot24> lock(make_snapshot24(0));

            constexpr std::uint64_t iterations = 100'000;

            for (std::uint64_t generation = 1; generation <= iterations; generation++) {
                const auto expected = make_snapshot24(generation);
                lock.store(expected);
                const auto actual = lock.load();
                REQUIRE(actual == expected);
                REQUIRE(valid(actual));
            }
        }
        SECTION("single writer many readers") {
            run_concurrent_stress<snapshot24>(
                [](const std::uint64_t generation) {
                    return make_snapshot24(generation);
                }
            );
            run_concurrent_stress<snapshot40>(
                [](const std::uint64_t generation) {
                    return make_snapshot40(generation);
                }
            );
        }
    }