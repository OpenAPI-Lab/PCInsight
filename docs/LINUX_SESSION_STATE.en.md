# Linux session state and inventory coverage (J01)

Later partial J04 update: [user I/O](LINUX_USER_IO.en.md) adds a numeric
UID I/O report with coverage and UID checks at both disk interval
endpoints. CPU and RAM retain username grouping. This update did not
complete F03 network support or J04.

This work completes the logind and utmp inventory and state contract,
with visible resources grouped by username. It does not add account
control, detach, or destructive actions. J02/J05/J06 remain separate.
Disk and network resources and their completeness are tracked in
J04/F03 and are not closed here.

## API and semantics

UserSession.linux_details is a Metric<LinuxSessionDetails>. provider
identifies login1 or utmp. Each field retains presence, support, and a
note. Check the Metric, not just the legacy UserSession.status text.

| Source/field | What it actually reports |
|---|---|
| login1 state | Native State property, without conversion to WTS connected/disconnected |
| login1 active | Foreground Active property; false is a known value, not a disconnect |
| login1 remote | Remote-origin property, not current connection health |
| login1 unix_uid | UID from the ListSessions tuple, not a global identity across namespaces |
| utmp state | reported_login: a USER_PROCESS record, not evidence of a live or active session |
| utmp active/remote/unix_uid | Unknown; not inferred from PID, hostname, or NSS |

State and Active are queried separately. Their reads are not atomic;
neither is derived from the other or forced to agree. Future state
tokens are retained when valid ASCII and at most 128 B, without inventing
their meanings. An invalid or missing state or denied access yields a
Metric without a value; legacy status reports unknown. A State error
does not clear valid Active for the same identity, and vice versa.
These properties also do not establish human presence, an unlocked
screen, or CPU activity.
[Native login1 contract](https://github.com/systemd/systemd/blob/main/man/org.freedesktop.login1.xml).

## Identity during queries

All properties use the unique owner obtained before ListSessions.
TimestampMonotonic is checked before and after reading Id, Name, User,
State, Active, and Remote. The provider requires a stable positive
timestamp, Id/Name/UID matching the enumerated row, and available
identity properties. If any is missing, changes, or does not match,
the row and UID reported by ListSessions are retained, but state,
active, and remote remain unknown and no logout_token is published.
Observed permission denial is distinguished from absence or generic error.

The token uses the verified timestamp, not a PID or TTY. Inventory
rejects duplicate IDs or object paths and verifies that the owner
is unchanged at the end. Enumeration, owner, parsing, or limit failure
propagates. The provider neither publishes partial inventory as complete
nor hides the failure behind a successful fallback.

J02 control retains its fresh checks before acting. A published token
does not guarantee authorization. Queries are not transactional or
immutable references. A-to-B-to-A reuse and changes outside the read
interval can still occur. The enumerated UID is not proof of current
identity when properties were not verified.

## Coverage and empty inventory

SystemSnapshot.linux_session_inventory_source is a Metric separate
from the rows:

- Known login1: ListSessions completed on the selected provider,
  including when it returned zero rows. This does not imply every
  host login is present.
- Known utmp: libc completed the record read, including for an empty
  file. This does not certify that utmp is current or tracks graphical
  or container sessions.
- No value: collection is disabled or the utmp source is absent.
  The note distinguishes the cases. An empty legacy list is not
  presented as “no users connected.”

Optional login1 through already installed sd-bus is preferred. If the
library, bus, or compatible service is unavailable, utmp is used.
prefer_logind_sessions=false explicitly selects utmp. The provider does
not mix sources or infer equivalence between their IDs. No dependencies
are installed, and loginctl, who, w, and other programs are not run.
elogind compatibility depends on the same API and available sd-bus
transport; native validation across all distributions is not claimed.

The utmp reader distinguishes observed ENOENT from normal completion
or libc ESRCH. Other read errors propagate. It does not validate the
file byte by byte or remove old records. Its mutex covers this provider,
not other host utmpx/NSS usage.

## Resources and limits

CPU and RAM sums remain grouped by username for visible processes, not
by session or globally validated UID. Multiple sessions for the same
user repeat that total; do not count it twice. include_processes=false
hides the list without preventing collection needed for the totals.
First CPU without a baseline remains unknown. Known partial metrics
are estimated and include counts and a note. Absence, NaN, invalid
range, overflow, or wholly inaccessible data do not become zero.
RSS can count shared pages multiple times. See [Linux users](LINUX_USERS.en.md).

Conflicting metadata in duplicate rows is checked rather than choosing
an arbitrary UID or state. Aggregation limits remain 10,000 sessions,
100,000 records or processes, and a 16 MiB payload. D-Bus strings are
limited to 4,096 B, paths to 1,024, and owner to 255. D-Bus calls allow
five seconds each, and the five-second enumeration budget is checked
between rows. There are now more queries per row, so this is **not**
a five-second overall deadline. NSS, libc, and opening the bus have
no total deadline here either.

The 0.x API changes the UserSession/SystemSnapshot layout. Rebuild the
library and **all** consumers. The action virtual ABI and history
format are unchanged. Windows leaves Linux fields absent and retains
its WTS provider.

## Tests

tests/linux_session_state.cpp covers native and future states, known
false, independent fields, permissions, changed or absent identity
ID/name/UID/timestamp, conflicting duplicate metadata, and absence on
non-Linux backends. On Linux it queries real login1 and the public
Monitor read-only, without calling control methods.
tests/linux_users.cpp retains visible-sum tests and uses private utmp
for repeated sessions, unknown data, an empty file, and a missing
file. Only the test process changes and restores its utmp selector.

J01 completion covers these provider contracts, not certified sessions
across all distributions, hidden process resources, or remote
connection health. No real session is created, terminated, or
modified in testing. Additional hardware and distributions remain
under N02, and unverified actions are not counted complete.

On 2026-10-16, Linux Release passed 85/85 and Windows Release passed
105/105. Packages were reinstalled and rebuilt external consumers
exited 0. The Linux consumer also queried the new fields through
the installed API. Native Ubuntu WSL read one login1 session, one
known State, and one known Active flag. Those are availability counts,
not a claim that the session was in the foreground. No session control
was invoked. Private utmp fixtures verified three records, CPU/RAM sums,
an empty file with a known source, and a missing file without a known
source. Windows ran portable tests, not native logind/utmp. The final
incremental build had no warnings. Initial transient MSFS clock warnings
and the existing NOMINMAX warning were not treated as evidence for another
environment and did not lead to clock changes. Passing this suite does
not resolve prior intermittent N03 CPU or history risks. No IDs are
added to the project count.
