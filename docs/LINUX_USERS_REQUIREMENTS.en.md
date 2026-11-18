# N03: Linux users and sessions for the GUI

**Language:** [Español](LINUX_USERS_REQUIREMENTS.md) · English

Revision 164, 2026-11-06. Maps R16 and Users actions to
J01/J02/J04/J05/J06 and L01. **A session row is not a resource usage row
for that session.** CPU/RAM, disk, and network use different groupings.
Label their scopes in the GUI. Do not sum session rows or join domains
just because a name or UID matches.

| Field | API / unit | Scope and evidence |
| --- | --- | --- |
| User | `UserSession.id/username/display_name`, text | Identity and labels reported by login1 or utmp. Two sessions with the same name are still two rows. Neither the label nor the ID is a credential. [Inventory](LINUX_USERS.en.md). |
| Status | `linux_details.state/active/remote`, state/bool | Check the parent and each Metric. login1 exposes native state, foreground activity, and remote origin separately. `active=false` is not disconnection; `remote=true` does not prove a live connection. utmp provides `reported_login`, not active or remote state or an inferred UID. [Contract](LINUX_SESSION_STATE.en.md), [tests](../tests/linux_session_state.cpp). |
| Inventory source | `snapshot.linux_session_inventory_source`, text | Read separately from `user_sessions`. An empty list does not establish global absence of sessions. Optional login1 falls back to utmp according to availability; `prefer_logind_sessions=false` requests utmp. Failure of an available enumerator is not a successful empty result. [Contract](LINUX_SESSION_STATE.en.md). |
| CPU | `UserSession.cpu_percent`, % of visible capacity | Estimated total for visible processes grouped by **username**; the first interval may be absent. It repeats on sessions with the same name and is not per-session or whole-machine CPU. See `resource_scope`. [Contract](LINUX_USERS.en.md), [per-process CPU](LINUX_PROCESS_CPU.en.md). |
| Memory | `UserSession.memory_bytes`, RSS in B | Estimated visible sum by **username**; shared RSS can be counted more than once. It is neither unique physical RAM nor memory attributed to a session. Uses the same `resource_scope` as CPU. [Contract](LINUX_USERS.en.md). |
| Disk | `linux_io_details.disk_bytes_per_second`, B/s | Partial aggregate by **reported real UID**, not username or session. See `scope`, `real_uid`, `matching_processes`, and `disk_coverage`. UID is validated around reads and across process interval endpoints. Not physical throughput. [Contract](LINUX_USER_IO.en.md), [tests](../tests/linux_user_io.cpp). |
| Network | `snapshot.linux_user_network`, `partial_*_bytes_per_second`, payload B/s | One UID per F03 **observer domain**, independent of session inventory. Not per-session or total on-wire traffic. Do not substitute legacy network fields from `UserSession` or `linux_io_details`. [Contract](LINUX_USER_NETWORK.en.md), [tests](../tests/linux_user_network.cpp). |

## Sampling and coverage

`include_user_sessions=true` requests inventory. Turning off
`include_processes` does not prevent internal collection for CPU/RAM and
disk. Keep the same Monitor for deltas. Check support and notes. Keep a
known zero as zero, but do not convert absence or a permission denial to
zero. Visible scopes are partial because of permissions, namespaces, NSS,
and sample continuity. A utmp session with no UID does not get disk metrics
by matching its username. For disk, `known` and `missing` describe the
matches. `permission_denied` and `identity_unverified` are potentially
overlapping subsets of `missing`. `overflow` indicates that the sum cannot
be represented, not that the value was saturated.

Network requires `include_linux_user_network` and
`include_linux_process_network` and an observed F03 capture configured
with prior authorization. The flags do not start it. It does not require
`include_user_sessions`. Show the boot token, observer's user domain, and
coverage. The report also preserves producer, epoch, and time bounds for
the scan. Do not join by UID alone across boots, namespaces, or observers,
or with a session reported by a bus in another environment.
`initial_real_uids` can contain multiple kernel identities mapped to one
observed UID. A UID without a login can have a row.

Keep `selected_processes`, `idle_processes`, `observed_buckets`, and
`known/missing/permission_denied/overflow` per TCP send/receive and UDP
send/receive. Aggregates are partial rates, not cumulative counters.
No observed buckets does not mean zero traffic. Do not infer complete
process coverage from manual selection. When present,
`linux_network_admission` describes the corresponding admission.
[Public API](../include/pcinsight/linux_user_network.hpp).

## Explicit actions

| Action | API and limits |
| --- | --- |
| Disconnect | `disconnect_user` is unsupported by the included Linux adapters. `query_linux_disconnect_capability` and `disconnect_available` let you show why (known false for recognized login1/utmp). Do not use logout, signals, lock, or another operation as a fallback. [Contract](LINUX_DISCONNECT.en.md). |
| Log out | `terminate_user_session(logout_token)` is separate and destructive; confirm the potential loss of work. Use the token from current inventory, not an ID, PID, or TTY. `logout_available` does not establish permission. The backend revalidates identity and does not elevate. After acceptance, an error does not establish that nothing happened. Removal of the session object does not prove that all its processes exited. [Contract](LINUX_LOGOUT.en.md), [private test](../tests/linux_logout_native.cpp). |
| Manage user accounts | `AccountManagementIntegration` is an optional host integration, not a native editor. Configure a read-only query and a trusted navigation callback. It queries again for each request; only `available + true` invokes the callback. Do not run inferred desktop commands or elevate. `host_invoked` can be true even on error; do not retry blindly. [Contract](ACCOUNT_MANAGEMENT.en.md), [tests](../tests/account_management.cpp). |
| Run new task | `launch_process(LaunchRequest)` is the same explicit action as in other panels. Do not launch as the user in a row or associate a launch with a session by name. [Contract](LINUX_LAUNCH.en.md). |

## SDK example

[linux_users.cpp](../examples/linux_users.cpp) displays inventory, metrics,
and independent network data without joining or totaling sessions. It
does not print `logout_token`, invoke actions, or start capture.
`--self-test` covers repeated sessions, valid UID and zero, permission
denial, absence of legacy fallback, and separate network data.
`--sample` makes two read-only Linux observations. Empty session inventory
or a missing metric is still a possible result. The
[external consumer](../tests/users_consumer/CMakeLists.txt) uses the
installed SDK without requiring CMake in the GUI. No ABI or library changes.

Verification: Linux Release passed 198/198 tests. Both example tests were
repeated ten times, ten portable MSVC runs passed, and 20 local links were
checked. The installed SDK consumer exited 0 in self-test and sample modes.
Logs: `work/users-contract-*`. This does not newly validate real logout.
