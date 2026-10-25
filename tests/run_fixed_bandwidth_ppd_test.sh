#!/bin/sh
set -eu

build_dir=${BUILD_DIR:-.build/tests}
project_dir=$(pwd)
mkdir -p "$project_dir/$build_dir"
work_dir=$(mktemp -d "$project_dir/$build_dir/fixed-bandwidth.XXXXXX")
source_dir="$work_dir/ppd"
ppd_dir="$work_dir/generated"

trap 'rm -rf "$work_dir"' EXIT
mkdir -p "$source_dir" "$ppd_dir"

for source in ppd/*.defs ppd/*.in ppd/compile.sh; do
    tr -d '\r' < "$source" > "$source_dir/${source#ppd/}"
done

(
    cd "$source_dir"
    sh ./compile.sh samsung.drv.in -I . -d "$ppd_dir"
    sh ./compile.sh xerox.drv.in -I . -d "$ppd_dir"
)

python3 tests/fixed_bandwidth_ppd_test.py "$ppd_dir"
