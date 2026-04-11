include_guard(GLOBAL)
add_library(nova_project_options INTERFACE)

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
  -fsanitize=address
  -fno-omit-frame-pointer

  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:Release>:-march=native>
  $<$<CONFIG:Release>:-mtune=native>
  $<$<CONFIG:Release>:-fstrict-vtable-pointers>
  $<$<CONFIG:Release>:-ffunction-sections>
  $<$<CONFIG:Release>:-fdata-sections>

  $<$<CONFIG:RelWithDebInfo>:-O3>
  $<$<CONFIG:RelWithDebInfo>:-g>
  $<$<CONFIG:RelWithDebInfo>:-march=native>
  $<$<CONFIG:RelWithDebInfo>:-mtune=native>
  $<$<CONFIG:RelWithDebInfo>:-fstrict-vtable-pointers>
  $<$<CONFIG:RelWithDebInfo>:-ffunction-sections>
  $<$<CONFIG:RelWithDebInfo>:-fdata-sections>
)

target_link_options(nova_project_options INTERFACE
  -Wl,-z,nodlopen
  -Wl,-z,noexecstack
  -Wl,-z,relro
  -Wl,-z,now
  -Wl,--as-needed
  -Wl,--no-copy-dt-needed-entries
  -fsanitize=address

  $<$<CONFIG:Release>:-fuse-ld=lld>
  $<$<CONFIG:Release>:-flto=thin>
  $<$<CONFIG:Release>:-Wl,--gc-sections>
  $<$<CONFIG:Release>:-Wl,--icf=safe>

  $<$<CONFIG:RelWithDebInfo>:-fuse-ld=lld>
  $<$<CONFIG:RelWithDebInfo>:-flto=thin>
  $<$<CONFIG:RelWithDebInfo>:-Wl,--gc-sections>
  $<$<CONFIG:RelWithDebInfo>:-Wl,--icf=safe>
)
