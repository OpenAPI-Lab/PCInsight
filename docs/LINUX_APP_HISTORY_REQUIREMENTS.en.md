# N03: Linux app history for the GUI

**Language:** [Español](LINUX_APP_HISTORY_REQUIREMENTS.md) · English

Revision 162, 2026-11-06. Maps R14 to H01/H02/H04/H05 and the L01 action.
CPU, network, and notifications are three independent reports. A matching
label, path, or date does not prove that they represent the same app or period.
The library does not invent counters to fill every column in a row.

| Requested data | API and unit | Scope and verification |
| --- | --- | --- |
| Name | `LinuxAppHistoryEntry.id/name` | CPU and network group by the exact observed executable path. `name` is a label, not a package or publisher. Replacing an executable at the same path may interrupt attribution without creating another row. [CPU](LINUX_APP_HISTORY.en.md), [network](LINUX_NETWORK_APP_HISTORY.en.md). |
| CPU time | `snapshot.linux_app_history` → `observed_cpu_time`, cumulative ns | Covers the valid interval for visible processes since observation began. It is not a percentage or a 60-second chart window. Short-lived, invisible, or earlier processes may not count. [Test](../tests/linux_app_history.cpp). |
| Network | `snapshot.linux_network_app_history` → `network_bytes`, cumulative B | Instrumented TCP/UDP payload for selected, attributable processes. Not interface throughput, on-wire bytes, or total app traffic. CPU and notifications are absent. [Test](../tests/linux_network_app_history.cpp). |
| Notifications | `LinuxNotificationObserver::poll/report` → `apps[].accepted_notify_calls`, calls | Successful Notify replies observed and correlated with the pinned server. Replacements are included; `replacement_calls` is a subset, so do not add it. This is not a count of displayed, distinct, or read notifications. [Contract](LINUX_NOTIFICATIONS.en.md), [private test](../tests/linux_notifications_native.cpp). |
| Notification identity | `sender_unique_name` and `declared_desktop_entry` | The D-Bus connection within that observation and an unauthenticated sender hint, which may be absent. Never join with CPU or network by hint, app text, or a connection name from another session. [Correlation test](../tests/linux_notifications.cpp). |
| Run new task | `SystemActions::launch_process` | An explicit user action, not a collector or capture authorization. Launching does not automatically create an attributable history record. [Contract](LINUX_LAUNCH.en.md). |

## Reading and presentation

- **CPU:** Keep one `Monitor` and enable `include_app_history`. Turning off
  `include_processes` does not prevent internal process reads. Turning off
  history clears its accumulation; do not use that flag as a harmless pause.
  The first observation may not yield a known CPU value. Two valid
  observations with a zero delta can produce one.
- **Network:** Requires both `include_app_history` and
  `include_linux_process_network` plus an already authorized F03 capture in
  observed mode (manual selection or visible-process admission). Flags do
  not attach BPF. Strict/declared mode does not feed this report. Do not
  request a sample with the network flag disabled on a shared Monitor if
  you intend to keep its capture active.
- **Notifications:** Use a separate observer and informed consent.
  Construction does not connect. An explicit `start` may require local
  libsystemd and bus permissions. Call `poll` from the same thread.
  Present pending, expired, rejected, unacknowledged calls and polling
  limits as diagnostics, not as accepted calls. Do not log titles, bodies,
  or other content.
- Check `metric_is_known` on both the outer report and each counter.
  Absence is not zero; a known empty list is not failure; an inconsistent
  or denied report does not permit display of its rows.
  `app_history_view(snapshot)` selects the native CPU report. For Linux
  network history, pass its Metric explicitly. `AppHistoryOrigin::linux_observed`
  alone **does not distinguish CPU from network**; keep the source with the
  view. [API](../include/pcinsight/app_history.hpp), [tests](../tests/app_history_api.cpp).

Keep separate GUI sections per source. Show `period_start` and `period_end`
for each period. Do not automatically reconcile clocks, coverage, or identity.
If the CPU report is missing, `note` can distinguish a clock going backward
from an invalid timestamp or too many observations. Show it as a diagnostic,
not a stable error code, and do not interpret the absent report as zero.
An observed period does not reconstruct earlier activity or equal a lifetime
total. Example indices are output positions, not stable keys.

## Explicit saving, not capture

`LinuxHistoryStore` saves observed rows with value, support, and note.
It does not save the outer report's status, permissions, baselines, or
capture resources. CPU and network require separate files. The merge key is
`(id, period_start)` normalized to milliseconds. Repeating the same period
replaces it rather than adding to it. A valid update is rejected if it
decreases a known counter or makes it unknown. Do not call `save` after a
load failure or replace the file with an empty list derived from an absent
report. Serialize load, merge, and save, and retain whole periods.
`LinuxNotificationReport` is not persisted.
[Format and limits](LINUX_HISTORY_STORE.en.md), [tests](../tests/linux_history_store.cpp).

Legacy `HistoryStore` preserves raw data. `import_legacy_history` keeps only
identities and periods as evidence for presentation: **all** counters remain
unknown even if the raw value was positive. Do not promote those numbers to
measurements or mix formats. [Legacy contract](HISTORY.en.md), [tests](../tests/history_store.cpp).

## SDK example

[linux_app_history.cpp](../examples/linux_app_history.cpp) uses only public
headers and the example formatter. With no arguments it displays help.
`--self-test` uses fixtures with matching path and period, a known zero,
legacy import, and a denied report. `--sample` makes two Linux reads with
CPU history. It can observe visible executables without displaying the
process table. It does not start network capture, connect the D-Bus observer,
save history, or run actions. The
[external consumer](../tests/app_history_consumer/CMakeLists.txt) validates
the installed SDK; it does not require CMake in the GUI. The sample is a
smoke test, not an oracle for totals or new physical validation of F03/H05.

Verification: Linux Release passed 194/194 tests. Both example tests were
repeated ten times, and ten portable MSVC runs passed. The installed SDK
consumer exited 0 in self-test and sample modes. Sixteen local links were
checked. Logs: `work/app-history-contract-*`. No changes to the library,
ABI, or formats. N03 remains open; this result does not resolve any
historical failure on its own.
