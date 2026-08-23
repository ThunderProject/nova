include_guard(GLOBAL)
add_library(nova_project_options INTERFACE)

function(nova_enable_unity_build target)
    if(NOVA_ENABLE_UNITY_BUILD)
        set_target_properties(${target} PROPERTIES
            UNITY_BUILD ON
            UNITY_BUILD_BATCH_SIZE 16
        )
    endif()
endfunction()

target_compile_options(nova_project_options INTERFACE
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wconversion
  -Wsign-conversion
  -Wshadow
  -Wnon-virtual-dtor
  -Woverloaded-virtual
  -Wold-style-cast
  -Wcast-align
  -Wunused
  -Wnull-dereference
  -Wdouble-promotion
  -Wformat=2
  -Wimplicit-fallthrough
  -Wmissing-declarations
  -Wunreachable-code
  -Wundef
  -Wno-c2y-extensions
  -Winvalid-utf8

  -U_FORTIFY_SOURCE
  -D_FORTIFY_SOURCE=3
  -D_GLIBCXX_ASSERTIONS

  -fstack-clash-protection
  -fstack-protector-strong
  -fstrict-flex-arrays=3
  -ftrivial-auto-var-init=zero

  #-fsanitize=address
  -fno-omit-frame-pointer

  $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>:-fcf-protection=full>

  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:Release>:-march=native>
  $<$<CONFIG:Release>:-mtune=native>
  $<$<CONFIG:Release>:-fdata-sections>
  $<$<CONFIG:Release>:-flto=thin>
  #$<$<CONFIG:Release>:-fsanitize=cfi>
  #$<$<CONFIG:Release>:-fsanitize-trap=cfi>
  $<$<CONFIG:Release>:-fwhole-program-vtables>
  $<$<CONFIG:Release>:-fstrict-vtable-pointers>
  $<$<CONFIG:Release>:-ffunction-sections>
  $<$<CONFIG:Release>:-fvisibility=hidden>
  $<$<CONFIG:Release>:-fvisibility-inlines-hidden>

  $<$<CONFIG:RelWithDebInfo>:-O3>
  $<$<CONFIG:RelWithDebInfo>:-g>
  $<$<CONFIG:RelWithDebInfo>:-march=native>
  $<$<CONFIG:RelWithDebInfo>:-mtune=native>
  $<$<CONFIG:RelWithDebInfo>:-flto=thin>
  $<$<CONFIG:RelWithDebInfo>:-fsanitize=cfi>
  $<$<CONFIG:RelWithDebInfo>:-fsanitize-trap=cfi>
  $<$<CONFIG:RelWithDebInfo>:-fwhole-program-vtables>
  $<$<CONFIG:RelWithDebInfo>:-fstrict-vtable-pointers>
  $<$<CONFIG:RelWithDebInfo>:-ffunction-sections>
  $<$<CONFIG:RelWithDebInfo>:-fdata-sections>
)

target_link_options(nova_project_options INTERFACE
  -Wl,-z,nodlopen
  -fuse-ld=lld

  -Wl,-z,noexecstack

  -Wl,-z,relro
  -Wl,-z,now

  -Wl,-z,text
  -Wl,-z,separate-code
  -Wl,-z,defs

  -Wl,--as-needed
  -Wl,--no-copy-dt-needed-entries
  -Wl,--fatal-warnings

  #-fsanitize=address

  $<$<CONFIG:Release>:-flto=thin>
  #$<$<CONFIG:Release>:-fsanitize=cfi>
  #$<$<CONFIG:Release>:-fsanitize-trap=cfi>

  $<$<CONFIG:Release>:-Wl,--gc-sections>
  $<$<CONFIG:Release>:-Wl,--icf=safe>
  $<$<CONFIG:Release>:-Wl,--strip-all>

  $<$<CONFIG:RelWithDebInfo>:-flto=thin>
  $<$<CONFIG:RelWithDebInfo>:-fsanitize=cfi>
  $<$<CONFIG:RelWithDebInfo>:-fsanitize-trap=cfi>

  $<$<CONFIG:RelWithDebInfo>:-Wl,--gc-sections>
  $<$<CONFIG:RelWithDebInfo>:-Wl,--icf=safe>
)
