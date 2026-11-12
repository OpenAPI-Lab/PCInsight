#!/usr/bin/env bash
# Link the same consumer object against two installed Linux SDK revisions.
set -euo pipefail

if [[ $# -ne 4 ]]; then
    printf 'Usage: bash %s COMPILER OLD_SDK NEW_SDK NEW_EVIDENCE_DIR\n' "$0" >&2
    exit 2
fi

source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
compiler=$(command -v -- "$1")
old=$(cd -- "$2" && pwd -P)
new=$(cd -- "$3" && pwd -P)
if [[ ! -f $compiler || ! -x $compiler || $old == "$new" ]]; then
    printf 'Expected an executable compiler and two distinct SDK prefixes\n' >&2
    exit 2
fi

find_archive() {
    local prefix=$1
    local -a archives
    mapfile -t archives < <(find "$prefix" -type f -name libpcinsight.a)
    if [[ ${#archives[@]} -ne 1 || ! -f $prefix/include/pcinsight/sdk_headers.hpp ]]; then
        printf 'Expected an installed PCInsight SDK: %s\n' "$prefix" >&2
        return 2
    fi
    printf '%s\n' "${archives[0]}"
}

old_archive=$(find_archive "$old")
new_archive=$(find_archive "$new")
mkdir -- "$4"
evidence=$(cd -- "$4" && pwd -P)

"$compiler" --version > "$evidence/compiler.txt"
sha256sum -- "$old_archive" "$new_archive" > "$evidence/libraries.sha256"

consumers=(gui_read_consumer result_contract)
if [[ -f $old/include/pcinsight/linux_network_addresses.hpp ]]; then
    consumers+=(network_address_consumer)
else
    printf 'Address query consumer not checked: old SDK predates its public header\n'
fi
for consumer in "${consumers[@]}"; do
    obj="$evidence/$consumer.o"
    "$compiler" -std=c++20 -O2 -DNDEBUG -I"$old/include" -c \
        "$source_dir/tests/linux_abi/$consumer.cpp" -o "$obj"
    for revision in original updated; do
        library=$old_archive
        if [[ $revision == updated ]]; then
            library=$new_archive
        fi
        executable="$evidence/$consumer-$revision"
        "$compiler" "$obj" "$library" -pthread -ldl -o "$executable"
        timeout 10s "$executable"
        printf '%s %s: OK\n' "$consumer" "$revision"
    done
done
