# K03: internal opt-in OpenRC executor

Update 159: the [installed public API](LINUX_OPENRC_CONTROL_PUBLIC.en.md)
now provides pinned selection, per-request consent, revocation, and runner
ownership. The executor now retains that selection instead of reopening
different files at dispatch. The revision 158 contract otherwise stands;
references below to pending integration are historical. Acceptance on real
OpenRC remains pending.

Revision 158, 2026-11-02. Implementation in `src/linux_openrc_control.*`.
At this revision, it is not an installed API or a Monitor/SystemActions
action. The explicit [revision 148 exception](LINUX_OPENRC_CONTROL_DECISION.en.md)
allows scripts/runtime for control only, never for collection. There is no
installation, elevation, or use of real services.

## Contract

`dispatch_openrc_control` requires enabled and explicitly_requested. Both are
off by default; without them it returns before querying files. Only start,
stop, and restart ServiceAction values are accepted, with no options, extra
commands, or shell fallback. The ELF runtime and absolute script come from
trusted mechanism configuration; a catalog name or index grants NO authority.
It does not look up an executor through PATH or certify that an ELF is OpenRC
from its name or header.

Each directory is pinned without symlinks using the existing directory guard,
without connecting to D-Bus or loading OpenRC/librc for dispatch. Ancestors
must belong to root or the configured owner; the final directory must belong
to the configured owner. Sticky directories are rejected by default and
explicitly allowed only in private tests. Files must be regular, have the
same owner, and have no group/other write access, special bits, or multiple
hardlinks. Names are bounded and literal. The executable runtime is at most
16 MiB; the nonempty script is at most 1 MiB. Device/inode/size/mode/UID/GID/
link count/mtime/ctime are checked before and after opening and before
releasing the child. The runtime executes through the pinned FD, not by
reopening its name.

The [reviewed OpenRC runtime](https://raw.githubusercontent.com/OpenRC/openrc/master/src/openrc-run/openrc-run.c)
accepts the script in argv1, resolves its path, and uses its basename and
configuration. The original path is therefore retained rather than replaced
by an FD number. The three arguments are the runtime, script, and one literal
verb. This direct route avoids running rc-service as a discovery helper.
Compatibility with real versions and distributions remains pending; not
every implementation with that name is guaranteed to accept this contract.

The child stays gated until the parent obtains a pidfd and revalidates the
targets. A failure there prevents execution. PCInsight's post-fork code does
only async-signal-safe setup, with no allocators or callbacks of its own.
It uses libc fork: the host's pthread_atfork callbacks can run and must be
trusted not to change targets, FDs, or credentials or interfere with the
protocol. This does not isolate against hostile callbacks, handlers, or
reapers inside the host. The child uses NO_NEW_PRIVS, cwd `/`, an empty signal
mask, and default dispositions for TERM/INT/HUP/PIPE/CHLD. Parent signal
configuration is unchanged. The new environment contains only a fixed PATH,
LANG=C, and LC_ALL=C, with no inherited HOME/RC_*/LD_*. Inherited FDs,
including high-numbered ones, are closed; stdio uses a verified `/dev/null`.
Both reserved exec/error descriptors are CLOEXEC. Set-ID callers and
SIGCHLD ignored/SA_NOCLDWAIT are rejected. No target process group is created.

## Result, waiting, and ownership

The CLOEXEC channel reports errors before exec. EOF does NOT certify
execution because the child could also die. A successful response only says
the runner was released without an exec error being reported.
runtime_may_have_executed is conservative: after release there is no automatic
retry, cancellation, or undo. The ACK wait has a cooperative 2-second budget.
Syscalls, fork, filesystem access, exec, and cleanup have no hard deadline or
guarantee against a blocked kernel or filesystem.

Keep every returned child until its exit is consumed through wait, even
after an uncertain result. wait(0..30000ms) checks the pidfd and reaps with
[waitid P_PIDFD (ABI 3)](https://raw.githubusercontent.com/torvalds/linux/v6.12/include/uapi/linux/wait.h),
not a bare PID. It retains historical exit_code or signal, never current
service state, readiness, or hook/dependency success. Timeout does not
cancel. terminate_runner requires an explicit request and signals only the
runner through its pidfd, not a tree, group, or dependencies. Terminating
the runner does NOT stop the service. The destructor closes the FD; it
does NOT kill or create a hidden reaper. Abandoning a child without waiting
can leave a zombie. The host must honor this contract.

## Limits and pending acceptance

This is not a sandbox or a secure, deployed root helper. It retains process
credentials, namespace, cgroup, and resources. NO_NEW_PRIVS prevents gaining
privileges through exec but does not restrict what an already privileged caller
can do. The host must serialize forks, credential/namespace/cwd/environment/FD
changes, and external reapers. The handle rejects use from another process
or UID and serializes its own methods.

Trusted owners must serialize changes to scripts, configuration, and runtime.
A pinned FD does not freeze content; OpenRC reopens the script by path.
Revalidation is not an atomic snapshot, protection against a hostile root
or owner, or an ABA guarantee. Hooks, the installed shell and OpenRC,
configuration, and dependencies are part of the trusted base and can affect
more than one service. They are not tested here. At revision 158, public
selection/consent integration and isolated acceptance with real OpenRC were
still pending. Runit/s6/SysV retain their pending items; SysV does not receive
this exception. Real L05 deployment is deferred; this unit adds no transport.

## Private tests

`tests/linux_openrc_control.cpp` copies its OWN ELF into a protected mkdtemp
directory. This model checks argv, script readability, environment, cwd, NNP,
and FD absence, and returns codes 3/7/11 by verb. It is NOT OpenRC, does not
run service scripts, and installs no tools. Tests cover default-off, consent,
invalid verbs, sticky without permission, owner/mode/symlink/hardlink/FIFO/NUL/
invalid path, non-ELF or malformed runtime, timeout, explicit TERM/KILL of
the private child, post-fork context, an injected external reaper, FD closure,
and cleanup. Release assertions remain active, with a 20-second alarm and a
30-second CTest timeout.

Final revision 158 verification: Linux 187/187, four OpenRC tests repeated ten
times (40 executions), and ten MSVC portable-rejection runs. The installed
SDK and two rebuilt/relinked consumers exited 0. FD guards and private cleanup
passed. Logs: `work/openrc-control-*-delivery*`; final builds had no warnings.
The first build had two test errors and two code warnings, which were fixed;
initial logs were retained. An MSVC unused-parameter warning was fixed with
maybe_unused, then rebuilt and retested. An intermediate build detected mount
clock skew without changing clocks or mtimes; the delivery build was rebuilt
and verified clean. The last portable adjustment uses
`work/openrc-control-*-portable-clean.log` and
`work/openrc-control-msvc-*-delivery-clean.log`.
This is NOT remote CI or real OpenRC acceptance. K03 remains E and the official
count remains C60/E10/P3/B2 plus 3 excluded X: 60/75 = 80%, with 20% open.
