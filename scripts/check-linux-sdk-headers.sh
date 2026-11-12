#!/usr/bin/env bash
# Compile each installed public header in its own empty C++ translation unit.
set -euo pipefail
if [[ $# -ne 2 ]]; then
    printf 'Usage: bash %s COMPILER SDK_PREFIX\n' "$0" >&2
    exit 2
fi
compiler=$(command -v -- "$1")
prefix=$(cd -- "$2" && pwd -P)
shopt -s nullglob
headers=("$prefix"/include/pcinsight/*.hpp)
if [[ ${#headers[@]} -eq 0 ]]; then
    printf 'No installed public headers found\n' >&2
    exit 2
fi
for header in "${headers[@]}"; do
    printf 'Checking %s\n' "${header##*/}"
    "$compiler" -std=c++20 -Wall -Wextra -Werror -I"$prefix/include" \
        -include "$header" -x c++ -fsyntax-only /dev/null
done
printf 'Standalone public headers passed: %s\n' "${#headers[@]}"
