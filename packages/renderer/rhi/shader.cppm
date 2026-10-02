module;

#include "core/result.h"
#include <cstddef>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

export module nova.render.rhi.shader;

export namespace nova::render::rhi {
    class shader_blob final {
    public:
        shader_blob() = default;

        [[nodiscard]] static nova::result<shader_blob> load(const std::string_view path) {
            std::ifstream stream(std::string{path}, std::ios::binary | std::ios::ate);

            if(!stream) [[unlikely]] {
                return nova::err(std::format("Failed to open shader: {}", path));
            }

            const auto end = stream.tellg();
            
            if(end <= 0) [[unlikely]] {
                return nova::err(std::format("Shader is empty: {}", path));
            }

            shader_blob result;
            result.m_data.resize(static_cast<std::size_t>(end));

            stream.seekg(0, std::ios::beg);
            stream.read(
                reinterpret_cast<char*>(result.m_data.data()),
                static_cast<std::streamsize>(result.m_data.size())
            );

            if(!stream) [[unlikely]] {
                return nova::err(std::format("Failed to read shader: {}", path));
            }

            return result;
        }

        [[nodiscard]] const void* data() const noexcept {
            return m_data.data();
        }

        [[nodiscard]] std::size_t size() const noexcept {
            return m_data.size();
        }
    private:
        std::vector<std::byte> m_data;
    };
}
