# Confirmed systemd service control

K04 update: [Linux user services](LINUX_USER_SERVICES.en.md) adds explicit control
for the effective user with a verified unique owner, reusing this page's job
broker. The generic API retains system scope. The user-manager exclusions
at the end describe the historical K02 evidence.

Completed and verified on 2026-10-14: start, stop, and restart through
`SystemActions::control_service`, waiting for the systemd job result.
This complements the [live systemd inventory](LINUX_SYSTEMD.en.md).

Previously, the implementation returned success when StartUnit/StopUnit/
RestartUnit was accepted. The API now registers for JobRemoved before
sending the request, calls Subscribe, and waits for completion of the
**job path returned for that request**. A result that arrives during the
synchronous call is retained until that job path is known. Jobs from
other operations are not confused with it, and the reported name need
not equal an alias used to start the service.

`done` means the job succeeded. Failed, canceled, timeout, dependency,
skipped, and unknown or absent states are not success. A oneshot service can
finish successfully without remaining running. A Type=simple service can
return job done before it is ready to handle requests. The result promises
neither application readiness nor a permanent state.

## Input, permissions, and safety

- Canonical `.service` name, up to 255 ASCII bytes, including instances and
  `\xHH` escapes. Empty input, NUL, slashes, control characters, malformed
  escapes, and invalid enum values are rejected before opening the bus.
  File paths are not accepted.
- Optional libsystemd is loaded at runtime; no SDK, systemctl, or shell is
  required. If systemd is not the active manager or the API is missing,
  the result is not_supported.
- The caller's privileges are unchanged, and no services are created to
  execute an action. Authorization depends on D-Bus/systemd and their
  policies. AccessDenied and InteractiveAuthorizationRequired are reported
  as permission_denied.
- NoSuchUnit is reported as not_found. Configuration and execution failures
  do not become “does not exist” or success. Messages contain the operation
  and D-Bus code or job result, not constructed commands.
- The systemd `replace` mode is used as before and can replace a previous job
  for the unit. Dependencies and propagation follow systemd policy. A failed
  restart may have stopped the service; rollback is not promised.

## Waiting and resources

Processing has a 30-second deadline, a 5-second D-Bus timeout per call, and
waits of up to one second per iteration. Opening and authenticating the bus
depend on libsystemd. There is no strict overall 30-second guarantee.

If the bus is lost, the job identity is missing, a limit is reached, or
confirmation does not arrive in time, the API returns an error with an
**unknown outcome**. It does not cancel the accepted job, which can finish
later. The GUI should not blindly retry or conclude there were no effects.

The bus, slot, messages, error, and library are owned through RAII. The
dedicated connection closes at the end, and systemd removes the subscription
on disconnect. The C callback does not let exceptions escape. Limits are
1,024 early results, fields up to 1,024 bytes, and 100,000 processed messages.
Inconsistent signals do not establish success.

## Completion criteria and evidence

Release: Linux 12/12 and Windows 24/24. Fixtures cover names/enums, done/error
results, job correlation, early/late signals, aliases, bounds, and contradictory
signals. Windows tests only these portable components. Packages were
installed and an external CMake consumer was rebuilt and successfully run on
Linux and Windows after the final changes.

**Native** Ubuntu WSL integration explicitly ran the fixture as root and
was not part of normal CTest. Two uniquely named transient PCInsight
services run only the test binary itself, with no shell or utilities:

- Waiting service: start, stop, idempotent stop, start, restart, and stop.
  The inventory confirms running/inactive and a new PID after restart.
- Oneshot returning 7: job failed is reported as actual failure, not
  successful acceptance. The inventory confirms failed.
- Both are stopped and removed. The fixture resets **only its own** failed
  state before releasing references to allow garbage collection. That
  cleanup ResetFailedUnit was not added to the product API and does not
  clear user diagnostics.

An initial cleanup test showed that systemd retained the failed fixture.
Cleanup was corrected and the full cycle successfully repeated. The
earlier leftover unit was already absent when checked. No changes to
preexisting services were requested and no permanent configuration files
were written.

This historical closure does not implement OpenRC/runit/s6 control, user
manager support, a polkit helper, job cancellation, or GUI authorization.
**Systemd** inventory and control are implemented and verified; the
cross-platform section still has pending items.

Primary references: [systemd D-Bus: jobs, Subscribe, and JobRemoved](https://wiki.freedesktop.org/www/Software/systemd/dbus/),
[systemd job waiting](https://github.com/systemd/systemd/blob/main/src/shared/bus-wait-for-jobs.c),
[sd-bus declarations](https://github.com/systemd/systemd/blob/main/src/systemd/sd-bus.h).
