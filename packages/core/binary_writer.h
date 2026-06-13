#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>
#include <string_view>
#include "libassert/assert.hpp"
#include <span>

namespace nova {
    class binary_writer {
    public:
        constexpr explicit binary_writer(std::size_t reserve_bytes = 0, std::endian endian = std::endian::little) noexcept
            :
            m_endian(endian)
        {
            DEBUG_ASSERT(endian == std::endian::little, "Only little endian is currently supported");
            m_buffer.reserve(reserve_bytes);
        }

        [[nodiscard]] std::vector<std::uint8_t> take() && noexcept {
            return std::move(m_buffer);
        }

        [[nodiscard]] constexpr std::size_t size() const noexcept {
            return m_buffer.size();
        }

        [[nodiscard]] constexpr bool empty() const noexcept {
            return m_buffer.empty();
        }

        [[nodiscard]] constexpr std::span<const std::uint8_t> view() const noexcept {
            return {
                m_buffer.data(), m_buffer.size()
            };
        }
        constexpr void reserve(std::size_t size) noexcept {
            DEBUG_ASSERT(size < m_buffer.max_size());
            m_buffer.reserve(size);
        }

        constexpr void write_u8(std::uint8_t value) noexcept { write(value); }
        constexpr void write_i8(std::int8_t value) noexcept { write(value); }
        constexpr void write_u16(std::uint16_t value) noexcept { write(value); }
        constexpr void write_i16(std::int16_t value) noexcept { write(value); }
        constexpr void write_u32(std::uint32_t value) noexcept { write(value); }
        constexpr void write_i32(std::int32_t value) noexcept { write(value); }
        constexpr void write_u64(std::uint64_t value) noexcept { write(value); }
        constexpr void write_i64(std::int64_t value) noexcept { write(value); }
        constexpr void write_f32(float value) noexcept { write(value); }
        constexpr void write_f64(double value) noexcept { write(value); }

        constexpr void write_bytes(std::span<const std::byte> bytes) noexcept {
            const auto size = m_buffer.size();
            DEBUG_ASSERT(size + bytes.size() <= m_buffer.max_size());

            m_buffer.resize(size + bytes.size());
            std::memcpy(m_buffer.data() + size, bytes.data(), bytes.size());
        }

        constexpr void write_string(std::string_view value) noexcept {
            [[maybe_unused]] const auto size = m_buffer.size();
            DEBUG_ASSERT(size + value.size() <= m_buffer.max_size());
            DEBUG_ASSERT(value.size() <= std::numeric_limits<std::uint32_t>::max());

            write_u32(static_cast<std::uint32_t>(value.size()));
            const auto bytes = std::as_bytes(
                std::span {
                    value.data(),
                    value.size()
                }
            );
            write_bytes(bytes);
        }

        template<class T>
        requires(std::is_trivially_copyable_v<T>)
        constexpr void write(const T& data) noexcept {
            const auto size = m_buffer.size();
            DEBUG_ASSERT(size + sizeof(T) <= m_buffer.max_size());

            m_buffer.resize(size + sizeof(T));
            std::memcpy(m_buffer.data() + size, &data, sizeof(data));
        }
    private:
        std::vector<std::uint8_t> m_buffer;
        [[maybe_unused]] std::endian m_endian;
    };
}

