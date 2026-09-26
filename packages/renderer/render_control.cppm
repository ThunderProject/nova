module;

#include <atomic>
#include <cstdint>

export module nova.render.control;

import nova.platform.window;

export namespace nova::render {
    class render_control final {
    public:
        explicit render_control(const platform::extent2d extent) noexcept
            :
            m_extent(pack(extent))
        {}

        void request_stop() noexcept {
            m_stop.store(true, std::memory_order::relaxed);
        }

        [[nodiscard]] bool stop_requested() const noexcept {
            return m_stop.load(std::memory_order::relaxed);
        }

        void set_extent(const platform::extent2d extent) noexcept {
            m_extent.store(pack(extent), std::memory_order::relaxed);
        }

        [[nodiscard]] platform::extent2d extent() const noexcept {
            return unpack(m_extent.load(std::memory_order::relaxed));
        }
    private:
        [[nodiscard]] static constexpr std::uint64_t pack(const platform::extent2d extent) noexcept {
            return static_cast<std::uint64_t>(extent.width) << 32 | static_cast<std::uint64_t>(extent.height);
        }

        [[nodiscard]] static constexpr platform::extent2d unpack(const std::uint64_t value) noexcept {
            return {
                .width = static_cast<std::uint32_t>(value >> 32),
                .height = static_cast<std::uint32_t>(value)
            };
        }

        std::atomic_bool m_stop;
        std::atomic_uint64_t m_extent;
    };
}
