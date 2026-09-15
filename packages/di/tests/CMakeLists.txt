
find_package(Catch2 3 REQUIRED)

add_executable(nova_di_tests
    singleton_test.cpp
)

target_compile_features(nova_di_tests PRIVATE cxx_std_23)
nova_enable_unity_build(nova_di_tests)

target_link_libraries(nova_di_tests
    PRIVATE
      nova_di
      Catch2::Catch2WithMain
      nova_project_options
)

include(CTest)
include(Catch)
catch_discover_tests(nova_di_tests)
