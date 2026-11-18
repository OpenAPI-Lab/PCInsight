# I07: Internal exit sensor for a selected process

Revision 105 adds a SEPARATE provider for direct release counters:
[final resource counters](STARTUP_FINAL_RESOURCES.en.md). The lifecycle event
contract is unchanged; the event does not become a final counter.

Revision 104, 2026-10-24. INTERNAL component in linux_startup_exit.hpp/.cpp,
with no public API, Monitor integration, or automatic recorder activation.
It captures a lifecycle event, NOT final CPU/disk counters or autostart cause.
I07 remains open; the event is not converted to a StartupProcessSample or an
impact measurement.

## Observation point and scope

The [Linux 6.6 do_exit source](https://github.com/torvalds/linux/blob/v6.6/kernel/exit.c)
calls taskstats_exit(tsk,group_dead) after setting exit_code and before other
cleanup stages. group_dead indicates the last live thread; entering that
function does not mean cleanup is complete or the process has been reaped.
The [taskstats documentation](https://www.kernel.org/doc/html/latest/accounting/taskstats.html)
distinguishes thread and group statistics. Their counters are not assumed to be
interchangeable, and zeroes in unrelated fields are not treated as measurements.

The FENTRY hook attaches to `taskstats_exit(task_struct*,int)->void`, verified
against real BTF. The initial TGID filter runs BEFORE accessing arguments,
credentials, pointers, or maps. For the selected target, it accepts only
group_dead=1; the leader's exit with another thread still alive does NOT publish
the event. It verifies that task* equals current and checks the initial real
UID and the leader's start_boottime. The hook is a global kernel function, but
no data is recorded for other processes. It does not subscribe to taskstats
netlink broadcasts or register CPU listeners.

Fixed data: group_dead marker, initial TGID/UID, leader start time, and a
CLOCK_MONOTONIC timestamp at function entry. No names, arguments, environment,
network/file contents, CPU, I/O, exit code, or .desktop associations. This is
the kernel monotonic clock: do not automatically convert it to consumer
timepoints with time namespace offsets or to wall-clock time. One selection
per instance, a one-shot event, and no variable buffer or eviction.

## Identity and lifecycle

`LinuxStartupSelectedExitCapture::start(true,pidfd,pid,start_ticks)` requires
explicit consent and a target that is still alive. It duplicates the pidfd,
obtains an authenticated kernel/procfs association through the existing pidfd
permission broker (two kernel observations with context checks before and
after), attaches, and revalidates the same identity. It does not infer the
initial TGID from the local PID or convert ticks to a kernel identity. A change
or exit during setup rejects the capture and closes all resources. There is no
promise of recovering a process that died before capture was armed.

The map retains the event even after the real parent reaps the target and its
PID disappears from procfs. `sample` does not reopen the vanished PID or call
waitid on an unrelated process. It uses the held pidfd to publish `pidfd_exited`
SEPARATELY from the event. The event can be observed before pidfd confirms exit;
its timestamp is not presented as the exact instant the process ended.

Before and after reading, it checks its own boot/observer/namespace context,
caller, and euid. Forking causes reads to be rejected and closes ONLY inherited
references without writing to the parent's maps; the original capture keeps
working. Domain changes are not translated automatically. The caller serializes
calls and keeps the same FD table, without unshare or external closing/reuse of
the capture's descriptors.

## Publication, absence, and failures

Seven-element u64 array: state 0/1, sticky failure, group_dead, TGID, UID,
start_ns, timestamp. One writer (the last thread) writes the payload and
publishes state last via atomic exchange. Failure, identity mismatch, or a
duplicate sets a sticky bit via OR; no counter wraps. The reader checks state
before and after and validates all seven slots. If publication overlaps a read,
the result is temporarily absent and can be retried without resetting the map;
no partial payload is returned. Actual failures close all resources. State 0
is a valid observation without an event, NOT proof the process is still alive.
A ready pidfd without an event is an error or absence, not a fabricated event
or counters.

`stop` and the destructor close the link, program, map, and pidfd. No pinning,
files, services, persistent kernel configuration, or installation. Sampling
the event again returns the SAME timestamp, without summing it or fabricating
a new observation.

## Requirements and reuse

Optional provider for 64-bit little-endian Linux with BTF, an attachable
taskstats_exit, FENTRY trampolines, required helpers/atomics, and BPF/ptrace/LSM
permissions. Absence or denial does not run tools or install packages. It
reports permission_required where a permission issue can be identified; other
failures are reported as unsupported with a reason. Distro/kernel compatibility
is not universal.

It reuses F03's schema, identity joins, and native map reads, NOT network
traffic. Identity bootstrap still requires the existing network BTF schema.
This is a declared internal dependency; universal independence from those
hooks is not claimed. The exit hook resolver is independent of socket types.

read_linux_kernel_btf_blob factors out the bounded 64 MiB reader with the same
file, identity, and truncation validation. read_linux_network_btf retains its
contract. No public layout, vtable, or store changes and no new Windows functions.

## Evidence

`tests/linux_startup_exit.cpp`: minimal BTF, exact prototype, ambiguity, all
truncations, decoder/state/UID 0/identity/failure/zero-without-event, bytecode
bounds, filtering before context/map access, and rejection without authorization.
Release builds keep assertions enabled. Evidence of kernel behavior is not based
on fixtures alone.

`pcinsight-linux-startup-exit-native --authorized-private-exit` is MANUAL, not
CTest. It uses prior authorization for temporary probes filtered to private
processes, without extending that authorization to general capture. The test
creates its own parent and target grandchild. The observer's waitid returns
ECHILD; setup uses native pidfd/identity joins; an unselected sentinel exit
is ignored; and a forked copy is rejected without affecting the parent. The
real parent calls waitpid and confirms reaping BEFORE sample. Procfs no longer
has the process, the event and identity are retained, pidfd confirms exit, and
repeated reads are stable.

TWO real WSL cases were tested: single-threaded and multithreaded, with the
leader a zombie while the worker remains alive. The first does not create
unrelated events. In the second, leader-only exit yields neither an event nor
pidfd_exited; both appear when the final worker exits, even after reaping.
The target is not a direct child of the observer, and no baseline or final
sample is read from procfs after exit. No CPU/disk reconstruction or support
for processes that were never selected. Probes were removed, the FD census
was restored, and children exited normally. Watchdogs: 25 s/20 s; actual
runtime under 5 s.

Logs work/startup-exit-native.log and startup-exit-native-threaded.log preserve
successful runs. The initial fixture had incorrect mutation indices, which
were corrected without relaxing the schema; startup-exit-fixture.log was
retained. The initial warning about an ignored cleanup write was fixed by
preserving its result; logs were left intact.

Remaining work for I07 at this revision: verifiable final CPU/disk counters
with thread/group semantics, coverage of short lifetimes for processes and
descendants NOT preselected, reliable autostart evidence and integration with
the recorder/inventory, and durable diagnostics. This component does not
authorize activating the host at login or expanding capture to real apps.
