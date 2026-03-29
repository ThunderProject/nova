include_guard(GLOBAL)
add_library(nova_project_options INTERFACE)

if (MSVC)
    target_compile_options(nova_project_options INTERFACE
        /permissive-
        /Zc:__cplusplus # Makes MSVC report the correct C++ version in __cplusplus
        /utf-8 # Treats source files as UTF-8.
        /W4 # Enables a high warning level
        /WX # Treat warnings as errors.
        /wd4100 # Disable warning C4100: unreferenced parameter
        #/w14242 # Do not enable warning C14242: conversion, possible loss of data
        /w14254 # warn conversion from bitfield to smaller type
        /w14263 # warn member function does not override base virtual function
        /w14265 # warn class has virtual functions but destructor is not virtual
        /w14287 # warn signed/unsigned mismatch
        /we4289 # warn loop variable used outside loop scope
        /w14296 # warn expression always evaluates to boolean constant
        /w14311 # warn pointer truncation or type mismatch issues
        /w14549 # warn operator precedence confusion
        /w14555 # warn expression has no effect
        /w14906 # warn narrowing conversio

        /external:anglebrackets # Treat angle brackets as external headers
        /external:W0 # Disable external header warnings

        $<$<CONFIG:Release>:/O2> # Optimize for speed
        $<$<CONFIG:Release>:/GL> # Whole Program Optimization
        $<$<CONFIG:Release>:/Gw> # Optimize global data
        $<$<CONFIG:Release>:/Gy> # Enable function-level linking
        $<$<CONFIG:Release>:/Oi> # Enable intrinsic functions
        $<$<CONFIG:Release>:/arch:AVX2> # Use AVX2 instructions

        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/GL>
        $<$<CONFIG:RelWithDebInfo>:/Gw>
        $<$<CONFIG:RelWithDebInfo>:/Gy>
        $<$<CONFIG:RelWithDebInfo>:/Oi>
        $<$<CONFIG:RelWithDebInfo>:/Zi> # Generate debug information
        $<$<CONFIG:RelWithDebInfo>:/arch:AVX2>
    )

    target_link_options(nova_project_options INTERFACE
        $<$<CONFIG:Release>:/LTCG> # Link-time code generation
        $<$<CONFIG:Release>:/OPT:REF> # Optimize for reference
        $<$<CONFIG:Release>:/OPT:ICF> # Optimize for identical code folding

        $<$<CONFIG:RelWithDebInfo>:/LTCG>
        $<$<CONFIG:RelWithDebInfo>:/OPT:REF>
        $<$<CONFIG:RelWithDebInfo>:/OPT:ICF>
        $<$<CONFIG:RelWithDebInfo>:/DEBUG:FULL> # Generate full debug information
    )
endif()
