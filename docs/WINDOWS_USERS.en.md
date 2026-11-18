# Windows WTS session inventory

Monitor queries local sessions when include_user_sessions=true. The option
defaults to false. The WTS reader is independent of process collection.
Monitor adds the aggregation documented in [Windows user resources](WINDOWS_USER_RESOURCES.en.md),
collecting process data internally even when the process list is hidden.
It uses no external programs, PowerShell, WMI, elevation, or temporary files.
wtsapi32 ships with Windows and requires no additional installation.

## Public contract

The inventory returns one UserSession row per WTS ID with a nonempty
reported account. id has the form wts:N; username is DOMAIN\user when a
domain exists, or just user when it does not. display_name is the account
name without its domain, NOT the full profile name. Multiple sessions
for one account remain separate. Listener sessions, which do not represent
a login, and entries with no reported account are skipped.

status preserves the WTS connection state: active, connected, connect_query,
shadow, disconnected, idle, listen, reset, down, or init. listen is mapped,
but its rows are omitted from the account inventory. Future or unknown states
preserve their number as unknown(N) rather than becoming active. Connection
state does not establish whether the session is unlocked, someone is using it,
or applications are consuming resources.

The standalone WTS reader uses resource_scope=windows-wts-inventory-only.
CPU, RAM, disk, and network are absent with a note that this reader does not
account for them. It neither sums processes by username nor attributes
resource usage to a session because it shares an account. These metrics are
separate: the [resource provider](WINDOWS_USER_RESOURCES.en.md) reports observed
CPU and RAM in Monitor without claiming complete coverage, and leaves disk
and network absent.

disconnect_available and logout_available are false: this provider is
inventory only. logout_token is empty. wts:N is NOT a verified persistent
identity for actions; IDs can be reused. This provider does not implement
disconnect, logout, account management, or launch under another user's
credentials. The existing launch action still uses the caller's context.

## Failures and limits

Enumeration or query failures propagate std::system_error with a Win32 code
instead of returning a partial or empty list as a complete inventory.
Monitor propagates that failure. A session that disappears during a query
can invalidate the sample; the caller can request another. No permissions
are changed and there are no hidden retries. An empty result means only
that this query reported no accounts, not that the machine has no users.

Limits: 10,000 entries, 4,096 UTF-16 code units per field, and 16 MiB of
final identity text. Checks cover null pointers with nonzero counts,
duplicate or reserved IDs, byte lengths and alignment, termination inside
the buffer, valid UTF-16, and a complete injected API. The whole list is
validated before it is returned. WTSFreeMemory releases all buffers even
on errors or exceptions. The reader trusts buffers returned by the system
API; it is not a parser for arbitrary memory pointers. The bounds do not
impose a timeout on blocked WTS calls.

Enumeration and identity queries are not an atomic snapshot. The reader
does not claim protection against concurrent ID reuse or account changes.
It does not guarantee complete coverage when the caller lacks query
permissions or when the service or Windows version imposes restrictions.

## Verification

Injected tests cover multiple sessions for one account, an empty domain,
Unicode, known and future states, listeners and entries without accounts,
empty lists, API and permission errors, truncated or malformed identities,
bounds and duplicates, buffer cleanup even on error, and incomplete APIs.
The native read-only test identifies the current local session with
ProcessIdToSessionId and exercises Monitor opt-in and opt-out with process
rows hidden. No real session is disconnected or ended.

Release results: Windows 63/63 and Linux 49/49. Libraries and headers were
installed and external consumers rebuilt and run on both systems. The
Windows consumer also queries users with process rows hidden. Linux tests
regressions in its own backend; compiling on Linux does not establish WTS
coverage. At this checkpoint, MSVC, ARM64, real multi-user RDS, and actual
service denials remained unvalidated. Errors, permissions, and multiple
sessions were tested through an injected API.

Official sources: [WTS enumeration](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsenumeratesessionsw),
[identity query](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsquerysessioninformationw),
and [connection states](https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/ne-wtsapi32-wts_connectstate_class).
