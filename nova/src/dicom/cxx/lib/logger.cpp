#include "logger.h"
#include <ranges>
#include "enum_utils.h"
#include "../api/log_api.h"

namespace rn = std::ranges;
namespace vi = rn::views;

void nova::ipc_sink::write_log(quill::MacroMetadata const*,
    uint64_t,
    std::string_view,
    std::string_view,
    std::string const&,
    std::string_view, quill::LogLevel lvl,
    std::string_view,
    std::string_view,
    std::vector<std::pair<std::string, std::string>> const*,
    std::string_view msg,
    std::string_view log_statement)
{
    const auto levelStr = enum_to_string(lvl)
              | vi::transform([](const auto c) { return static_cast<char>(std::tolower(c)); })
              | rn::to<std::string>();

    api::send_log(levelStr, std::string(msg));
}

void nova::ipc_sink::flush_sink() noexcept { }

void nova::ipc_sink::run_periodic_tasks() noexcept { }

void nova::logger::init() {
    const quill::BackendOptions backendOptions {
        .thread_name = "nova cxx logger",
        .enable_yield_when_idle = true,
        .sleep_duration = std::chrono::milliseconds(0),
    };

    quill::Backend::start(backendOptions);
    auto sink = quill::Frontend::create_or_get_sink<ipc_sink>("ipc_sink");
    m_logger = quill::Frontend::create_or_get_logger("nova_cxx", std::move(sink));
    m_logger->set_log_level(quill::LogLevel::Debug);
}

void nova::logger::shutdown() {
    quill::Backend::stop();
}
