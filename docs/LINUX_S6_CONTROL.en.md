# s6: native control with explicit delivery

Revision 136 adds [condition observation](LINUX_S6_OBSERVATION.en.md) and
read_record() tied to the same handle. It does not confirm liveness,
readiness, or completion caused by the request.

Revision 134, part of K03. API: `LinuxS6Control` in `pcinsight/linux_s6.hpp`.

```cpp
auto target = pcinsight::LinuxS6Control::open("/service/mi-servicio");
if (!target.value) { /* show availability/note */ return; }
auto delivery = (*target.value)->request(pcinsight::ServiceAction::stop, consent);
// bool(delivery) describes DELIVERY; delivery.completed remains UNKNOWN.
```

No s6-svc/CLI, external library, exec, shell, script, sudo, installation,
worker, or wait. Only the existing supervisor can run its hooks or run/finish
programs while processing an authorized request; their effects are not
guaranteed to be harmless or isolated. This is not the OpenRC revision 132
exception, which remained unauthorized at this checkpoint, or an I07
autostart launcher.

## Commands and meaning

| Public action | Protocol bytes |
|---|---|
| start | u |
| stop | d |
| restart | cru |

Restart means resume, request the configured restart, and leave the desired
state up. The provider does not substitute t for the configured s6 down signal.
This is PCInsight's contract for the c/r/u sequence, not a transaction or a
promise that restart finished. Effects depend on state, configuration, other
actors, and hooks. The supervisor can refuse or fail to start, or finish can
decide not to restart. The provider does not create or remove down files with
persistent D/U/Q commands. A trusted tree using the reviewed protocol is
required. The provider neither negotiates a version nor identifies a
supervisor by directory name, FIFO, or PID.

`request(action, authorized)` checks authorization before any control I/O.
Invalid actions do not write. Failed preflight, no reader, or denied
permission means zero bytes delivered. A call never retries a partial write
that delivered bytes: it makes at most one write of one or three bytes,
retrying only EINTR when no bytes were delivered. The write is small and
atomic for a Linux FIFO, but three commands are NOT one atomic supervisor
job. EPIPE/EAGAIN does not trigger implicit retransmission.

`requested_bytes/written_bytes` describe the write attempt. They stay zero
if that phase is never reached. Messages and results are prepared before
writing so that no later allocation can hide a delivery that occurred.
A true boolean result means the full write was accepted, NOT that anyone
read or processed the commands, that a live authenticated supervisor exists,
or that the daemon changed state or is ready. `completed` is ALWAYS UNKNOWN,
even with full delivery. The [raw 35-byte record](LINUX_S6.en.md) is not
interpreted as causal confirmation.

## Identity and safety

`open()` pins service/supervise/control with CLOEXEC descriptors. The root
must be explicit and absolute, with no NUL and at most 32,768 bytes.
Directories use openat2 with NO_MAGICLINKS; ordinary symlinks are allowed.
Control must be a real FIFO, not a symlink, regular file, or device. Nothing
is created, and construction does not open the FIFO for writing. Opening a
handle can succeed without a reader, status, or lock; it does not imply that
a request can be delivered.

The handle retains boot/procfs/PID/start/UID/namespaces/eUID context. Each
request revalidates context, supervise relative to the pinned service, and
control device/inode before writing. The handle is not reused after fork
or an incompatible context change. It stays pinned to the target rather than
following a later change to the original alias or path. The host serializes
fork/setns/credentials/FDs and object lifetime; calls on the same handle are
protected by a mutex.

To write, the provider reopens ONLY its own validated FD through /proc/self/fd
with WRONLY/NONBLOCK, and verifies identity, selection, and context before
the write. A replaced FIFO does not redirect the request to another object.
No unchecked path is opened for I/O. Usable openat2 and procfs are required,
with no unsafe fallback. There is no atomic snapshot, guarantee against all
ABA changes, or reader authentication. The host must not treat the pins as
supervisor or process identity tokens.

SIGPIPE is temporarily blocked ONLY in the calling thread. If write returns
EPIPE, a newly generated signal is consumed only if none was already pending;
a preexisting pending signal is preserved, and the mask is restored. There
is no global handler or SIG_IGN change. The host coordinates concurrent SIGPIPE
generation; an external signal arriving during the window cannot always be
distinguished. The private helper accepts only a NONBLOCK WRONLY FIFO and
u/d/cru; it is not an arbitrary command-writing API.

## Evidence and remaining work

`tests/linux_s6_control.cpp` uses only private trees and FIFOs. It covers
absence without creation, invalid type/link, consent/invalid enum/no reader,
exact u/d/cru bytes with completed UNKNOWN, no down/status/lock creation,
rejection after fork, backpressure/EAGAIN without retransmission, rejection
of replaced control/supervise, EPIPE without process termination, preserved
pending SIGPIPE and restored mask, rejected RDWR descriptor, directory aliases,
and no FD leaks. It runs neither s6-supervise nor a real service. MSVC validates
only arguments and non-Linux fallback. The SDK consumer links the public
symbols. The fixture does not stand in for real s6 acceptance.

Revision 135 adds an [explicit inventory](LINUX_S6_INVENTORY.en.md) without
actionable handles or proof of live services. At this revision, K03 remains
open for observation/integration, real s6/runit acceptance, and other init
systems. Fixtures demonstrate delivery to the kernel/private reader, not
command interpretation, readiness, or real s6 liveness. The destructor only
closes descriptors; it does not send stop or undo. ServiceInfo and Monitor
autodetection are unchanged. Inventory does not turn a raw record into a
live service or a path into proof of the service manager.

Primary protocol sources:
[s6-svc](https://skarnet.org/software/s6/s6-svc.html),
[s6-supervise.c](https://raw.githubusercontent.com/skarnet/s6/master/src/supervision/s6-supervise.c),
[writectl](https://raw.githubusercontent.com/skarnet/s6/master/src/libs6/s6_svc_writectl.c).
The safety and byte-count contract above is PCInsight's own, not a copy of s6-svc.
