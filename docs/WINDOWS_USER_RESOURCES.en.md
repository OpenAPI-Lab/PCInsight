# Visible resources by Windows session and account

Monitor calculates CPU and memory for opted-in WTS session reporting using
visible processes. include_user_sessions requires internal process collection
even when include_processes=false. The process list is hidden after resource
calculation. Disabling both clears CPU baselines. No polling, daemon, or CLI
is added.

## Association and scope

ProcessInfo.windows_session_id contains TokenSessionId from the primary
token read through the process handle, not a later numeric PID lookup.
The account and session ID are read from the same token handle. This is a
Metric: denied permission or an invalid query yields absence, never an
invented session zero. It is absent on Linux. This field is not a
persistent token for actions.

A process contributes only if its session ID is available, its FILETIME
creation timestamp is valid and available, and its account exactly matches
the WTS row's account. Username alone is not used: two sessions for one
account have independent sums. Processes under another account within the
same session (services or RunAs), unknown accounts, other sessions, and
unverifiable identities are not attributed. Letter case, aliases, and
format differences between WTS and LookupAccountSid can leave processes
unattributed. Equivalence is not guessed and SID validation is not claimed.
PID 0 is treated as the idle pseudo-process, not a user's workload.

resource_scope=windows-visible-processes-by-wts-session-and-account. The
metrics are estimated: only known values are summed, with counts of known
and missing values and globally unattributed processes in the notes. The
figures are not full account totals, Task Manager session totals, or causal
attribution. Process enumeration, WTS, permissions, and time windows offer
partial, non-atomic coverage. Concurrent session ID reuse and token changes
are not ruled out.

CPU sums per-process percentages normalized by all logical processors.
The first sample has no delta; zero is not forced. RAM sums working sets,
including shared pages that can be counted more than once. It is not unique
physical memory, private working set, or PSS. Disk and network are summed
only if a provider already produced valid rates. At this checkpoint, the
Windows backend does not produce those per-process rates and leaves both
absent. Aggregation does not convert general I/O into disk or network usage.

Each metric is handled independently. With no known values, it is nullopt;
permission_required is retained if any attributed value was denied. With
some known values, the sum is estimated without extrapolation. NaN, infinity,
CPU outside 0–100, and incompatible support states are omitted. A CPU sum
above 100 or uint64 overflow yields absence, without clamping or wrapping.
Only valid measured zero can contribute to a known zero. A session without
attributable processes is not shown as consuming zero.

## Limits and verification

The portable I/O-free core is fill_windows_user_resources. Limits: 10,000
sessions, 100,000 processes, 32,768 bytes per identity, and 16 MiB of input
identity data. WTS IDs must be canonical and unique, and PIDs cannot be
duplicated. Invalid inputs throw without publishing partial changes. Implicit
tokens or IDs are not accepted, and truncated lists are not treated as complete.
The core does not mutate processes or sessions or require elevation.

Tests cover separate sessions for one account, different accounts, measured
zero, permissions and partial or absent results, NaN/infinity and overflow,
duplicates and limits, malformed IDs, invalid identity/account/session, and
transactional publication. The native WTS/Monitor test checks RAM for the
current session, CPU on the second sample, hidden processes, reset after
opt-out, and TokenSessionId for the calling process. No real sessions were
closed or disconnected. Real multi-user RDS and other permission and hardware
combinations still require validation. The 0.x API was extended; rebuild the
library and consumers.

Release results: Windows 64/64 and Linux 50/50. Libraries and headers were
installed and external consumers rebuilt and run on both systems. Portable
aggregation also runs on Linux; native TokenSessionId and WTS association
were verified only on Windows. The portable test does not validate that
native provider.

Official sources: [TokenSessionId](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ne-winnt-token_information_class)
and [GetTokenInformation](https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-gettokeninformation).
See the [WTS inventory and its limits](WINDOWS_USERS.en.md).
