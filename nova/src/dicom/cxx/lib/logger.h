#pragma once
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"
#include <libassert/assert.hpp>

namespace nova {
    class ipc_sink final : public quill::Sink {
    public:
        ipc_sink() = default;

        void write_log(quill::MacroMetadata const*, uint64_t,
                 std::string_view, std::string_view,
                 std::string const&, std::string_view,
                 quill::LogLevel lvl, std::string_view,
                 std::string_view,
                 std::vector<std::pair<std::string, std::string>> const*,
                 std::string_view msg, std::string_view log_statement) override;

        void flush_sink() noexcept override;
        void run_periodic_tasks() noexcept override;
    };

    class logger {
    public:
        static void init();
        static void shutdown();

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
