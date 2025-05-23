#pragma once
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"
#include <string_view>
#include <libassert/assert.hpp>

namespace nova {
    class logger {
    public:
        static void init() {
            const quill::BackendOptions backendOptions {
                .thread_name = "nova cxx logger",
                . enable_yield_when_idle = true,
                .sleep_duration = std::chrono::milliseconds(0),
            };

            quill::Backend::start(backendOptions);
            auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console");
            m_logger = quill::Frontend::create_or_get_logger("nova_cxx", std::move(console_sink));
            m_logger->set_log_level(quill::LogLevel::Debug);
        }

        static void shutdown() {
            quill::Backend::stop();
        }

        template<typename... Args>
        constexpr static void debug(const char* fmt, Args&&... args) {
            DEBUG_ASSERT(m_logger != nullptr);
            LOG_DEBUG(m_logger, "{}", std::vformat(fmt, std::make_format_args(args...)));
        }

        template<typename... Args>
        constexpr static void info(const char* fmt, Args&&... args) {
            DEBUG_ASSERT(m_logger != nullptr);
            LOG_INFO(m_logger, "{}", std::vformat(fmt, std::make_format_args(args...)));
        }

        template<typename... Args>
        constexpr static void warn(const char* fmt, Args&&... args) {
            DEBUG_ASSERT(m_logger != nullptr);
            LOG_WARNING(m_logger, "{}", std::vformat(fmt, std::make_format_args(args...)));
        }

        template<typename... Args>
        constexpr static void error(const char* fmt, Args&&... args) {
            DEBUG_ASSERT(m_logger != nullptr);
            LOG_ERROR(m_logger, "{}", std::vformat(fmt, std::make_format_args(args...)));
        }

        template<typename... Args>
        constexpr static void fatal(const char* fmt, Args&&... args) {
            DEBUG_ASSERT(m_logger != nullptr);
            LOG_CRITICAL(m_logger, "{}", std::vformat(fmt, std::make_format_args(args...)));
        }
    private:
        static inline quill::Logger* m_logger{nullptr};
    };
}
