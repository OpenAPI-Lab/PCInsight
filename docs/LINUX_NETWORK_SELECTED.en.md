# Private capture of a selected actor

Revision 69 adds [identity from a pidfd](LINUX_NETWORK_PIDFD.en.md), with a private
test in which the child does not disclose its kernel token. The revision 68
declared-token contract below does not become global discovery or association.

Revision 68 extends the internal session component, without an installed API
or Monitor integration. `start_selected(authorized, kernel_identity, socket_netns_cookie)`
loads the three existing probes for **one** initial-domain TGID/generation/UID
and one socket namespace cookie. It does not capture all processes or
enumerate actors.

## Contract and separation from SELF

The caller declares the token: positive TGID no greater than INT32_MAX, nonzero
start_boottime, exact uint32 UID (zero is valid), and nonzero cookie. Missing
authorization or invalid input makes no capture calls. Missing or estimated
BTF, hook, or namespace support prevents loading. The kernel still applies
the early TGID filter and generation, UID, and cookie guards for each operation
with a positive result.

**Accepting the token neither authenticates its origin nor verifies that a
procfs PID corresponds to that TGID or that the actor exists and is alive.**
A coherent fresh map with zero means no operations were counted in the declared
scope, not that the process exists or is alive. A provider using this path
must obtain and revalidate a trusted token and check liveness separately.
A local PID, NStgid, or start_ticks is not automatically converted to this
token. TEST_RUN does not look up other PIDs, and uid_map is not inferred.

`PrivateNetworkSessionRead.target_kind` distinguishes `self` from
`selected_actor`. The existing SELF method keeps local discovery; selected
capture does not use that discovery as a substitute for the target. The SELF
sampler checks the kind and never reports a selected actor as its own creator.
The session's read_caller_identity requires active SELF. The extracted internal
unattached discovery function always reads the **caller**, never the selected
actor.

The lifecycle is shared: load everything before attachment, roll back on
failure, clean up on destruction, detach while retaining the map for guarded
reads, and stop idempotently. Restart requires stop, which clears the previous
kind and identity; old counters are not reassigned. No pinning, daemon,
configuration, shell, or external application dependency. A selected session
does not automatically stop when its target dies; the owner must remove it.
The test does so after confirming exit. Its map is not presented as evidence
of a live process.

## Native test: cooperative private child

The session executable's manual `--authorized-private-child-session` mode
is **not CTest**. It uses prior temporary WSL authorization scoped to private
test processes. The parent creates its own child and pipes before any
attachment. The child performs only unattached SELF discovery, obtains its
procfs token and SO_NETNS_COOKIE, and sends those data to the parent. It waits
for commands before generating IPv4/IPv6 TCP/UDP loopback traffic through
send/sendto/sendmsg.

The parent checks that the reported local PID matches the child it created
with fork and keeps a pidfd for liveness polling. This **does not** show that
the pidfd authenticates the reported TGID/start or implement a secure IPC
attestation protocol. The child is known, private, cooperative code. No process
outside the test is queried or instrumented, and no application content is
exchanged.

Verified on WSL with exit 0: child local PID 2075, kernel TGID 2600, initial
UID 0, TCP 4,096/4,096 B, UDP 256/256 B, and eight excluded peeks. Identical
parent traffic does not change the child's map. New child traffic after
detach does not change the counters. A deliberately different start token
or UID produces sticky quality failure and absence, not a fabricated zero.
The test observes exit through pidfd, removes the session, and compares
initial and final FD counts. On failure, a private child guard cleans up and
reaps. Parent and child watchdogs are 20 s and 15 s, with 2 s socket timeouts.
SIGPIPE/SIGCHLD policy is local to the test executable, not the library or
system. No files or namespaces are created.
Evidence: `work/network-selected-native-linux.log`.

Extended fixtures cover opt-in and format checks without calls, eight failure
and exception points with rollback, map FD 0, metadata and kind, rejection of
selected capture masquerading as SELF, and restart into the previous mode.
The native joined SELF test also exited 0 on repetition: four rates and UID
against the oracle, idle, fork, restart, and no FD leaks.

At this checkpoint F03 remains E: arbitrary actor discovery and revalidation,
trusted procfs/app/user joins, dynamic multi-actor and netns buckets, additional
coverage, and public exposure are still pending. Selected capture is not
integrated into the SELF reducer or history, does not populate total network
fields in ProcessInfo/UserSession, and does not close H04/J04. Windows remains
deferred.

Linux Release regression: 119/119 without skips; 11 network tests repeated 20
times each; installed consumer rebuilt and exiting 0. No build warnings.
Logs: `work/network-selected-{build,focused,suite,repeat,install,consumer}-linux.log`
and `work/network-selected-self-regression-linux.log`. No Windows builds or
captures were run. An initial patch did not apply because of context, leaving
no partial changes; it was reapplied with less context before building.
