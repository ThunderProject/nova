include(${PROJECT_SOURCE_DIR}/cmake/install_nri.cmake)

add_executable(nova_renderer
    main.cpp
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
    rhi/device.cppm
    rhi/core.cppm
    rhi/command_context.cppm
    rhi/frame_context.cppm
    rhi/swapchain.cppm
    rhi/queue.cppm
    rhi/renderer.cppm
    rhi/gpu_waiter.cppm
    graph/resource.cppm
    graph/pass.cppm
    graph/render_graph.cppm
    graph/compiler.cppm
    passes/clear.cppm
    passes/lines.cppm
    runtime/pass_context.cppm
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
    nova_project_options
    magic_enum::magic_enum
)
nova_enable_unity_build(nova_renderer)
