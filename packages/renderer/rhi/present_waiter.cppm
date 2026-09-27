module;

#include "result.h"
#include <NRI.h>
#include <NRIDescs.h>
#include <assert.hpp>
#include <atomic>
#include <coroutine>
#include <exception>
#include <quill/bundled/fmt/base.h>
#include <semaphore>
#include <stop_token>
#include <thread>

export module nova.render.rhi.present_waiter;

import cpu_scheduler;
import nova.di.singleton;
import nova.render.rhi.swapchain;

export namespace nova::render::rhi {
    class present_waiter final {
        struct request {
            nova::cpu_scheduler<>* scheduler{nullptr};
            nova::schedulable_work* work{nullptr};
            swapchain* chain{nullptr};
        };
    public:
        present_waiter()
            :
            m_thread([this](const std::stop_token stop) noexcept { run(stop); })
        {}

        present_waiter(const present_waiter&) = delete;
        present_waiter& operator=(const present_waiter&) = delete;
        present_waiter(present_waiter&&) = delete;
        present_waiter& operator=(present_waiter&&) = delete;

        ~present_waiter() noexcept {
            DEBUG_ASSERT(m_request.load(std::memory_order::acquire) == nullptr);

            m_thread.request_stop();
            m_wakeup.release();

            if(m_thread.joinable()) {
                m_thread.join();
            }
        }

        class awaiter final {
        public:
            awaiter(present_waiter& waiter, swapchain& chain)
                :
                m_waiter(&waiter),
                m_scheduler(&nova::ioc::ioc().resolve<cpu_scheduler<>>()),
                m_chain(&chain)
            {}

            [[nodiscard]] static constexpr bool await_ready() noexcept {
                return false;
            }

            template<nova::schedulable_promise Promise>
            void await_suspend(const std::coroutine_handle<Promise> current) noexcept {
                m_request.scheduler = m_scheduler;
                m_request.work = nova::as_work(current);
                m_request.chain = m_chain;
                m_waiter->enqueue(&m_request);
            }

            [[nodiscard]] nova::result<nova::ok,  nri::Result> await_resume() const noexcept {
                const auto result = m_waiter->m_result.load(std::memory_order::acquire);

                if(result != nri::Result::SUCCESS) {
                    return nova::err(result);
                }

                return nova::ok{};
            }

        private:
            present_waiter* m_waiter;
            nova::cpu_scheduler<>* m_scheduler;
            swapchain* m_chain;
            request m_request;
        };

        [[nodiscard]] awaiter wait(swapchain& swapchain) {
            return awaiter { *this, swapchain };
        }
    private:
        void enqueue(request* const req) noexcept {
            DEBUG_ASSERT(request != nullptr);

            request* expected = nullptr;

            const auto inserted = m_request.compare_exchange_strong(
                expected,
                req,
                std::memory_order::release,
                std::memory_order::relaxed
            );

            if(!inserted) {
                std::terminate();
            }

            m_wakeup.release();
        }

        void run(const std::stop_token stop) noexcept {
            for(;;) {
                m_wakeup.acquire();

                if(stop.stop_requested()) {
                    return;
                }

                auto* const request = m_request.load(std::memory_order::acquire);

                DEBUG_ASSERT(request != nullptr);
                DEBUG_ASSERT(request->scheduler != nullptr);
                DEBUG_ASSERT(request->work != nullptr);
                DEBUG_ASSERT(request->chain != nullptr);

                const auto result = request->chain->wait_for_present();

                m_result.store(result, std::memory_order::release);
                m_request.store(nullptr, std::memory_order::release);

                const auto posted = request->scheduler->post_external(request->work);

                if(!posted) [[unlikely]] {
                    std::terminate();
                }
            }
        }

        std::atomic<request*> m_request{nullptr};
        std::atomic<nri::Result> m_result{nri::Result::SUCCESS};
        std::binary_semaphore m_wakeup{0};
        std::jthread m_thread;
    };
}
