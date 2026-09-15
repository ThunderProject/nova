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
#include <quill/sinks/FileSink.h>
#include <libassert/assert.hpp>
#include <cstdint>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "magic_enum/magic_enum.hpp"

using namespace std::chrono_literals;
namespace rn = std::ranges;
namespace vi = std::views;

namespace {
    [[nodiscard]] std::filesystem::path log_path() {
        if(const char* home = std::getenv("HOME"); home != nullptr) {
            return std::filesystem::path{home}/".local"/"state"/"nova"/"nova.log";
        }

        return std::filesystem::temp_directory_path()/"nova"/"nova.log";
    }

    inline quill::Logger* logger_instance{nullptr};
    std::atomic<nova::logFunc> logger_callback{nullptr};
}

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
        const auto level_str = magic_enum::enum_name(lvl)
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
    auto cb_sink = quill::Frontend::create_or_get_sink<log_sink>("ipc_sink");

    const auto path = log_path();
    std::filesystem::create_directories(path.parent_path());

    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        path.string(),
        [] {
            quill::FileSinkConfig cfg;
            cfg.set_open_mode('a');

            return cfg;
        }(),
        quill::FileEventNotifier{}
    );

    quill::PatternFormatterOptions formatter {
        "%(time) [%(thread_id)] %(short_source_location) LOG_%(log_level) %(logger) %(message)", 
        "%H:%M:%S.%Qns",
        quill::Timezone::LocalTime
    };

    logger_instance = quill::Frontend::create_or_get_logger(
        "nova", 
        { std::move(cb_sink), std::move(file_sink) }, 
        formatter
    );
    logger_instance->set_log_level(quill::LogLevel::Debug);
}

void nova::logger::shutdown() {
    if(logger_instance != nullptr) {
        logger_instance->flush_log();
    }

    quill::Backend::stop();
    logger_instance = nullptr;
}

void nova::logger::set_log_callback(logFunc cb) {
    logger_callback.store(cb, std::memory_order_relaxed);
}

void nova::logger::write(const severity level, const std::string_view message, const std::source_location location) noexcept {
    if(logger_instance == nullptr) {
        return;
    }

    try {
        const auto quill_level = [&] {
            switch(level) {
                case severity::debug: return quill::LogLevel::Debug;
                case severity::info: return quill::LogLevel::Info;
                case severity::warn: return quill::LogLevel::Warning;
                case severity::error: return quill::LogLevel::Error;
                case severity::fatal: return quill::LogLevel::Critical;
            }
            std::unreachable();
        }();

        QUILL_LOG_RUNTIME_METADATA(
            logger_instance,
            quill_level,
            location.file_name(),
            static_cast<std::uint32_t>(location.line()),
            location.function_name(),
            "{}",
            message
        );
    }
    catch(...) { // NOLINT(bugprone-empty-catch)
    }
}
