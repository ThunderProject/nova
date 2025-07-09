#include "log_api.h"
#include <atomic>

std::atomic<nova::api::logFunc> loggerCallback{nullptr};

void nova::api::set_log_callback(logFunc cb) {
    loggerCallback.store(cb, std::memory_order_relaxed);
}

void nova::api::send_log(const std::string &level, const std::string &msg) {
    if (const auto cb = loggerCallback.load(std::memory_order_relaxed)) {
        cb(level.c_str(), msg.c_str());
    }
}
