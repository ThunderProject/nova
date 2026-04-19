add_library(nova_core INTERFACE)

target_include_directories(nova_core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(nova_core INTERFACE cxx_std_23)

find_package(libassert REQUIRED)
find_package(unordered_dense REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(nova_core
    INTERFACE
    unordered_dense::unordered_dense
    nlohmann_json::nlohmann_json
    nova_project_options
    libassert::assert
)

if(NOVA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
