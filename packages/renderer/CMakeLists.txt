#target_sources(nova_platform
#    PUBLIC
#    FILE_SET cxx_modules TYPE CXX_MODULES FILES
#    window/window.cppm
#)

add_executable(nova_renderer
    main.cpp
)

target_compile_features(nova_renderer PUBLIC cxx_std_26)

find_package(libassert REQUIRED)

nova_enable_unity_build(nova_renderer)

set_property(
    TARGET nova_renderer
    PROPERTY OPTIMIZE_DEPENDENCIES OFF
)

target_link_libraries(
    nova_renderer
    PUBLIC
    libassert::assert
    PRIVATE
    nova_platform
    nova_concurrency
    nova_logging
    nova_di
    nova_core
    nova_project_options
)
nova_enable_unity_build(nova_renderer)
