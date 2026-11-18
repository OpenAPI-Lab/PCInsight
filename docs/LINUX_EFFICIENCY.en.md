# Linux CPU and I/O priority preset

Revision 41 status: the limited advanced scope was accepted on 2026-10-19;
see the [L03 decision](LINUX_EFFICIENCY_DECISION.en.md). L03 closes within
that limit, NOT with checked identity or universal coverage. The generic
call is disabled; only the advanced workflow with explicit acknowledgment
remains available. The revision 17 and legacy sections preserve historical
evidence and do not describe a current public bypass.

## Per-thread workflow and restoration receipt (partial L03, revision 17)

The additive API in `<pcinsight/linux_efficiency.hpp>` provides
`apply_linux_efficiency_unchecked(pid, expected_start_ticks, true)` and
`restore_linux_efficiency_unchecked(receipt, true)`. The last argument
explicitly acknowledges numeric PID/TID risk; its default, false, rejects
the operation without mutation. This does not replace
`SystemActions::set_efficiency_mode_checked`, which remains unsupported on
Linux. Revision 41 disables generic access to the legacy preset described below.

The workflow enumerates `/proc/PID/task`, captures each thread's nice and raw
ioprio values, and retains process and thread `start_ticks`. It checks caller
procfs/PID context, the list, and tokens before mutation, rejecting incomplete
capture, duplicate/reserved IDs, invalid priorities, or observed changes.
Limits: 4,096 threads, 64 KiB stat reads, and up to three EINTR retries per
read. No external utilities, new packages, suspension, ptrace, helpers, or
elevation are used.

It requests nice=max(original,10) and idle ioprio, skipping writes already
satisfied. It checks identity and the current value before each field,
reads back after writing, then checks priorities for all captured threads
and the final list. A changed list before writing prevents changes; a
changed list afterward means partial coverage, not success. Equal lists
at two instants do NOT prove there were no threads in between. New threads,
descendants, and exec are not tracked; exec does not necessarily change
start_ticks. Nice and ioprio do not guarantee savings.

The receipt retains exact originals per thread (including I/O class NONE=0,
not replaced by best-effort 4), requested values, and `nice_pending`/
`io_pending` flags. Each successful write sets its flag even if read-back
fails; the receipt is not lost on error. Restoration handles only those
fields. A value already at its original clears the flag without a write.
A value at the requested setting is restored and verified. A different
observed value is rejected as a conflict without overwriting it. The ENTIRE
receipt is validated before acting, including duplicates, ranges, and
leader/token correspondence. Untrusted receipts are not accepted.

The workflow stops at the first error. It is not a transaction or automatic
rollback. Restoration can fail with I/O already restored and CPU pending
because of CAP_SYS_NICE/RLIMIT_NICE. Use the **returned receipt** for a retry:
completed fields have false flags and outstanding fields stay pending. Do
not promise restoration will succeed without additional permissions.
A terminated thread can stop the rest of the operation; it is not silently
skipped.

Tokens only mitigate detectable changes. Priority syscalls do not accept
pidfds, so a race remains between reads and writes, including reading a
priority and restoring it. Do not call this checked, safe against reuse,
or compare-and-swap. ioprio shared through CLONE_IO can affect threads
outside the captured set. The recorded write is retained and restored
without claiming isolation. The receipt is ephemeral for the current boot
and namespace, not persistent identity or authorization. The caller must
retain it; reapplying creates another receipt with the values at that
moment and does not recover the originals from an earlier call. These free
functions do not go through AuditedActions. An integration that needs
auditing must explicitly record the request, result, and receipt.

Example (PID/token come from a sample in the current boot and namespace):

```cpp
#include <pcinsight/linux_efficiency.hpp>
auto update = pcinsight::apply_linux_efficiency_unchecked(pid, start_ticks, true);
// Keep update.receipt even if !update.result: partial changes are possible.
// Restore later ONLY if the user requests it, not in a destructor.
auto restored = pcinsight::restore_linux_efficiency_unchecked(update.receipt, true);
// Keep restored.receipt if restoration did not finish.
```

Windows returns not_supported for these functions; existing EcoQoS behavior
is unchanged. The additive API does not modify existing layouts or virtual
methods. Rebuild and relink consumers to use the new symbols. In revision 17
L03 remained E because this workflow does not offer checked identity or track
future threads. Revision 41 records explicit acceptance of that limit,
without claiming those guarantees or awarding fractional credit for the receipt.

Verification on 2026-10-17: final Release suites Linux 85/85 and Windows
105/105, both installed consumers rebuilt and relinked and exiting 0, and
20/20 consecutive linux-efficiency runs on Linux. Assertions were active.
Fixtures cover incomplete or changing capture, invalid IDs/tokens/receipts,
bounds, priorities -1/19, context/API/read/write/read-back errors, restoration
conflicts, partial changes, idempotent retries, and simulated shared I/O.
Windows runs injected logic and native rejection, not Linux syscalls.
Native Linux: a private two-thread child at nice 19 had no CPU promotion
and its original I/O was restored exactly. Another private two-thread child
at nice 5/19 had its leader moved to 10; CPU restoration was denied with
errno 13 without elevation, the leader's I/O was restored, and its CPU flag
remained pending. The caller was unchanged and the children were reaped.
No unrelated processes were used. Workspace logs:
`work/efficiency-threads-build-{linux,windows}.log`,
`work/efficiency-threads-suite-{linux,windows}.log`,
`work/efficiency-threads-native-linux.log`, and `work/efficiency-threads-repeat-linux.log`.
Intermediates are not included in the package.

## Legacy main-thread preset (internal and historical since revision 41)

The original Linux fix for `SystemActions::set_efficiency_mode(pid, enabled)`
remains as an internal regression algorithm. Since revision 41 the public
call returns not_supported without mutation. Do not reuse it to apply or
undo changes or as a substitute for the receipt. The historical algorithm
was a main-thread priority preset, **not Windows EcoQoS**, a hardware power
policy, or a guarantee of lower consumption. It does not run external
nice/renice/ionice tools.

- enabled=true: nice=max(current nice, 10), I/O class idle (data 0).
  A process at nice 19 stays at 19; enabling never requests CPU priority
  promotion to nice 10.
- enabled=false: nice=0 and I/O best-effort 4. These are explicit fixed
  values, not restoration of past priorities. Raising CPU priority may
  require CAP_SYS_NICE/RLIMIT_NICE; no elevation or helper installation.

## Errors and partial changes

The earlier version ignored ioprio_set and reported “restored” priorities
after imposing 0/best-effort 4. The corrected historical algorithm reads
both priorities before mutation and rejects PID 0/1/self/outside INT_MAX,
incomplete APIs, nice values outside -20..19, and unknown I/O encodings.
getpriority=-1 is not interpreted as an error unless errno is set.

It requests I/O first, then CPU, skipping writes already satisfied.
If I/O fails, it does not request CPU. If CPU fails after I/O changed, it
returns the real error with a **partial result** message. It does not
automatically roll back, which could fail on permissions, interfere with
concurrent changes, or affect a reused PID. It reads both priorities at the
end; mismatches and read failures are not reported as success. Successful
reads confirm values only at those instants, not an atomic transaction or
continued values after external changes.

EPERM/EACCES maps to permission_denied, ESRCH to not_found, ENOSYS/EOPNOTSUPP
to not_supported, EINVAL to invalid_argument, and the rest to system_error.
There are no hidden processes or threads, file creation, or changes to the
device scheduler, cgroups, or caller priorities.

## Scope and safety

PRIO_PROCESS/IOPRIO_WHO_PROCESS syscalls use numeric PID/TID and do not
accept pidfds. This variant retains PID reuse risk before or between
syscalls. `set_efficiency_mode_checked` remains not_supported on Linux:
a stat check followed by setpriority(PID) is not presented as protection
equivalent to pidfd termination. A preset with stable identity or persistent
references needs a separate design and is not completed by this unit.

The legacy preset applies to the indicated leader/TID, not all threads,
new threads, or descendants. Linux nice is per-thread. A shared CLONE_IO
context can propagate effects to other threads or processes sharing it.
I/O effectiveness depends on scheduler, device, and operation type;
accepting or reading a priority does not prove effective throttling.
Affinity, clocks, CPU scheduling class, and CPU quota are unchanged. The
API has no global deadline.

Since revision 41, Linux `ProcessInfo::efficiency_mode_available` reports
false with available support for the disabled generic path. The old
owner/root heuristic was removed; it did not establish capabilities,
RLIMIT_NICE, seccomp, or scheduler behavior. False does not say priority
syscalls do not exist. The advanced option is separate and never an
automatic fallback. `efficiency_mode_requested` remains reserved for the
Windows EcoQoS provider; a mode request is not inferred from matching
Linux priorities.

IOPRIO uses native syscalls and ABI constants (process=1, idle=3,
best-effort=2, shift=13). Headers or kernels without those syscalls produce
not_supported before changing CPU. Unknown priority flags or hints fail
closed rather than being blindly reinterpreted or removed. No external
dependencies are added.

## Historical legacy preset verification

2026-10-14: Linux Release 18/18, Windows Release 29/29; packages installed
and external consumers rebuilt and run. In the native test without
elevation, disabling returned permission_denied (errno 13) with I/O changed
and CPU still at nice 10. Both priorities were read to confirm the partial
state rather than relying on the message alone.

Injected tests cover valid nice -1, nice 19 preserved, idempotence, false
preset, permission/absence/invalid encoding, I/O failure with no CPU write,
CPU failure with partial I/O changes, denied or mismatched verification,
and incomplete APIs. Write order is checked. Native Linux tests use only
two temporary children: nice 0 to 10 plus idle, repeat, disable with success
or explicit permission denial, and nice 19 without promotion. All children
are reaped. The caller's nice/I/O are verified unchanged. AuditedActions
preserves both stages and the result. Windows runs only portable injected
logic; that build does not establish native Linux coverage.

## Revision 41 and consumer migration

Accepting the scope activates no operation and grants no privileges. The
GUI must leave the advanced option off by default, show the PID/TID reuse
warning, and obtain acknowledgment before passing true. Always keep the
returned receipt, even on error. Restoration requires another explicit
action and can fail partially. Do not use set_efficiency_mode(pid,false)
to restore: it is disabled since this revision and previously imposed
defaults. Do not modify a SystemActions subclass to bypass backend policy.

The 0.x behavior changes while signatures, layouts, and vtables remain
compatible; rebuild, relink, and test consumers. Do not convert a
not_supported error into silent opt-in. Checked remains unavailable.
There are no guarantees of savings, new-thread or descendant coverage,
stable identity, or restoration without permissions. No new helper,
elevation, ptrace, or cgroup. Current evidence: work/efficiency-optin-*.log.
Final regression: Linux 108/108, Windows 122/122 Release, 20 Linux repetitions,
and both installed consumers rebuilt, relinked, and exiting 0. Final builds
had no warnings. No unrelated processes or caller priorities were modified,
and no private children were left unreaped.

References: [getpriority/setpriority](https://man7.org/linux/man-pages/man2/setpriority.2.html),
[ioprio_get/set](https://man7.org/linux/man-pages/man2/ioprio_set.2.html).
