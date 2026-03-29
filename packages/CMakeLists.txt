cmake_minimum_required(VERSION 4.3.0)

project(nova_software)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(NOVA_BUILD_TESTS ON)

enable_testing()
include(CTest)

find_package(Catch2 3 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ProjectOptions)

add_subdirectory(core)
add_subdirectory(logging)
