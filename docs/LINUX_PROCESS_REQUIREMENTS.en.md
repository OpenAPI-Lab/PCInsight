# N03: Linux Processes, Details, and process actions

Revision 161, 2026-11-06. Covers requirement R13 and the associated
Processes/Details actions under existing IDs D01/D02/D03/D04/F03/L01/L02/L03/L04
and delivery N03. It neither adds subtasks nor certifies other panels. The GUI
shows two views of the same `SystemSnapshot.processes[]` row; there is no need
for a separate Details API.

## Sampling and row selection

Keep one `Monitor` and request `SampleOptions.include_processes=true`.
Two continuous observations yield rates when their sources are valid. A first
interval, omitted PID, inaccessible read, reset, or changed identity leaves
rates unknown. The list contains visible, accepted procfs inventory, not all
physical processes across all machines or namespaces.

Do not key GUI selection by row position, name, or PID alone. Keep the PID and
`linux_start_ticks` without converting ticks to seconds or a date, within the
same host, boot, and PID namespace. An exec can preserve both values. The pair
is not an executable generation, cryptographic identity, or pidfd retained
from the sample. An observation can become stale before display or an action
request. CPU, RSS, state, NSS, ELF, disk, and the process rows are not read as
one transaction.

Use `metric_is_known` along with support and note to determine availability.
Known zero and false are not absence. Prefer the Metrics in `linux_details`
over empty legacy strings. Do not substitute username=root, description=name,
architecture=x64, or status=running. Raw `pid`, `parent_pid`, and
`thread_count` come from the initial accepted stat and lack individual
availability. They neither identify a parent durably nor count an entire app
or process tree. The example labels them as row scalars.

## Field matrix — `ProcessInfo` prefix

| Requested data | API / unit | Scope and evidence |
| --- | --- | --- |
| Name | linux_details.name / text | Observed Name, normalized and escaped for display. Not a product name or unique name. [Details](LINUX_PROCESS_DETAILS.en.md), [test](../tests/linux_process_details.cpp). |
| PID | pid / PID visible in the caller's namespace | From validated stat and positive in the native collector. Do not use as durable identity or turn it into a process-group ID for signaling. [Parser](../tests/linux_process_stat.cpp). |
| Parent and thread count | parent_pid / PID; thread_count / count | Observed stat scalars. These are software threads, not CPU logical processors. Do not assume the parent still exists or that the app has no other processes. Same parser. |
| Status/running | linux_details.status / kernel state | R/S/D/Z/T and other states retain Linux semantics. Sleeping is not terminated; a zombie is not proof of an application doing useful work. Do not collapse everything to running/not running or predict future state. Same evidence as Details. |
| User/system | linux_details.real_uid, username / UID and text | Real UID, not effective UID; UID 0 is valid. NSS `getpwuid_r` may fail to resolve, block, or query identity services. Do not equate Windows “system” with every root process or invent root on failure. Same evidence as Details. |
| CPU usage | cpu_percent / % of visible CPU capacity | `100 * (delta utime + delta stime) / CLK_TCK / process_interval / visible_CPUs`. Estimated, not percent of one core, a cgroup quota, power, or frequency-weighted utilization. Do not add child CPU times. [CPU](LINUX_PROCESS_CPU.en.md), [test](../tests/linux_process_cpu.cpp). |
| Memory usage | memory_bytes / B | Estimated VmRSS, not private, unique, proportional (PSS), virtual, or swap memory. Shared pages may repeat; do not sum as unique physical RAM. Multiply kB from status by 1024. [RSS](LINUX_PROCESS_MEMORY.en.md), [test](../tests/linux_process_memory.cpp). |
| Disk | disk_bytes_per_second / B/s | Separate rate from the delta of `read_bytes + write_bytes` in proc/io, with its own clock and verified identity and continuity. Estimated, not rchar/wchar, a percentage, physical per-disk throughput, or completed writes. [I/O](LINUX_PROCESS_IO.en.md), [test](../tests/linux_process_io.cpp). |
| Disk UID attribution | linux_details.disk_rate_real_uid / UID | Stable UID observed around the counters and at both interval endpoints. It may be missing even when the per-process rate is known. Does not establish constant credentials or a causal session. [User I/O](LINUX_USER_IO.en.md), [test](../tests/linux_user_io.cpp). |
| Legacy network | network_bytes_per_second / B/s | Unsupported in the Linux collector. Do not fill it with partial selected-capture data. See the separate report below. [Integration](LINUX_PROCESS_NETWORK_MONITOR.en.md), [test](../tests/linux_process_network_monitor.cpp). |
| Architecture | linux_details.architecture / ELF label | Class, endianness, and e_machine of the pinned executable; recognizes supported combinations. Do not infer x64 from 64 bits or substitute the CPU, host, or Windows architecture. Unidentifiable scripts and wrappers remain unknown. [Description/ELF](LINUX_PROCESS_DESCRIPTION.en.md), [test](../tests/linux_process_details.cpp). |
| Description | linux_details.description, description_source / text | Estimated unlocalized XDG Comment associated with the executable through evidence, not signed text or a commercial product name. Ambiguity, unsupported wrappers or arguments, a changed source, or a missing comment leaves it unknown. `include_linux_process_descriptions=false` disables scanning. [Contract](LINUX_PROCESS_DESCRIPTION.en.md), [test](../tests/linux_process_metadata.cpp). |
| Lifetime token | linux_start_ticks / ticks since boot | Field 22 of stat. Zero can be a valid reading, but checked termination rejects it. It is not FILETIME, a date, or a global token. [Termination](LINUX_TERMINATE.en.md), [test](../tests/linux_terminate.cpp). |
| Efficiency available | efficiency_mode_available / bool | Linux reports known false for the generic/checked path. This does not mean every advanced preset or OS privilege is impossible. [Decision](LINUX_EFFICIENCY_DECISION.en.md), [test](../tests/linux_efficiency.cpp). |
| Efficiency requested | efficiency_mode_requested / bool | Windows field. Do not turn Linux absence into false or use this field to verify the advanced variant. Same efficiency contract. |

CPU usage is normalized against all visible CPUs: one busy thread on a
32-CPU system is about 3.125%, not 100%. Summing process rows is not guaranteed
to match global CPU utilization; windows and coverage differ. D04 superseded
the historical note in [LINUX_PROCESSES](LINUX_PROCESSES.en.md) about dividing by
global ticks. Do not use that as the current formula.

## Selected network capture: partial data, not a fabricated total

`snapshot.linux_process_network` is a `Metric<LinuxProcessNetworkSnapshot>`
defined in the [public header](../include/pcinsight/linux_process_network.hpp).
`include_linux_process_network` neither starts nor authorizes capture.
Capture requires explicit setup with consent, suitable kernel support and
permissions, and verified selection. The example does not do this, even
with `--sample`.

| Data | API / unit | Consumer rule |
| --- | --- | --- |
| Availability and scope | Outer Metric, coverage | Check the parent before any child. Strict, declared-bucket, and observed modes have different scopes. All measure instrumented TCP/UDP operations, not total, interface, or on-wire traffic. |
| Domain identity | observer_boot_token, observer, producer_initial_tgid, producer_start_boottime_ns | Compare observation domains, not loose UIDs, names, or PIDs. Tokens are not credentials or authorization. |
| Epoch and sweep | selection_epoch; counter_sweep_started_ns/finished_ns / CLOCK_MONOTONIC ns | Changes, resets, and readmissions break continuity. Do not interpolate across epochs or mix clocks with boot ticks or CLOCK_BOOTTIME ns. Batches are not an atomic snapshot. |
| Bucket identity | processes[].process, processes[].kernel | Preserve procfs, namespace, inode, PID, and start information plus initial TGID, UID, boottime, and cookie as supplied by the provider. A process can have multiple rows or cookies. Do not join by PID plus name or sum by position. |
| Accumulated observed bytes | payload_bytes_since_start / four-element array of payload B | Order: TCP send, TCP receive, UDP send, UDP receive. Counts from capture origin, not the full process lifetime or activity before setup. |
| Rates per lane | payload_bytes_per_second[4] / payload B/s | Estimated Metric per lane. Missing first, recovered, or warmup rates are not zero. A known accumulated zero does not imply a known zero rate. |
| Interval | nominal_interval_ns / ns | Shared nominal interval, not perfect simultaneity across counters. |
| Actors without buckets | observed_processes / rows | Observed mode can contain an admitted actor with no positive activity observed. Do not invent a cookie, bucket, or zero rate. An empty list also does not imply capture is disabled. |

The example prints identities, scopes, and all four lanes separately. It does
not join or sum them into `ProcessInfo.network_bytes_per_second`. Tests cover
the same PID with different start ticks, an unknown parent that contains data,
known zero totals without rates, and an actor without a bucket. They do not
test real BPF capture. For an existing authorized capture, see
[observed mode](LINUX_PROCESS_NETWORK_OBSERVED.en.md),
[selection](LINUX_NETWORK_SELECTION_OWNER.en.md),
[admission](LINUX_NETWORK_ADMISSION_MONITOR.en.md), and
[acceptance](LINUX_NETWORK_ACCEPTANCE.en.md). Earlier evidence is retained, not
relabelled as new capture from this work. The maximum of 1,024 actors and 16
cookies per actor is structural, not a throughput or cadence guarantee.

## GUI actions are separate from sampling

[actions.hpp](../include/pcinsight/actions.hpp) exposes `SystemActions`.
The example checks signatures at compile time; it never creates or invokes
the backend. Processes and Details share these actions, rather than using
copies per panel. Run new task in Performance, App history, Startup, and
Users is also the same launch operation, not a different metrics provider.

| Button | API | Confirmation, effects, and limits |
| --- | --- | --- |
| Run new task | launch_process(LaunchRequest) | Requires explicit intent. Executable, arguments, and working directory are NUL-free bytes; arguments are literal. Do not concatenate shell text or automatically execute a metadata command. PATH, cwd, environment, and inherited descriptors follow the contract. No sandbox or readiness guarantee. Scripts with a shebang or explicit interpreter can be run by this action, **never for collection**. [Launch](LINUX_LAUNCH.en.md), [test](../tests/linux_launch.cpp). |
| Preferred End task | terminate_process_checked(pid, start_ticks, force) | Explicitly confirm possible loss of work. Use a recent sample in the same domain and a known nonzero token. Rejects PID 0, 1, self, and out-of-range values; pins a pidfd, revalidates identity, and signals that descriptor. `force=false` sends SIGTERM; true sends SIGKILL. No automatic escalation or process-tree or group termination. Success confirms exit through a poll of at most 5 s, not readiness or an exit code. A timeout may have sent a signal. [Contract](LINUX_TERMINATE.en.md), [test](../tests/linux_terminate.cpp). |
| PID-only End task | terminate_process(pid, force) | Uses a pidfd after opening it but does not protect the interval from sample to open. Do not automatically fall back to this path when a checked token, identity, or permissions check fails. Same contract and test. |
| Normal efficiency mode | set_efficiency_mode / set_efficiency_mode_checked | Linux returns `not_supported` without mutation. Do not promise EcoQoS, a safe checked operation, or guaranteed physical savings. [Decision](LINUX_EFFICIENCY_DECISION.en.md). |
| Optional advanced efficiency mode | apply_linux_efficiency_unchecked / restore_linux_efficiency_unchecked, linux_efficiency.hpp | Requires explicit acceptance of numeric PID/TID risk and is off by default. Applies nice/I/O presets to captured threads, not new threads or descendants. Partial effects are possible; retain the receipt and outstanding entries even on error. Restoration is explicit, with no fabricated defaults or guaranteed rollback. [Contract](LINUX_EFFICIENCY.en.md), [test](../tests/linux_efficiency.cpp). The example does not run it. |
| Optional auditing | AuditedActions | Logs calls that pass through the decorator. It does not authenticate the user or guarantee tamper-proof persistence. A requested entry without completed can mean an unknown outcome. Advanced standalone variants are not automatically audited. [Contract](AUDIT.en.md), [test](../tests/audit.cpp). |

`launch_process` may return a `process_id` even if a channel error leaves
the result uncertain. The parent must reap that child with `waitpid` or its
own supervisor; there is no hidden reaper. `terminate_process_checked` does
not reap another parent's children; their parent retains that responsibility.
No action invokes sudo or pkexec by itself. The L05 privileged helper and
operational consent remain deferred; a UI boolean is not certification.
Requesting only a sample, properties, or a table refresh never enables an
advanced path.

## Example and validation

[linux_processes.cpp](../examples/linux_processes.cpp) uses only the public
SDK and the example formatting helper shared with Performance. It shows help
by default. `--self-test` uses synthetic data without querying the OS or
relying on assertions that can be compiled out. `--sample` enumerates
visible processes but only prints SELF. It does not restrict collection to
one PID. The smoke test disables XDG descriptions to avoid scanning;
CPU, RSS, state, ELF, and NSS still use their normal sources and permissions.
NSS or system calls can block. The 20-second alarm and 30-second CTest limit
are harness limits, not an API service-level guarantee.

Terminal output includes field, unit, support, value, and note. ASCII control
characters are escaped; this is neither JSON nor a UTF-8 serializer. Indices
are local to the output, not GUI keys. It does not print arguments or working
directory, launch or terminate processes, or change niceness. The
[external consumer](../tests/process_consumer/CMakeLists.txt) compiles the
same program against the installed SDK, with no private headers and no
requirement to create the GUI's CMake configuration now. The shared formatting
code also revalidates the Performance revision 160 tests. No ABI, layout, or
library behavior changes were introduced.

This work does not close App history, Startup, Users, or Services or their
actions; their traceability remains separate. It also does not close N03:
the overall audit and delivery, historical risks not reproduced or fixed,
and physical acceptance remain open. SysV without explicit authorization,
L05/I07/Windows, and G07/N02/B08 retain their existing status.

Final verification: Linux Release passed 192/192. Four example tests were
repeated ten times, and 20 portable MSVC runs passed. The installed SDK
consumer exited 0 in self-test and SELF sample modes. Thirty-nine local
links were checked. Logs: `work/process-contract-*`. This was not remote CI
or additional physical acceptance.
