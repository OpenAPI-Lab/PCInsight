# Local MSVC x64 validation

M01 unit, revision 51, 2026-10-21. This document distinguishes compiler
validation from hardware support and CI. The library remains at version
0.2.0. It does not declare API/ABI 1.0 or enable experimental providers.

## Environment and reproduction

Visual Studio 18 Community, MSVC 19.51.36260.0 (toolset 14.51.36231), Windows
SDK 10.0.26100.0, CMake 4.4.2, Visual Studio 18 2026 generator, and x64
platform. The tests use installed tools, without downloads or system
configuration changes. All verification artifacts stay under work/.

From the workspace root, with cmake and ctest on PATH:

```powershell
cmake -S outputs/pcinsight -B work/build-msvc-release -G "Visual Studio 18 2026" -A x64 -DCMAKE_CXX_FLAGS_INIT=/MP2
cmake --build work/build-msvc-release --config Release --parallel 2
ctest --test-dir work/build-msvc-release -C Release --output-on-failure
cmake --install work/build-msvc-release --config Release --prefix work/install-msvc
cmake -S outputs/pcinsight/tests/consumer -B work/consumer-msvc -G "Visual Studio 18 2026" -A x64 -DCMAKE_PREFIX_PATH="$PWD/work/install-msvc" -DCMAKE_CXX_FLAGS_INIT=/utf-8
cmake --build work/consumer-msvc --config Release --parallel 2
& ./work/consumer-msvc/Release/consumer.exe
```

Check the exit code of EVERY stage before continuing. The generator is
multi-configuration: select Release when building, testing, and installing,
not through CMAKE_BUILD_TYPE. Existing test assertions stay active with
/UNDEBUG. The new encoding test uses static_assert, independent of NDEBUG.

/MP2 is an option for this verification only, not a library requirement.
With two simultaneous projects it limits compilation to four processes.
Initialize CMAKE_CXX_FLAGS_INIT to preserve platform and exception flags;
do not replace all of CMAKE_CXX_FLAGS with /MP2. An older cache can need
explicit reconfiguration. Do not reuse the same build for MinGW and MSVC.
[The /MP contract](https://learn.microsoft.com/en-us/cpp/build/reference/mp-build-with-multiple-processes).

## Verifiable fixes

The initial native build found that Microsoft SDK winternl.h does not declare
NtSetInformationFile, FILE_RENAME_INFORMATION, or FileRenameInformation,
which the Startup provider used through MinGW headers. No WDK is installed.
The provider now declares the documented ABI signature and structure internally,
loads the SAME symbol dynamically, and retains class 10, a handle-pinned
RootDirectory, and ReplaceIfExists=false. It checks field and class-selector
sizes and offsets. MinGW also compares them with its independent declarations.
There is no switch to MoveFile or pathname-based operation, copy/delete, or
another fallback. Checked identity, scope, privileges, and consent are unchanged.
References: [NtSetInformationFile](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntsetinformationfile)
and [FILE_RENAME_INFORMATION](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_rename_information).
Do not substitute the SMB/RDP wire format for this native layout.

CMake sets /utf-8 for C++ on this project's targets, not for the resource
compiler or as an exported consumer requirement. This avoids depending on
the user's code page for source and narrow protocol literals.
tests/utf8_literals.cpp checks the expected UTF-8 bytes of direct and escaped
literals and wide/UTF-32 forms at compile time. It is an explicit portability
guard, not a claim that text corruption was reproduced on this host before
the change. [The /utf-8 contract](https://learn.microsoft.com/en-us/cpp/build/reference/utf-8-set-source-and-executable-character-sets-to-utf-8).

Type warnings were fixed without disabling diagnostics: fixed NVML array
sizes 128/256 explicitly converted to unsigned, ID and buffer constants typed
as unsigned/uint8_t, a local frequency variable renamed, and the ASCII GUID
from StringFromGUID2 explicitly converted. Telemetry algorithms are unchanged,
and estimates are not relabeled exact. No public headers, signatures, layouts,
vtables, or persisted formats change. Updating the library and consumers
together remains the existing 0.x policy.

MSVC also found a BUILD failure in windows_history_store. The test constructed
a system_clock::time_point from -1 ns, which cannot be represented implicitly
by that STL's 100 ns clock. It now uses one negative native tick, retaining
the requirement to reject it before millisecond normalization. No cast rounds
it to zero, and the store, format, and validators are unchanged.
[History](HISTORY.en.md) and [Windows history store](WINDOWS_HISTORY_STORE.en.md)
were reviewed before changing the test. The private service-name ASCII conversion
warning is fixed explicitly. The manual SCM executable is built, NOT run for
this unit, and prior consent is not reused to create another service.

## Evidence and limits

Final Release results: MSVC 127/127, MinGW 127/127, and Linux 111/111. Three
separate installations, with external consumers rebuilt, relinked, and exiting
0. MSVC .lib and MinGW .a are not mixed. MSVC repeated UTF-8, Startup file
control, Windows history, and PDH tests 20 times each (80 runs). Linux repeated
UTF-8, CPU frequency, NVML, DRM discovery, and Windows history tests 20 times
each (100 runs).

work/msvc-verified-build-windows.log and msvc-native-suite-windows.log record
the corrected full MSVC build and suite. msvc-repeat-windows.log records the
80 repetitions. msvc-native-positive-windows.log records positive private file
control and CPU/firmware reads. msvc-install-windows.log and
msvc-consumer-*-windows.log record the package and separate consumer. Final
MinGW/Linux regression logs are msvc-post-clock-{build,suite}-{mingw,linux}.log;
final consumer logs are msvc-final-consumer-{mingw,linux}.log. Earlier attempts
are retained and are NOT counted as passing suites. The build before the clock
test fix ended with C2440; the later build exited 0.

No warnings were disabled. Detected C type and declaration warnings were
fixed. MSVC still emits command-line diagnostic D9025 because /UNDEBUG
overrides /DNDEBUG to keep Release assertions enabled. It is not described
as a build with no warnings at all. Final MinGW/Linux builds had no warnings.

Positive MSVC Startup control is verified only in private fixtures:
removal/restoration, bytes, recovery/conflicts/identity/audit, and cleanup.
No real Startup data was modified, no manual SCM action was run, and no real
HKLM Run entry was changed. The integration test reports a Ryzen 9 7950X,
32 logical processors, DDR5 DIMM RAM at 5,200 MT/s, and 2/4 devices represented
by firmware. Those are host-specific data, not a universal matrix. Multi-group
PDH is tested with a model and native queries, not on certified physical
multi-group hardware. M01 remains open for stable compression and multi-group
validation, with no new closure or partial credit.

The first sandboxed build did not progress past the initial rule and was
interrupted. The build with native compiler access did progress. The precise
cause of the stall was not established, and UAC, ACLs, and antivirus were not
reconfigured to bypass it. The first native build was also interrupted after
recording declaration errors and warnings. An attempt to add /MP2 by replacing
CMAKE_CXX_FLAGS dropped /EHsc and produced C4530. It was interrupted and
reconfigured with FLAGS_INIT, preserving exception support. That attempt is
not certified.

A local MSVC suite is not remote CI (N01), nor does it validate ARM64/x86,
other SDK/MSVC versions, or hardware with multiple CPU groups. It does not
make the experimental compression provider stable or documented. Vulkan
headers were not found with MSVC in this configuration, and dependent
features retain their declared availability. This does not certify every
GPU API, K03, or the G07/N02 physical matrices. M05 remains deferred by
explicit user choice.
