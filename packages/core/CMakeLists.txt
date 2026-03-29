add_library(nova_core INTERFACE)

target_include_directories(nova_core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(nova_core INTERFACE cxx_std_23)
target_link_libraries(nova_core
    INTERFACE
    nova_project_options
)

if(NOVA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
