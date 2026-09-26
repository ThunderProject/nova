#include "logging/logger.h"
#include "core/scope_exit.h"
#include <coroutine>
#include <print>

import cpu_scheduler;
import nova.platform.window;
import nova.di.singleton;
import nova.render.control;
import core.crash_handler;

nova::coro::task<void> render_main(
    nova::render::render_control& control, 
    [[maybe_unused]] nova::platform::presentation_handle presentation
) {
    auto& scheduler = nova::ioc::ioc().resolve<nova::cpu_scheduler<>>();

    while(!control.stop_requested()) {
        const auto extent = control.extent();

        if(extent.empty()) {
            co_await scheduler.yield();
            continue;
        }
    }
    co_return;
}

int main() {
    try {
        const auto crash_handler_result = nova::crash::install();
        if(!crash_handler_result.has_value()) {
            std::println(
                stderr, 
                "Failed to install crash handler: error={}, errno={}",
                static_cast<int>(crash_handler_result.error().code),
                crash_handler_result.error().system_error
            );
            return EXIT_FAILURE;
        }

        nova::logger::init();
        const nova::scope_exit logger_shutdown{ [] noexcept { nova::logger::shutdown(); }};

        nova::logger::info("Starting Nova renderer");

        auto& scheduler = nova::ioc::ioc().register_service<nova::cpu_scheduler<>>();
        const nova::scope_exit scheduler_shutdown { [&scheduler] noexcept { scheduler.shutdown(); } };

        nova::logger::info("Scheduler initialized with {} workers", scheduler.worker_count());

        nova::platform::window window{
            {
                .title = "Nova",
                .size = {.width = 1600, .height = 900},
                .resizable = true
            }
        };

        nova::logger::info("Platform window initialized");

        auto render_control = nova::render::render_control(window.framebuffer_size());
        auto render_fut = scheduler.submit(render_main(render_control, window.presentation()));

        while(!window.should_close()) {
            window.poll_events();
            render_control.set_extent(window.framebuffer_size());
        }

        render_control.request_stop();
        render_fut.get();

        nova::logger::info("Shutting down Nova renderer");

        return 0;
    }
    catch(const std::exception& e) {
        nova::logger::fatal("Fatal renderer error: {}", e.what());
        return -1;
    }
    catch(...) {
        nova::logger::fatal("Fatal renderer error: unknown exception");
        return -1;
    }
}
