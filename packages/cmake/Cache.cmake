function(nova_enable_build_cache)
    if (NOT NOVA_USE_BUILD_CACHE)
        return()
    endif()

    find_program(NOVA_SCCACHE_PROGRAM sccache)

    if (NOT NOVA_USE_BUILD_CACHE)
        message(WARNING "NOVA_USE_BUILD_CACHE=ON but cache program was not found")
        return()
    endif()

    message(DEBUG "sccache enabled: ${NOVA_SCCACHE_PROGRAM}")

    set(CMAKE_C_COMPILER_LAUNCHER
        "${NOVA_SCCACHE_PROGRAM}"
        CACHE STRING "C compiler launcher" FORCE
    )
    set(CMAKE_CXX_COMPILER_LAUNCHER
        "${NOVA_SCCACHE_PROGRAM}"
        CACHE STRING "CXX compiler launcher" FORCE
    )
endfunction()
