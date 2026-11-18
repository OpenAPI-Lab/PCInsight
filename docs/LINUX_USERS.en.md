# Reported sessions and per-user resource usage on Linux

Partial J04 update: [user I/O](LINUX_USER_IO.en.md) replaces disk
attribution by username with interval-based attribution by numeric UID
and typed coverage. Legacy disk and network fields mirror that report;
utmp rows without a UID have no attributed I/O. CPU and RAM still use
username. References below to summing disk and network by username describe
the previous contract, not the current one. F03 network work was still
pending at this checkpoint.

J01 update: [session state](LINUX_SESSION_STATE.en.md) completes per-field
login1 state, activity, and origin, and adds explicit coverage for the
selected source, including empty inventory and absent sources. The utmp
aggregation contract below remains; reported_login does not become active.

Completed work: aggregate visible resources by username while retaining
multiple utmp session records. The earlier version replaced the previous
session for the same user, summed missing metrics as zero, and could
display all zeros with include_processes=false.

## Contract

- One row per unique ut_line, with the username from its USER_PROCESS
  record. Identical duplicates are deduplicated. The same ID with a
  different username or status fails explicitly rather than choosing
  a user arbitrarily.
- status=`reported_login`: utmp reports a login. It does not prove
  that the session is active, unlocked, or connected, or that the
  recorded PID still refers to the same process.
- `resource_scope="linux-visible-processes-by-username"` describes
  aggregated visible processes for that username, **not an individual
  login or TTY**. Two sessions for the same user repeat the same total.
  The GUI must group or deduplicate by user before calculating totals.
  Do not add those rows as distinct users.
- When user information is requested, Monitor collects processes internally
  even with include_processes=false. The process list is not returned
  when disabled. The first CPU sample remains unknown until deltas are
  available. Hiding the list does not eliminate procfs collection cost.
- Metrics are calculated separately. A valid read of zero is preserved.
  No accessible values means nullopt, never zero. If there are no
  values and a read required permission, permission_required is preserved.
- A known sum is estimated, with known and missing counts. Procfs
  enumeration can be incomplete or restricted by permissions, hidepid,
  or namespaces and is not atomic. Availability of every visible value
  does not certify a whole-system total. Partially known data retains an
  explicitly estimated sum without extrapolating the unknown portion.
- NaN, infinity, negative or greater-than-100 CPU values per process,
  and values with unsupported or permission_required support are treated
  as absent or invalid. A CPU sum above 100 or uint64 overflow yields
  nullopt. There is no clamp, wraparound, or invented saturation.
- Duplicate or zero PIDs invalidate input to avoid counting twice.
  An empty or unknown username is not associated with a login. The
  collector no longer invents UID 0 or root when /proc/PID/status
  does not provide Uid.

## Attribution limits

Association uses usernames reported by NSS and utmp, not a globally
validated UID. Truncated utmp names, UID aliases, namespaces, or NSS
changes can prevent a match or represent one account under different
names. The real UID reported by status is used; this does not determine
causal responsibility for resources or which processes belong to a
specific session. Summed RSS can count shared pages more than once.
It is not unique physical RAM or PSS. Disk sums per-process procfs rates
and do not establish physical active time. Linux has no universal
per-process network provider; no false zeros are presented.

USER_PROCESS records do not certify all connected users. Containers,
distributions, and graphical sessions may not maintain utmp. A missing
file returns an empty list of reported records, **not proof that there
are no sessions**. Read failures other than EOF/not-found propagate
without passing off a partial enumeration as complete. J01 adds public
metadata for the selected source, not proof of whole-host coverage.
login1 is a separate optional provider with native state and its own
limits; see [session state](LINUX_SESSION_STATE.en.md).

getutxent uses global libc state. The mutex serializes only this
provider; the host must coordinate other utmp/NSS reads and changes.
The provider does not call utmpxname, change the path, or write real
utmp. There is no external CLI or helper and no overall libc/procfs/NSS
deadline. Limits: 10,000 sessions, 100,000 examined utmp records or
received processes, 4,096-byte strings, and at most 16 MiB of identity
payload. Exceeded limits or invalid identity raise an explicit exception.

This work does not validate actual Disconnect, account management, or
session control. The later [Disconnect safety fix](LINUX_DISCONNECT.en.md)
removes the legacy utmp/SIGHUP path. disconnect_available is false for
this provider. Do not use the inventory test as evidence of working
detach or logind control. The public UserSession type expanded in 0.x;
rebuild the library and consumers.

## Verification

On 2026-10-14, Linux Release passed 19/19 and Windows Release passed
30/30. Packages were installed and rebuilt external consumers ran on
both systems.

Portable tests cover multiple sessions, deduplication and conflicts, a
user without processes, absent and partial metrics and zero, permissions,
duplicate PID, invalid names, NaN and infinity, out-of-range sums, and
limits. Windows verifies only this injected logic, not utmp or procfs.
The Linux test uses private utmp with two sessions for the calling user
and one without processes. DEAD_PROCESS is ignored. With process rows
hidden, Monitor returns known RAM, unknown first CPU, and matching
calculated second CPU values for both sessions. Missing network stays
unknown. Only the test process's utmp selector is restored and its
private files removed. No real session is created, edited, or disconnected.

References: [utmpx/libc and shared state](https://man7.org/linux/man-pages/man3/getutxent.3.html)
and [utmp](https://man7.org/linux/man-pages/man5/utmp.5.html).
