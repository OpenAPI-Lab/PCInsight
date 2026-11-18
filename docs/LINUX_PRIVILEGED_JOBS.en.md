# L05 — bounded asynchronous job tracking

Revision 154, 2026-11-02. Internal unit complete, with no real services used.
L05 remains E pending receiver limits, deployment/process/loader hardening,
and acceptance with real polkit. Execution stays OFF by default. No helper
is installed, registered, or activated.

## Outcome separate from dispatch

PrivilegedExecutionResult retains job_queued as confirmation that a job receipt
was received and adds job, a shared PrivilegedJobObservation data object.
snapshot() is thread-safe and does NOT use sd-bus, execute actions, or poll
services. It only reports what the connection owner's event loop observed.

States are pending, completed (JobRemoved done), failed (canceled/timeout/failed/
dependency/skipped), and unknown. succeeded is true for done, false for those
known unsuccessful results, and unknown when ambiguous. The raw result, whether
recognized or from a future version, is retained separately. A future result
is not interpreted as failure or success. A completed job does NOT mean the
service is currently running. Canceled or timeout does NOT prove there were
no effects or that the operation was effectively canceled.

## Subscription and correlation

Before dispatch, the connection installs a JobRemoved match for the verified
UNIQUE OWNER, exact path, and interface. It calls Subscribe on that same owner
if the connection is not already subscribed. Subscribe uses autostart=0 and
interactive message flag=0, and verifies reply type, sender, empty signature,
and matching cookies. If it fails, Start/Stop/Restart is not sent. One
successful subscription is reused per connection and owner. When the owner
changes, old pending observations are dropped and the connection subscribes
to the new owner. No default bus is used. Closing the connection removes the
client subscription, with no Unsubscribe RPC in destructors. Job information
is not a grant.

The callback checks bus, sender, message type 4, signature uoss, complete body,
and limits. It requires a canonical nonzero uint32 job path and matching
expected ID, path, and unit. The executor receipt now requires that same
canonical path, with no zero, leading zeros, or overflow. Up to 64 signals
are retained before the receipt is selected. Afterward, other paths are ignored
without penalizing a busy system. Up to 4,096 events are accepted for the
observed path. Identical duplicates are idempotent; conflicts or a different
unit on the same path yield unknown. Unresolved aliases can also yield unknown:
equivalence with systemd's primary name is not assumed.

The observation is prepared before sending. After Subscribe, the code
revalidates Manager, caller, policy, connection, and deadline before the
last checkpoint. It does not wait for jobs by calling sd_bus_process inside
the handler. The host must continue processing the bus serially after returning
the receipt. Without an event loop there are no new confirmations.

## Time, memory, and ownership

The mechanism configures a monotonic observation timeout of 1 ms–30 s,
defaulting to 30 s. It is not an aggregate authorization, handshake, or I/O
deadline. Expiration yields unknown without CancelJob, rollback, or retry.
Late signals do not revive an unknown observation. If the timeout expires
before dispatch, the operation is not sent. snapshot checks expiry without
creating a background thread or timer.

Native slots and callbacks are owned by the connection, whose use and closure
are serialized, NOT by the data object returned to the caller. Dropping or
copying the result from another thread does not destroy a slot or touch the
bus. Each connection allows up to 64 pending watches. Completed or expired
watches are removed when preparing another operation. The 64-watch limit is
implemented but was not stress-tested against the broker in this unit. The
64-entry early buffer does have boundary tests.

The owner's destructors remove matches before closing the bus and library.
Detected closure or revocation turns pending observations unknown without
canceling the operation. Confirmed historical completions remain in data
copies. There is no reference cycle with the connection. The signal callback
catches errors and does not let exceptions cross the C ABI. State reads and
events use a mutex. This internal object is a trusted C++ contract, not a
boundary against malicious code in the same process or an installed SDK API.

## Revision 154 evidence

tests/linux_privileged_jobs.cpp covers signals before receipt, done, five
negative results, future results, duplicates and conflicts, 5,000 unrelated
jobs ignored, incorrect units, ID/path/uint32 range/leading zeros/overflow,
deadlines and late events, the 64/65 buffer boundary, closure while pending,
and preservation of historical results.

tests/linux_privileged_receiver.cpp uses a REAL private broker and libsystemd,
with SIMULATED Authority and Manager. It now runs 50 receiver requests, 29
Authority checks, 16 modeled control attempts, and two Subscribe calls: one
rejected and one successful and reused. The Manager sends JobRemoved before
the receipt and uses different IDs per operation. Cases cover failed Subscribe
without dispatch, done/failed, an unexpected alias, a future result, duplicates,
contradiction, wrong signature, missing signal/timeout, and a forged client
signal that confirms nothing. Revoking the anchor turns the last pending
observation unknown without erasing a historical completed result. FD and
cleanup checks pass. No real service, host bus, or host polkit was controlled.

Linux 182/182 plus 120 L05 runs (12 tests repeated ten times, including ten
broker runs); 20 portable MSVC state-machine/argument/fallback runs; and two
reinstalled and relinked SDK consumers exiting 0. Initial, full, verified,
and final builds were clean, and the final suite passed 182 tests. Logs:
work/privileged-jobs-*. No public API/layout/format changes, Monitor/SystemActions
changes, or remote CI.

Update 155 adds a [shared budget](LINUX_PRIVILEGED_BUDGET.en.md) for the handler's
RPC phases. It is cooperative, NOT an absolute process or transport deadline.
The revision 154 job-observation timeout remains independent.

Historical revision 154 plan: receiver limits and an aggregate deadline.
Deployment, loader, policy, and real polkit consent remain pending. Do not
install, activate, elevate, or touch real services without specific authorization.
OpenRC revision 148 stays limited to explicit control; SysV/K03/I07/UAC are
unchanged. C60/E10/P3/B2 plus three excluded X: 80% closed, 20% open.

Primary sources:
[Manager and JobRemoved](https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.systemd1.html)
and [sd-bus matches](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_match_signal.html).
The states, limits, ownership, and conservative policy are PCInsight contracts.
