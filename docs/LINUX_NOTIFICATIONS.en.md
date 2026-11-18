# Observed Linux notifications: H05

This optional native provider captures traffic only after the caller enables
it. LinuxNotificationObserver connects directly to D-Bus through the system's
libsystemd, if available. It does not run dbus-monitor, gdbus, dbus-send, shells,
package managers, or helper apps. It installs and downloads nothing.

## Usage and consent

```cpp
#include <pcinsight/linux_notifications.hpp>

pcinsight::LinuxNotificationObserver notifications; // Does not connect or capture.
pcinsight::LinuxNotificationOptions options;
options.consent_to_monitor_notification_traffic = true; // User consent.
auto started = notifications.start(options);
if (started) {
    auto observed = notifications.poll(); // Call regularly from the same thread.
    if (observed.value) {
        for (const auto& row : observed.value->apps) {
            // accepted_notify_calls: accepted requests, NOT displayed/read notices.
            // declared_desktop_entry: unauthenticated hint; may be absent.
        }
    }
    notifications.stop(); // Closes ONLY our connection, not the service.
}
```

The GUI must obtain informed consent before setting the flag. Monitoring
notification traffic can reveal app activity, and the connection temporarily
receives messages that contain content. PCInsight does not retain or print
app_name, title, body, icon, actions, or other hints. It retains only the
connection identity, the desktop-entry hint when usable, counters, and times.
The raw D-Bus message exists temporarily in transport-library buffers; this
does NOT promise cryptographic erasure from RAM or concealment from debuggers.
There are no disk writes or automatic persistence.

Without the flag, start returns invalid_argument before opening the bus.
Monitor, Sampler, constructors, the test consumer, and history imports do not
implicitly enable it. Installing the library does not install a capture service.

## What is counted and how it is identified

The provider observes Notify calls in the org.freedesktop.Notifications
protocol at its standard path, addressed to the well-known service or its
pinned unique owner. Each call that expects a reply is held temporarily by
(unique sender, serial). Only a successful reply from the pinned owner to that
same sender with a matching reply_serial increments accepted_notify_calls.
A duplicate, unrelated, pre-period, or unmatched reply does not increment it.
A service error reply increments rejected_calls, not the accepted count.

accepted_notify_calls includes updates and replacements. replacement_calls
is the subset with a nonzero replaces_id and a validated returned identity.
It does not count distinct notification windows, displayed notifications,
user actions, or read messages. Do Not Disturb, grouping, and visual
suppression cannot be inferred from a reply. The GUI should label this
metric as accepted requests and preserve the scope note, not present it as
a universal notification total.

NO_REPLY_EXPECTED is counted separately as unconfirmed_no_reply_calls, without
waiting for a reply or assuming acceptance or rejection. Pending and expired
unanswered calls have separate counters; they are never converted into zero
accepted calls. An unanswered call expires after 30 seconds of monotonic
time observed by the caller.

Rows group by (unique sender connection, declared desktop-entry hint).
The hint can be missing, incorrect, or impersonate another app. The provider
does not verify it by reading files or turn it into a PID/executable/UID/username.
It does not use app_name as identity. Without a usable hint, the row keeps its
per-connection count and declared_desktop_entry remains absent: attribution to
an app is unknown. Two connections with the same hint remain separate; a broker
or portal can represent multiple apps and one app can use multiple connections.
Do not sum by name as if it proved causal origin. Identities apply only within
this bus and period, not across restarts or different buses.

## Periods and availability

The result is Metric<LinuxNotificationReport>, estimated and limited to
observed calls with a valid correlation. A known empty list means no
acceptances were observed; it does NOT prove zero notifications for every app.
period_start/period_end are wall-clock labels; observed_elapsed is monotonic.
Capture starts while requesting monitor enablement, not at OS boot. Setup can
have gaps. There is no retroactive history or guarantee of complete bus delivery.

poll processes up to the configured 1–4,096 messages (default 256) without
explicitly waiting. There is no hidden thread or indefinite wait.
poll_budget_reached says the budget was reached, not that loss occurred.
The caller must call again and choose a cadence suited to the workload; the
broker can impose limits and disconnect a slow reader. report does not drain
the bus; it returns the last observation.

stop keeps the finite report of observations already made, with running=false.
Its endpoint is the last poll, not a promise that pending messages were drained.
A new valid capture starts a new period and resets counters. Do not retry or
concatenate periods as one cumulative count without preserving identity and
coverage. Connection, protocol, limit, or overflow errors or a change in owner
invalidate the current period and return absence, not stale data or zero.
Earlier reports copied by the GUI remain limited historical observations.
There is no automatic reconnect.

This API does NOT populate the legacy AppHistoryEntry.notifications integer or
merge desktop-entry identity with per-executable CPU history. The GUI can
present it as a separate history source with its own period and availability.
HistoryStore does not serialize it. H04/network and M08/Windows remain separate.

## Bus, permissions, and unavailable sources

By default, only /run/user/&lt;geteuid&gt;/bus is used. The caller can
explicitly choose another local filesystem Unix socket via bus_socket_path.
Paths are bounded and character-restricted; the field is not a D-Bus address.
There is no DBUS_SESSION_BUS_ADDRESS, autolaunch, tcp, unixexec, abstract
address, endpoint list, or fallback to the system bus. SO_PEERCRED must
indicate the same effective UID on the control and monitor connections.
This does not authenticate software running under that UID and is not a root helper.

The provider queries GetNameOwner without starting the service and pins its
unique name. It verifies the owner again after BecomeMonitor and watches
NameOwnerChanged from the bus. It requests only Notify, replies/errors from
the owner, and changes to its name, not an empty rule that captures everything.
It sends no notifications and closes none. After switching to monitor mode,
it makes no queries on that connection; the separate temporary control
connection is closed.

BecomeMonitor requires broker permission. A denial reports
permission_required/permission_denied without elevating privileges, relaxing
policy, enabling legacy eavesdropping, or capturing covertly. A missing service,
bus, or optional API results in unsupported. libsystemd is an optional protocol
client; it does not require systemd as init. Without a compatible library or
a supported service/broker, the source stays unavailable.

Only the standard FDO protocol is covered. GTK/GNotification interfaces, portal
calls before translation to the service, private KDE/vendor interfaces, browsers,
earlier buffers, other users/namespaces/buses, and notifications that never pass
through this endpoint do NOT become zero or imply complete coverage. Support is
not claimed for every compositor, distribution, or application.

Limits: 1,024 pending calls, 2,048 rows, desktop hints up to 4,096 B, and
256 hints per call. D-Bus serials must be valid and uint64 counters reject
overflow. Unique-name predicates cover the numeric format of the brokers
tested. Setup uses 2-second call timeouts, not an overall syscall deadline.
The provider's limits do not precisely bound RSS, buffers, or decoding in
libsystemd or the broker. The class is confined to one thread; the host
coordinates its use.

## Tests and compatibility

tests/linux_notifications.cpp covers consent/moved-from/stop, periods,
sender/serial identity, replies, replacements, rejection, unknown apps,
expiration, clocks, and bounds/overflow with Release assertions enabled.
tests/linux_notifications_native.cpp connects the public API to a simulated
notification server over a REAL private D-Bus broker as a normal user. It
checks 3 accepted calls, 1 replacement, 1 rejected call, 1 unknown app, and
1 no-reply call, plus stop/new period, owner changes, and a real BecomeMonitor denial.

Only the harness uses an already installed dbus-daemon as a private server,
without a shell and with config/socket under mkdtemp. It does not query data
via CLI or change the session/system bus. It cleans up and reaps its children
and removes its files. The native target is included in CTest when that test
binary exists; absence does not count as a pass. Production never launches it.

In the first native test, the fixture client had not completed Hello. In the
second, sd_bus_send with a null cookie marked NO_REPLY_EXPECTED. Both logs
were retained, the harness was corrected, and explicit production coverage for
no-reply requests was added. Assertions were not weakened, and the failures
were not attributed to N03 CPU/history. Logs: work/notifications-focused-linux-{first,handshake}.log.

The new linux_notifications.hpp API and pimpl do not change existing layouts,
the virtual ABI, SystemSnapshot, or history formats. Consumers must link the
new library to use the symbols. Windows runs portable fixtures and native-provider
rejection tests, not real Linux capture.

Verified on 2026-10-19: Linux 106/106 and Windows 118/118 Release passed;
10/10 native fixture repetitions and the final direct run exited 0 without
skips. Final builds had no warnings. Packages were reinstalled, and external
consumers on both platforms were rebuilt, relinked, and ran to exit 0 without
enabling user capture. Logs: work/notifications-{build,suite,install,consumer}-{linux,windows}.log,
work/notifications-repeat-linux.log, and work/notifications-native-linux.log.
H05 moves from P to C: an opt-in observer with documented attribution,
privacy, periods, and a positive native result, not just fields or unsupported
results. H04/M08/N03 remain open at this checkpoint.

Primary references: [Desktop Notifications protocol](https://specifications.freedesktop.org/notification/latest/protocol.html),
[D-Bus BecomeMonitor](https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-become-monitor),
[sd-bus sending and expected replies](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_send.html).
