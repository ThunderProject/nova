module;

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <format>
#include <print>
#include <cstdint>
#include <ctime>
#include <limits>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>
#include <cpuid.h>
#include <mutex>
#include "hash_map.h"

export module core.profiler;

import guard;
import mutex;

namespace nova::profiler {
    export template<std::size_t N>
    struct fixed_string {
        consteval fixed_string(const char(&text)[N]) noexcept {
            for(std::size_t i = 0; i < N; ++i) {
                value[i] = text[i];
            }
        }

        [[nodiscard]] constexpr const char* data() const noexcept {
            return value.data();
        }

        [[nodiscard]] std::string_view view() const noexcept {
            return { value.data(), N - 1};
        }

        std::array<char, N> value{};
    };

    template<std::size_t N>
    fixed_string(const char (&)[N]) -> fixed_string<N>;

    export struct timestamp {
        std::uint64_t ticks;
        std::uint32_t cpu{};
    };

    constexpr std::size_t region_capacity = 512;
    static_assert(std::has_single_bit(region_capacity), "Profiler capacity must be a power of two");

    [[nodiscard]] inline timestamp read_timestamp() noexcept {
        uint32_t aux = 0;

        asm volatile("" ::: "memory");
        __builtin_ia32_lfence();
        
        const auto ticks = __builtin_ia32_rdtscp(&aux);

        __builtin_ia32_lfence();
        asm volatile("" ::: "memory");

        return {
            .ticks = ticks,
            .cpu =  aux
        };
    }

    template<fixed_string Name>
    inline constexpr auto name_storage = Name;

    template<fixed_string Name>
    [[nodiscard]] consteval std::uint64_t name_hash() noexcept {
        // FNV-1a 64 bit
        std::uint64_t hash = 14695981039346656037ull;

        for(const char c : Name.value) {
            if(c == 0) {
                break;
            }

            hash ^= static_cast<unsigned char>(c);
            hash *= 1099511628211ull;
        }

        // Zero means "empty slot".
        return hash == 0 ? 1 : hash;
    }

    struct region_entry {
        std::uint64_t id{};
        const char* name{};
        std::uint64_t calls{};
        std::uint64_t inclusive_ticks{};
        std::uint64_t self_ticks{};
        std::uint64_t min_ticks{std::numeric_limits<std::uint64_t>::max()};
        std::uint64_t max_ticks{};
        std::uint64_t migrated_calls{};
    };

    struct scope_node {
        scope_node* parent{};
        std::uint64_t child_ticks{};
    };

    struct thread_state {
        [[nodiscard]] region_entry* find_or_create(const std::uint64_t id, const char* const name) noexcept {
            auto index = static_cast<std::size_t>(id) & (region_capacity - 1);

            for(std::size_t probe = 0; probe < region_capacity; ++probe) {
                auto& entry = regions[index];

                if(entry.id == 0) {
                    entry.id = id;
                    entry.name = name;

                    return &entry;
                }

                if(entry.id == id && entry.name == name) {
                    return &entry;
                }

                index = (index + 1) & (region_capacity - 1);
            }

            ++dropped_samples;
            return nullptr;
        }

        void clear() noexcept {
            regions = {};
            current_scope = nullptr;
            dropped_samples = 0;
        }

        std::array<region_entry, region_capacity> regions{};
        scope_node* current_scope{nullptr};
        std::uint64_t dropped_samples{};
        std::uint64_t thread_id{};
    };

    struct registry {
        registry() {
            threads.lock()->reserve(32);
        }
        nova::guard<std::vector<std::unique_ptr<thread_state>>, nova::mutex> threads{std::in_place};
    };

    [[nodiscard]] registry& states() {
        static auto* instance = new registry;
        return *instance;
    }

    [[nodiscard]] thread_state* create_thread_state() noexcept {
        try {
            auto state = std::make_unique<thread_state>();
            state->thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());

            auto* const raw_state = state.get();
            auto& registry = states();

            registry.threads.lock()->emplace_back(std::move(state));

            return raw_state;
        }
        catch(...) {
            return nullptr;
        }
    }

    [[nodiscard]] thread_state* local_state() noexcept {
        thread_local thread_state* state = create_thread_state();
        return state;
    }

    inline std::atomic_bool enabled{false};
    inline std::uint64_t measurement_overhead_ticks{};
    inline double counter_hz{};
    inline std::uint64_t session_begin_ticks{};
    inline std::uint64_t session_end_ticks{};
    inline std::once_flag calibration_once;

     [[nodiscard]] std::uint64_t calibrate_overhead() noexcept {
        auto best = std::numeric_limits<std::uint64_t>::max();

        for(std::size_t i = 0; i < 4096; ++i) {
            const auto begin = read_timestamp();
            const auto end = read_timestamp();

            best = std::min(best, end.ticks - begin.ticks); 
        }
        return best;
    }

    [[nodiscard]] double cpuid_counter_hz() noexcept {
        uint32_t eax = 0;
        uint32_t ebx = 0;
        uint32_t ecx = 0;
        uint32_t edx = 0;

        if(__get_cpuid_max(0, nullptr) < 0x15) {
            return 0.0;
        }

        __cpuid_count(0x15, 0, eax, ebx, ecx, edx);

        if(eax == 0 || ebx == 0 || ecx == 0) {
            return 0.0;
        }

        // TSC frequency = crystal_frequency * numerator / denominator
        return static_cast<double>(ecx) * static_cast<double>(ebx) / static_cast<double>(eax);
    }

    [[nodiscard]] double calibrate_counter_hz() {
        if(const auto hz = cpuid_counter_hz(); hz > 0.0) {
            return hz;
        }

        std::array<double, 5> estimates{};

        for(auto& estimate : estimates) { 
            const auto a0 = read_timestamp();
            const auto c0 = std::chrono::steady_clock::now();
            const auto a1 = read_timestamp();

            std::this_thread::sleep_for(std::chrono::milliseconds{5});

            const auto b0 = read_timestamp();
            const auto c1 = std::chrono::steady_clock::now();
            const auto b1 = read_timestamp();

            const auto begin = a0.ticks + ((a1.ticks - a0.ticks) / 2);
            const auto end = b0.ticks + ((b1.ticks - b0.ticks) / 2);

            const auto seconds = std::chrono::duration<double>(c1 - c0).count();

            estimate = static_cast<double>(end - begin) / seconds;
        }

        std::ranges::sort(estimates);
        return estimates[estimates.size() / 2];
    }

    void ensure_calibrated() {
        std::call_once(calibration_once, [] {
            measurement_overhead_ticks = calibrate_overhead();
            counter_hz = calibrate_counter_hz();
        });
    }

    [[nodiscard]] std::uint64_t corrected_ticks(const std::uint64_t raw) noexcept {
        return raw > measurement_overhead_ticks
            ? raw - measurement_overhead_ticks
            : 0;
    }

    void record(
        thread_state& state, 
        const std::uint64_t id, 
        const char* const name, 
        const std::uint64_t inclusive,
        const std::uint64_t self,
        const bool migrated
    ) noexcept {
        auto* const entry = state.find_or_create(id, name);

        if(entry == nullptr) {
            return;
        }

        ++entry->calls;

        entry->inclusive_ticks += inclusive;
        entry->self_ticks += self;

        entry->min_ticks = std::min(entry->min_ticks, inclusive);
        entry->max_ticks = std::max(entry->max_ticks, inclusive);

        entry->migrated_calls += static_cast<std::uint64_t>(migrated);
    }

    export [[nodiscard]] inline timestamp tick() noexcept {
        return read_timestamp();
    }

    export [[nodiscard]] inline std::uint64_t elapsed_ticks(const timestamp begin, const timestamp end) noexcept {
        return corrected_ticks(end.ticks - begin.ticks);
    }

    export template<fixed_string Name>
    class scope {
    public:
        scope() noexcept {
            if(!enabled.load(std::memory_order::acquire)) {
                return;
            }

            m_state = local_state();

            if(m_state == nullptr) {
                return;
            }

            m_node.parent = m_state->current_scope;
            m_state->current_scope = &m_node;

            m_begin = read_timestamp();
        }

        scope(const scope&) = delete;
        scope& operator=(const scope&) = delete;
        scope(scope&&) = delete;
        scope& operator=(scope&&) = delete;

        ~scope() noexcept {
            if(m_state == nullptr) {
                return;
            }

            const auto end = read_timestamp();

            m_state->current_scope = m_node.parent;

            const auto raw_ticks = end.ticks - m_begin.ticks;
            const auto corrected = corrected_ticks(raw_ticks);

            const auto self = corrected > m_node.child_ticks
                ? corrected - m_node.child_ticks
                : 0;

            if(m_node.parent != nullptr) {
                m_node.parent->child_ticks += corrected;
            }

            record(*m_state, name_hash<Name>(), name_storage<Name>.data(), corrected, self, m_begin.cpu != end.cpu);
        }
    private:
        thread_state* m_state{nullptr};
        scope_node m_node{};
        timestamp m_begin;
    };

    export template<fixed_string Name>
    void record(const timestamp begin, const timestamp end) noexcept {
        if(!enabled.load(std::memory_order::acquire)) {
            return;
        }

        auto* const state = local_state();
        if(state == nullptr) {
            return;
        }

        const auto duration = elapsed_ticks(begin, end);

        record(*state, name_hash<Name>(), name_storage<Name>.data(), duration, duration, begin.cpu !=  end.cpu);
    }

    export void reset() {
        if(enabled.load( std::memory_order_acquire)) {
            return;
        }

        auto& registry = states();
        auto lk = registry.threads.lock();

        for(auto& state : *lk) {
            state->clear();
        }

        session_begin_ticks = 0;
        session_end_ticks = 0;
    }

    export void start() {
        enabled.store(false, std::memory_order_release);

        ensure_calibrated();
        reset();

        session_begin_ticks = read_timestamp().ticks;
        session_end_ticks = 0;

        enabled.store(true, std::memory_order_release);
    }

    export void stop() noexcept {
        enabled.store(false, std::memory_order_release);

        session_end_ticks = read_timestamp().ticks;
    }

    export [[nodiscard]] bool is_running() noexcept { return enabled.load(std::memory_order_acquire); }

    export [[nodiscard]] double counter_frequency_hz() {
        ensure_calibrated();

        return counter_hz;
    }

    export [[nodiscard]] double ticks_to_nanoseconds(const std::uint64_t ticks) {
        return static_cast<double>(ticks) * 1.0e9 / counter_frequency_hz();
    }

    struct aggregate_region {
        std::string_view name;
        std::uint64_t calls{};
        std::uint64_t inclusive_ticks{};
        std::uint64_t self_ticks{};
        std::uint64_t min_ticks { std::numeric_limits<std::uint64_t>::max() };
        std::uint64_t max_ticks{};
        std::uint64_t migrated_calls{};
    };

    export void report() {
        ensure_calibrated();

       if(enabled.load(std::memory_order_acquire)) {
            std::println("Nova profiler: stop() before report()");
            return;
        }

        std::vector<aggregate_region> regions;
        nova::hash_map<std::string_view, std::size_t> indices;
    
        std::uint64_t dropped = 0;
        std::size_t thread_count = 0;

        auto& registry = states();

        {
            auto lk = registry.threads.lock();
            thread_count = lk->size();

            for(const auto& state : *lk) {
                dropped += state->dropped_samples;

                for(const auto& entry : state->regions) {
                    if(entry.id == 0 || entry.calls == 0) {
                        continue;
                    }

                    const auto [it, inserted] = indices.try_emplace(entry.name, regions.size());

                    if(inserted) {
                        regions.push_back(aggregate_region { .name = entry.name });
                    }

                    auto& dst = regions[it->second];

                    dst.calls += entry.calls;
                    dst.inclusive_ticks += entry.inclusive_ticks;
                    dst.self_ticks += entry.self_ticks;
                    dst.min_ticks = std::min(dst.min_ticks, entry.min_ticks);
                    dst.max_ticks = std::max(dst.max_ticks, entry.max_ticks);
                    dst.migrated_calls += entry.migrated_calls;
                }
            }
        }

        std::ranges::sort(regions, {}, &aggregate_region::self_ticks);
        std::ranges::reverse(regions);

        const auto wall_ticks = session_end_ticks > session_begin_ticks
            ? session_end_ticks - session_begin_ticks
            : 0;

        std::uint64_t total_self = 0;
        std::uint64_t total_migrated = 0;
        std::uint64_t total_calls = 0;

        for(const auto& region : regions) {
            total_self += region.self_ticks;
            total_migrated += region.migrated_calls;
            total_calls += region.calls;
        }

        const auto microseconds = [](const std::uint64_t ticks) { return static_cast<double>(ticks) * 1.0e6 / counter_hz; };
        const auto milliseconds = [](const std::uint64_t ticks) { return static_cast<double>(ticks) * 1.0e3 / counter_hz; };

        std::println(
            "Nova profiler\n"
            "wall={:.3f} ms  counter={:.3f} GHz  tick-pair-overhead={} ticks  threads={}  migrations={}/{}  dropped={}\n",
            milliseconds(wall_ticks),
            counter_hz / 1.0e9,
            measurement_overhead_ticks,
            thread_count,
            total_migrated,
            total_calls,
            dropped
        );

        const auto header = std::format(
            "{:<34}{:>12}{:>12}{:>12}{:>12}{:>12}{:>12}{:>11}{:>11}",
            "Region",
            "Calls",
            "Incl ms",
            "Self ms",
            "Avg us",
            "Min us",
            "Max us",
            "Wall %",
            "Work %"
        );

        std::println("{}", header);
        std::println("{}", std::string(header.size(), '-'));

        for(const auto& region : regions) {
            const auto wall_percent = wall_ticks != 0
                ? 100.0 * static_cast<double>(region.inclusive_ticks) / static_cast<double>(wall_ticks)
                : 0.0;

            const auto work_percent = total_self != 0
                ? 100.0 * static_cast<double>(region.self_ticks) / static_cast<double>(total_self)
                : 0.0;

            const auto minimum = region.min_ticks == std::numeric_limits<std::uint64_t>::max()
                ? 0
                : region.min_ticks;

            const auto average = region.calls != 0
                ? microseconds(region.inclusive_ticks) / static_cast<double>(region.calls)
                : 0.0;

            const auto row = std::format(
                "{:<34}{:>12}{:>12.3f}{:>12.3f}{:>12.3f}{:>12.3f}{:>12.3f}{:>11.3f}{:>11.3f}",
                region.name,
                region.calls,
                milliseconds(region.inclusive_ticks),
                milliseconds(region.self_ticks),
                average,
                microseconds(minimum),
                microseconds(region.max_ticks),
                wall_percent,
                work_percent
            );

            std::println("{}", row);
        }
    }
}
