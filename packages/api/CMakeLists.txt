add_library(nova_api
    dicom_api.cpp
)

target_include_directories(nova_api
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(nova_api PUBLIC cxx_std_23)
nova_enable_unity_build(nova_api)

find_package(quill REQUIRED)
find_package(libassert REQUIRED)
find_package(magic_enum REQUIRED)

target_link_libraries(nova_api
    PUBLIC
    quill::quill
    libassert::assert
    PRIVATE
    nova_project_options
    magic_enum::magic_enum
)

# if(NOVA_BUILD_TESTS)
#     add_subdirectory(tests)
# endif()
