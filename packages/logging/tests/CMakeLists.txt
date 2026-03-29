find_package(Catch2 3 REQUIRED)

add_executable(nova_logger_tests
    logger_test.cpp
)

target_compile_features(nova_logger_tests PRIVATE cxx_std_23)

function(enable_msvc_optimizations target_name)
    if (MSVC)
        target_compile_options(${target_name} PRIVATE
            /permissive-
            /Zc:__cplusplus
            /utf-8
            /W4
            /WX
            /w14242
            /w14254
            /w14263
            /w14265
            /w14287
            /we4289
            /w14296
            /w14311
            /w14545
            /w14546
            /w14547
            /w14549
            /w14555
            /w14619
            /w14640
            /w14826
            /w14905
            /w14906
            /w14928
        )

        # Silence warnings from standard/external headers included with <>
        target_compile_options(${target_name} PRIVATE
            /external:anglebrackets
            /external:W0
        )
        target_compile_options(${target_name} PRIVATE
            $<$<CONFIG:Release>:
                /O2         # maximize speed
                /GL         # whole program optimization
                /Gw         # whole-program global data optimization
                /Gy         # function-level linking
                /Oi         # intrinsic functions
                /arch:AVX2  # use avx2
            >
            $<$<CONFIG:RelWithDebInfo>:
                /O2
                /GL
                /Gw
                /Gy
                /Oi
                /Zi
                /arch:AVX2
            >
        )

        target_link_options(${target_name} PRIVATE
            $<$<CONFIG:Release>:
                /LTCG
                /OPT:REF
                /OPT:ICF
            >
            $<$<CONFIG:RelWithDebInfo>:
                /LTCG
                /OPT:REF
                /OPT:ICF
                /DEBUG:FULL
            >
        )
    endif()
endfunction()

#enable_msvc_optimizations(nova_core_tests)

target_link_libraries(nova_logger_tests
    PRIVATE
        nova_logging
        Catch2::Catch2WithMain
        nova_project_options
)

include(CTest)
include(Catch)
catch_discover_tests(nova_logger_tests)
