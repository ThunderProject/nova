#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include "result.h"
#include "logger.h"

namespace nova {

    template<class T>
    concept json_deserializable = requires(nlohmann::json nlj)
    {
        { nlj.get<T>() } -> std::same_as<T>;
    };

    class json {
    public:
        template<json_deserializable T>
        static [[nodiscard]] result<T> parse(const std::filesystem::path& path) noexcept {
            try {
                std::ifstream file(path);
                if (!file.is_open()) {
                    logger::error("json::parse failed to open file '{}'", path.string());
                    return std::unexpected("Failed to open file");
                }
                return nlohmann::json::parse(file).get<T>();
            }
            catch (const std::exception& e) {
                logger::error("json::parse failed to parse file. Reason: {}", e.what());
                return std::unexpected("Failed to parse json");
            }
        }
    };
}
