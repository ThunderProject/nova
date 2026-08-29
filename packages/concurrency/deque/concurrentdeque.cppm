module;
#include <atomic>
#include <bit>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>
#include "libassert/assert.hpp"

export module concurrentdeque;

namespace nova {
    template<class T, class Allocator = std::allocator<std::atomic<T>>>
    requires std::same_as<typename std::allocator_traits<Allocator>::value_type, std::atomic<T>>
    class concurrentringbuffer {
        using allocator_traits = std::allocator_traits<Allocator>;
        using pointer = allocator_traits::pointer;
    public:
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = typename allocator_traits::size_type;

        constexpr explicit concurrentringbuffer(size_type requested_capacity, const Allocator& allocator = Allocator())
            :
            m_allocator(allocator)
        {
            if (requested_capacity == 0) [[unlikely]] {
                throw std::invalid_argument{"concurrentringbuffer capacity must be greater than zero"};
            }

            if (requested_capacity > maximum_capacity(m_allocator)) [[unlikely]] {
                throw std::length_error{"concurrentringbuffer capacity is too large"};
            }

            const auto capacity = std::bit_ceil(requested_capacity);
            m_capacity_mask = capacity - 1;
            m_buffer = allocator_traits::allocate(m_allocator, capacity);

            size_type constructed = 0;

            try {
                auto* const first = data();
                for (; constructed != capacity; ++constructed) {
                    allocator_traits::construct(m_allocator, first + constructed);
                }
            }
            catch (...) {
                dispose(constructed);
                throw;
            }
        }

        concurrentringbuffer(const concurrentringbuffer&) = delete;
        concurrentringbuffer(concurrentringbuffer&&) = delete;
        concurrentringbuffer& operator=(const concurrentringbuffer&) = delete;
        concurrentringbuffer& operator=(concurrentringbuffer &&) = delete;

        constexpr ~concurrentringbuffer() noexcept {
            dispose(capacity());
        }

        [[nodiscard]] constexpr auto capacity() const noexcept { return m_capacity_mask + 1; }

        [[nodiscard]] static constexpr auto maximum_capacity(const Allocator& allocator) noexcept {
            return std::bit_floor(allocator_traits::max_size(allocator));
        }

        constexpr void write_at(const size_type index, const T& value) noexcept {
            data()[index & m_capacity_mask].store(value, std::memory_order_relaxed);
        }

        [[nodiscard]] auto read_at(const size_type index) const noexcept {
            return data()[index & m_capacity_mask].load(std::memory_order_relaxed);
        }
    private:
        [[nodiscard]] constexpr std::atomic<T>* data() noexcept {
            return std::to_address(m_buffer);
        }

        [[nodiscard]] constexpr const std::atomic<T>* data() const noexcept {
            return std::to_address(m_buffer);
        }

        constexpr void dispose(size_type constructed) noexcept {
            DEBUG_ASSERT(constructed <= capacity());

            const auto* first = data();
            while(constructed != 0) {
                allocator_traits::destroy(m_allocator, first + --constructed);
            }
            allocator_traits::deallocate(m_allocator, m_buffer, capacity());
        }

        [[no_unique_address]]
        Allocator m_allocator;

        pointer m_buffer{};
        size_type m_capacity_mask{};
    };

    export enum class Flavor : std::uint8_t { Fifo, Lifo };

    /**
     * @brief a lock-free concurrent work stealing deque.
     *
     * Only the owining thread may call push() and pop(). Multiple threads may
     * concurrently call steal() and steal_batch().
     *
     * This buffer never grows and push() will fail if the queue is full.
     *
     * @tparam T The type stored by the deque. T must satisfy the requirements of std::atomic<T>
     * @tparam flavor Determines whether the deque pops in FIFO or LIFO order.
     */
    export template<class T, Flavor flavor = Flavor::Lifo>
    class concurrent_deque {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        static_assert(std::atomic<T>::is_always_lock_free);

        using buffer_type = concurrentringbuffer<T>;
    public:
        using size_type = std::size_t;
    private:
        using signed_size_type = std::make_signed_t<size_type>;

        static_assert(std::numeric_limits<size_type>::digits >= 3);
    public:
        static constexpr size_type maximum_capacity = size_type{ 1 } << (std::numeric_limits<size_type>::digits - 2);
        
        explicit concurrent_deque(size_type capacity = 1024)
            :
            m_buffer(validate_capacity(capacity))
        {}

        concurrent_deque(const concurrent_deque&) = delete;
        concurrent_deque& operator=(const concurrent_deque&) = delete;
        concurrent_deque(concurrent_deque&&) noexcept = delete;
        concurrent_deque& operator=(concurrent_deque&&) noexcept = delete;
        ~concurrent_deque() noexcept = default;

        [[nodiscard]] bool push(const T& item) noexcept {
            const auto bottom = m_bottom.load(std::memory_order_relaxed);
            auto top = m_cached_top;

            if(is_full(bottom, top)) [[unlikely]] {
                top = m_top.load(std::memory_order_acquire);
                m_cached_top = top;

                if(is_full(bottom, top)) [[unlikely]] {
                    return false;
                }
            }

            m_buffer.write_at(bottom, item);
            m_bottom.store(bottom + 1, std::memory_order_release);
            return true;
        }

        [[nodiscard]] std::optional<T> pop() noexcept {
            if constexpr (flavor == Flavor::Fifo) {
                return pop_top();
            }
            else if constexpr (flavor == Flavor::Lifo) {
                return pop_bottom();
            }
            else {
                std::unreachable();
            }
        }

        [[nodiscard]] std::optional<T> steal() noexcept {
            auto top = m_top.load(std::memory_order_acquire);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            const auto bottom = m_bottom.load(std::memory_order_acquire);

            if(distance(bottom, top) <= 0) {
                return std::nullopt;
            }

            const auto item = m_buffer.read_at(top);
            if(!m_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) [[unlikely]] {
                return std::nullopt;
            }
            return item;
        }

        [[nodiscard]] std::optional<std::vector<T>> steal_batch(const size_type batch_size) noexcept {
            if(batch_size == 0) {
                return std::vector<T>{};
            }

            const auto limit = std::min(batch_size, capacity());

            std::vector<T> result;
            result.reserve(limit);

            if constexpr (flavor == Flavor::Fifo) {
                auto attempt = limit;

                while (attempt != 0) {
                    auto top = m_top.load(std::memory_order_acquire);
                    std::atomic_thread_fence(std::memory_order_seq_cst);
                    const auto bottom = m_bottom.load(std::memory_order_acquire);

                    const auto length = distance(bottom, top);

                    if(length <= 0) [[unlikely]] {
                        return std::nullopt;
                    }

                    const auto count = std::min(attempt, static_cast<size_type>(length));
                    result.clear();

                    for(size_type i = 0; i != count; ++i) {
                        result.emplace_back(m_buffer.read_at(top + i));
                    }

                    auto expected = top;
                    if(m_top.compare_exchange_strong(expected, top + count, std::memory_order::seq_cst, std::memory_order_relaxed)) {
                        return std::make_optional(std::move(result));
                    }

                    if(count == 1) {
                        return std::nullopt;
                    }

                    attempt = count / 2;
                }
                return std::nullopt;
            }
            else {
                for(size_type i = 0; i != limit; ++i) {
                    auto item = steal();
                    if(!item) {
                        break;
                    }
                    result.emplace_back(std::move(item.value()));
                }

                if(result.empty()) {
                    return std::nullopt;
                }
                return std::make_optional(std::move(result));
            }
        }

        [[nodiscard]] auto size() const noexcept {
            const auto bottom = m_bottom.load(std::memory_order_relaxed);
            const auto top = m_top.load(std::memory_order_relaxed);

            const auto length = distance(bottom, top);

            return length > 0
                ? static_cast<size_type>(length)
                : size_type{0};
        }

        [[nodiscard]] bool empty() const noexcept {
            return size() == 0;
        }

        [[nodiscard]] size_type capacity() const noexcept {
            return m_buffer.capacity();
        }
    private:
        [[nodiscard]] std::optional<T> pop_bottom() noexcept {
            const auto bottom = m_bottom.load(std::memory_order::relaxed) - 1;
            m_bottom.store(bottom, std::memory_order_relaxed);

            std::atomic_thread_fence(std::memory_order_seq_cst);
            auto top = m_top.load(std::memory_order_relaxed);

            const auto len = distance(bottom, top);

            if(len < 0) [[unlikely]] {
                m_bottom.store(bottom + 1, std::memory_order_relaxed);
                m_cached_top = top;
                return std::nullopt;
            }

            const auto item = m_buffer.read_at(bottom);
            if(len == 0) [[unlikely]] {
                if(!m_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order::relaxed)) {
                    m_bottom.store(bottom + 1, std::memory_order_relaxed);
                    m_cached_top = top;
                    return std::nullopt;
                }

                m_bottom.store(bottom + 1, std::memory_order_relaxed);
                m_cached_top = top + 1;
            }

            return item;
        }

        [[nodiscard]] std::optional<T> pop_top() noexcept {
            auto top = m_top.load(std::memory_order_acquire);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            const auto bottom = m_bottom.load(std::memory_order::acquire);

            if(distance(bottom, top) <= 0) {
                m_cached_top = top;
                return std::nullopt;
            }

            const auto item = m_buffer.read_at(top);

            if(!m_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) [[unlikely]] {
                m_cached_top = top;
                return std::nullopt;
            }

            m_cached_top = top + 1;
            return item;
        }

        [[nodiscard]] static size_type validate_capacity(const size_type capacity) {
            if(capacity == 0 || capacity > maximum_capacity) [[unlikely]] {
                throw std::invalid_argument("capacity is outside supported range");
            }
            return capacity;
        }

        [[nodiscard]] constexpr bool is_full(const size_type bottom, const size_type top) noexcept {
            return bottom - top >= capacity();
        }

        [[nodiscard]] static constexpr auto distance(const size_type lhs, const size_type rhs) noexcept {
            return std::bit_cast<signed_size_type>(lhs - rhs);
        }

        buffer_type m_buffer;

        alignas(std::hardware_destructive_interference_size)
        std::atomic<size_type> m_top{ 0 };

        alignas(std::hardware_destructive_interference_size)
        std::atomic<size_type> m_bottom{0};
        size_type m_cached_top{0};
    };
}