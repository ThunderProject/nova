module;

#include <concepts>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <utility>
#include <libassert/assert.hpp>

export module guard;

namespace nova {
    template<class T>
    concept MutexType = requires(T& lock) {
        { lock.lock() } -> std::same_as<void>;
        { lock.unlock() } -> std::same_as<void>;
        { lock.try_lock() } -> std::same_as<bool>;
    };

    template<class T>
    concept SharedMutexType = MutexType<T> && requires(T& lock) {
        { lock.lock_shared() } -> std::same_as<void>;
        { lock.unlock_shared() } -> std::same_as<void>;
        { lock.try_lock_shared() } -> std::same_as<bool>;
    };

    export template<class Datum, class Lock>
    class [[nodiscard]] locked_ptr {
    public:
        locked_ptr(Datum* datum, Lock&& lock) noexcept(std::is_nothrow_move_constructible_v<Lock>)
            :
            m_datum(datum),
            m_lock(std::move(lock))
        {
            DEBUG_ASSERT(m_datum != nullptr);
        }

        locked_ptr(const locked_ptr&) = delete;
        locked_ptr& operator=(const locked_ptr&) = delete;

        locked_ptr(locked_ptr&& other) noexcept(std::is_nothrow_move_constructible_v<Lock>)
            :
            m_datum(std::exchange(other.m_datum, nullptr)),
            m_lock(std::move(other.m_lock))
        {}

        locked_ptr& operator=(locked_ptr&& other) noexcept(std::is_nothrow_move_assignable_v<Lock>) {
            if(this != &other) {
                m_lock = std::move(other.m_lock);
                m_datum = std::exchange(other.m_datum, nullptr);
            }
            return *this;
        }

        ~locked_ptr() = default;

        [[nodiscard]] Datum& get() const noexcept {
            DEBUG_ASSERT(m_datum != nullptr);
            return *m_datum;
        }

        [[nodiscard]] Datum& operator*() const noexcept {
            return get();
        }

        [[nodiscard]] Datum* operator->() const noexcept {
            return std::addressof(get());
        }

        [[nodiscard]] Lock& get_lock() noexcept {
            return m_lock;
        }

        [[nodiscard]] const Lock& get_lock() const noexcept {
            return m_lock;
        }
    private:
        Datum* m_datum;
        [[no_unique_address]] Lock m_lock;
    };

    template<class Datum, class Lock>
    locked_ptr(Datum*, Lock&&) -> locked_ptr<Datum, std::remove_cvref_t<Lock>>;

    /**
     * @brief Thread-safe guard around a datum.
     *
     * @tparam Datum Protected datum type.
     * @tparam Mtx Mutex type. Defaults to std::shared_mutex.
     */
    export template<class Datum, MutexType Mtx = std::shared_mutex>
    class guard {
    public:
        using value_type = Datum;
        using mutex_type = Mtx;

        template<class... Args>
        requires std::constructible_from<Datum, Args...>
        explicit guard(std::in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<Datum, Args...>)
            :
            m_datum(std::forward<Args>(args)...)
        {}

        guard() noexcept(std::is_nothrow_default_constructible_v<Datum>)
        requires std::default_initializable<Datum>
            :
            m_datum{}
        {}

        guard(const guard&) = delete;
        guard& operator=(const guard&) = delete;

        guard(guard&& other) noexcept
        requires std::move_constructible<Datum>
            :
            m_datum(move_from(other))
        {}

        guard& operator=(guard&& other) noexcept
        requires std::is_move_assignable_v<Datum> {
            if(this != &other) {
                std::scoped_lock lock(m_mutex, other.m_mutex);
                m_datum = std::move(other.m_datum);
            }
            return *this;
        }

        [[nodiscard]] auto lock() {
            std::unique_lock lock(m_mutex);
            return locked_ptr{std::addressof(m_datum), std::move(lock)};
        }

        [[nodiscard]] auto lock() const {
            std::unique_lock lock(m_mutex);
            return locked_ptr{std::addressof(m_datum), std::move(lock)};
        }

        [[nodiscard]] auto read() requires SharedMutexType<Mtx> {
            std::shared_lock lock(m_mutex);
            return locked_ptr{std::addressof(std::as_const(m_datum)), std::move(lock)};
        }

        [[nodiscard]] auto read() const requires SharedMutexType<Mtx> {
            std::shared_lock lock(m_mutex);
            return locked_ptr{std::addressof(m_datum), std::move(lock)};
        }

        [[nodiscard]] auto write() requires SharedMutexType<Mtx> {
            return lock();
        }

        template<class Callable>
        requires std::invocable<Callable, Datum&>
        decltype(auto) with_lock(Callable&& callable) {
            std::lock_guard lock(m_mutex);
            return std::invoke(std::forward<Callable>(callable), m_datum);
        }

        template<class Callable>
        requires std::invocable<Callable, const Datum&>
        decltype(auto) with_lock(Callable&& callable) const {
            std::lock_guard lock(m_mutex);
            return std::invoke(std::forward<Callable>(callable), std::as_const(m_datum));
        }

        template<class Callable>
        requires SharedMutexType<Mtx> && std::invocable<Callable, const Datum&>
        decltype(auto) with_read_lock(Callable&& callable) {
            std::shared_lock lock(m_mutex);
            return std::invoke(std::forward<Callable>(callable), std::as_const(m_datum));
        }

        template<class Callable>
        requires SharedMutexType<Mtx> && std::invocable<Callable, const Datum&>
        decltype(auto) with_read_lock(Callable&& callable) const {
            std::shared_lock lock(m_mutex);
            return std::invoke(std::forward<Callable>(callable), std::as_const(m_datum));
        }

        template<class Callable>
        requires SharedMutexType<Mtx> && std::invocable<Callable, Datum&>
        decltype(auto) with_write_lock(Callable&& callable) {
            return with_lock(std::forward<Callable>(callable));
        }

        [[nodiscard]] Mtx& mutex() noexcept {
            return m_mutex;
        }

        [[nodiscard]] const Mtx& mutex() const noexcept {
            return m_mutex;
        }

    private:
        [[nodiscard]] static Datum move_from(guard& other) {
            std::unique_lock lock(other.m_mutex);
            return std::move(other.m_datum);
        }
        mutable Mtx m_mutex{};
        Datum m_datum;
    };
}