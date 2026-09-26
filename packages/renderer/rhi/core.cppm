module;
#include "result.h"
#include <NRI.h>
#include <format>
#include <magic_enum/magic_enum.hpp>
export module nova.render.rhi.core;

export namespace nova::render::rhi {
    [[nodiscard]] std::string rhi_error(const nri::Result res) {
        return std::format("failed with NRI result {}", magic_enum::enum_name(res));
    }

    [[nodiscard]] nova::result<nova::ok> check(const nri::Result res) {
        if(res != nri::Result::SUCCESS) [[unlikely]] {
            return nova::err(rhi_error(res));
        }
        return nova::ok{};
    }
}
