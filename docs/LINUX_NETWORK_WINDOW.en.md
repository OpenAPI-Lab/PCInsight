# SELF network windows connected to the reducer

Revision 66 connects `LinuxPrivateNetworkSession` to `LinuxNetworkAccounting`
through `src/linux_network_sampler.{hpp,cpp}`. Capture, validation, and rates
work together for **one SELF actor and one declared socket-namespace cookie**.
This is internal and experimental, not an installed API, Monitor integration,
or general per-process/app/user network reporting. F03 is still in progress at
this checkpoint; H04/J04 are not closed.

Revision 67 adds a separate opt-in [SELF join with procfs](LINUX_NETWORK_SELF_JOIN.en.md).
It does not turn this into an arbitrary actor join.

## Scopes without invented conversions

The private scope contract distinguishes `namespace_inode` from
`kernel_initial`. The latter requires numeric namespace IDs of zero to
mean not applicable, not a known inode of zero. BPF helpers return TGID and
UID in the kernel's initial domain, and the scope labels them accordingly.
It never uses getpid/getuid or a procfs inode as if they translated those IDs.
It does not join procfs, usernames, or sessions.

`self_single_socket_namespace` requires exactly one row matching the creator's
TGID/start. `enumeration_complete=true` certifies **only that declared row**,
not all processes, sockets, or namespaces on the system. The four lanes represent
payload from the instrumented hooks and retain documented exclusions and limits;
they are not interface data, ACKs, or total physical traffic. Cumulative values
can include operations during sequential hook attachment and do not certify
complete startup coverage. The first rate baseline is taken only after start
with all three links active.

The scope contains a boot token observed through procfs, creator TGID/start,
and a nonzero serial epoch per capture instance. The atomic serial is shared
by samplers in this linked copy of the library and never wraps. Creator identity
prevents collisions when fork inherits the serial. It is not a global UUID
across independent library/DSO copies or a means of merging external producers.
Each sampler owns its own reducer.

The native reader opens `/proc/sys/kernel/random/boot_id` without following
the final symlink, verifies procfs with fstatfs, reads up to 38 bytes, and
accepts only a canonical lowercase hexadecimal UUID that is not all zero,
with an optional newline. It does not authenticate the host against an
administrator or container replacing procfs. The token scopes the observer;
do not promote it to proof of global host identity.

## Lifecycle and sampling

Without explicit authorization, the sampler does not query sources or load
probes. A zero cookie, incomplete API, or absent or estimated boot token
prevents startup. Capture remains limited to SELF and accepts no external
PID or global filter. It retains the three previous probes, nine-entry map,
BTF, and guards, without tools, packages, or new configuration.

Each sample reads CLOCK_MONOTONIC before and after the map read. Both points
must be exact, nonzero, and ordered; the read must not overlap the previous
valid read and can last no more than one second. Both times are retained in
the result. Rates use differences between read end times. They are
**estimated**, not atomic operation timestamps or exact physical rates.
The counter is coherent under the guards; its timestamp is not inserted
atomically by BPF into the same snapshot.

The first sample has known accumulated counters and absent rates. The next
clean interval of up to 60 s yields floor(delta*1e9/ns) for TCP send/receive
and UDP send/receive using the tested reducer. Measured idle yields zero.
The per-UID sum is always partial and estimated, with known/missing coverage:
it includes only the captured actor, not every process or session for that
UID. It is not copied to UserSession.

Absence, denial, quality failure, overflow, an invalid clock, or an exception
discards all sampler baselines. The next recovered read does not produce
a rate across the gap. The sampler does not invent a lost-event counter:
the map is direct, and observed failures invalidate the sample rather than
being replaced with zero. Uninstrumented paths remain outside this validation.

Stop is idempotent and closes capture. Restart obtains a new epoch without
inheriting rates. getpid is used **only** to recognize a copy inherited by
fork. If it differs from the local creator, sample closes the inherited
descriptors and rejects the read rather than publishing parent traffic as
the child's SELF. It does not translate IDs. The API is not thread-safe:
serialize calls and do not fork during a call. Injectable contexts are
borrowed and must outlive the sampler.

## Evidence

New fixtures in `tests/linux_network_sampler.cpp` cover UUID, opt-in,
boot/support, domains, totals/rates/UID, zero, permission, quality/overflow/
writers, exceptions, zero time, clock reversal, the one-second limit, gaps,
the 60-second limit, epoch/restart, and fork. The reducer adds rejection
tests for invalid SELF scopes, unrelated rows, and unknown enums, retaining
earlier tests, arithmetic checks, and the 10,000-case oracle.

Manual `pcinsight-linux-network-session-native --authorized-self-window`,
NOT CTest, uses prior temporary private WSL authorization. The real test
exited 0: TCP 4,096/4,096 B, UDP 256/256 B, eight excluded peeks, and all
four rates and the UID sum matched the integer oracle for the measured
646,362 ns interval. It verified idle zero, fork rejection, a clean
restart epoch/baseline, and no FD leaks. The interval is run-dependent,
not a guaranteed frequency. The watchdog was 20 seconds and socket timeout
2 seconds. No setns/unshare, retained content, daemon, or persistent objects.
Log: `work/network-window-native-linux.log`.

Linux Release regression: 117/117; nine network tests repeated 20 times
each; installed consumer rebuilt and exiting 0. The first build warned
of 0.27 s clock skew in a startup-impact dependency. The verified build
had no warnings. Logs:
`work/network-window-{build,verified-build,focused,suite,repeat,install,consumer}-linux.log`.

F03 work pending at this checkpoint: multiple actors and dynamic netns
buckets, exact identity joins with procfs/apps/users, coverage of other paths,
and public integration. SELF coherence does not certify general capture.
No public API, ABI, or history changes and no Windows progress in this unit.

Primary source for the domains:
[Linux 6.6 identity helpers](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/bpf/helpers.c).
See also [session](LINUX_NETWORK_SESSION.en.md), [cookie](LINUX_NETWORK_NAMESPACE.en.md),
and [reducer](LINUX_NETWORK_ACCOUNTING.en.md).
