#!/bin/sh
set -eu

logs=.build/test-logs
jobs=${JOBS:-2}
warnings=0

mkdir -p "$logs"

run_build() {
    name=$1
    shift
    make distclean >/dev/null 2>&1
    if ! make DRV_ONLY=1 WARNINGS_AS_ERRORS=1 V=1 -j"$jobs" "$@" \
        >"$logs/$name.log" 2>&1; then
        cat "$logs/$name.log"
        return 1
    fi
    cat "$logs/$name.log"
    if grep -E '^[^:]+:[0-9]+:[0-9]+: (warning|error):' \
        "$logs/$name.log" >/dev/null; then
        warnings=1
    fi
    sh tests/run_protocol_output_test.sh optimized/rastertoqpdl
}

run_build normal
run_build no-threads DISABLE_THREADS=1
run_build no-jbig DISABLE_JBIG=1
run_build sanitizers \
    "OPTIM_CXXFLAGS=-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer" \
    "LDFLAGS=-fsanitize=address,undefined"

sh tests/run_fixed_bandwidth_tests.sh
sh tests/run_fixed_bandwidth_ppd_test.sh
make distclean >/dev/null 2>&1

if [ "$warnings" -ne 0 ]; then
    echo "compiler diagnostics found; inspect $logs" >&2
    exit 1
fi

echo "all Docker matrix checks passed"
