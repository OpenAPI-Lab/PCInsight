# L05 — shared RPC budget and aggregate deadline

Revision 155, 2026-11-02. Internal unit complete. This is a COOPERATIVE
deadline with bounded RPCs, not preemption, real-time scheduling, or remote
cancellation. L05 remains E. No helper was installed or activated, and
there are no public actions or real services involved.

## One budget per request

PrivilegedRequestBudget uses steady_clock, a counter, and a mutex.
Only the mechanism configures it: a timeout of 1 ms–30 s and 1–128
instrumented RPC operations, defaulting to 5 s and 96 operations. Invalid
configuration, an unavailable or backwards clock, or exhaustion rejects
new calls. It never returns timeout zero, which sd-bus could interpret as
the default timeout. Each claim deducts one operation and returns the
minimum of the per-call cap, 5 s, and time remaining, without extending
the deadline. The count is diagnostic, NOT a grant, ticket, or reusable
permission.

ControlSystemService snapshots the configuration and shares the SAME
budget across daemon/identity queries, CheckAuthorization, Manager queries,
AddMatch, Subscribe, and dispatch. Changing the gate or configuration in
flight invalidates the flow. AuthorizeSystemService through the owned
connection also uses a per-request budget of 5 s and 96 operations.
Internal legacy/raw adapters without an explicit budget retain their
previous contract and are not presented as hardened receivers.

The budget starts inside the handler, not when accepting the connection
or receiving the first byte. It includes elapsed time between checkpoints,
including local work. Queries proceed only if time remains. This is not
a deadline for the handshake, initial configuration load, or transport
before the handler.

## Applying the budget to the native ABI

Queries that use sd_bus_call_method and the internal AddMatch temporarily
get a default timeout no greater than the time remaining through an RAII
lease. The original timeout is restored on normal exit and on exceptions.
Connection use is always serialized. Missing setter/getter functions do
not cause fallback to an unlimited timeout. Explicit sd_bus_call calls
receive the time remaining directly. There are no retries.

The quota counts instrumented RPC points, conservatively including match
registration as one operation. It does NOT count every possible internal
libsystemd message, receiver replies, RemoveMatch cleanup, or asynchronous
job traffic. It is not a global or per-UID rate limit or a process-wide
queue, byte, or CPU limit.

If the budget is exhausted before dispatch, there is no send attempt. If
it expires after the attempt, the outcome is dispatch_unknown even if the
Manager later replies positively. The operation MAY have run. There is no
rollback, CancelJob, or guaranteed cancellation of a polkit agent. A new
request gets a NEW budget rather than inheriting an exhausted one or reusing
earlier authorization. The [revision 154 job-observation deadline](LINUX_PRIVILEGED_JOBS.en.md)
is separate and can continue after the handler returns.

## Limits of the guarantee

The budget cannot interrupt dlopen or constructors, blocked filesystem or
procfs operations, C++ code, or kernel scheduling. A pause after timeout
calculation can delay invocation. Later checks preserve uncertainty; they
do not undo dispatch. Cleanup, slot removal, and process configuration are
not timed transactions. This therefore claims neither an absolute wall-time
limit for the entire receiver nor an atomic revocation boundary. Rate
limits and admission, queue and size limits before decoding, and deployment,
process, and loader hardening remain pending at this checkpoint.

## Revision 155 evidence

tests/linux_privileged_budget.cpp uses a controlled clock to check decreasing
remaining time, per-call caps, operation counts and the 128-operation limit,
invalid bounds, expiration, clock reversal and exceptions, restoration
after success or exceptions, and setter failure. It does not change the
system clock.

tests/linux_privileged_receiver.cpp retains revision 154 cases and adds six
requests: invalid zero timeout and zero RPC quota, a one-RPC quota exhausted
before Authority, an Authority reply held with a 200 ms budget and no
dispatch, a Manager reply held after simulated acceptance resulting in
dispatch_unknown, and recovery with a fresh request. Held replies have a
one-second guard. The receiver must return BEFORE each fake is released;
discarding a late result is not enough. The test verifies that the native
timeout is restored to exactly 2 s on the owning thread after EVERY request.
The Manager counts exactly one attempt in the uncertain case, with no retry.

Each run has 56 requests, 32 Authority checks, 18 Manager calls, and two
Subscribe calls. The broker and libsystemd are REAL and PRIVATE; Authority
and Manager are SIMULATED, and no real service is involved. Linux 183/183
plus 130 L05 runs (13 tests repeated ten times, including ten broker runs);
20 MSVC budget/argument/fallback runs; and two reinstalled and relinked SDK
consumers exiting 0. Private FD and cleanup checks passed. An initial
clock-skew warning was retained. The final incremental build and 183-test
suite were clean without changing clocks or mtimes. Logs:
work/privileged-budget-*. No new public API, layouts, formats, Monitor/SystemActions
changes, real polkit, installation, activation, elevation, acceptance, or
remote CI.

Update 156 implements [receiver/sender admission and queue-pressure checks](LINUX_PRIVILEGED_ADMISSION.en.md)
before RPCs. These are not byte limits or pre-receive limits. Revision 155
evidence above is historical. Deployment and transport before the handler
remain pending.

Historical revision 155 plan: admission and receiver transport/queue limits
before real deployment. Real polkit policy and consent and protected process
and loader setup remain pending. Do not install, activate, elevate, or
control real services without specific authorization. OpenRC revision 148
stays limited to explicit control; SysV/K03/I07/UAC are unchanged. Official
count: C60/E10/P3/B2 plus three excluded X; 60/75 = 80% closed and
15/75 = 20% open.

Primary sources: [sd-bus timeouts](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_set_method_call_timeout.html)
and [sd_bus_call](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_call.html).
The quota, injectable test clock, and conservative policy are PCInsight
contracts.
