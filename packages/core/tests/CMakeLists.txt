find_package(Catch2 3 REQUIRED)

add_executable(nova_core_tests
    indirect_test.cpp
)

target_compile_features(nova_core_tests PRIVATE cxx_std_23)
nova_enable_unity_build(nova_core_tests)

target_link_libraries(nova_core_tests
    PRIVATE
        nova_core
        Catch2::Catch2WithMain
        nova_project_options
)

include(CTest)
include(Catch)
catch_discover_tests(nova_core_tests)
