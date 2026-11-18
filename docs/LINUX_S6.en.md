# s6: native status record

Revision 134 adds [s6 FIFO control](LINUX_S6_CONTROL.en.md), separate from the
raw record. Full delivery does NOT confirm liveness, readiness, or completion.
Revision 135 adds an [explicit inventory with loggers](LINUX_S6_INVENTORY.en.md)
without treating these records as proof of live services.

Revision 133, part of K03. Public API: `pcinsight/linux_s6.hpp`.

```cpp
auto record = pcinsight::read_linux_s6_status_record("/service/mi-servicio");
// Check value/support/note. This is a RECORD, not proof of a live service.
```

`decode_linux_s6_status_record(span)` validates the current 35-byte format,
and `read_linux_s6_status_record(path)` reads it directly without running
s6-svstat, other programs or scripts, or using s6/skalibs libraries. It does
not install dependencies, create directories, acquire locks, open FIFOs for
I/O, write requests, or send signals. It does not integrate automatically
with Monitor or with ServiceInfo/systemd/runit.

## Data contract

The result preserves two 12-byte TAI64N stamps without conversion to Unix
time or uptime, a big-endian u64 reported PID, a big-endian u16 wait status,
and four flags: paused, finishing, want_up, and ready_reported. These are
not the runit layout or state enumeration. The decoder accepts exactly 35
bytes, nanoseconds below 1e9 in both stamps, and known bits in the flags byte.
Other sizes or bits yield UNKNOWN; there is no automatic version adaptation.
`ready_changed_at_tai64n` retains the raw readystamp, which is not assumed to
refer exclusively to when the app became ready.

The PID is retained as u64, without narrowing it to a local PID. It can
belong to a different namespace. During finish it may refer to the process
running finish, not the main daemon. It is NOT used as identity, for signaling,
or as proof of liveness. The wait word can describe an earlier exit, not a
current exit code. Flags are preserved without inventing relationships between
them: zero PID with ready=true is valid supervisor data.
`ready_reported` does NOT mean the app is alive or healthy.

**Available means the binary record was read and validated**, not that the
supervisor is live, the state is current, readiness is effective, or a
request succeeded. The file can outlive the supervisor. By design, the
provider returns the record even without a supervisor, FIFO, or lock; it
does not turn it into running. This unit exposes no known liveness field,
probes no lock, and authenticates no peer.

## Selection and consistency

Use an explicit, trusted absolute service path of at most 32,768 bytes
with no NUL. openat2 with NO_MAGICLINKS pins the service and supervise
directories; ordinary directory symlinks are supported. Status must be a
real, regular 35-byte file, not a link. The provider first pins it with
O_PATH/NOFOLLOW and checks type and size, then reopens **only that owned FD**
through /proc/self/fd for I/O, verifying device and inode. It does not open
an unchecked path that could have changed to a FIFO or device.

The provider reads at most 36 bytes twice, checks that mtime/ctime/size stay
stable during each read, and requires identical content across both reads.
It then revalidates the service/supervise selection and the boot/procfs/PID/
start/UID/namespaces/eUID context. Errors, truncation, changed selection or
context, or differing content yield UNKNOWN, not zero or down. A missing root
is not created. Usable openat2 and procfs are required; no unsafe fallback is
used. This does not guarantee an atomic snapshot, detection of all ABA changes,
maximum I/O latency, or that the file stays unchanged afterward. The host
serializes fork/setns/credentials/FD-context changes. Each call reads afresh,
with no thread, implicit polling, or cache of the last good value.

## Evidence and remaining work

`tests/linux_s6.cpp` covers invalid sizes 0–34 and 36, byte order, boundary
and invalid TAI values in both stamps, every known flag combination and
reserved bits, full-width u64 PID, PID 0 with ready=true, native reads from
a private tree without a supervisor, a missing root not created, ordinary
aliases, status link/FIFO/directory rejected without blocking, rejection of
the runit 20-byte format, repeated reads, and no FD leaks. The native test
also checks permission_required with a private unreadable file when the
test user is not root; it does not pretend that check ran under root. MSVC
tests only pure contracts and the non-Linux fallback. The SDK consumer links
the new header and symbols. This does not claim deterministic tests for
every race or acceptance with a real s6 supervisor.

Revision 133 did not include control. Revision 134 adds the consent-gated
[FIFO handle](LINUX_S6_CONTROL.en.md), revision 135 adds inventory, and revision
136 adds [condition observation](LINUX_S6_OBSERVATION.en.md). K03 remains open
for liveness evidence, real acceptance, and integration; SysV and other
pending work are unchanged. At this checkpoint, OpenRC script/runtime
control remained deferred without explicit authorization for exception 132:
“continue” did not grant it. Collection still uses no external applications.
The I07 observer is unchanged.

Primary format sources: [s6 header](https://raw.githubusercontent.com/skarnet/s6/master/src/include/s6/supervise.h),
[unpack](https://raw.githubusercontent.com/skarnet/s6/master/src/libs6/s6_svstatus_unpack.c),
and [supervisor](https://raw.githubusercontent.com/skarnet/s6/master/src/supervision/s6-supervise.c).
The [s6_svc_ok check](https://raw.githubusercontent.com/skarnet/s6/master/src/libs6/s6_svc_ok.c)
uses the lock. This unit neither replicates it nor claims equivalence.
