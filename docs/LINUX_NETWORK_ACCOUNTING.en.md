# Internal socket activity accounting (F03 in progress)

Revision 66 update: the [SELF sampler](LINUX_NETWORK_WINDOW.en.md) connects
private capture to this reducer for one declared actor and cookie. Its private
scope distinguishes the namespace_inode and kernel_initial domains and
single-row SELF coverage from declared buckets. It does not establish global
enumeration or a procfs join. The general contract and requirements below remain
in force.

`src/linux_network_accounting.hpp/.cpp` reduces cumulative per-actor counters
to TCP/UDP rates and UID sums. This is portable code with no syscalls, loader,
capture, threads, or storage. It adds no public API, is not connected to Monitor
at this checkpoint, and does not populate network fields in ProcessInfo or
UserSession. **Fixtures do not demonstrate TCP/UDP capture in the kernel.**

## Input contract for the future producer

Four uint64 counters per bucket: TCP send/receive and UDP send/receive.
They are intended to represent payload bytes from successful socket operations
observed by the producer, not TCP bytes acknowledged, retransmissions, headers,
interface traffic, or exclusive socket ownership. Defining and validating
MSG_PEEK/MSG_TRUNC, uncovered operations, asynchronous I/O and io_uring, and
actual hook signatures remains the producer's responsibility. This module
does not equate a call with a physical transfer.

Bucket identity consists of TGID, the leader's start_boottime, real UID
**at operation time**, and the socket's network namespace cookie. Do not infer
the actor from the FD, inode, socket UID, or holder. Credential changes produce
different buckets; counters for a previous UID are not reassigned to the
current process owner. Output can contain multiple rows for one TGID. It is
not a list of current sessions and cannot be multiplied by the number of
sessions for the same user.

The scope includes boot, a unique producer-instance epoch, and explicit PID
and UID domains. A future producer must resolve those domains, verify their
correspondence with procfs and sessions, and change epoch if it restarts or
recreates maps. Do not convert start_boottime to start_ticks through this
reducer. None of these tokens is verified against the kernel here; they are
input preconditions.

A Support::available input certifies the producer contract: complete
enumeration, a nonzero monotonic timestamp, coherent rows, and a monotonic
lost-update counter. The producer must check concurrent read consistency and
loss counters before making that claim; a map walk without guarantees is
not enough. A row marked coherent=false or saturated=true can preserve bucket
coverage without supplying numbers. The producer's declared hooks must cover
all four lanes. A producer that lacks UDP or either direction cannot mark
the input available or fill the missing lane with zero. Enumeration and
coherence default to false; an empty structure does not certify a read.
Estimated support for the entire snapshot does not satisfy the contract
and is rejected. Revision 66 certifies only the single instrumented SELF
row, not the general multi-actor producer contract.

## Windows, losses, and aggregation

- The first sample exposes counters only; rates require two endpoints.
- Rates require the same scope and bucket, increasing time, and an interval
  of at most 60 s. This is a limit between samples, **not** a chart buffer or
  60-second history.
- A decrease in any counter invalidates all four bucket rates for that
  interval. Saturation or inconsistency prevents using the affected endpoints.
- A new loss invalidates all rates for the interval that crosses it. A
  subsequent interval without new loss can be measured. If any loss occurred
  since startup, accumulated totals remain estimated lower bounds.
- A disappeared bucket leaves no baseline; its return is a first sample.
- Absence, denied permission, duplicates, invalid scope/time, a backwards
  clock or loss counter, partial enumeration, or more than 4,096 rows clears
  all baselines.
- A scope change or an excessively long interval establishes new baselines.

Rates are floor(delta_bytes * 1e9 / interval_ns), with no floating-point
arithmetic or potentially overflowing uint64 multiplication. Portable binary
division stays exact even at UINT64_MAX. An out-of-range result is absent,
not silently saturated. A valid zero delta does yield a zero rate.

UID sums include only buckets with known rates and are always estimated and
partial, with known/missing/overflow for each of the four lanes. Absence is
not added as zero. Sum overflow invalidates that Metric while preserving
coverage counts. No UID with a zero rate is fabricated when no bucket has a
measured rate. A complete empty enumeration yields an empty list, not zero
global traffic. Do not mix or add these payloads with TCP_INFO acknowledged
or received bytes or interface counters.

The reducer is not thread-safe. Each input is capped at 4,096 rows, and
state is bounded to the previous sample with no global cache. Ordering is
deterministic by key and UID. This does not validate a BPF blob or map or
impose a hard time limit on the producer.

## Verification and remaining work

Fixtures cover all four TCP/UDP lanes, a 2 s interval, known zero, identity/
UID/boot/epoch/namespaces, loss and regression, saturation, inconsistency,
disappearance, empty or partial enumeration, duplicates, absence/permission/
contradictory support, bounds, rate and sum overflow, and uint64 boundaries.
A deterministic portable oracle covers 10,000 arithmetic cases in which
reference multiplication does not overflow.

Final Release results: Linux 99/99 and Windows 113/113; both installed
consumers were relinked and exited 0. Final builds had no warnings. The
first sandboxed Windows run passed 98/113 with 15 failures involving
registry/service/temporary-file access and PDH. Outside isolation it passed
113/113 without changing code for those tests. The failed log
`work/network-accounting-suite-windows.log` is retained alongside
`network-accounting-verified-suite-windows.log` and the final
`network-accounting-final-suite-windows.log`. Initial and final Linux runs
passed 99/99 in `network-accounting-suite-linux.log` and
`network-accounting-verified-suite-linux.log`. Build, install, and consumer
logs share the same prefix. These passing suites do not establish that
historical N03 risks were fixed.

At this checkpoint F03 remains E. Work still includes a loader, bytecode,
safe attachment, a real accounting producer, verifiable filters and scope,
compatibility and privileges, coverage and positive TCP/UDP tests, public
integration and windows, and identity joins with users and apps. J04/H04
are not closed. This unit loads no probes and changes no privileges. On
2026-10-18 the user explicitly authorized a future temporary WSL test,
filtered to private processes and removed afterward, with the necessary
privileges but without permanent configuration or tool installation.
This module adds no IDs to the tracker to inflate progress.
