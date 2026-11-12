#!/usr/bin/env bash
# Create a local archive from an already audited Linux SDK. No publishing.
set -euo pipefail

if [[ $# -ne 4 || ! ${4-} =~ ^pcinsight-sdk-[a-zA-Z0-9._-]+\.tar\.gz$ ]]; then
    printf 'Usage: bash %s SDK_PREFIX EXPECTED_MANIFEST NEW_OUTPUT_DIR ARCHIVE_NAME\n' "$0" >&2
    exit 2
fi

source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
prefix=$(cd -- "$1" && pwd -P)
manifest_dir=$(cd -- "$(dirname -- "$2")" && pwd -P)
manifest=$manifest_dir/$(basename -- "$2")
name=$4
if [[ ${prefix##*/} != sdk || ! -f $manifest ]]; then
    printf 'Expected a dedicated prefix named sdk and its recorded manifest\n' >&2
    exit 2
fi
output_parent=$(cd -- "$(dirname -- "$3")" && pwd -P)
output=$output_parent/$(basename -- "$3")
case "$output" in
    "$prefix"|"$prefix"/*)
        printf 'Output directory must be outside the source SDK prefix\n' >&2
        exit 2
        ;;
esac

check_payload() {
    cmake "-DSDK_PREFIX=$1" "-DEXPECTED_MANIFEST=$manifest" \
        -P "$source_dir/tests/linux_delivery/sdk_payload_manifest.cmake"
}

check_payload "$prefix"
mkdir -- "$output"
mkdir -- "$output/extracted"
# Archive only the validated manifest entries, not unrelated directory contents.
sed -n '2,$s/^[0-9a-f]\{64\}  \([^\r]*\)\r\{0,1\}$/sdk\/\1/p' "$manifest" |
    tar --numeric-owner --owner=0 --group=0 --no-recursion -czf "$output/$name" \
        -C "${prefix%/*}" --verbatim-files-from -T -
tar -xzf "$output/$name" -C "$output/extracted"
check_payload "$output/extracted/sdk"
(
    cd -- "$output"
    sha256sum -- "$name" > SHA256SUMS
    sha256sum -c SHA256SUMS
)
