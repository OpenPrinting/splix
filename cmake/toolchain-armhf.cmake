# cmake/toolchain-armhf.cmake
set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)

set(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

find_program(_QEMU_ARM NAMES qemu-arm-static qemu-arm)
if(_QEMU_ARM)
    set(CMAKE_CROSSCOMPILING_EMULATOR "${_QEMU_ARM}")
endif()

set(CMAKE_FIND_ROOT_PATH
    /usr/lib/arm-linux-gnueabihf
    /usr/include/arm-linux-gnueabihf
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(ENV{PKG_CONFIG_PATH}    "")
set(ENV{PKG_CONFIG_LIBDIR}  "/usr/lib/arm-linux-gnueabihf/pkgconfig:/usr/share/pkgconfig")
