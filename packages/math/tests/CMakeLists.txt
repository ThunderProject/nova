
find_package(Catch2 3 REQUIRED)

add_executable(nova_math_tests
    vector_test.cpp
)

target_compile_features(nova_math_tests PRIVATE cxx_std_26)
nova_enable_unity_build(nova_math_tests)

target_link_libraries(nova_math_tests
    PRIVATE
      nova_math
      Catch2::Catch2WithMain
      nova_project_options
)

include(CTest)
include(Catch)
catch_discover_tests(nova_math_tests)
