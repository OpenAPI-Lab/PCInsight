# Observed cookies without a manual list: internal engine

Revision 81 follow-up: [internal selected windows](LINUX_NETWORK_DISCOVERED_WINDOWS.en.md)
now own a pidfd, verify the identity join and context at both endpoints,
and provide per-cookie rates. The engine described here still accepts only
a token; do not use it directly as public process selection. The description
below preserves revision 80 evidence.

Revision 80 adds start_discovering_selected/read_discovered to the PRIVATE
LinuxPrivateNetworkSession. It accepts only the selected actor's kernel token,
not a cookie or socket inventory. It is not yet a public API, rate provider,
or procfs join: the token must come from an authenticated higher-level source.
The revision 78–79 public APIs still use declared cookies. F03 remains open.

## Registry and bytecode

The engine reuses live BTF, initial TGID/UID and leader-start filters, and the
inet{,6}_sendmsg/sock_recvmsg hooks. It reads the SOCKET namespace, not the
process's current namespace. Each NONZERO cookie seen in a positive
instrumented operation is registered on first use in one of 16 slots. Each
slot stores a cookie, four cumulative payload counters (TCP send/receive and
UDP send/receive), and receive exclusions. Positive excluded receives can
also register a cookie without contributing payload. It does not discover
idle sockets or unrelated processes. There are no wire bytes, packets,
contents, addresses, interface data, or coverage of every I/O path.

The ARRAY map has 105 u64 cells: a nine-cell prefix with failures, overflow,
writers, and sequence at indices 4/6/7/8, plus 16 six-cell slots. CMPXCHG64
compares the cookie with zero and claims a free slot, or uses the previous
value to select that slot if another thread claimed the same cookie. If it
held a different cookie, the scan continues. The scan is bounded and unrolled,
with no unbounded loop, eviction, deletion, or slot reuse during capture.
R0 and compare-and-exchange semantics were checked against the
[official BPF ISA](https://kernel.org/doc/html/latest/bpf/standardization/instruction-set.html#atomic-operations).
The kernel, JIT, and verifier must support the instructions. Load failure
returns absence with a reason, not a fallback to unfiltered capture.

An existing cookie keeps updating its slot even when all 16 are occupied.
A 17th distinct cookie sets a sticky GLOBAL failure. The engine neither
reports 16 rows as if nothing were lost nor recycles a row to hide loss.
Zero cookies, nulls, read/identity/update failures, and overflow also fail
closed. It does not generate or invent cookies: net.net_cookie must be
initialized and validated through BTF. The engine does not force
initialization in another namespace; zero remains unavailable.

## Reading and consistency

The global writer/sequence guard covers slot claims, totals, exclusions,
errors, and overflow. Bytes, exclusions, and sequence use atomic addition
with sticky overflow. Cookies are immutable once claimed. The reader obtains
writer/sequence, then the full map, then sequence/writer, with up to eight
retries on overlap. It rejects global failure, overflow, changes to the
sequence/writer checks, an incompatible prefix, counters in a slot without a
cookie, gaps or duplicates, cookies without completed activity, or more
cookies than updates. It does not combine sweeps to fill a partial sample.

Before activity, the result is a known EMPTY list, not cookie zero or
fabricated rows. A slot or zero counter is not an inventory or global total.
Classic read rejects this mode, and read_discovered rejects the other mode.
Detach freezes the maps and still permits reads labeled detached. Stop
closes everything and clears the mode; restart creates a new empty map.
Flags are not reset in place to conceal loss.

This private layer does not own a pidfd or revalidate procfs, liveness, or
observer context on reads. It is not public PID selection and has no
collector fork guard. Do not use arbitrary tokens or publish these data as
current apps or users. At this revision, integration with the selected layer
still needs owned pidfds, endpoint context checks, windows/rates/epochs/reset,
and multiple actors, followed by API/Monitor/Sampler and general policy.
Charts, persisted history, and legacy totals are unchanged.

## Evidence

The existing linux-network-classified/session tests cover 16 cookies, return
to a known cookie, sticky failure on the 17th, high bits in 64-bit cookies,
TCP/UDP IPv4/IPv6 send/receive, exclusions, and simulation of a peer claiming
the same or another cookie just before CMPXCHG, without duplicate slots or
double-counted payload. They also cover errors, UID/start/zero/null/overflow,
corrupt or inconsistent registry data, denial/estimated support, rollback
across eight acquisition and exception cases, and detach/stop/restart and
mode separation. Release checks stay enabled.

Manual `--authorized-private-observed-cookies`, NOT CTest: two private
children, with capture limited to ONE. The kernel token is obtained through
the pidfd/procfs join before startup; the producer receives NO cookie. The
child initializes its cookie via SO_NETNS_COOKIE only as an independent
oracle. The result is empty before activity, then the observed cookie
matches the oracle: TCP 12,288/12,288, UDP 768/768, and 24 peeks, including
two concurrent threads of the same actor. The other child and parent are
excluded. Detach freezes counters; stop leaks no FDs. Restart is empty,
then concurrent activity yields TCP 8,192/8,192, UDP 512/512, and 16 peeks.
Both children exit normally. Only ONE real namespace was tested. The
16-cookie capacity and races between different cookies are fixture cases,
not evidence for two real namespaces or uninitialized cookies. Parent and
child watchdogs are 60 s and 50 s, not hard BPF deadlines; no automatic elevation.

work/network-discovery-native-linux.log records the positive result, and
declared-native-linux records the successful Monitor regression for declared
buckets. The first build failed because of assert macro parentheses around
a template comma. The fixture was fixed without weakening the producer
and the log retained. Failure paths explicitly initialize the stack for
the verifier. No new dependency, CLI, SDK, setns/unshare, or global capture.

Result: Linux Release 126/126 without skips, two focused tests repeated
20 times each, a warning-free build, and the installed consumer rebuilt,
linked, and exiting 0. The library and 2/2 focused MSVC tests passed with
no new C++ warnings; expected D9025 keeps assertions active. No public API
or layout changes and no new full Windows suite or consumer run. The
revision 75 consumer failure remains in N03. No universal hardware, kernel,
or distro certification. F03/H04/J04 remain open; revision 80 retains
57/75 = 76.0% closed and 18/75 = 24.0% open, with no new IDs or partial credit.
