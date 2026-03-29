#pragma once
#include <cstddef>
#include <format>
#include <string_view>

namespace nova {
    using logFunc = void(*)(const char* level, const char* message, size_t msg_size);

    class logger {
    public:
        static void init();
        static void shutdown();

        template<class... Args>
        constexpr static void debug(const char* fmt, Args&&... args) {
            debug(std::vformat(fmt, std::make_format_args(args...)));
        }

        template<class... Args>
        constexpr static void info(const char* fmt, Args&&... args) {
            info(std::vformat(fmt, std::make_format_args(args...)));
        }

        template<class... Args>
        constexpr static void warn(const char* fmt, Args&&... args) {
            warn(std::vformat(fmt, std::make_format_args(args...)));
        }

        template<class... Args>
        constexpr static void error(const char* fmt, Args&&... args) {
            error(std::vformat(fmt, std::make_format_args(args...)));
        }

        template<class... Args>
        constexpr static void fatal(const char* fmt, Args&&... args) {
            fatal(std::vformat(fmt, std::make_format_args(args...)));
        }

        static void set_log_callback(logFunc cb);
    private:
        static void debug(std::string_view message);
        static void info(std::string_view message);
        static void warn(std::string_view message);
        static void error(std::string_view message);
        static void fatal(std::string_view message);
    };
}
