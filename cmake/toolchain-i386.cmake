# cmake/toolchain-i386.cmake
set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(CMAKE_C_COMPILER   i686-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER i686-linux-gnu-g++)

find_program(_QEMU_I386 NAMES qemu-i386-static qemu-i386)
if(_QEMU_I386)
    set(CMAKE_CROSSCOMPILING_EMULATOR "${_QEMU_I386}")
endif()

set(CMAKE_FIND_ROOT_PATH
    /usr/lib/i386-linux-gnu
    /usr/include/i386-linux-gnu
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(ENV{PKG_CONFIG_PATH}    "")
set(ENV{PKG_CONFIG_LIBDIR}  "/usr/lib/i386-linux-gnu/pkgconfig:/usr/share/pkgconfig")
