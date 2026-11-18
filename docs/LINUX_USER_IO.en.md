# Disk rates by UID and user I/O coverage (J04)

Revision 101: network data is now published in the observer's per-UID
`linux_user_network` report; see [Linux user network](LINUX_USER_NETWORK.en.md).
It is not joined to sessions by name or UID alone or duplicated across
sessions. The revision 23 disk implementation described here keeps its API
and semantics. J04 is verified as disk rates by reported UID plus a separate
network report with an explicit domain, not a global per-session total.
Legacy UserSession network data remains absent with a note directing
consumers to the new report. It is not the authoritative network API.

Earlier F03 update: [Linux TCP](LINUX_TCP.en.md) implements a partial TCP
sampler by socket_uid, separate from this real-UID report. It does not
fold that partial metric into total process or user network traffic or
complete J04/H04.

J04 requires disk **and network** with explicit identity and coverage.
The original implementation corrected disk rates and published coverage.
The network portion is completed from revision 101 through the F03
projection described above. Missing data is not turned into zero, and
sockets or interface counters are not taken as proof of exclusive
ownership by an account.

## Read API

`UserSession::linux_io_details` is a `Metric<LinuxUserIoDetails>` separate
from J01's CPU/RAM grouping by username. It contains:

- scope=`linux-visible-processes-by-reported-real-uid` and real_uid with
  support and note.
- input_processes: process rows supplied to aggregation, not all /proc
  candidates or all host processes.
- processes_without_uid: input rows without a known real UID. This count
  applies to the whole input and is not attributed to a user by name.
- matching_processes: count of rows whose real UID matches the session's
  reported UID. Absent without a numeric session UID; known zero when
  comparison was possible but no rows matched. Neither means measured
  zero disk activity.
- disk_coverage: known and missing partition the matching rows.
  permission_denied and identity_unverified are potentially overlapping
  subsets of missing. overflow means the sum is outside uint64, never
  saturation or wraparound.
- disk_bytes_per_second: estimated sum of known rates attributed to that
  UID. Missing reads leave a partial sum with explicit coverage, without
  extrapolation. If none are known, the metric is absent, with permission
  status if denial was observed.
- network_bytes_per_second: absent. Use linux_user_network without
  joining to an unverified session domain. Network values are not
  aggregated without a provider attribution contract, even when a
  caller-constructed ProcessInfo contains a number in that field.

Legacy UserSession disk and network fields mirror this report. They no
longer publish disk sums by username as if those established numeric
ownership. CPU and RAM retain the prior scope described by resource_scope;
do not interpret it as the scope of linux_io_details. Non-Linux backends
leave the new report absent rather than returning an empty Linux report.

## Disk identity and timing

The collector preserves D02 deltas and per-process timing without changing
the process rate. It adds a status read after the counters as well as the
earlier one. Every status and counter read checks PID/start_ticks under
the existing contract. Only a known real UID that matches before and after
qualifies that observation for user attribution. The tracker requires the
same UID in both interval observations and a valid D02 rate.

`ProcessInfo::linux_details.disk_rate_real_uid` publishes that estimated
attribution. The first sample, changed, absent, or denied UID, changed
identity, reset counters, invalid clock, or invalid rate does not yield
an attributable UID. Recovery after absence requires two consecutive
valid observations. A UID change does not necessarily invalidate the
per-process rate, but it prevents counting that rate for the new user
during the transition. Reset, opt-out, and batch errors clear attribution
baselines along with D02 baselines.

Aggregation requires a known current real UID, a known start_ticks token,
and a matching rate UID. Username never substitutes for those fields.
UID 0 is known and valid, not nullopt. Different names with the same UID
share a total; the same name with different UIDs does not mix I/O. A utmp
session without a numeric UID has no attributed I/O. The provider does
not invent a UID from NSS, the record's PID, or a possibly truncated name.
login1 uses the UID reported by ListSessions, not proof of a live session's
current or continuous identity.

Multiple sessions with the same UID repeat the user's total; **do not add
them together**. Deduplicate by UID and scope within the same environment
before charting totals. Do not combine reports from different samples,
hosts, boots, or namespaces.

## Remaining limits

This is **sampled** attribution for visible processes, not exact causal
accounting. Matching UIDs before and after a read does not rule out
A-to-B-to-A changes or identify who caused deferred, shared, or waited-for
child I/O. Real UID is not necessarily euid, fsuid, or socket owner.
Tick tokens are neither global nor immutable. The provider does not
certify UID-space compatibility between an external host bus and a
container's procfs. Numbers are interpreted only in the declared local
view, without a global guarantee or automatic translation.

Per-process rates can cover different intervals. The sum is not from one
atomic synchronized interval. It is bytes/s from read_bytes+write_bytes,
not active time or physical device throughput. Permissions and hidepid,
omitted processes, exited processes, and short lifetimes can exclude
resources even if every matching row is known. input_processes and
processes_without_uid do not quantify hidden data.
See [per-process disk I/O](LINUX_PROCESS_IO.en.md).

Aggregation limits remain 10,000 sessions, 100,000 processes, and 16 MiB
of identities. PID 0, duplicate PIDs, and conflicting duplicate sessions
are rejected. The collector may omit exited or inaccessible processes;
it does not claim a whole-machine total. With include_user_sessions=true,
include_processes=false still collects resources and only hides the
returned process list.

For network data, /proc/PID/net describes the network namespace, not
traffic exclusive to that PID. Do not split adapter counters across users,
infer traffic from socket counts, or use rchar/wchar as network bytes.
The optional native F03 source defines units, identity, windows, coverage,
and privileges. Its per-user projection has a separate report and is
not joined to this session row without verifying the domain.

The 0.x API expands LinuxProcessDetails/ProcessInfo and
UserSession/SystemSnapshot. Rebuild the library and **all** consumers.
It does not change the action virtual ABI or history storage, add
utilities or packages, or perform control actions on other users'
accounts, sessions, or processes.

## Verification

Tests in tests/linux_user_io.cpp, tests/linux_process_io.cpp, and
tests/linux_users.cpp cover UID 0 and measured zero, aliases and repeated
UIDs, same username with different UIDs, absent UID, missing or mismatched
token or rate UID, permissions, partial sums, overflow, UID changes
within or between observations, recovery, and reset. They verify that
network data without a provider does not become a positive sum or zero.
Linux integration writes and fsyncs only its own temporary file, checks
test-process attribution, aggregates with synthetic session labels, and
verifies the public path with processes hidden. It does not create or
modify sessions.

On 2026-10-17, Linux Release passed 92/92 and Windows Release passed
109/109. Both installed external consumers were rebuilt and relinked
and exited 0. They read the new fields and, on Linux, the public report
with processes hidden. Windows verifies its process and session rows
leave Linux fields absent. Native WSL: test process UID 1000, 8,779,070 B/s
after writing and fsyncing 131,072 B to its own file. The visible UID sum
was 8,779,070 B/s, with four known and two missing reads. This is not a
physical benchmark or a sustained rate. That aggregation used synthetic
session labels; the real public path yielded one session report with
processes hidden. Host users, credentials, sessions, and services were
not changed.

Logs: work/user-io-build-{linux,windows}.log,
work/user-io-verified-build-{linux,windows}.log,
work/user-io-focused-{linux,windows}.log, work/user-io-suite-{linux,windows}.log,
and work/user-io-native-linux.log. Initial builds recorded transient MSFS
clock skew on Linux and an existing NOMINMAX warning in a Windows test.
Later incremental verification had no warnings; the clock was not changed
and initial logs were retained. These runs do not resolve historical N03
risks. At that checkpoint J04 moved from P to E: disk, identity, and coverage
were implemented, with F03 network work pending. No closure, new IDs, or
partial credit were recorded; the count remained 46/75 (61.3%).

Primary references: [procfs Uid and I/O accounting](https://www.kernel.org/doc/html/latest/filesystems/proc.html)
and [proc_pid_net namespace scope](https://kernel.googlesource.com/pub/scm/docs/man-pages/man-pages/+/refs/tags/man-pages-6.11/man/man5/proc_pid_net.5).
