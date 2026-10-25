#!/bin/sh
set -eu

cxx=${CXX:-c++}
build_dir=${BUILD_DIR:-.build/tests}

mkdir -p "$build_dir"

"$cxx" \
    -std=gnu++98 \
    -Wall \
    -Wextra \
    -Werror \
    -Iinclude \
    -c src/band_layout.cpp \
    -o "$build_dir/band_layout_legacy.o"

"$cxx" -std=gnu++98 -Wall -Wextra -Werror -Iinclude \
    -c src/io_utils.cpp -o "$build_dir/io_utils_legacy.o"

"$cxx" \
    -std=c++23 \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Werror \
    -fsanitize=address,undefined \
    -fno-sanitize-recover=all \
    -fno-omit-frame-pointer \
    -Iinclude \
    tests/fixed_bandwidth_test.cpp \
    src/band_layout.cpp \
    -o "$build_dir/fixed_bandwidth_test"

"$build_dir/fixed_bandwidth_test"

"$cxx" -std=c++23 -Wall -Wextra -Wpedantic -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    -fno-omit-frame-pointer -Iinclude tests/io_utils_test.cpp \
    src/io_utils.cpp -o "$build_dir/io_utils_test"
"$build_dir/io_utils_test"

"$cxx" -std=c++23 -Wall -Wextra -Wpedantic -Werror \
    -Wno-variadic-macros \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    -fno-omit-frame-pointer -Iinclude tests/serialization_test.cpp \
    src/band.cpp src/bandplane.cpp src/io_utils.cpp \
    -o "$build_dir/serialization_test"
"$build_dir/serialization_test"
