add_library(nova_dicom
    dicom_reader.cpp
)

target_include_directories(nova_dicom
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(nova_dicom PUBLIC cxx_std_23)

find_package(quill REQUIRED)
find_package(libassert REQUIRED)
find_package(DCMTK REQUIRED)

target_link_libraries(nova_dicom
    PUBLIC
    quill::quill
    libassert::assert
    dcmtk::dcmtk
    PRIVATE
    nova_project_options
)

if(NOVA_BUILD_TESTS)
    #add_subdirectory(tests)
endif()
