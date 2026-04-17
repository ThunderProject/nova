add_library(nova_core INTERFACE)

target_include_directories(nova_core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(nova_core INTERFACE cxx_std_23)
nova_enable_unity_build(nova_core)

find_package(libassert REQUIRED)

target_link_libraries(nova_core
    INTERFACE
    nova_project_options
    libassert::assert
)

if(NOVA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
