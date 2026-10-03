module;

#include "result.h"
#include <NRI.h>
#include <NRIDescs.h>
#include <Extensions/NRIMeshShader.h>
#include <assert.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

export module nova.render.passes.lines;

import nova.render.graph.resource;
import nova.render.graph.pass;
import nova.render.graph.render_graph;
import nova.render.runtime.pass_context;
import nova.render.rhi.device;
import nova.render.rhi.shader;
import nova.render.rhi.core;
import nova.render.color;
import nova.math.vector;

export namespace nova::render::passes {
    inline constexpr std::uint32_t line_mesh_group_size = 32;

    struct line_position {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};

        auto operator<=>(const line_position&) const = default;
    };

    struct line_instance {
        line_position start;
        float width{1.0f};

        line_position end;
        std::uint32_t color{nova::graphics::colors::white.packed_rgba8()};
    };

    static_assert(sizeof(line_instance) == 32);
    static_assert(offsetof(line_instance, start) == 0);
    static_assert(offsetof(line_instance, width) == 12);
    static_assert(offsetof(line_instance, end) == 16);
    static_assert(offsetof(line_instance, color) == 28);

    struct line_push_constants {
        float viewport_width;
        float viewport_height;

        std::uint32_t line_count;
        float aa_radius;
    };

    static_assert(sizeof(line_push_constants) == 16);

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

        line_pass(const line_pass&) = delete;
        line_pass& operator=(const line_pass&) = delete;

        line_pass(line_pass&& rhs) noexcept
            :
            m_device(std::exchange(rhs.m_device, nullptr)),
            m_pipeline_layout(std::exchange(rhs.m_pipeline_layout, nullptr)),
            m_pipeline(std::exchange(rhs.m_pipeline, nullptr)),
            m_target(rhs.m_target),
            m_instances(rhs.m_instances),
            m_pass(rhs.m_pass)
        {}

        line_pass& operator=(line_pass&& rhs) noexcept {
            if(this == &rhs) {
                return *this;
            }

            destroy();

            m_device = std::exchange(rhs.m_device, nullptr);
            m_pipeline_layout = std::exchange(rhs.m_pipeline_layout, nullptr);
            m_pipeline = std::exchange(rhs.m_pipeline, nullptr);
            m_target = rhs.m_target;
            m_instances = rhs.m_instances;
            m_pass = rhs.m_pass;

            return *this;
        }

        ~line_pass() noexcept {
            destroy();
        }

        [[nodiscard]] nova::result<nova::ok> init(rhi::device& device, const nri::Format target_format) {
            DEBUG_ASSERT(m_pipeline_layout != nullptr);
            DEBUG_ASSERT(m_pipeline != nullptr);

            m_device = &device;

            auto mesh_shader = rhi::shader_blob::load(NOVA_SHADER_DIR "/lines.mesh.spv");
            if(!mesh_shader) {
                return nova::err(std::move(mesh_shader.error()));
            }

            auto frag_shader = rhi::shader_blob::load(NOVA_SHADER_DIR "/lines.fragment.spv");
            if(!frag_shader) {
                return nova::err(std::move(frag_shader.error()));
            }

            nri::RootConstantDesc constants = {};
            constants.registerIndex = 0;
            constants.size = sizeof(line_push_constants);
            constants.shaderStages = nri::StageBits::MESH_SHADER | nri::StageBits::FRAGMENT_SHADER;

            nri::RootDescriptorDesc lines_desc = {};
            lines_desc.registerIndex = 0;
            lines_desc.descriptorType = nri::DescriptorType::STRUCTURED_BUFFER;
            lines_desc.shaderStages = nri::StageBits::MESH_SHADER;

            nri::PipelineLayoutDesc pipeline_layout_desc = {};
            pipeline_layout_desc.rootRegisterSpace = 0;
            pipeline_layout_desc.rootConstants = &constants;
            pipeline_layout_desc.rootConstantNum = 1;
            pipeline_layout_desc.rootDescriptors = &lines_desc;
            pipeline_layout_desc.rootDescriptorNum = 1;
            pipeline_layout_desc.shaderStages = nri::StageBits::MESH_SHADER | nri::StageBits::FRAGMENT_SHADER;

            auto result = rhi::check(device.core().CreatePipelineLayout(
                device.native(),
                pipeline_layout_desc,
                m_pipeline_layout
            ));

            if(!result) {
                return nova::err(std::move(result.error()));
            }

            const std::array shaders = {
                nri::ShaderDesc {
                    .stage = nri::StageBits::MESH_SHADER,
                    .bytecode = mesh_shader->data(),
                    .size = mesh_shader->size(),
                    .entryPointName = "line_mesh"
                },
                nri::ShaderDesc {
                    .stage = nri::StageBits::FRAGMENT_SHADER,
                    .bytecode = frag_shader->data(),
                    .size = frag_shader->size(),
                    .entryPointName = "line_fragment"
                }
            };

            nri::ColorAttachmentDesc color{};
            color.format = target_format;
            color.colorBlend = {
                .srcFactor = nri::BlendFactor::ONE,
                .dstFactor = nri::BlendFactor::ONE_MINUS_SRC_ALPHA,
                .op = nri::BlendOp::ADD
            };
            color.alphaBlend = {
                .srcFactor = nri::BlendFactor::ONE,
                .dstFactor = nri::BlendFactor::ONE_MINUS_SRC_ALPHA,
                .op = nri::BlendOp::ADD
            };
            color.colorWriteMask = nri::ColorWriteBits::RGBA;
            color.blendEnabled = true;

            nri::GraphicsPipelineDesc pipeline_desc = {};
            pipeline_desc.pipelineLayout = m_pipeline_layout;
            pipeline_desc.inputAssembly.topology = nri::Topology::TRIANGLE_LIST;
            pipeline_desc.rasterization.fillMode = nri::FillMode::SOLID;
            pipeline_desc.rasterization.cullMode = nri::CullMode::NONE;
            pipeline_desc.rasterization.frontCounterClockwise = false;
            pipeline_desc.rasterization.depthClamp = false;
            pipeline_desc.rasterization.lineSmoothing = false;
            pipeline_desc.rasterization.conservativeRaster = false;
            pipeline_desc.rasterization.shadingRate = false;
            pipeline_desc.outputMerger.colors = &color;
            pipeline_desc.outputMerger.colorNum = 1;
            pipeline_desc.shaders = shaders.data();
            pipeline_desc.shaderNum = shaders.size();
            pipeline_desc.robustness = nri::Robustness::VK;

            result = rhi::check(device.core().CreateGraphicsPipeline(
                    device.native(),
                    pipeline_desc,
                    m_pipeline
                )
            );

            if(!result) {
                return nova::err(std::move(result.error()));
            }
            return nova::ok{};
        }

        [[nodiscard]] nova::result<nova::ok> record(
            runtime::pass_context& ctx,
            const std::uint32_t line_count,
            const std::uint32_t width,
            const std::uint32_t height
        ) const {
            DEBUG_ASSERT(m_target.valid());
            DEBUG_ASSERT(m_instances.valid());
            DEBUG_ASSERT(m_pass.valid());

            if(line_count == 0) {
                return nova::ok{};
            }

            nri::AttachmentDesc attachment{};
            attachment.descriptor = &ctx.color_attachment(m_target);
            attachment.loadOp = nri::LoadOp::LOAD;
            attachment.storeOp = nri::StoreOp::STORE;

            nri::RenderingDesc rendering{};
            rendering.colors = &attachment;
            rendering.colorNum = 1;

            auto& core = ctx.core();
            auto& command_buffer = ctx.command_buffer();

            core.CmdBeginRendering(command_buffer, rendering);
            core.CmdSetPipelineLayout(command_buffer, nri::BindPoint::GRAPHICS, *m_pipeline_layout);
            core.CmdSetPipeline(command_buffer, *m_pipeline);

            nri::SetRootDescriptorDesc root_descriptor{};
            root_descriptor.rootDescriptorIndex = 0;
            root_descriptor.descriptor = &ctx.shader_resource(m_instances);
            root_descriptor.offset = 0;
            root_descriptor.bindPoint = nri::BindPoint::GRAPHICS;

            core.CmdSetRootDescriptor(command_buffer, root_descriptor);

            const line_push_constants constants {
                .viewport_width = static_cast<float>(width),
                .viewport_height = static_cast<float>(height),
                .line_count = line_count,
                .aa_radius = 1.0f
            };

            nri::SetRootConstantsDesc root_constants{};
            root_constants.rootConstantIndex = 0;
            root_constants.data = &constants;
            root_constants.size = sizeof(constants);
            root_constants.offset = 0;
            root_constants.bindPoint = nri::BindPoint::GRAPHICS;

            core.CmdSetRootConstants(command_buffer, root_constants);

            const nri::Viewport viewport {
                .x = 0.0f,
                .y = 0.0f,
                .width = static_cast<float>(width),
                .height = static_cast<float>(height),
                .depthMin = 0.0f,
                .depthMax = 1.0f,
                .originBottomLeft = false
            };

            core.CmdSetViewports(command_buffer, &viewport, 1);

            const nri::Rect scissor {
                .x = 0,
                .y = 0,
                .width = static_cast<nri::Dim_t>(width),
                .height = static_cast<nri::Dim_t>(height)
            };

            core.CmdSetScissors(command_buffer, &scissor, 1);

            const auto group_count = (line_count + line_mesh_group_size - 1) / line_mesh_group_size;

            ctx.mesh_shader().CmdDrawMeshTasks(command_buffer, { .x = group_count, .y = 1, .z = 1 });

            core.CmdEndRendering(command_buffer);

            return nova::ok{};
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

        void reset_graph_binding(const graph::texture_handle target, const graph::buffer_handle instances) noexcept {
            DEBUG_ASSERT(target.valid());
            DEBUG_ASSERT(instances.valid());

            m_target = target;
            m_instances = instances;
            m_pass = {};
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
        void destroy() noexcept {
            if(m_device == nullptr) {
                return;
            }

            auto& core = m_device->core();

            if(m_pipeline != nullptr) {
                core.DestroyPipeline(m_pipeline);
                m_pipeline = nullptr;
            }

            if(m_pipeline_layout != nullptr) {
                core.DestroyPipelineLayout(m_pipeline_layout);
                m_pipeline_layout = nullptr;
            }
            m_device = nullptr;
        }

        rhi::device* m_device{nullptr};
        nri::PipelineLayout* m_pipeline_layout{nullptr};
        nri::Pipeline* m_pipeline{nullptr};

        graph::texture_handle m_target;
        graph::buffer_handle m_instances;
        graph::pass_handle m_pass;
    };
}
