# L05 — absolute anchor and owned native connection

Revision 152, 2026-11-02. Internal unit implemented and tested. This does
NOT certify secure process or loader bootstrap or install or activate a
helper. L05 remains E.

src/linux_privileged_connection.hpp/.cpp adds PrivilegedDirectory and
PrivilegedBusConnection with RAII ownership and a receiver bound to that
connection. It does not invoke external programs, elevate UID, or obtain a
D-Bus address from the environment, a default bus, or a connection inherited
from the caller.

## Configuration anchor

open_privileged_directory opens from `/` one component at a time using
O_DIRECTORY/O_NOFOLLOW/O_CLOEXEC. It accepts a lexical absolute path up to
4,096 bytes and 32 components from a restricted ASCII set. It rejects
`.`/`..`, empty components, trailing slash, spaces, symlinks, and expansions.
It retains every descriptor and checks device/inode, UID/GID/mode, and
the link to the parent. Ancestors must belong to root or the configured
trusted UID. The final directory must belong to the configured UID, without
group/other write bits or special bits.

Writable ancestors are rejected by default even with the sticky bit. Tests
under /tmp explicitly set allow_sticky_ancestors, allowing sticky on a
trusted owner's ancestor, never on the final protected directory. Do NOT
enable that exception implicitly in deployment. No host permission changes.
The [revision 151 reader](LINUX_PRIVILEGED_POLICY.en.md) can borrow the
anchor FD; the caller must not close it.

The anchor records PID/EUID and mount/user namespace inodes of the current
THREAD through /proc/thread-self/ns and revalidates them on use. It does not
use /proc/self/ns, which could observe the leader instead. Missing procfs or
namespaces causes rejection, not assumed context. This unit did not run
unshare/setns or verify real namespace transitions. It is not a transaction
against hostile root or filesystem behavior.

## Explicit connection

The endpoint is restricted to an anchor, a simple socket name, an expected
socket-node UID, an expected SO_PEERCRED UID, and a nonzero timeout of at most
five seconds. The two UIDs mean different things: the node owner need not
be the UID recorded by the kernel for the peer. Neither is discovered or
accepted from the payload. It requires a filesystem-namespace socket with
S_ISSOCK, verified ownership, and one link. Abstract sockets, symlinks, and
address-injection syntax are rejected. The full Unix path is at most 107
bytes. The socket mode may allow broad connection access (for example, 666);
directories protect the path, and credentials authenticate the peer.

The connector creates a NEW sd_bus with the exact unix:path address, client
mode, anonymous=0, negotiate_fds=0, interactive_authorization=0,
exit_on_disconnect=0, and an explicit timeout. It completes Hello and
requires ready state and a unique name, a Unix stream socket, valid peer PID
and configured UID through SO_PEERCRED, the actual pathname, and a CLOEXEC
descriptor. It revalidates the path and process before publishing. There is
no broker or Authority discovery or autolaunch, helper name request,
registration, permanent loop, or automatic reconnect.

Each successful connection gets a monotonically increasing per-process
generation without wrap. Explicit close and destruction use close+unref
without flushing; creation failure also cleans up. A change to process,
EUID, namespaces, anchor, socket node, or readiness prevents publishing a
handle and persistently revokes the connection, even if permissions are later
restored. Reconnecting requires a new object. identity_config derives the
endpoint, peer UID, and connection generation; the policy generation comes
from the revision 151 store.

receive_owned_privileged_request requires that the message belong to exactly
that connection. It delegates to the managed revision 151 receiver and checks
connection, anchor, and generation again before returning the observation.
A detected change yields UNKNOWN/stale even if Authority replied positively.
This is not a ticket or executor. Revalidation and serialization immediately
before a future action are still needed.

Serialize connection use, getters, handlers, the loop, and close. Do NOT close
with a pending handler or operation or retain a raw pointer beyond its
owner. Do not reuse objects or handles after fork/exec. A forked child
must go to exec/_exit without destroying inherited buses. PID guards do not
make a bus shared after fork safe. This does not promise protection against
malicious privileged C++ code in the same process.

## Revision 152 tests

tests/linux_privileged_connection.cpp checks paths, ownership, mode,
FD_CLOEXEC, default sticky rejection, explicit sticky allowance for test
ancestors only, symlink rejection, detected directory replacement, a
non-socket node, address injection, invalid timeout, anchor loss and recovery,
and no FD leaks. MSVC covers arguments and fallback only, without simulating
Linux syscalls.

tests/linux_privileged_receiver.cpp now uses the owned connector for the REAL
receiver rather than its test sd-bus constructor. The private broker and
libsystemd are REAL; Authority is SIMULATED. It checks a second connection
with a higher generation, close invalidating the handle and generation,
rejection of incorrect node and peer UIDs, and disabled FD negotiation,
interaction, and exit-on-disconnect. It retains all 21 revision 151 cases
and adds a directory permission change during a positive reply: the result
is UNKNOWN/stale and the connection remains revoked after permissions are
restored. Each run has 22 requests and nine checks. The parent removes the
broker and private files. No host bus, polkitd, real services, or new privileges.

Linux 180/180 plus 100 focused L05 runs (ten tests repeated ten times,
including ten broker runs); 20 MSVC argument/fallback runs; and two
reinstalled and relinked SDK consumers exiting 0. Private FD checks and
cleanup passed. Two initial indentation warnings were fixed and preserved in
work/privileged-connection-build.log. Verified and final builds were clean.
Other logs: work/privileged-connection-*. No new public API, layout, or format.

## Remaining scope

Update 153: internal [restricted execution](LINUX_PRIVILEGED_EXECUTION.en.md)
is OFF by default, with a new ControlSystemService method separate from
authorization-only, and tests with a simulated Manager. The revision 152
results above are historical. No real acceptance or deployment. Uncertainty
after dispatch and lack of atomicity are documented explicitly.

The process and executable, environment, loader, and dependencies must be
trusted BEFORE using this code. dlopen(libsystemd.so.0) is not a hardened
loader and does not sanitize LD_PRELOAD/LD_LIBRARY_PATH already processed.
UIDs and paths remain mechanism configuration, not a secure installer or
real polkit policy. No watcher, aggregate handshake/query/I/O deadline,
global queue or size limits, or guaranteed remote cancellation. This unit
does not establish real root/helper/polkit acceptance or human consent,
or include an executor, installation, activation, elevation, or remote CI.

Next at this checkpoint: bounds, a restricted execution flow, and hardened
deployment before real acceptance. Do not install, activate, or elevate
without specific authorization. OpenRC revision 148 stays limited to explicit
control; SysV/K03/I07/UAC are unchanged. Official count: C60/E10/P3/B2, with
three X excluded; 60/75 = 80% closed and 15/75 = 20% open.

Primary sources: [sd-bus ABI](https://raw.githubusercontent.com/systemd/systemd/main/src/systemd/sd-bus.h),
[Unix sockets](https://man7.org/linux/man-pages/man7/unix.7.html),
[sticky/unlink](https://man7.org/linux/man-pages/man2/unlink.2.html),
[namespace identity](https://man7.org/linux/man-pages/man7/namespaces.7.html),
and [proc/thread-self](https://man7.org/linux/man-pages/man5/proc_thread-self.5.html).
Restrictions, generations, and revocation are PCInsight contracts.
