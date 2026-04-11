add_library(nova_logging
    logger.cpp
)

target_include_directories(nova_logging
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(nova_logging PUBLIC cxx_std_23)

find_package(quill REQUIRED)
find_package(libassert REQUIRED)
find_package(magic_enum REQUIRED)

target_link_libraries(nova_logging
    PUBLIC
    quill::quill
    libassert::assert
    PRIVATE
    nova_project_options
    magic_enum::magic_enum
)

if(NOVA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
