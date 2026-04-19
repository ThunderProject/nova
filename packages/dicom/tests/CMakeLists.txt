find_package(Catch2 3 REQUIRED)

add_executable(nova_dicom_tests
    dicom_reader_test.cpp
)

target_compile_features(nova_dicom_tests PRIVATE cxx_std_23)
target_compile_definitions(nova_dicom_tests
    PRIVATE
        NOVA_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_data"
)
nova_enable_unity_build(nova_dicom_tests)

target_link_libraries(nova_dicom_tests
    PRIVATE
        nova_dicom
        Catch2::Catch2WithMain
        nova_project_options
)

include(CTest)
include(Catch)
catch_discover_tests(nova_dicom_tests)
