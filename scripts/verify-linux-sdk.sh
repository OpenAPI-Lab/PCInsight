#!/usr/bin/env bash
# Verify a prebuilt Linux Release tree; no publishing, global install or sudo.
set -euo pipefail

if [[ $# -ne 2 ]]; then
    printf 'Usage: bash %s RELEASE_BUILD NEW_EVIDENCE_DIR\n' "$0" >&2
    exit 2
fi

source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
build_dir=$(cd -- "$1" && pwd -P)
jobs=${PCINSIGHT_CI_JOBS:-2}
if [[ ! $jobs =~ ^[1-9][0-9]*$ ]]; then
    printf 'PCINSIGHT_CI_JOBS must be a positive integer\n' >&2
    exit 2
fi
if ! grep -qx 'CMAKE_BUILD_TYPE:STRING=Release' "$build_dir/CMakeCache.txt"; then
    printf 'Expected a configured single-config Release build\n' >&2
    exit 2
fi
compiler=$(sed -n 's/^CMAKE_CXX_COMPILER:[^=]*=//p' "$build_dir/CMakeCache.txt")
if [[ ! -f $compiler || ! -x $compiler ]]; then
    printf 'Expected an executable C++ compiler in the Release build cache\n' >&2
    exit 2
fi

# Fail if it exists; do not clean or overwrite an earlier run's evidence.
mkdir -- "$2"
evidence=$(cd -- "$2" && pwd -P)
prefix=$evidence/sdk

run() {
    local log=$1
    shift
    printf '\n== %s ==\n' "$log"
    "$@" 2>&1 | tee "$evidence/$log.log"
}

run suite ctest --test-dir "$build_dir" --output-on-failure --no-tests=error --timeout 60 --parallel "$jobs"
run install cmake --install "$build_dir" --config Release --prefix "$prefix"
run payload cmake "-DSDK_PREFIX=$prefix" "-DAUDIT_OUTPUT=$evidence" -P "$source_dir/tests/linux_delivery/check_payload_manifest.cmake"
run standalone-headers bash "$source_dir/scripts/check-linux-sdk-headers.sh" "$compiler" "$prefix"

run delivery-configure cmake -S "$source_dir/tests/linux_delivery" -B "$evidence/delivery" \
    -DCMAKE_BUILD_TYPE=Release "-DCMAKE_CXX_COMPILER=$compiler" "-DPCINSIGHT_AUDIT_PREFIX=$prefix"
run delivery-build cmake --build "$evidence/delivery" --parallel "$jobs"
run delivery-test ctest --test-dir "$evidence/delivery" --output-on-failure --no-tests=error --parallel "$jobs"

run marker-configure cmake -S "$source_dir/tests/sdk_headers_consumer" -B "$evidence/marker" \
    -DCMAKE_BUILD_TYPE=Release "-DCMAKE_CXX_COMPILER=$compiler" "-DCMAKE_PREFIX_PATH=$prefix"
run marker-build cmake --build "$evidence/marker" --parallel "$jobs"
run marker-test ctest --test-dir "$evidence/marker" --output-on-failure --no-tests=error

# Compile as a plain C++ consumer using the compiler from the Release build.
# Do not take headers or libraries from the source/build tree.
mapfile -t archives < <(find "$prefix" -type f -name libpcinsight.a)
if [[ ${#archives[@]} -ne 1 ]]; then
    printf 'Expected one installed static archive\n' >&2
    exit 2
fi
run direct-build "$compiler" -std=c++20 -O2 -DNDEBUG -I"$prefix/include" \
    "$source_dir/tests/linux_abi/gui_read_consumer.cpp" "${archives[0]}" \
    -pthread -ldl -o "$evidence/gui-read"
run direct-test timeout 10s "$evidence/gui-read"

# This source/runtime contract is portable across Linux build profiles. Do not
# impose the optional, compiler-specific layout baseline on the CI runner.
run contract-build "$compiler" -std=c++20 -O2 -DNDEBUG -I"$prefix/include" \
    "$source_dir/tests/linux_abi/result_contract.cpp" "${archives[0]}" \
    -pthread -ldl -o "$evidence/result-contract"
run contract-test timeout 10s "$evidence/result-contract"
