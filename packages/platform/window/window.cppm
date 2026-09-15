module;

#include <stdexcept>
#include <utility>
#define GLFW_INCLUDE_NONE
#include <cstdint>
#include <string_view>
#include <string>
#include <GLFW/glfw3.h>

export module nova.platform.window;

class platform_runtime {
public:
    platform_runtime() {
        if(glfwInit() != GLFW_TRUE) [[unlikely]] {
            const char* description{nullptr};
            glfwGetError(&description);

            const std::string error = description != nullptr 
                ? std::string(description) 
                : "Failed to initialize platform runtime";

            throw std::runtime_error(error);
        }
    }

    platform_runtime(const platform_runtime&) = delete;
    platform_runtime& operator=(const platform_runtime&) = delete;
    platform_runtime(platform_runtime&&) = delete;
    platform_runtime& operator=(platform_runtime&&) = delete;

    ~platform_runtime() {
        glfwTerminate();
    }
};

void ensure_platform_runtime() {
    static platform_runtime runtime;
}

[[nodiscard]] std::uint32_t dimension(const int value) noexcept {
    return static_cast<std::uint32_t>(std::max(value, 0));
}

export namespace nova::platform {
    struct extent2d {
        [[nodiscard]] constexpr bool empty() const noexcept {
            return width == 0 || height == 0;
        }

        auto operator<=>(const extent2d&) const = default;

        std::uint32_t width;
        std::uint32_t height;
    };

    struct window_desc {
        std::string_view title{"Nova"};
        extent2d size {.width = 1600, .height = 900};
        bool resizable = true;
    };

    class window final {
    public:
        explicit window(window_desc desc = {}) {
            ensure_platform_runtime();
            
            glfwDefaultWindowHints();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);

            const std::string title{desc.title};

            m_handle = glfwCreateWindow(
                static_cast<int>(desc.size.width),
                static_cast<int>(desc.size.height),
                title.c_str(),
                nullptr,
                nullptr
            );

            if(m_handle == nullptr) [[unlikely]] {
                const char* description{nullptr};
                glfwGetError(&description);

                const std::string error = description != nullptr
                    ? std::string(description)
                    : "Failed to create platform window";

                throw std::runtime_error(error);
            }
        }

        ~window() {
            if(m_handle != nullptr) {
                glfwDestroyWindow(m_handle);
            }
        }

        window(const window&) = delete;
        window& operator=(const window&) = delete;

        window(window&& rhs) noexcept
            :
            m_handle(std::exchange(rhs.m_handle, nullptr))
        {}

        window& operator=(window&& rhs) noexcept {
            if(this == &rhs) [[unlikely]] {
                return *this;
            }

            if(m_handle != nullptr) {
                glfwDestroyWindow(m_handle);
            }

            m_handle = std::exchange(rhs.m_handle, nullptr);
            return *this;
        }

        [[nodiscard]] bool should_close() const noexcept {
            return glfwWindowShouldClose(m_handle) == GLFW_TRUE;
        }

        void request_close() noexcept {
            glfwSetWindowShouldClose(m_handle, GLFW_TRUE);
        }

        void poll_events() const noexcept { glfwPollEvents(); }

        [[nodiscard]] extent2d size() const noexcept {
            int width{};
            int height{};

            glfwGetWindowSize(m_handle, &width, &height);

            return {
                .width = dimension(width),
                .height = dimension(height)
            };
        }

        [[nodiscard]] extent2d framebuffer_size() const noexcept {
            int width{};
            int height{};

            glfwGetFramebufferSize(m_handle, &width, &height);

            return {
                .width = dimension(width),
                .height = dimension(height)
            };
        }
    private:
        GLFWwindow* m_handle;
    };
}
