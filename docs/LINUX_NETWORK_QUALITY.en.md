# Identity and quality of private counters (F03 in progress)

Revision 64 update: the [internal SELF session](LINUX_NETWORK_SESSION.en.md)
extracts start/read/detach/stop with tested ownership and rollback. The
following preserves historical revision 30 evidence; network namespace
handling, joins, and integration were still pending.

Revision 30 adds the manual --authorized-quality mode to the existing test.
It uses the explicit 2026-10-18 authorization: private WSL processes only,
temporary root, and no permanent configuration, pinning, or installed
tools. Neither CTest nor Monitor runs it. There are no new public APIs,
layouts, ABI changes, or history formats; it is not yet the production provider.

The unattached RAW_TRACEPOINT identity program runs locally through TEST_RUN.
It writes the kernel TGID, initial-domain real UID, and leader start_boottime
to its own three-entry map. Failed BTF or helper reads or a nonzero status
prevent accepting partial results. It does not translate PID/UID from
procfs or sessions.

The probe keeps the early TGID filter and checks group_leader.start_boottime
and kernel UID before reading context or socket data. The same TGID with a
different generation or UID gets no bytes. A rejected identity is recorded
as a quality failure, not known zero traffic. Threads of the same leader
share the correct token. Offsets come from BTF, not approximate start_ticks
conversions.

With identity configured, the private ARRAY has nine entries: 0–3 for
TCP/UDP payload, 4 for identity/read failures, 5 for excluded positive
receives, 6 for the sticky overflow flag, 7 for active writers, and 8 for
the sequence of completed updates. Each write enters slot 7, updates the
counter or flag and the sequence, and exits slot 7 at the end. It uses
atomic fetch-add/fetch-OR, not a tracing spin_lock. A lookup that fails
after entry leaves the active writer count above zero: it fails closed
rather than producing a valid metric. Each instance must use the exact
fresh private ARRAY9 schema.

Fetch-add detects wrapped uint64 addition and sets flag 6, including for
the sequence. **It detects overflow; it does not saturate the raw value.**
The flag is never cleared within the instance. The validator refuses to
publish the affected values even if the raw counter wrapped. Lowering or
clearing the flag live does not repair the data. The writer count is bounded
by actual concurrency and is not cumulative.

The manual reader checks writer/sequence, counters, then sequence/writer,
with seq_cst fences between phases. It accepts only when both writer counts
are zero, the sequences match, and there are no flags or quality failures.
It tries up to eight times under contention; if still busy it reports
absence, not zero. Exclusions for PEEK/TRUNC/ERRQUEUE remain explicit.
Validation was tested on WSL x86_64 and does not guarantee memory ordering
or coherence on every architecture or kernel.

Portable fixtures extend tests/linux_network_classified.cpp: identity
discovery, high uint32 UID, tokens/UID/nulls/read failures, writes always
under the guard, byte and sequence overflow, and sticky flags. The validator
rejects active writers, changed sequences, and incomplete quality.

Real run exited 0: PID 475/TGID 1640, start 103233269011 ns, initial UID 0.
The matrix exercised send/sendto/sendmsg over TCP/UDP IPv4/IPv6 and two
private threads of the same leader: TCP 6144/6144, UDP 384/384, 12 excluded
peeks, zero failures, 36 updates, and zero active writers. Ten coherent
reads and zero unavailable reads during the concurrent run do **not** show
that overlap rejection was exercised live; fixtures cover that case.
Child exclusion and teardown were checked. Incorrect start/UID tokens
were rejected without bytes. Byte and sequence overflow injected into
private maps yielded absence with balanced guards. No real UID or clock
was changed. Evidence: work/network-quality-native-linux.log and logs
with the same prefix.

Final Release regression: Linux 101/101 and Windows 115/115; both installed
consumers were relinked and exited 0. Final builds had no warnings. Logs:
network-quality-suite-{linux,windows}.log and network-quality-verified-build-*
under work/. No tests were skipped or relaxed to obtain the positive result.

At this checkpoint F03 remains E: work is still needed to extract a
producer with appropriate lifecycle and API, handle namespace cookies,
scopes and procfs-to-app/user joins, declare complete coverage, integrate
with the reducer and Monitor, and test more kernels and hardware. The
experimental map is not connected automatically; J04 and H04 are not closed.
Earlier N03 risks remain. The user asked to skip stages that take too long,
so this unit does not expand beyond its test and regression and is not
counted as a closed subtask.

Primary references: [Linux 6.6 identity helpers](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/bpf/helpers.c),
[atomic ISA](https://docs.kernel.org/bpf/standardization/instruction-set.html),
and [spin_lock restrictions](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/uapi/linux/bpf.h).
