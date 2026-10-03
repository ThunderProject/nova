function(nova_add_library TARGET TYPE)
    set(NORMAL_SOURCES)
    set(MODULE_SOURCES)

    foreach(SOURCE IN LISTS ARGN)
        get_filename_component(EXT "${SOURCE}" EXT)

        if(EXT STREQUAL ".cppm" OR
           EXT STREQUAL ".ixx"  OR
           EXT STREQUAL ".mpp"  OR
           EXT STREQUAL ".cxxm")
            list(APPEND MODULE_SOURCES "${SOURCE}")
        else()
            list(APPEND NORMAL_SOURCES "${SOURCE}")
        endif()
    endforeach()

    add_library(${TARGET} ${TYPE}
        ${NORMAL_SOURCES}
    )

    if(MODULE_SOURCES)
        target_sources(${TARGET}
            PUBLIC
                FILE_SET CXX_MODULES
                FILES
                    ${MODULE_SOURCES}
        )
    endif()
endfunction()