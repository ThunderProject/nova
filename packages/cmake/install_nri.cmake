include_guard(GLOBAL)

include(FetchContent)
include(CheckIPOSupported)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Nova NRI configuration currently supports Linux only")
endif()

set(NOVA_NRI_VERSION "v180" CACHE STRING "NRI version used by Nova")

set(NRI_STATIC_LIBRARY                     ON  CACHE BOOL "" FORCE)
set(NRI_ENABLE_VK_SUPPORT                  ON  CACHE BOOL "" FORCE)
set(NRI_ENABLE_NONE_SUPPORT                OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_WGPU_SUPPORT                OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_D3D11_SUPPORT               OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_D3D12_SUPPORT               OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_VALIDATION_SUPPORT          OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_NVTX_SUPPORT                OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_NIS_SDK                     OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_NGX_SDK                     OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_FFX_SDK                     OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_XESS_SDK                    OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_IMGUI_EXTENSION             OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_NVAPI                       OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_AMDAGS                      OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_AGILITY_SDK_SUPPORT         OFF CACHE BOOL "" FORCE)
set(NRI_ENABLE_WAYLAND_SUPPORT             ON  CACHE BOOL "" FORCE)
set(NRI_ENABLE_XLIB_SUPPORT                OFF CACHE BOOL "" FORCE)
set(NRI_STREAMER_THREAD_SAFE               OFF CACHE BOOL "" FORCE)

find_path(
    NOVA_WAYLAND_INCLUDE_DIR
    NAMES wayland-client.h
)

if(NOT NOVA_WAYLAND_INCLUDE_DIR)
    message(
        FATAL_ERROR
        "NRI Wayland support requires wayland-client.h.\n"
        "On Arch Linux: sudo pacman -S wayland"
    )
endif()

FetchContent_Declare(
    nri
    GIT_REPOSITORY https://github.com/NVIDIA-RTX/NRI.git
    GIT_TAG ${NOVA_NRI_VERSION}
    GIT_SHALLOW TRUE
    GIT_PROGRESS FALSE
    SYSTEM EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(nri)

if(NOT TARGET NRI)
    message(FATAL_ERROR "NRI target was not created")
endif()

if(NOT TARGET NRI_VK)
    message(FATAL_ERROR "NRI Vulkan backend was not created")
endif()

set(_nova_nri_targets
    NRI
    NRI_Shared
    NRI_VK
)

foreach(_target IN LISTS _nova_nri_targets)
    if(TARGET ${_target})
        target_compile_options(
            ${_target}
            PRIVATE
                -Wno-inconsistent-missing-override
                $<$<CONFIG:Release>:-O3>
                $<$<CONFIG:Release>:-march=native>
                $<$<CONFIG:Release>:-mtune=native>
                $<$<CONFIG:Release>:-fsplit-lto-unit>
        )
    endif()
endforeach()

check_ipo_supported(
    RESULT _nova_nri_ipo_supported
    OUTPUT _nova_nri_ipo_error
    LANGUAGES CXX
)

if(_nova_nri_ipo_supported)
    foreach(_target IN LISTS _nova_nri_targets)
        if(TARGET ${_target})
            set_property(
                TARGET ${_target}
                PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
            )
        endif()
    endforeach()
else()
    message(
        WARNING
        "IPO/LTO unavailable for NRI: ${_nova_nri_ipo_error}"
    )
endif()

#example: target_link_libraries(my_project PRIVATE nova::nri)
add_library(nova_nri INTERFACE)
add_library(nova::nri ALIAS nova_nri)

target_link_libraries(
    nova_nri
    INTERFACE
        NRI
)

message(
    STATUS
    "Nova NRI: ${NOVA_NRI_VERSION} | Vulkan | Linux | Wayland | static"
)
