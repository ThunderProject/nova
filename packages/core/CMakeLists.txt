add_library(nova_core STATIC)

target_include_directories(nova_core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(nova_core INTERFACE cxx_std_26)

find_package(libassert REQUIRED)
find_package(unordered_dense REQUIRED)
find_package(nlohmann_json REQUIRED)

target_sources(nova_core
    PUBLIC
        FILE_SET cxx_modules
        TYPE CXX_MODULES
        FILES
            profiler.cppm
            # whatever else:
            # foo.cppm
            # bar.cppm
)


target_link_libraries(nova_core
    PUBLIC
        unordered_dense::unordered_dense
        nlohmann_json::nlohmann_json
        nova_project_options
        libassert::assert
    PRIVATE
        nova_concurrency
)

if(NOVA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
