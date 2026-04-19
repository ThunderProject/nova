#pragma once
#include "result.h"
#include <concepts>
#include <exception>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>

namespace nova {
    template<class T>
    concept json_deserializable = requires(nlohmann::json nlj)
    {
        { nlj.get<T>() } -> std::same_as<T>;
    };

    template<class T>
    concept json_serializable = requires(T t)
    {
        { nlohmann::json(t) } -> std::same_as<nlohmann::json>;
    };

    class json {
    public:
        template<json_deserializable T>
        [[nodiscard]] static result<T> parse(const std::filesystem::path& path) noexcept {
            try {
                std::ifstream file(path);
                if (!file.is_open()) {
                    return nova::err("Failed to open file");
                }
                return nlohmann::json::parse(file).get<T>();
            }
            catch (const std::exception& e) {
                return nova::err("Failed to parse json");
            }
        }

        template<json_serializable T>
        [[nodiscard]] static std::string to_string(const T& obj) noexcept {
            try {
                return nlohmann::json(obj).dump();
            }
            catch (const std::exception& e) {
                return "";
            }
        }
    };
}
