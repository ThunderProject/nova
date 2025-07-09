#pragma once
#include <string>

namespace nova::api {
    using logFunc = void(*)(const char* level, const char* message);

    extern "C" __declspec(dllexport) void set_log_callback(logFunc cb);
    void send_log(const std::string& level, const std::string& msg);
}
