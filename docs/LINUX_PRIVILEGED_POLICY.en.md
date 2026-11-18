# L05 — protected rule loading and reload generations

Revision 151, 2026-11-02. Internal unit implemented and tested. No installed
or activated helper, executor, or privileged deployment. L05 remains E.

src/linux_privileged_policy.hpp/.cpp adds a restricted parser, a descriptor-
anchored Linux reader, and PrivilegedPolicyStore. It obtains no paths, UIDs,
or FDs from the client, environment variables, or external commands. The
directory descriptor and trusted UID come from a future secure bootstrap;
the default UID is zero. Private tests explicitly pass their current UID
without changing file ownership or elevating privileges. This does NOT make
an arbitrary root safe. The reader checks the anchor itself, not all its
ancestors or how the caller opened it.

## Restricted format

```text
PCINSIGHT-SERVICE-POLICY 1
private.service 7
blocked.service 0
```

The exact header and a final LF are required. Each rule line contains a
literal unit name, one space, and one digit from 0 to 7. Bits are start=1,
stop=2, and restart=4; zero denies. No comments, CR, tabs, blank lines, NUL,
expansion, wildcards, duplicates, or partial rule acceptance. Names follow
the [revision 145 contract](LINUX_PRIVILEGED_PREFLIGHT.en.md). A header
without rules is valid but denies every operation. Limits: 256 rules and
96 KiB total.

## Native reading

The relative path may have up to 4,096 bytes and 16 components, each up to
255 bytes and restricted to ASCII letters, digits, hyphen, dot, and underscore.
Empty components, `.`, and `..` are rejected. There are no absolute paths or
working-directory dependencies. The reader duplicates the input FD with
CLOEXEC without closing it or changing its offset. It traverses directories
with openat/O_DIRECTORY/O_NOFOLLOW/CLOEXEC and checks the expected owner and
absence of group/other write bits or special bits on the anchor, directories,
and file. Only a regular file with one hard link is accepted. fstatat without
following symlinks checks the type before opening; identity is compared after
opening. O_NONBLOCK prevents waiting on a FIFO, and FIFOs are rejected before
reading. The reader does not write or automatically repair permissions.

Reads are limited to 96 KiB. The reader checks the number of bytes read and
compares identity, UID, GID, mode, link count, size, mtime, and ctime before
and after, both through the descriptor and by name. Pinned directories and
their links to parents are revalidated. Any failure discards the entire set,
without partial rules or fallback. Owned FDs close through RAII. Stat checks
are not an atomic transaction: the filesystem, anchor, and administrator UID
are assumed trustworthy, with no hostile privileged writer. A privileged
owner can still move nodes or write after the final check.

## Reload and receiver

PrivilegedPolicyStore starts inactive. Each refresh first invalidates the
previous policy and advances the generation, even if content is unchanged or
the read fails. It publishes only a complete, owned snapshot; explicit
invalidate also revokes it. Refreshes are serialized. A mutex protects
snapshot/is_current/invalidate. Invalidation during a load prevents its result
from being published. The counter does not wrap; exhaustion keeps the store
inactive. The value is revoked before allocating notes that could fail.
Each snapshot copy keeps its data, but NOT current validity or permission.

receive_native_privileged_policy_request takes a store snapshot, binds its
generation to the context, and checks that it is current before and after
identity observation and CheckAuthorization. A detected reload or invalidation
discards the reply even when Authority returned a positive authorization.
The store must outlive the handler. The
[revision 150 variant](LINUX_PRIVILEGED_RECEIVER.en.md) accepting a vector
directly remains a trusted internal adapter contract, not client input.

There is NO watcher. File changes without refresh/invalidate do not update
the store by themselves. There is also no atomic guarantee between the final
check and later use of the report. It remains an observation, not a ticket
or executor. Future execution must revalidate and coordinate configuration
and identity immediately before acting.

## Revision 151 evidence

tests/linux_privileged_policy.cpp covers the portable parser, masks 0–7,
a header with no rules, format/NUL/duplicates, and 256/257 bounds. Real Linux
tests use a private directory and cover nested reads, invalid paths/UIDs,
intermediate and final symlinks, hardlinks, FIFO, insecure file/directory/
anchor modes, excessive size, invalid contents, generations, no stale fallback,
recovery, disappearance, and no FD leaks.

tests/linux_privileged_receiver.cpp now uses a loaded policy rather than
literal rules injected into the receiver. Broker and libsystemd remain REAL
and private; Authority is SIMULATED. Each run uses 21 requests and eight
checks, retaining revision 150 cases and adding invalidation from the Authority
callback while the query is pending (the positive reply becomes unknown),
the next request making no query, recovery, reload with insecure permissions
and no fallback, and final recovery. No polkitd, host bus, activation,
prompts, real services, or elevation.

Linux 179/179 plus 90 focused L05 runs (nine tests repeated ten times, including
ten private-broker runs); 20 MSVC parser/argument/fallback runs (without simulated
Linux); two reinstalled and relinked SDK consumers exiting 0. Initial, full,
and final Linux builds were clean. Logs: work/privileged-policy-*. No new
public API, layout, formats, or remote CI.

## Remaining work

Update 152: the [owned anchor and connector](LINUX_PRIVILEGED_CONNECTION.en.md)
are implemented, with validated absolute opening, a receiver bound to its own
connection, and generations and revocation on changes. Process/loader bootstrap
and acceptance remain pending; revision 151 numbers are historical evidence.

Remaining work includes protected process/anchor bootstrap and environment
and dynamic loader, trusted endpoint/broker and Authority UID configuration,
host connection lifetime and reloads, transport/FD/queue limits and an
aggregate deadline, real polkit policy and consent, a restricted executor,
and acceptance under specific authorization. Existing writable descriptors or
mappings are not detected. Bootstrap must not adopt files created by an
untrusted owner. O_NONBLOCK does not impose a deadline on regular files or
remote filesystems. Remote cancellation is not guaranteed. This does not
defend against compromised root, kernel, or filesystem or replace host isolation.

Next at this checkpoint: bootstrap, connection, and deployment conditions
without installing, elevating, or activating anything. OpenRC revision 148
stays limited to explicit control; SysV/K03/I07/UAC are unchanged.
Audited official count: C60/E10/P3/B2, with three X excluded; 60/75 = 80%
closed and 15/75 = 20% open.

Primary sources for the primitives used:
[open/openat](https://man7.org/linux/man-pages/man2/open.2.html) and
[stat/fstatat](https://man7.org/linux/man-pages/man2/stat.2.html).
The grammar, bounds, epochs, and additional restrictions are PCInsight decisions.
