module;

#include "result.h"

#include <NRI.h>
#include <assert.hpp>
#include <atomic>
#include <coroutine>
#include <cstdint>
#include <exception>
#include <stop_token>
#include <thread>
#include "logging/logger.h"

export module nova.render.rhi.gpu_waiter;

import cpu_scheduler;
import nova.di.singleton;
import nova.render.rhi.frame_context;
import nova.render.rhi.swapchain;
import thread_parker;

export namespace nova::render::rhi {
    class gpu_waiter final {
        enum class wait_kind : std::uint8_t {
            present,
            frame
        };

        struct request {
            wait_kind kind{};
            nova::cpu_scheduler<>* scheduler{nullptr};
            nova::schedulable_work* work{nullptr};
            swapchain* chain{nullptr};
            frame_context* frame{nullptr};
        };

    public:
        gpu_waiter()
            :
            m_thread([this](const std::stop_token& stop) noexcept {
                run(stop);
            })
        {}

        gpu_waiter(const gpu_waiter&) = delete;
        gpu_waiter& operator=(const gpu_waiter&) = delete;
        gpu_waiter(gpu_waiter&&) = delete;
        gpu_waiter& operator=(gpu_waiter&&) = delete;

        ~gpu_waiter() noexcept {
            DEBUG_ASSERT(m_request.load(std::memory_order::acquire) == nullptr);

            m_thread.request_stop();
            m_parker.unpark();

            if(m_thread.joinable()) {
                m_thread.join();
            }
        }

        class present_awaiter final {
        public:
            present_awaiter(gpu_waiter& waiter, swapchain& chain, nova::cpu_scheduler<>& scheduler)
                :
                m_waiter(&waiter),
                m_scheduler(&scheduler),
                m_chain(&chain)
            {}

            [[nodiscard]] static constexpr bool await_ready() noexcept {
                return false;
            }

            template<nova::schedulable_promise Promise>
            void await_suspend(const std::coroutine_handle<Promise> current) noexcept {
                m_request.kind = wait_kind::present;
                m_request.scheduler = m_scheduler;
                m_request.work = nova::as_work(current);
                m_request.chain = m_chain;
                m_request.frame = nullptr;

                m_waiter->enqueue(&m_request);
            }

            [[nodiscard]] nova::result<nova::ok, nri::Result> await_resume() const noexcept {
                const auto result = m_waiter->m_present_result.load(std::memory_order::acquire);

                if(result != nri::Result::SUCCESS) {
                    return nova::err(result);
                }

                return nova::ok{};
            }

        private:
            gpu_waiter* m_waiter;
            nova::cpu_scheduler<>* m_scheduler;
            swapchain* m_chain;
            request m_request;
        };

        class frame_awaiter final {
        public:
            frame_awaiter(gpu_waiter& waiter, frame_context& frame, nova::cpu_scheduler<>& scheduler)
                :
                m_waiter(&waiter),
                m_scheduler(&scheduler),
                m_frame(&frame)
            {}

            [[nodiscard]] bool await_ready() const noexcept {
                return m_frame->complete();
            }

            template<nova::schedulable_promise Promise>
            void await_suspend(const std::coroutine_handle<Promise> current) noexcept {
                m_request.kind = wait_kind::frame;
                m_request.scheduler = m_scheduler;
                m_request.work = nova::as_work(current);
                m_request.chain = nullptr;
                m_request.frame = m_frame;

                m_waiter->enqueue(&m_request);
            }

            static void await_resume() noexcept {}

        private:
            gpu_waiter* m_waiter;
            nova::cpu_scheduler<>* m_scheduler;
            frame_context* m_frame;
            request m_request;
        };

        [[nodiscard]] present_awaiter wait_present(swapchain& chain, cpu_scheduler<>& scheduler) {
            return present_awaiter{*this, chain, scheduler};
        }

        [[nodiscard]] frame_awaiter wait_frame(frame_context& frame, cpu_scheduler<>& scheduler) {
            return frame_awaiter{*this, frame, scheduler};
        }
    private:
        void enqueue(request* const req) noexcept {
            DEBUG_ASSERT(req != nullptr);

            request* expected = nullptr;

            if(!m_request.compare_exchange_strong(
                expected,
                req,
                std::memory_order::release,
                std::memory_order::relaxed
            )) [[unlikely]] {
                logger::error("GPU waiter enqueue failed: another request is already pending");
                std::terminate();
            }

            m_parker.unpark();
        }

        void run(const std::stop_token& stop) noexcept {
            for(;;) {
                m_parker.park();
                
                if(stop.stop_requested()) {
                    return;
                }

                auto* const req = m_request.load(std::memory_order::acquire);

                DEBUG_ASSERT(req != nullptr);
                DEBUG_ASSERT(req->scheduler != nullptr);
                DEBUG_ASSERT(req->work != nullptr);

                switch(req->kind) {
                    case wait_kind::present:
                        DEBUG_ASSERT(req->chain != nullptr);
                        m_present_result.store(req->chain->wait_for_present(), std::memory_order::release);
                        break;
                    case wait_kind::frame:
                        DEBUG_ASSERT(req->frame != nullptr);
                        req->frame->wait();
                        break;
                }

                m_request.store(nullptr, std::memory_order::release);

                if(!req->scheduler->post_external(req->work)) [[unlikely]] {
                    logger::error("GPU waiter failed to resume suspended coroutine: scheduler rejected external work");
                    std::terminate();
                }
            }
        }

        std::atomic<request*> m_request{nullptr};
        std::atomic<nri::Result> m_present_result{nri::Result::SUCCESS};

        nova::thread_parker m_parker{};
        std::jthread m_thread;
    };
}
