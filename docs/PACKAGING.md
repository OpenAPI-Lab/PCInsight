# Installation and compatibility

PCInsight 0.2 is delivered as a static C++20 library. Shared libraries are not
offered yet: the public structs expose standard-library types and the API is
still experimental. Rebuild consumers with the same compiler, runtime and
compatible build flags as the library. No stable binary ABI is promised for 0.x.
Package version requests use exact matching until a compatibility policy for
1.0 is implemented.

A01 adds app_history.hpp/AppHistorySample/app_history_view/import_legacy_history
and metric_is_known. LegacyAppHistoryEntry aliases the original raw type;
HistoryStore symbols/layout and v1/v2 bytes are unchanged. No layout change to
existing structs in A01. Rebuild consumers using the new functions and link the
updated library; previous layout-change rebuild requirements still apply.
No measured-view to legacy implicit conversion; see METRIC_AVAILABILITY.md.

Linux process details now append `ProcessInfo::linux_details` with per-field
availability and append `SampleOptions::include_linux_process_descriptions`.
Rebuild the static library AND every consumer with the new installed headers.
This is an API0.x layout change, not binary-compatible with previous objects.
Linux descriptions are estimated desktop Comment associations with source,
not authenticated program labels; see LINUX_PROCESS_DESCRIPTION.md.

Linux CPU history adds `LinuxAppHistoryEntry` and appends
`SystemSnapshot::linux_app_history`. This is another public layout change in0.x:
rebuild the library AND all consumers; never mix previous objects/headers with
the new static library. Legacy `app_history` stays empty on Linux; use the new
availability-aware report. HistoryStore does not persist it. See LINUX_APP_HISTORY.md.

Sampler now adds SamplerSample/latest_timed/timed_history, with monotonic60s
retention. Default capacity changes to0 (automatic,61 points at1s); explicit caps
remain additional limits. A new run clears graph history. Rebuild library and
consumers for new symbols/defaults. A03 does not change SystemSnapshot layout;
previous H01 changes still apply. See SAMPLER_WINDOW.md for behavioral compatibility.

C03 appends MemoryInfo::linux_details (LinuxMemoryDetails/composition/zram types).
This changes MemoryInfo and containing SystemSnapshot layout in0.x. Rebuild the
library AND ALL consumers against new installed headers, including Windows;
never mix previous objects/headers. See LINUX_MEMORY_COMPOSITION.md. Firmware
installed-minus-usable remains an explicitly labeled estimate, not hardware-only
reservation. C03 closure still requires resolving/accepting that limit.

F02 appends NetworkAdapter::linux_details (Metric<LinuxNetworkDetails>) with
source-aware identity/classification. Rebuild the library AND ALL consumers on
both platforms. Linux legacy model/connection_type now mirror known metadata or
empty; never modalias-as-model or Ethernet by name fallback. Windows collection
unchanged. See LINUX_NETWORK_IDENTITY.md; no stable binary ABI in0.x.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix /your/prefix
```

Consumers set `CMAKE_PREFIX_PATH` to that prefix and use:

```cmake
find_package(pcinsight 0.2.0 EXACT CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE pcinsight::pcinsight)
```

The installed package resolves Threads and propagates the platform's dynamic
loader link library. It requires no source-tree paths. To verify an installation,
configure `tests/consumer` as a separate project with that prefix and run its
`consumer` executable. Repeat after moving the installation to a new prefix.

Collecting Linux data requires Linux; a successful Windows build checks the
portable components and the implemented native Windows backend, not Linux.

The checked Windows termination API adds a virtual method to SystemActions and
a creation-time metric to ProcessInfo. Rebuild all consumers; do not mix old
headers or objects with the updated static library. The default checked action
on backends that do not implement it returns not_supported without falling back
to PID-only termination.
