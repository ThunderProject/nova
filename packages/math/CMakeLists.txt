add_library(nova_math STATIC)
set_property(
    TARGET nova_math
    PROPERTY OPTIMIZE_DEPENDENCIES OFF
)


target_sources(nova_math
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
    vector.cppm
)

target_compile_features(nova_math PUBLIC cxx_std_26)
nova_enable_unity_build(nova_math)

find_package(libassert REQUIRED)

target_link_libraries(nova_math
    PRIVATE
    libassert::assert
    nova_project_options
)

nova_enable_unity_build(nova_math)

if(NOVA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
