#include "logger.h"
#include <atomic>
#include <cctype>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/backend/BackendOptions.h>
#include <quill/core/LogLevel.h>
#include <quill/core/MacroMetadata.h>
#include <quill/sinks/Sink.h>
#include <libassert/assert.hpp>
#include <cstdint>
#include <ranges>
#include <chrono>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "core/enum_utils.h"

using namespace std::chrono_literals;
namespace rn = std::ranges;
namespace vi = std::views;

static inline quill::Logger* logger_instance{nullptr};
static std::atomic<nova::logFunc> logger_callback{nullptr};

class log_sink final : public quill::Sink {
public:
    log_sink() = default;

    void write_log(quill::MacroMetadata const*, uint64_t,
            std::string_view, std::string_view,
            std::string const&, std::string_view,
            quill::LogLevel lvl, std::string_view,
            std::string_view,
            std::vector<std::pair<std::string, std::string>> const*,
            std::string_view msg, std::string_view) override
    {
        const auto level_str = nova::enum_to_string(lvl)
            | vi::transform([](auto c) { return std::tolower(c); })
            | rn::to<std::string>();

        if (const auto cb = logger_callback.load(std::memory_order_relaxed)) {
            cb(level_str.c_str(), msg.data(), msg.size());
        }
    }

    void flush_sink() noexcept override {}
    void run_periodic_tasks() noexcept override {}
};

void nova::logger::init() {
    const quill::BackendOptions backend_options {
        .thread_name = "nova cxx logger",
        .enable_yield_when_idle = true,
        .sleep_duration = 0ms,
    };

    quill::Backend::start(backend_options);
    auto sink = quill::Frontend::create_or_get_sink<log_sink>("ipc_sink");
    logger_instance = quill::Frontend::create_or_get_logger("nova_cxx", std::move(sink));
    logger_instance->set_log_level(quill::LogLevel::Debug);
}

void nova::logger::shutdown() {
    quill::Backend::stop();
    logger_instance = nullptr;
}

void nova::logger::set_log_callback(logFunc cb) {
    logger_callback.store(cb, std::memory_order_relaxed);
}

void nova::logger::debug(std::string_view message) {
    DEBUG_ASSERT(logger_instance != nullptr);
    LOG_DEBUG(logger_instance, "{}", message);
}

void nova::logger::info(std::string_view message) {
    DEBUG_ASSERT(logger_instance != nullptr);
    LOG_INFO(logger_instance, "{}", message);
}

void nova::logger::warn(std::string_view message) {
    DEBUG_ASSERT(logger_instance != nullptr);
    LOG_WARNING(logger_instance, "{}", message);
}

void nova::logger::error(std::string_view message) {
    DEBUG_ASSERT(logger_instance != nullptr);
    LOG_ERROR(logger_instance, "{}", message);
}

void nova::logger::fatal(std::string_view message) {
    DEBUG_ASSERT(logger_instance != nullptr);
    LOG_CRITICAL(logger_instance, "{}", message);
}
