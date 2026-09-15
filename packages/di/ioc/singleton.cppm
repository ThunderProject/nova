module;
#include <atomic>
#include <concepts>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <type_traits>
#include <libassert/assert.hpp>
export module nova.di.singleton;

import mutex; 

namespace nova::ioc {
    template<class T>
    inline constinit std::atomic<T*> published{nullptr};

    template<class T>
    struct factory_base {
        using create_function = T*(*)(factory_base*);
        using destroy_function = void(*)(factory_base*) noexcept;

        create_function create;
        destroy_function destroy;
    };

    template<class T>
    inline constinit factory_base<T>* factory = nullptr;

    template<class T>
    inline constinit nova::mutex slot_mutex{};

    template<class T>
    struct service_result {
        using type = std::remove_cvref_t<T>;
    };

    template<class T>
    struct service_result<std::unique_ptr<T>> {
        using type = T;
    };
 
    template<class T>
    using service_result_t = typename service_result<std::remove_cvref_t<T>>::type;

    template<class F, class T>
    concept factory_for = std::invocable<F&> && (
        std::same_as<std::invoke_result_t<F&>, T> ||
        std::same_as<std::remove_cvref_t<std::invoke_result_t<F&>>, std::unique_ptr<T>>
    );

    template<class T, class... Args>
    inline constexpr bool single_factory_argument = false;

    template<class T, class F>
    inline constexpr bool single_factory_argument<T, F> = factory_for<std::decay_t<F>, T>;

    template<class T, class F>
    requires factory_for<F,T>
    class factory_model final : public factory_base<T> {
    public:
        explicit factory_model(F fn) noexcept(std::is_nothrow_move_constructible_v<F>)
            :
            factory_base<T> {
                .create = std::addressof(create_impl),
                .destroy = std::addressof(destroy_impl)
            },
            m_fn(std::move(fn))  
        {}
    private:
        static T* create_impl(factory_base<T>* base) {
            DEBUG_ASSERT(base != nullptr);

            auto& self = *static_cast<factory_model*>(base);
            using result_type = std::invoke_result_t<F&>;

            if constexpr (std::same_as<result_type, T>) {
                return new T(std::invoke(self.m_fn));
            }
            else {
                return std::invoke(self.m_fn).release();
            }
        }
        
        static void destroy_impl(factory_base<T>* base) noexcept {
            delete static_cast<factory_model*>(base);
        }

        [[no_unique_address]] F m_fn;
    };

    template<class T>
    struct factory_lifetime {
        ~factory_lifetime() {
            if(auto* ptr = std::exchange(factory<T>, nullptr); ptr != nullptr) {
                ptr->destroy(ptr);
            }
        }
    };

    template<class T>
    inline void arm_factory_lifetime() noexcept {
        static factory_lifetime<T> lifetime;
        (void)lifetime;
    }

    template<class T>
    struct service_lifetime {
        ~service_lifetime() {
            delete published<T>.exchange(nullptr, std::memory_order_relaxed);
        }
    };

    template<class T>
    inline void arm_service_lifetime() noexcept {
        static service_lifetime<T> lifetime;
        (void)lifetime;
    }

    export class container {
    public:
        template<class T, class... Args>
        requires std::constructible_from<T, Args...> && (!single_factory_argument<T, Args...>)
        T& register_service(Args&&... args) const {
            std::scoped_lock lk{slot_mutex<T>};

            if(published<T>.load(std::memory_order::relaxed) != nullptr || factory<T> != nullptr) [[unlikely]] {
                throw std::logic_error("singleton service is already registered");
            }

            auto inst = std::make_unique<T>(std::forward<Args>(args)...);
            auto* ptr = inst.get();

            arm_service_lifetime<T>();
            published<T>.store(inst.release(), std::memory_order::release);

            return *ptr;
        }

        template<class T, class F>
        requires factory_for<std::decay_t<F>, T>
        void register_service(F&& fn) const {
            register_factory<T>(std::forward<F>(fn));
        }

        template<class F>
        requires std::invocable<std::decay_t<F>&>
        void register_service(F&& fn) const {
            using function_type = std::decay_t<F>;
            using result_type = std::invoke_result_t<function_type&>;
            using service_type = service_result_t<result_type>;

            static_assert(factory_for<function_type, service_type>, "factory must return T or std::unique_ptr<T>");

            register_factory<service_type>(std::forward<F>(fn));
        }

        template<class T>
        [[nodiscard]]
        T& resolve() const {
            if(auto* ptr = published<T>.load(std::memory_order::acquire); ptr != nullptr) [[likely]] {
                return *ptr;
            }
            return resolve_cold<T>();
        }
    private:
        template<class T, class F>
        requires factory_for<std::decay_t<F>, T>
        static void register_factory(F&& fn) {
            std::lock_guard lk{slot_mutex<T>};

            if(published<T>.load(std::memory_order::relaxed) != nullptr || factory<T> != nullptr) [[unlikely]] {
                throw std::logic_error("singleton service is already registered");
            }

            using model = factory_model<T, std::decay_t<F>>;

            factory<T> = new model(std::forward<F>(fn));
            arm_factory_lifetime<T>();
        }

        template<class T>
        [[nodiscard, gnu::cold, clang::noinline]]
        static T& resolve_cold() {
            std::lock_guard lk{slot_mutex<T>};

            if(auto* ptr = published<T>.load(std::memory_order::relaxed); ptr != nullptr) {
                return *ptr;
            }

            auto* const factory_ptr = factory<T>;

            if(factory_ptr == nullptr) [[unlikely]] {
                throw std::logic_error("singleton service is not registered");
            }
            
            std::unique_ptr<T> instance{ factory_ptr->create(factory_ptr) };

            if(instance == nullptr) [[unlikely]] {
                throw std::logic_error("singleton factory returned null");
            }

            factory<T> = nullptr;
            factory_ptr->destroy(factory_ptr);

            auto* const ptr = instance.get();

            arm_service_lifetime<T>();

            published<T>.store(instance.release(), std::memory_order::release);

            return *ptr;
        }
    };

    export inline constexpr container global_ioc{};

    export [[nodiscard]] constexpr const container& ioc() noexcept {
        return global_ioc;
    }
}
