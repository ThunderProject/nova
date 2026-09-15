#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <source_location>
#include <utility>
#include <string_view>

namespace nova {
    using logFunc = void (*)(const char* level, const char* message, std::size_t msg_size);

    struct log_format {
        const char* value;
        std::source_location location;

        constexpr log_format(const char* value, std::source_location location = std::source_location::current()) noexcept
            :
            value(value),
            location(location)
        {}
    };

    class logger {
    public:
        static void init();
        static void shutdown();

        template<class... Args>
        static void debug(log_format fmt, Args&&... args) noexcept {
            format_and_write(severity::debug, fmt, std::forward<Args>(args)...);
        }

        template<class... Args>
        static void info(log_format fmt, Args&&... args) noexcept {
            format_and_write(severity::info, fmt, std::forward<Args>(args)...);
        }

        template<class... Args>
        static void warn(log_format fmt, Args&&... args) noexcept {
            format_and_write(severity::warn, fmt, std::forward<Args>(args)...);
        }

        template<class... Args>
        static void error(log_format fmt, Args&&... args) noexcept {
            format_and_write(severity::error, fmt, std::forward<Args>(args)...);
        }

        template<class... Args>
        static void fatal(log_format fmt, Args&&... args) noexcept {
            format_and_write(severity::fatal, fmt, std::forward<Args>(args)...);
        }

        static void set_log_callback(logFunc cb);
    private:
        enum class severity : std::uint8_t {
            debug,
            info,
            warn,
            error,
            fatal
        };

        template<class... Args>
        static void format_and_write(const severity level, const log_format fmt, Args&&... args) noexcept {
            try {
                if constexpr(sizeof...(Args) == 0) {
                    write(level, fmt.value, fmt.location);
                }
                else {
                    const auto message = std::vformat(fmt.value, std::make_format_args(args...));
                    write(level, message, fmt.location);
                }
            }
            catch(...) { // NOLINT(bugprone-empty-catch)
            }
        }

        static void write(severity level, std::string_view message, std::source_location location) noexcept;
    };
}
