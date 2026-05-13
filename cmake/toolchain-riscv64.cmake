# cmake/toolchain-riscv64.cmake
set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(CMAKE_C_COMPILER   riscv64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER riscv64-linux-gnu-g++)

find_program(_QEMU_RISCV64 NAMES qemu-riscv64-static qemu-riscv64)
if(_QEMU_RISCV64)
    set(CMAKE_CROSSCOMPILING_EMULATOR "${_QEMU_RISCV64}")
endif()

set(CMAKE_FIND_ROOT_PATH
    /usr/lib/riscv64-linux-gnu
    /usr/include/riscv64-linux-gnu
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(ENV{PKG_CONFIG_PATH}    "")
set(ENV{PKG_CONFIG_LIBDIR}  "/usr/lib/riscv64-linux-gnu/pkgconfig:/usr/share/pkgconfig")
