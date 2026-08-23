module;

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

export module seqlock;

namespace nova {
    extern "C" {
        void nova_seqlock_load_1(const std::uint64_t* seq, const void* src, void* dst) noexcept;
        void nova_seqlock_load_2(const std::uint64_t* seq, const void* src, void* dst) noexcept;
        void nova_seqlock_load_4(const std::uint64_t* seq, const void* src, void* dst) noexcept;
        void nova_seqlock_load_8(const std::uint64_t* seq, const void* src, void* dst) noexcept;
        void nova_seqlock_load_16(const std::uint64_t* seq, const void* src, void* dst) noexcept;
        void nova_seqlock_load_24(const std::uint64_t* seq, const void* src, void* dst) noexcept;
        void nova_seqlock_load_32(const std::uint64_t* seq, const void* src, void* dst) noexcept;
        void nova_seqlock_load_48(const std::uint64_t* seq, const void* src, void* dst) noexcept;
        void nova_seqlock_load_64(const std::uint64_t* seq, const void* src, void* dst) noexcept;
        void nova_seqlock_load_128(const std::uint64_t* seq, const void* src, void* dst) noexcept;
        void nova_seqlock_load_generic(const std::uint64_t* seq, const void* src, void* dst, std::size_t size) noexcept;

        void nova_seqlock_store_1(std::uint64_t* seq, void* dst, const void* src) noexcept;
        void nova_seqlock_store_2(std::uint64_t* seq, void* dst, const void* src) noexcept;
        void nova_seqlock_store_4(std::uint64_t* seq, void* dst, const void* src) noexcept;
        void nova_seqlock_store_8(std::uint64_t* seq, void* dst, const void* src) noexcept;
        void nova_seqlock_store_16(std::uint64_t* seq, void* dst, const void* src) noexcept;
        void nova_seqlock_store_24(std::uint64_t* seq, void* dst, const void* src) noexcept;
        void nova_seqlock_store_32(std::uint64_t* seq, void* dst, const void* src) noexcept;
        void nova_seqlock_store_48(std::uint64_t* seq, void* dst, const void* source) noexcept;
        void nova_seqlock_store_64(std::uint64_t* seq, void* dst, const void* src) noexcept;
        void nova_seqlock_store_128(std::uint64_t* seq, void* dst, const void* src) noexcept;
        void nova_seqlock_store_generic(std::uint64_t* seq, void* dst, const void* src, std::size_t size) noexcept;
    }

    template<std::size_t Size>
    inline void seqlock_load(const std::uint64_t* seq, const void* src, void* dst) noexcept {
        if constexpr(Size == 1) {
            nova_seqlock_load_1(seq, src, dst);
        }
        else if constexpr(Size == 2) {
            nova_seqlock_load_2(seq, src, dst);
        }
        else if constexpr(Size == 4) {
            nova_seqlock_load_4(seq, src, dst);
        }
        else if constexpr(Size == 8) {
            nova_seqlock_load_8(seq, src, dst);
        }
        else if constexpr(Size == 16) {
            nova_seqlock_load_16(seq, src, dst);
        }
        else if constexpr(Size == 24) {
            nova_seqlock_load_24(seq, src, dst);
        }
        else if constexpr(Size == 32) {
            nova_seqlock_load_32(seq, src, dst);
        }
        else if constexpr(Size == 48) {
            nova_seqlock_load_48(seq, src, dst);
        }
        else if constexpr(Size == 64) {
            nova_seqlock_load_64(seq, src, dst);
        }
        else if constexpr(Size == 128) {
            nova_seqlock_load_128(seq, src, dst);
        }
        else {
            nova_seqlock_load_generic(seq, src, dst, Size);
        }
    }

    template<std::size_t Size>
    inline void seqlock_store(std::uint64_t* seq, void* dst, const void* src) noexcept {
        if constexpr(Size == 1) {
            nova_seqlock_store_1(seq, dst, src);
        }
        else if constexpr(Size == 2) {
            nova_seqlock_store_2(seq, dst, src);
        }
        else if constexpr(Size == 4) {
            nova_seqlock_store_4(seq, dst, src);
        }
        else if constexpr(Size == 8) {
            nova_seqlock_store_8(seq, dst, src);
        }
        else if constexpr(Size == 16) {
            nova_seqlock_store_16(seq, dst, src);
        }
        else if constexpr(Size == 24) {
            nova_seqlock_store_24(seq, dst, src);
        }
        else if constexpr(Size == 32) {
            nova_seqlock_store_32(seq, dst, src);
        }
        else if constexpr(Size == 48) {
            nova_seqlock_store_48(seq, dst, src);
        }
        else if constexpr(Size == 64) {
            nova_seqlock_store_64(seq, dst, src);
        }
        else if constexpr(Size == 128) {
            nova_seqlock_store_128(seq, dst, src);
        }
        else {
            nova_seqlock_store_generic(seq, dst, src, Size);
        }
    }

    /**
    * @brief Single-writer, multi-reader sequence lock.
    *
    */
    export template<class T>
    requires(
        std::is_object_v<T> &&
        !std::is_array_v<T> &&
        !std::is_const_v<T> &&
        !std::is_volatile_v<T> &&
        std::is_trivially_copyable_v<T> &&
        std::is_implicit_lifetime_v<T> &&
        std::is_copy_constructible_v<T>
    )
    class alignas(128) seqlock {
    public:
        seqlock() noexcept(std::is_nothrow_default_constructible_v<T>)
        requires std::default_initializable<T>
        :
        seqlock(T{})
    {}

    explicit seqlock(const T& value) noexcept {
        std::memcpy(m_storage.data(), std::addressof(value), sizeof(T));
    }

    seqlock(const seqlock&) = delete;
    seqlock& operator=(const seqlock&) = delete;
    seqlock(seqlock&&) = delete;
    seqlock& operator=(seqlock&&) = delete;
    ~seqlock() = default;

    [[nodiscard]] T load() const noexcept {
        alignas(T) std::array<std::byte, sizeof(T)> value;
        seqlock_load<sizeof(T)>(std::addressof(m_sequence), m_storage.data(), value.data());
        return *std::start_lifetime_as<T>(value.data());
    }

    void store(const T& value) noexcept {
        seqlock_store<sizeof(T)>(std::addressof(m_sequence), m_storage.data(), std::addressof(value));
    }
    private:
        std::array<std::byte, sizeof(T)> m_storage;
        alignas(std::uint64_t) std::uint64_t m_sequence{0};
    };
}