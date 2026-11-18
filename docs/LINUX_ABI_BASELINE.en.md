# A05: Linux SDK compatibility baseline

**Language:** [Español](LINUX_ABI_BASELINE.md) · English

Revision 168, November 6, 2026. The SDK remains at 0.2.0. This baseline
helps review changes to commonly used public types. It does not claim that
the ABI is frozen or that a matching comparison is sufficient to freeze it.

## What it checks

- The [installed SDK probe](../tests/linux_abi/probe.cpp) checks selected
  signatures and unit and identity field types at compile time. It covers
  sampling, history, actions, service details, and startup properties.
  It does not invoke those actions. Linking also requires the
  [header marker](SDK_HEADER_COMPATIBILITY.en.md).
- It records `sizeof` and `alignof` for 28 selected types in the original
  baseline. It compares the profile before comparing rows: Linux x86_64,
  GCC or Clang, libstdc++ header date, `std::string` ABI, language mode,
  pointer size, and libstdc++ debug and assertion flags. A mismatched
  profile does not count as a pass.
- A size or alignment change fails the check and identifies the first
  differing row. Two fixtures check rejection of a changed size and of
  a different profile, with separate codes and diagnostics.

Since revision 204, the baseline also compares eight offsets in
`ActionResult` and `LaunchRequest` on libstdc++ profiles without debug
mode. A fixture changes one offset without changing size or alignment
and requires rejection.

This does not inspect every offset, vtable, calling convention, symbol,
flag, or semantic change that can affect ABI. Different layouts can share
the same size and alignment. The comparison characterizes the consumer
compiled under the recorded profile; it does not establish equivalence
for all library objects or third-party plugins. Do not use this file as
a portable baseline for another GCC version, libstdc++ version,
architecture, or MSVC.

## Reviewing 0.x changes

When a public header changes, review its fingerprint and rebuild consumers
against the same SDK. If a signature, field, unit, or meaning changes,
record the break or contract change even if sizes are unchanged. Track
incompatible persisted formats separately: this probe does not read or
migrate them. Do not update the baseline just to pass a test without
reviewing and recording the difference. For a new profile, keep separate
evidence instead of replacing the existing profile and calling it compatible.

Revision 201: [Clang 18.1.3 with libstdc++](../tests/linux_abi/baseline-linux-x86_64-clang18.txt)
has its own baseline. The 28 observed sizes and alignments match the GCC
13.3 rows in this environment, but the profiles are not interchangeable.
To reproduce with the Clang SDK, add `-DCMAKE_CXX_COMPILER=clang++` and
select the Clang file in `PCINSIGHT_ABI_BASELINE`. This does not add a
libc++ profile or freeze the 1.0 ABI.

Revision 229: both baselines add `LinuxInterfaceAddressList` (56 bytes,
alignment 8 on these profiles), and the probe checks the query signature.
There are now 29 measured types; the existing rows did not change.

Revision 202: separate relinking test. For each profile, the
`gui_read_consumer` and `result_contract` objects were compiled once
against older SDK headers. Each object was linked and run against both
its original library and the updated library: GCC 13.3 (185 → 194) and
Clang 18.1.3 (197 → 200), eight successful runs. This covers only those
interfaces, profiles, and revision pairs. Rebuilding after an SDK upgrade
is still recommended.

To repeat the test from the PCInsight root, use the compiler that built
both libraries and a new evidence directory:

```sh
bash scripts/check-linux-sdk-upgrade.sh g++ /path/old-sdk /path/new-sdk /path/new-evidence
```

The script preserves the object compiled with older headers, both executables
per consumer, the compiler version, and both library hashes. It rejects
reuse of the same prefix and does not overwrite evidence. If the old SDK
includes `linux_network_addresses.hpp`, it also checks that consumer;
otherwise it reports that API outside the comparison. A consumer unable to
query addresses returns 77 and does not count as a pass.

## Reproducing the baseline check

From the standalone repository root, after installing the Release SDK in a
local prefix, configure the external verifier. This does not require the GUI
to use CMake.

```sh
cmake -S tests/linux_abi -B work/build-linux-abi-168 \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PWD/work/sdk-headers-167-linux" \
  -DPCINSIGHT_ABI_BASELINE="$PWD/tests/linux_abi/baseline-linux-x86_64-gcc13.txt"
cmake --build work/build-linux-abi-168
ctest --test-dir work/build-linux-abi-168 --output-on-failure
```

The original workspace commands used `outputs/pcinsight/tests/linux_abi`.
`linux-abi-probe --dump` prints the profile and values for review without
rewriting the baseline. The checks do not run services, actions, or capture.
A05 remains open; this is not a 1.0 ABI declaration or a remote CI result.

Verification at revision 168: installed SDK 0.2.0, GCC 13.3.0, libstdc++
20240904, and 15 signature and type checks compiled. The
[recorded GCC baseline](../tests/linux_abi/baseline-linux-x86_64-gcc13.txt)
had 28 rows at that revision. The three tests passed ten consecutive runs,
and the Linux suite passed 200/200. Logs: `work/linux-abi-*`.
The official count at that checkpoint was 60/75 (80%).
