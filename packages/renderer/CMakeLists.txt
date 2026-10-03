include(${PROJECT_SOURCE_DIR}/cmake/install_nri.cmake)

add_executable(nova_renderer
    main.cpp
)

set(NOVA_SHADER_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

set(NOVA_SHADER_BINARY_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/shaders"
)

file(MAKE_DIRECTORY "${NOVA_SHADER_BINARY_DIR}")

target_compile_definitions(
    nova_renderer
    PRIVATE
    NOVA_SHADER_DIR="${NOVA_SHADER_BINARY_DIR}"
)
target_compile_features(nova_renderer PUBLIC cxx_std_26)

find_package(libassert REQUIRED)
find_package(magic_enum REQUIRED)

nova_enable_unity_build(nova_renderer)

set_property(
    TARGET nova_renderer
    PROPERTY OPTIMIZE_DEPENDENCIES OFF
)

target_sources(nova_renderer
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
    render_control.cppm
    frame.cppm
    renderer.cppm
    color.cppm
    rhi/device.cppm
    rhi/core.cppm
    rhi/command_context.cppm
    rhi/frame_context.cppm
    rhi/swapchain.cppm
    rhi/queue.cppm
    rhi/renderer.cppm
    rhi/gpu_waiter.cppm
    rhi/buffer.cppm
    rhi/shader.cppm
    graph/resource.cppm
    graph/pass.cppm
    graph/render_graph.cppm
    graph/compiler.cppm
    passes/clear.cppm
    passes/lines.cppm
    runtime/pass_context.cppm
)


find_program(
    SLANGC_EXECUTABLE
    NAMES slangc
    REQUIRED
)

set(NOVA_SHADER_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

set(NOVA_SHADER_BINARY_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/shaders"
)

file(MAKE_DIRECTORY "${NOVA_SHADER_BINARY_DIR}")

set(NOVA_LINE_SHADER
    "${NOVA_SHADER_SOURCE_DIR}/lines.slang"
)

set(NOVA_LINE_MESH_SPV
    "${NOVA_SHADER_BINARY_DIR}/lines.mesh.spv"
)

set(NOVA_LINE_FRAGMENT_SPV
    "${NOVA_SHADER_BINARY_DIR}/lines.fragment.spv"
)

add_custom_command(
    OUTPUT
        "${NOVA_LINE_MESH_SPV}"

    COMMAND
        "${SLANGC_EXECUTABLE}"
        "${NOVA_LINE_SHADER}"
        -entry line_mesh
        -stage mesh
        -target spirv
        -profile spirv_1_6
        -capability spvMeshShadingEXT
        -fvk-use-entrypoint-name
        -O3
        -o "${NOVA_LINE_MESH_SPV}"

    DEPENDS
        "${NOVA_LINE_SHADER}"

    VERBATIM
)

add_custom_command(
    OUTPUT
        "${NOVA_LINE_FRAGMENT_SPV}"

    COMMAND
        "${SLANGC_EXECUTABLE}"
        "${NOVA_LINE_SHADER}"
        -entry line_fragment
        -stage fragment
        -target spirv
        -profile spirv_1_6
        -fvk-use-entrypoint-name
        -O3
        -o "${NOVA_LINE_FRAGMENT_SPV}"

    DEPENDS
        "${NOVA_LINE_SHADER}"

    VERBATIM
)

add_custom_target(
    nova_renderer_shaders
    DEPENDS
        "${NOVA_LINE_MESH_SPV}"
        "${NOVA_LINE_FRAGMENT_SPV}"
)

add_dependencies(
    nova_renderer
    nova_renderer_shaders
)

target_compile_definitions(
    nova_renderer
    PRIVATE
    NOVA_SHADER_DIR="${NOVA_SHADER_BINARY_DIR}"
)

target_link_libraries(
    nova_renderer
    PRIVATE
    libassert::assert
    nova::nri
    nova_platform
    nova_concurrency
    nova_logging
    nova_di
    nova_core
    nova_math
    nova_project_options
    magic_enum::magic_enum
)
nova_enable_unity_build(nova_renderer)
