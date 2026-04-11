cmake_minimum_required(VERSION 3.31)
project(dicom_api)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

function(configure_arch_output TARGET)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
        set(NORMALIZED_ARCH "x86_64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(NORMALIZED_ARCH "arm64")
    else()
        set(NORMALIZED_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    if(NOT CMAKE_BUILD_TYPE)
        message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set. Use -DCMAKE_BUILD_TYPE=Release or Debug.")
    endif()

    string(TOLOWER "${CMAKE_BUILD_TYPE}" NORMALIZED_BUILD_TYPE)
    set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/${NORMALIZED_ARCH}/${NORMALIZED_BUILD_TYPE}")

    message(STATUS "Setting output dir to: ${OUTPUT_DIR}")
    set_target_properties(${TARGET} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
            ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
    )
endfunction()

message(STATUS "CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "CMAKE_PREFIX_PATH = ${CMAKE_PREFIX_PATH}")

add_library(${CMAKE_PROJECT_NAME} SHARED
        dicom_image.h
        dicom_image.cpp
        dicom.h
        dicom.cpp
        dicom_windowing.h
        dicom_windowing.cpp
)

configure_arch_output(${CMAKE_PROJECT_NAME})
#add_executable(${CMAKE_PROJECT_NAME}
#        main.cpp
#        dicom_image.h
#        dicom_image.cpp
#        dicom.h
#        dicom.cpp
#        dicom_windowing.h
#        dicom_windowing.cpp
#)

add_subdirectory(lib)
add_subdirectory(api)
if(MSVC)
    add_compile_options(/std:c++latest)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

find_package(libassert REQUIRED)
find_package(GDCM REQUIRED)
find_package(OpenCV REQUIRED )
find_package(libdeflate REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(unordered_dense REQUIRED)
find_package(quill REQUIRED)

target_link_libraries(${CMAKE_PROJECT_NAME}
        PRIVATE
        gdcm::gdcm
        opencv::opencv
        libdeflate::libdeflate_static
        libassert::assert
        nlohmann_json::nlohmann_json
        unordered_dense::unordered_dense
        quill::quill
)