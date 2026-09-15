add_library(nova_platform STATIC)
set_property(
    TARGET nova_platform
    PROPERTY OPTIMIZE_DEPENDENCIES OFF
)


target_sources(nova_platform
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
    window/window.cppm
)

target_compile_features(nova_platform PUBLIC cxx_std_26)
nova_enable_unity_build(nova_platform)

find_package(libassert REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

target_link_libraries(nova_platform
    PUBLIC
    libassert::assert
    PRIVATE
    glfw
    nova_project_options
)

nova_enable_unity_build(nova_platform)

#if(NOVA_BUILD_TESTS)
#    add_subdirectory(tests)
#endif()
