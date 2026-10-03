module;

#include <assert.hpp>
#include <cstddef>
#include <span>
#include <vector>

export module nova.render.frame;

import nova.render.passes.lines;
import nova.render.color;

export namespace nova::render {
    using position3 = passes::line_position;

    class frame final {
    public:
        frame() = default;

        explicit frame(const std::size_t expected_line_count) {
            m_lines.reserve(expected_line_count);
        }

        frame(const frame&) = delete;
        frame& operator=(const frame&) = delete;

        frame(frame&&) noexcept = default;
        frame& operator=(frame&&) noexcept = default;

        void draw_line(
            const position3 start, 
            const position3 end, 
            const graphics::color color, 
            const float width = 1.0f
        ) {
            DEBUG_ASSERT(width > 0.0f);

            m_lines.emplace_back(
                passes::line_instance{
                    .start = start,
                    .width = width,
                    .end = end,
                    .color = color.packed_rgba8()
                }
            );
        }

        void reserve_lines(const std::size_t count) {
            m_lines.reserve(count);
        }

        [[nodiscard]] std::span<const passes::line_instance> lines() const noexcept {
            return m_lines;
        }

        [[nodiscard]] std::size_t line_count() const noexcept {
            return m_lines.size();
        }

        [[nodiscard]] bool empty() const noexcept {
            return m_lines.empty();
        }

    private:
        std::vector<passes::line_instance> m_lines;
    };
}
