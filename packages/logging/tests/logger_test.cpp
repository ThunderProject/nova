#include "logger.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <format>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>

class log_guard {
public:
    log_guard() { nova::logger::init(); }
    ~log_guard() { nova::logger::shutdown(); }
};

class log_messages_queue {
public:
    void push(std::string_view msg) {
        std::scoped_lock lk{log_messages_mutex};
        log_messages.emplace(msg);
        m_signal.test_and_set(std::memory_order_release);
        m_signal.notify_one();
    }

    [[nodiscard]] std::string pop() {
        m_signal.wait(false, std::memory_order_acquire);
        std::scoped_lock lk{log_messages_mutex};
        auto msg = log_messages.front();
        log_messages.pop();
        m_signal.clear(std::memory_order_relaxed);
        return msg;
    }
private:
    std::queue<std::string> log_messages;
    std::mutex log_messages_mutex;
    std::atomic_flag m_signal;
};

static log_messages_queue log_messages{};

static void log_callback(const char* level, const char* msg, size_t msg_size) {
    log_messages.push(std::format("[{}] {}", level, std::string_view{msg, msg_size}));
}

TEST_CASE("logger") {
    SECTION("init and shutdown") {
        nova::logger::init();
        nova::logger::shutdown();

        SUCCEED();
    }

    log_guard guard{};
    nova::logger::set_log_callback(log_callback);

    SECTION("writes log messages correctly") {
        nova::logger::debug("debug test");
        CHECK(log_messages.pop() == "[debug] debug test");

        nova::logger::info("info test");
        CHECK(log_messages.pop() == "[info] info test");

        nova::logger::warn("warn test");
        CHECK(log_messages.pop() == "[warning] warn test");

        nova::logger::error("error test");
        CHECK(log_messages.pop() == "[error] error test");

        nova::logger::fatal("fatal test");
        CHECK(log_messages.pop() == "[critical] fatal test");

        SUCCEED();
    }
}
