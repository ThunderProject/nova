#include "logging/logger.h"
#include "core/result.h"
#include "core/scope_exit.h"
#include <coroutine>
#include <print>
#include <chrono>

import cpu_scheduler;
import core.crash_handler;
import nova.di.singleton;
import nova.platform.window;
import nova.render.control;
import nova.render.renderer;
import nova.render.frame;

nova::coro::task<nova::result<nova::ok>> render_main(
    nova::render::render_control& control, 
    nova::platform::presentation_handle presentation
) {
    using clock = std::chrono::steady_clock;

    auto fps_window_start = clock::now();
    std::uint64_t frame_count = 0;

    auto& scheduler = nova::ioc::ioc().resolve<nova::cpu_scheduler<>>();
    auto extent = control.extent();

    while(extent.empty() && !control.stop_requested()) {
        co_await scheduler.yield();
        extent = control.extent();
    }

    if(control.stop_requested()) {
        co_return nova::ok{};
    }

    auto renderer_res = nova::render::renderer::create(
        presentation,
        extent,
        {
            .clear_color = {
                .red = 176.0f / 255.0f,
                .green = 196.0f / 255.0f,
                .blue = 222.0f / 255.0f,
                .alpha = 1.0f
            }
        }
    );

    if(!renderer_res) {
        co_return nova::err(std::move(renderer_res.error()));
    }

    auto renderer = std::move(*renderer_res);

    while(!control.stop_requested()) {
        extent = control.extent();

        if(extent.empty()) {
            co_await scheduler.yield();
            continue;
        }

        auto frame = renderer.begin_frame();
        using nova::render::color;
        using nova::render::position3;

        frame.reserve_lines(16);

        frame.draw_line(
            position3{-0.9f, 0.8f, 0.0f},
            position3{0.9f, 0.8f, 0.0f},
            color{255, 255, 255, 255},
            0.5f
        );

        frame.draw_line(
            position3{-0.9f, 0.6f, 0.0f},
            position3{0.9f, 0.6f, 0.0f},
            color{255, 255, 255, 255},
            1.0f
        );

        frame.draw_line(
            position3{-0.9f, 0.4f, 0.0f},
            position3{0.9f, 0.4f, 0.0f},
            color{255, 255, 255, 255},
            1.5f
        );

        frame.draw_line(
            position3{-0.9f, 0.2f, 0.0f},
            position3{0.9f, 0.2f, 0.0f},
            color{255, 255, 255, 255},
            2.0f
        );

        frame.draw_line(
            position3{-0.9f, 0.0f, 0.0f},
            position3{0.9f, 0.0f, 0.0f},
            color{139, 92, 246, 255},
            4.0f
        );

        frame.draw_line(
            position3{-0.9f, -0.3f, 0.0f},
            position3{0.9f, -0.8f, 0.0f},
            color{255, 80, 80, 255},
            6.0f
        );

        frame.draw_line(
            position3{-0.9f, -0.8f, 0.0f},
            position3{0.9f, -0.3f, 0.0f},
            color{80, 255, 160, 180},
            10.0f
        );

        frame.draw_line(
            position3{0.0f, -0.9f, 0.0f},
            position3{0.0f, 0.9f, 0.0f},
            color{80, 160, 255, 220},
            2.0f
        );
        auto render_res = co_await renderer.render(std::move(frame));

        if(!render_res) {
            co_return nova::err(std::move(render_res.error()));
        }

        ++frame_count;

        const auto now = clock::now();
        const auto elapsed = now - fps_window_start;

        if(elapsed >= std::chrono::seconds{5}) {
            const auto seconds = std::chrono::duration<double>(elapsed).count();
            const auto fps = static_cast<double>(frame_count) / seconds;
            const auto frame_ms = 1000.0 / fps;

            nova::logger::info("FPS: {:.1f} | frame: {:.3f} ms", fps, frame_ms);

            frame_count = 0;
            fps_window_start = now;
        }

        if(*render_res == nova::render::frame_status::out_of_date) {
            auto resize_res = renderer.recreate_swapchain(presentation, control.extent());

            if(!resize_res) {
                co_return nova::err(std::move(resize_res.error()));
            }
        }
    }

    co_return nova::ok{};
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
            window.wait_events();
            render_control.set_extent(window.framebuffer_size());
        }

        render_control.request_stop();
        auto render_res = render_fut.get();

        if(!render_res) {
            nova::logger::fatal("Render thread failed: {}", render_res.error());

            return EXIT_FAILURE;
        }

        nova::logger::info("Shutting down Nova renderer");
        return EXIT_SUCCESS;
    }
    catch(const std::exception& e) {
        nova::logger::fatal("Fatal renderer error: {}", e.what());
        return EXIT_FAILURE;
    }
    catch(...) {
        nova::logger::fatal("Fatal renderer error: unknown exception");
        return EXIT_FAILURE;
    }
}
