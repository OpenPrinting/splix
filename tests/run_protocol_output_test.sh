#!/bin/sh
set -eu

binary=${1:-optimized/rastertoqpdl}
build_dir=${BUILD_DIR:-.build/tests}
cxx=${CXX:-c++}

mkdir -p "$build_dir"
work_dir=$(mktemp -d "$PWD/$build_dir/protocol.XXXXXX")
trap 'rm -rf "$work_dir"' EXIT

"$cxx" \
    -std=c++23 \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Werror \
    tests/raster_fixture.cpp \
    $(pkg-config --cflags --libs cups) \
    -o "$work_dir/raster_fixture"

"$work_dir/raster_fixture" "$work_dir/a5.ras"

check_model() {
    model=$1
    expected=$2
    PPD="$PWD/ppd/$model.ppd" \
        timeout 30s "$binary" 1 protocol protocol 1 PageSize=A5 \
        "$work_dir/a5.ras" > "$work_dir/$model.qpdl" \
        2> "$work_dir/$model.log"
    cat "$work_dir/$model.log"
    actual=$(sha256sum "$work_dir/$model.qpdl" | cut -d ' ' -f 1)
    if [ "$actual" != "$expected" ]; then
        echo "$model protocol hash mismatch: $actual" >&2
        exit 1
    fi
    echo "$model A5 protocol output preserved ($actual)"
}

check_model ml2010 \
    ba867847558f3aaab66396cb6905ef0a8d1f6d12468b577c3c0ad3c545298e3c
check_model ml1520 \
    efd1192e314342a6b05ef901d445fa0dceaee408ffcdbf0bc76611b5ea03da37
