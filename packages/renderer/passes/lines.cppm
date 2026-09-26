module;

#include <assert.hpp>
#include <cstddef>
#include <cstdint>

export module nova.render.passes.lines;

import nova.render.graph.resource;
import nova.render.graph.pass;
import nova.render.graph.render_graph;

export namespace nova::render::passes {
    struct line_position {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};

        auto operator<=>(const line_position&) const = default;
    };

    struct line_color {
        std::uint8_t red{255};
        std::uint8_t green{255};
        std::uint8_t blue{255};
        std::uint8_t alpha{255};

        [[nodiscard]] constexpr std::uint32_t packed() const noexcept {
            return static_cast<std::uint32_t>(red) |
                static_cast<std::uint32_t>(green) << 8 |
                static_cast<std::uint32_t>(blue) << 16 |
                static_cast<std::uint32_t>(alpha) << 24;
        }

        auto operator<=>(const line_color&) const = default;
    };

    struct line_instance {
        line_position start;
        float width{1.0f};

        line_position end;
        std::uint32_t color{line_color{}.packed()};
    };

    static_assert(sizeof(line_instance) == 32);
    static_assert(offsetof(line_instance, start) == 0);
    static_assert(offsetof(line_instance, width) == 12);
    static_assert(offsetof(line_instance, end) == 16);
    static_assert(offsetof(line_instance, color) == 28);

    class line_pass final {
    public:
        line_pass() noexcept = default;

        line_pass(const graph::texture_handle target, const graph::buffer_handle instances) noexcept
            :
            m_target(target),
            m_instances(instances)
        {
            DEBUG_ASSERT(target.valid());
            DEBUG_ASSERT(instances.valid());
        }

        [[nodiscard]] graph::pass_handle add_to(graph::render_graph& graph) {
            DEBUG_ASSERT(m_target.valid());
            DEBUG_ASSERT(m_instances.valid());
            DEBUG_ASSERT(!m_pass.valid());

            const auto target = m_target;
            const auto instances = m_instances;

            m_pass = graph.add_pass(
                {
                    .name = "lines",
                    .queue = graph::pass_queue::graphics
                },
                [target, instances](graph::pass_builder& builder) {
                    builder.read(
                        instances,
                        graph::resource_usage::shader_resource,
                        graph::shader_stage::mesh
                    );

                    builder.read_write(
                        target,
                        graph::resource_usage::color_attachment
                    );
                }
            );

            return m_pass;
        }

        void target(const graph::texture_handle target) noexcept {
            DEBUG_ASSERT(target.valid());
            DEBUG_ASSERT(!m_pass.valid());

            m_target = target;
        }

        void instances(const graph::buffer_handle instances) noexcept {
            DEBUG_ASSERT(instances.valid());
            DEBUG_ASSERT(!m_pass.valid());

            m_instances = instances;
        }

        [[nodiscard]] graph::texture_handle target() const noexcept {
            return m_target;
        }

        [[nodiscard]] graph::buffer_handle instances() const noexcept {
            return m_instances;
        }

        [[nodiscard]] graph::pass_handle handle() const noexcept {
            return m_pass;
        }

    private:
        graph::texture_handle m_target;
        graph::buffer_handle m_instances;
        graph::pass_handle m_pass;
    };
}
