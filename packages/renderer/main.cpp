#include "logging/logger.h"
#include "core/scope_exit.h"

import cpu_scheduler;
import nova.platform.window;
import nova.di.singleton;

int main() {
    nova::logger::init();
    const nova::scope_exit logger_shutdown{ [] noexcept { nova::logger::shutdown(); }};

    try {
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

        while(!window.should_close()) {
            window.poll_events();
        }

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
