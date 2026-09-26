module;

#include "result.h"
#include <NRI.h>
#include <NRIDescs.h>
#include <assert.hpp>

export module nova.render.passes.clear;

import nova.render.graph.resource;
import nova.render.graph.pass;
import nova.render.graph.render_graph;
import nova.render.runtime.pass_context;

export namespace nova::render::passes {
    struct clear_color {
        float red{0.02f};
        float green{0.02f};
        float blue{0.025f};
        float alpha{1.0f};

        auto operator<=>(const clear_color&) const = default;
    };

    class clear_pass final {
    public:
        clear_pass() noexcept = default;

        explicit clear_pass(const graph::texture_handle target, const clear_color color = {}) noexcept
            :
            m_target(target),
            m_color(color)
        {
            DEBUG_ASSERT(target.valid());
        }

        [[nodiscard]] graph::pass_handle add_to(graph::render_graph& graph) {
            DEBUG_ASSERT(m_target.valid());
            DEBUG_ASSERT(!m_pass.valid());

            const auto target = m_target;

            m_pass = graph.add_pass(
                {
                    .name = "clear",
                    .queue = graph::pass_queue::graphics
                },
                [target](graph::pass_builder& builder) {
                    builder.write(target, graph::resource_usage::color_attachment);
                }
            );
            return m_pass;
        }

        [[nodiscard]] nova::result<nova::ok> record(runtime::pass_context& context) const {
            DEBUG_ASSERT(m_target.valid());
            DEBUG_ASSERT(m_pass.valid());

            nri::AttachmentDesc attachment{};
            attachment.descriptor = &context.color_attachment(m_target);
            attachment.clearValue.color.f.x = m_color.red;
            attachment.clearValue.color.f.y = m_color.green;
            attachment.clearValue.color.f.z = m_color.blue;
            attachment.clearValue.color.f.w = m_color.alpha;
            attachment.loadOp = nri::LoadOp::CLEAR;
            attachment.storeOp = nri::StoreOp::STORE;

            nri::RenderingDesc rendering{};
            rendering.colors = &attachment;
            rendering.colorNum = 1;

            auto& core = context.core();
            auto& command_buffer = context.command_buffer();

            core.CmdBeginRendering(command_buffer, rendering);
            core.CmdEndRendering(command_buffer);

            return nova::ok{};
        }


        void target(const graph::texture_handle target) noexcept {
            DEBUG_ASSERT(target.valid());
            DEBUG_ASSERT(!m_pass.valid());
            m_target = target;
        }

        void color(const clear_color color) noexcept {
            m_color = color;
        }

        [[nodiscard]] graph::texture_handle target() const noexcept {
            return m_target;
        }

        [[nodiscard]] clear_color color() const noexcept {
            return m_color;
        }

        [[nodiscard]] graph::pass_handle handle() const noexcept {
            return m_pass;
        }
    private:
        graph::texture_handle m_target;
        clear_color m_color{};
        graph::pass_handle m_pass;
    };
}
