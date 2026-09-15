add_library(nova_di STATIC)
set_property(
    TARGET nova_di
    PROPERTY OPTIMIZE_DEPENDENCIES OFF
)


target_sources(nova_di
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES 
    ioc/singleton.cppm
)

target_compile_features(nova_di PUBLIC cxx_std_23)
nova_enable_unity_build(nova_di)

find_package(libassert REQUIRED)

target_link_libraries(nova_di
    PUBLIC
    libassert::assert
    nova_concurrency
    PRIVATE
    nova_project_options
)

if(NOVA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
