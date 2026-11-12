# Controlled pre-exec gate for future launch attribution (internal)

Checkpoint120,2026-10-29. Internal linux_gated_launch.hpp/cpp adds
launch_linux_gated(request, explicit_authorization, arm_callback). This is a REAL
Linux launch primitive and arming barrier, NOT a successful-exec/app/autostart
receipt. No new public action/vtable/SDK header replaces SystemActions.launch_process.
General Run New Task behavior remains unchanged. No library construction starts it.

## Order and stable objects

1. Validate explicit consent, absolute executable/cwd, bounded literal args/env,
   no elevated/split caller credentials. No PATH search or shell/script fallback.
2. openat2 O_PATH|CLOEXEC/NO_MAGICLINKS pins the target without opening a device/
   FIFO for IO. Require regular/non-setid; reopen ONLY our own pinned fd for a
   four-byte ELF-magic precheck and metadata comparison. Kernel still validates
   the actual executable. Pin is an object reference, NOT a content hash/signature.
3. Prepare child buffers/channels/results before clone. clone3 CLONE_PIDFD creates
   a direct child AND stable pidfd atomically. The child blocks on a private gate,
   sets NoNewPrivs and cannot enter the requested program before parent acceptance.
4. Parent validates boot/full caller and selected pidfd/procfs child identity,
   then calls arm_callback with BORROWED pidfd/O_PATH executable fd, full namespace
   identities and pinned executable stamp. Duplicate handles for later retention;
   originals expire on return. Do not close/reuse them or reap this gated child.
5. Callback success is followed by context/identity/credential/file revalidation.
   Only then does parent release the gate; child uses execveat AT_EMPTY_PATH on
   the pinned fd, not a fresh path lookup. Ordinary symlink retargeting cannot
   replace the selected object. MSG_NOSIGNAL avoids parent SIGPIPE on a dead gate.

Reject/callback error/exception before release cancels this private gated child
using its pidfd and reaps it. A reported pre-exec error is likewise reaped.
After release, a successful/no-error or unknown-channel result has a nonzero PID:
caller owns waitpid, no hidden reaper or post-exec kill. Unknown error-channel
state does NOT trigger a guess by bare PID. Descriptors are CLOEXEC/RAII-owned.

## Signals, privileges and cooperation

Caller credentials must remain unchanged. Child NNP is set without changing
parent NNP; current privileges are NOT dropped (root still launches as root).
Inherited environment/non-CLOEXEC descriptors/dynamic loader remain trusted inputs.
This is not a process sandbox, untrusted-host boundary or executable-content proof.

Raw kernel signal masking brackets clone and restores the parent before callback.
The current implementation requires the8-byte kernel mask ABI; unsupported ABI
fails before child creation. Child pre-exec path uses raw syscalls and prebuilt
buffers, no C++ allocation/iostream/environment lookup/libc cancellation or atfork
callbacks. Original child mask is restored after release, immediately before exec.
Signals/handlers may act after release; SIGKILL/SIGSTOP remain outside masking.
Host must coordinate credentials/environment/cwd, external reapers, fd lifecycle,
forks and calling-thread cancellation. Native filesystem/exec/channel operations
and arbitrary arming callbacks can block; no universal wall-time bound promised.

No fallback to unchecked fork/PID signaling on missing clone3/execveat/openat2/
pidfd facilities. No privilege elevation, external collection utility, startup
registration or host/login/service activation. Scripts/Terminal/DBus routing and
execute-only unreadable binaries are not silently approximated here.

## Evidence and next required work

Normal Release CTest launches ONLY its own helper ELF into a private directory.
It verifies pre-exec marker absence/identity/arming, literal args/cwd, child NNP,
ordinary symlink replacement still executing the pinned image, denied/throwing
arming (no program execution), pidfd cancellation of a stopped child, missing cwd,
real invalid-ELF exec failure, magic-link/script/FIFO rejection, reaping, FD count
and restored parent NNP/signal mask. No eBPF runs in this CTest.

Manual --authorized-private-gated additionally performs FOUR temporary selected
pidfd kernel-identity queries from the callback WHILE each private child is gated.
Each matches the boot/caller/local identity and authenticates TGID/kernel start-
boottime BEFORE release. Queries remove owned probe references on return; final
FD census remains exact. Test helper's post-exec fstat independently verifies the
pinned inode for positive cases. Unsupported platforms exercise a no-child
fallback contract, NOT positive launch coverage; this WSL run was POSITIVE.

The ordinary error-pipe EOF means ONLY no pre-exec error was reported: premature
death could also close it. It MUST NOT certify successful exec, app identity,
desktop-entry causality or automatic startup. No GIO marker/parent/bare PID guess
or caller claim is promoted to available startup origin. This component enables
the next stage: observe successful kernel exec on the authenticated selected child,
compare it with the pinned request and validate actual desktop-entry launch routing.
Trusted managed-autostart cause and other reapers/descendants remain I07 work.

Revision121 adds the separate [selected successful-exec provider](STARTUP_SELECTED_EXEC.md)
for that first observation and pinned-object comparison. The gate alone still
does not certify exec; validated desktop routing and managed cause remain open.

Evidence: work/startup-gated-launch-*. Initial GCC indentation warning corrected
with final clean build. No startup entries/apps outside private fixtures touched;
no archive/public layout change or GUI. N03 remains open. Final verification:
Linux148/148 without skips+20 focal runs; manual4 selected kernel joins before
release (two positive execs/two reported failures) and exact FD cleanup; installed
SDK/consumer exit0. MSVC10 unsupported/denial fallback runs, not Windows launch
support. All native positive checks passed on this WSL kernel, not universal Linux.
