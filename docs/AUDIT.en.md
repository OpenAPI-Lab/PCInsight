# Explicit action auditing

Revision 46 extension: the
[explicit Windows Startup-file action](WINDOWS_STARTUP_FILE_AUDIT.en.md)
is integrated into SystemActions/AuditedActions as its own operation.
Rebuild consumers and backends for the new 0.x virtual method. This does
not change Windows approval state.

Completed functional unit: `AuditedActions`, an opt-in SystemActions
decorator, records requests and results for current operations, including
explicit logout and later personal override/reset actions. It does not
run commands, elevate privileges, or automatically create files or threads.

2026-10-15 extension: the explicit Windows Run operation, separate from
generic startup, is integrated into SystemActions/AuditedActions. The
event omits the command, native bytes, and backend diagnostics. It retains
the ID, flag, and result according to the caller's privacy contract. See
[Run auditing](WINDOWS_RUN_AUDIT.en.md). Windows passed 76/76 and Linux passed
56/56; installed consumers exited 0. This does not validate a successful
native withdrawal or restoration; the provider's pending item 6801 remains.

```cpp
#include <pcinsight/audit.hpp>
#include <mutex>
#include <vector>

std::mutex audit_mutex;
std::vector<pcinsight::AuditEvent> events; // Example; production should bound and persist records.
pcinsight::AuditedActions actions(pcinsight::make_system_actions(),
    [&](const pcinsight::AuditEvent& event) {
        std::lock_guard lock(audit_mutex);
        events.push_back(event);
    }, "GUI local");
// Use actions instead of the unaudited backend. Construction does not change the OS.
```

The consumer supplies an `AuditSink` for each event. It can choose memory,
a file, a database, or a logging API. To retain an event, copy it during
the callback. For a persistent sink, returning without an exception must
mean that it met the consumer's required storage guarantees. PCInsight
does not verify that storage or provide a tamper-proof log.

## Verified contract

- Two stages, requested and completed, share an ID unique within the
  decorator. IDs are not globally unique across instances or reboots.
- Events include operation, date, monotonic execution duration, target/PID,
  expected Windows identity (or expected_linux_start_ticks for checked
  Linux termination), and force/enabled/elevated/service_action flags.
  actor_label is caller-supplied, **not an authenticated OS identity**.
- Events omit launch arguments, cwd, and backend or exception diagnostics.
  The target (executable, service, startup entry, or session) and label
  are caller data and may be sensitive. The consumer decides access,
  retention, and sanitization. Strings are bytes: UTF-8 is not validated,
  and newlines and NUL are not escaped. Use structured serialization
  rather than concatenating log strings.
- Target and label are limited to 4,096 bytes, with metadata_truncated.
  Truncation can split a UTF-8 sequence; the serializer must handle that.
  **Only the event is truncated**. The original full parameters reach
  the backend unchanged.
- Default require_request_record: if writing the requested event fails,
  the backend is not invoked and an explicit system_error is returned.
  The decorator also attempts to record the denial as completed with
  backend_invoked=false.
- Best_effort must be selected explicitly. Sink failure does not prevent
  execution.
- Failure to record completed does not make an already performed
  operation fail or promise rollback. `diagnostics()` reports failure
  counters and the latest failure type. The consumer must consult it
  and handle the loss.
- Backend exceptions are rethrown after an attempt to record completed
  with system_error and backend_threw=true. An exception does not prove
  that there were no effects. Do not automatically retry a destructive action.
- Exactly the requested method is invoked. Checked termination and
  efficiency preserve the creation token and never fall back to PID-only.

## Concurrency and limits

Backend access is serialized per instance. Sink callbacks run without
internal locks and may query diagnostics, but **must be thread-safe**.
Delivery order across concurrent IDs is not guaranteed. Correlate by ID
and stage, not arrival position. Avoid recursive callbacks that emit
actions indefinitely and backends that reenter the same decorator.
Do not destroy the instance while operations are in progress.

Callbacks are synchronous, with no timeout or cancellation. A slow sink
delays the operation. The decorator has no unbounded queue or event
history. IDs are not reused at the uint64 limit; allocation fails before
execution. Diagnostic counters saturate. Sink exceptions do not propagate.
Even if an internal failure prevents diagnostic updates, it does not
change the result of an operation already performed.

The decorator audits calls passing through it. It is not authorization,
a polkit/UAC helper, or observation of actions taken outside PCInsight.
It does not guarantee a completed event after a process crash or power loss.
A requested event can remain unfinished and must be treated as an unknown
outcome.

## Completion criteria and evidence

On 2026-10-14, a simulated backend verified eight actions, preservation of
parameters and identity, both stages, results and exceptions, redaction of
sensitive fields, event truncation without changing the action, fail-closed
and best-effort behavior, failure after execution, 200 concurrent operations
(400 events with no duplicate request IDs), diagnostics from the callback,
and invalid construction.

Linux Release passed 11/11 and Windows Release passed 23/23. These tests
changed no real process, service, or session. The external consumer of
the installed package uses the decorator only with PID=0 and token=0,
which is rejected without mutation. Installation, rebuild, and execution
of that consumer were verified on Linux and Windows with the new audit.hpp.

This closes the **action auditing API and its tests**. Tamper-proof persistent
storage, actor authentication, and privileged helpers are outside this
work and are not claimed to be implemented.
