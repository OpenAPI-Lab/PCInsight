# Reusable pidfd identity query, disarmed between calls

Revision 89 follow-up: the [INTERNAL shared producer](LINUX_NETWORK_SHARED_PRODUCER.en.md)
was validated with 17 private actors and three traffic links. Joins, windows,
rates, updates, and public integration were still pending. The identity query
described here remains in the existing provider; no checks are removed.

Revision 88, 2026-10-22. This internal F03 unit was completed and validated;
F03 as a whole remains IN PROGRESS at this checkpoint. No changes to public
API or layouts, GUI, persistence, packages, collection CLIs, or permanent
configuration. The 64/16/1,024 limits are unchanged.

## Reused resources, fresh identities

LinuxPidfdIdentityQuery retains two ARRAY maps and three loaded programs
(caller, thread, and fexit). The fexit program uses six map slots: the
five existing result slots and an exact caller_pid_tgid gate at slot 5.
A zero gate or a different thread or process exits BEFORE reading target
context. There is no task iterator or global enumeration. The link lasts for
the session but is NOT armed outside a synchronous pidfd_getfd(selected,-1,0)
query.

Each begin clears the caller map and reruns the caller and thread probes.
It validates TGID/UID/start/TID, clears ALL results, and arms only the
current thread. Switching threads needs no new load or attachment. The
native wrapper duplicates the pidfd, checks liveness, blocks and restores
signals, and makes the invalid-descriptor query without duplicating a file
from the target. ATTACH_REALCREDS and a permission result of zero are still
required. It checks syscall errors, probe quality, and liveness. Previous
permission and identity results are not reused.

Finish disarms BEFORE reading results. The same thread cannot run a second
call in parallel with its synchronous query; other threads do not match the
gate. Reset, arming, or read failure yields no partial result. If disarming
fails, the link and program are removed and the result rejected. Any residual
gate is explicitly cleared before attaching a replacement. Exceptions and
early exits cancel the transaction. Stop and destruction close all resources.
A forked copy is rejected and closes ITS references without writing to the
parent's shared maps, even if it inherited an armed state.

The join retains TWO independent kernel observations and all procfs/pidfd/
boot/context brackets. The sampler still joins before AND after reading
counters. Older injected sources retain join/join_layout; only the native
source opts into join_query using resources owned by the sampler. The earlier
standalone call retains its original temporary path.

Serialize calls in the same descriptor table. Do not manipulate owned FDs,
unshare the table, or use asynchronous cancellation. Syscalls have no
guaranteed hard deadline. Resources and links are not pinned and last only
for the authorized session. No capture or elevation starts automatically.

Direct BPF_MAP_UPDATE_ELEM uses an optional internal callback. Sources that
do not need this optimization do not require the callback. The
[Linux kernel documentation](https://docs.kernel.org/bpf/maps.html) describes
the map update operation.

## Tests and retained results

- linux-network-pidfd fixture: 40 transactions with three loads and one
  attachment, thread changes without new loads, fresh output identity,
  stale results cleared and absence without a trigger, ambiguous count
  rejected, startup/API/update/run/lookup/attach failures and exceptions,
  fork, and recovery after failed disarm. The bytecode interpreter verifies
  that a zero gate, another PID or TID, or a missing map prevents context
  reads. Mode, permission, and instrumented failure checks still reject
  invalid results.
- Intermediate prototype attaching and detaching PER QUERY: the native
  17-actor test FAILED (`work/network-query-reuse-native17-linux.log`,
  start at 21.4 s, first read unfinished before the children's 55 s
  lifetime expired). The result is retained and is NOT counted as a pass.
  Builds ran concurrently; this is not a benchmark or an isolated causal
  comparison.
- Final gated design: native 17-actor PASS
  (`work/network-query-gated-native17-linux.log`). From harness startup:
  prepared at 8 ms, start at 1,593 ms, empty sample at 1,620 ms, first batch
  at 1,647 ms, second at 1,676 ms, stop at 2,986 ms. The 27/27/29 ms
  differences INCLUDE IPC and private workload, not isolated syscall
  measurements or universal latency guarantees. The 55/60 s watchdogs
  were unchanged; no builds ran concurrently.
- The 17 private children using UDP payloads of 64–80 B passed four-rate
  checks with a common nominal interval, per-actor byte counts, partial UID
  sums, absence of initial rates, and parent-traffic exclusion. Capture
  retained its references after the owner released its own. All children
  exited normally, with no FD leaks. One real namespace was shared, with
  SO_NETNS_COOKIE as an independent oracle. The 64/1,024 cases remain fixtures.
- Native observed Sampler PASS with two children, switching between main,
  worker, and updater threads, concurrent AB→B→AB updates with history and
  epochs, retained counter origin, and no gap replay. Failed update closes
  capture without stopping the worker; restart does not rearm. No FD leaks.
  `work/network-query-gated-worker-linux.log`.
- Native strict PASS: four rates and UID sums, TCP 4,096 B and UDP 256 B
  per direction, eight excluded peeks, parent exclusion, fork rejection
  without affecting the parent, restart, stale PID/start and exited target
  rejection, and FD cleanup. A separate standalone test checks signal-mask
  restoration. `work/network-query-gated-strict-linux.log`.
- Linux Release 128/128 without skips, six focused tests repeated 20 times
  each (120 runs), and the installed external consumer rebuilt and exiting
  0. MSVC passed 6/6 focused tests. Final builds had no new C++ warnings;
  D9025 is expected to keep assertions active. Logs:
  `work/network-query-gated-*`. No full Windows suite. N03 remains open for
  the revision 75 consumer's 0xC0000409 failure.

## What remains incomplete

At revision 88 there is no shared BPF traffic producer, automatic admission
or removal from inventory, or general capture for all processes and apps.
There is still one capture per selected actor, with bounded registries,
estimated rates, and partial UID sums. There is no certification of 64 real
actors, multiple real namespaces, or support for all kernels, distros,
or privilege configurations. Next: a shared multi-actor producer and
explicit coverage and capacity before automatic admission. Tests remain
restricted to authorized private actors.

Official count: 57/75 = 76.0% closed; 18/75 = 24.0% open. This improvement
does not close F03/H04/J04. Revision 87 failures remain as historical evidence.
