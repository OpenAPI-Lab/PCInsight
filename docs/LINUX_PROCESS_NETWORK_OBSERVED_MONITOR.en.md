# Monitor/Sampler: opt-in observed cookies

Revision 93: these public paths now use the
[shared backend](LINUX_NETWORK_SHARED_PUBLIC.en.md), not per-actor fan-out.
Native Monitor, Sampler worker, and owner tests passed with updates, gaps,
epochs, and cleanup. No API change or implicit rearming; general admission
was still pending.

Revision 85 follow-up: [candidate and pidfd management](LINUX_NETWORK_SELECTION_OWNER.en.md)
for callers, NOT general enumeration or selection or automatic rearming.

Revision 84 connects observed_namespace_buckets to the existing Monitor and
Sampler lifecycle. It reuses the public three-argument overloads, with no
new method, layout, flag, or persistence format. The revision 83 layout
change still requires rebuilding ALL consumers using older headers.
F03 remains open: this is explicit selection, NOT general app inventory or
capture.

## Usage

```cpp
#include <pcinsight/monitor.hpp>
#include <pcinsight/sampler.hpp>

pcinsight::Metric<std::uint64_t> prepare_monitor(
    pcinsight::Monitor& monitor,
    std::span<const pcinsight::LinuxProcessNetworkSelection> selection,
    bool consent)
{
    // 1–16 verified PID/start/pidfd entries; zero cookie in ALL rows.
    return monitor.start_linux_process_network(consent, selection,
        pcinsight::LinuxProcessNetworkSelectionMode::observed_namespace_buckets);
}

// After successful setup, on the caller's worker:
// pcinsight::SampleOptions options;
// options.include_linux_process_network = true;
// auto s = monitor.sample(options);
// Check metric_is_known(s.linux_process_network) and each Metric.
```

For Sampler, construct with include_linux_process_network=true, run authorized
three-argument setup WHILE STOPPED, then call the worker's start(). The include
flag alone does not authorize BPF. No CLI or dependencies are installed, no
automatic elevation is performed, and callers need not create CMake before
their GUI. Keep setup, sampling, and shutdown off the GUI thread: these APIs
are synchronous, with no hard BPF deadline.

## Lifecycle rules

- The TWO-argument overload is ALWAYS strict, even after stopping observed
  capture. It does not inherit the previous mode. Explicitly pass three
  arguments for observed mode.
- Setup rejects replacing or changing an active capture. Sampler requires a
  stopped worker and the flag enabled. Without consent, it neither calls the
  provider nor changes active capture. Unknown enums are rejected before
  invoking the provider.
- An older injected adapter without start_mode supports only strict mode.
  Never activate observed mode through the old callback. Setup failure,
  epoch zero, estimated results, or exceptions clean up the provider without
  partial capture or silent rearming.
- Monitor sample with the flag false closes capture and pidfds and withdraws
  the result. Setting it true again does not rearm. Stopping the worker also
  closes capture. Restarting the worker does NOT reauthorize it; worker
  start or a selection update cannot reactivate stopped capture.
- update_linux_process_network_selection requires FRESH consent. Sampler
  supports live updates, serialized with Monitor, collection, and stop,
  using existing pending-update priority and guarded wakeups. It preserves
  observed mode and retained counters, changes the epoch, and resets baselines
  together. A re-added actor does not replay traffic from the gap.
- A failed adapter update closes capture even if the backend rejected input
  shape without mutating. The worker continues sampling other metrics and
  reports network absent. Another update does not authorize a new session.
  Rearming requires explicit setup.

## Results and in-memory history

SystemSnapshot.linux_process_network retains observed coverage and all
observed_processes associations, including actors with zero buckets.
processes contains BUCKETS, not a fabricated zero-valued process row.
A new bucket has no rate at first; a second clean observation can give a
zero or positive rate. Initial UID sums are partial, not totals or local
accounts. completed_updates is actor-wide and repeats by bucket; do not
sum it. See the [public contract](LINUX_PROCESS_NETWORK_OBSERVED.en.md).

Sampler stores these samples IN MEMORY with epochs, brackets, and metadata.
Selection updates do not rewrite old samples. Do not calculate deltas across
epochs or from an absent row; check Metric and nominal_interval_ns. This
revision adds no durable history format or general app/user join; H04/J04
remain open. Legacy ProcessInfo/UserSession/AppHistoryEntry fields do not
receive invented totals.

## Verification

linux-process-network-monitor tests declared and observed modes, consent,
invalid enums, active setup and strict default after stop, missing start_mode
callback, four setup failures by mode, five update failures including
observed mode, denied/recovered/throwing reads, opt-out without rearming,
and preservation of empty metadata. Sampler tests verify a disabled flag
and setup while running are rejected before touching descriptors. The
installed consumer links both observed-mode overloads and tests denial
without capture.

Manual `--authorized-private-observed-monitor` uses the Monitor/SystemSnapshot
API with two private children, closes original pidfds, moves an active
capture, and verifies empty metadata and late arrival without a rate.
Two concurrent threads per actor produce A TCP 12,288/12,288, UDP 768/768,
and 24 peeks, with twice that for B. Checks include four rates and UID sum
against an integer oracle, parent exclusion, fork rejection, update consent,
removal/re-addition and gap handling, epochs and resets, group rejection
twice after exit, recovery after removal, and an empty stop/restart.
Default setup returns to strict after stop. Opt-out closes FDs and the flag
alone does not rearm. No FD leaks.

Manual `--authorized-private-observed-worker` tests observed setup with a
real worker, two selected actors with input cookie zero, initially empty
metadata, and later buckets. It checks four deltas and rates and UID sums in
history with common nominal intervals, first-bucket absence of rates, and
correct known/missing coverage. It exercises AB→B→AB with the worker running,
concurrent updates and history reads, retained counter origin, no gap replay
for the new actor, and group-wide epochs and reset. Denied update does not
change capture; failed update closes capture while the worker continues.
Another update does not rearm, default setup is strict after stop, and
restart is unarmed. FDs are cleaned up and both children exit.

Only ONE real namespace was shared. Children initialize SO_NETNS_COOKIE
for an independent oracle; the producer receives no input cookies. This
does not test multiple real namespaces, global capture, or 16 real actors.
Parent/child watchdogs are 60 s/50 s, with frame waits bounded to 12 s;
there is no hard BPF deadline. Probes are limited to authorized private
actors. No permanent configuration, new package, CLI, or global capture.

The native declared-bucket Monitor regression passed. Linux Release 126/126
without skips, three tests repeated 20 times each, an installed consumer
rebuilt, linked, and exiting 0, and no build warnings. MSVC passed 3/3
focused shared-code tests with no new C++ warnings; expected D9025 keeps
assertions active. No new full Windows suite or consumer run. The revision
75 general consumer failure remains N03; full compatibility is not
established. No new layout beyond revision 83, GUI, persistence, or dependency.

Logs: work/network-observed-monitor-*. Next at this checkpoint: general
process selection and coverage policy with explicit identity, removals,
and bounds, keeping tests private and bounded and requesting authorization
if capture needs expansion. Do not close F03 on the success of a manual
selection; H04/J04 remain pending.
