# End task on Linux: identity, pidfd, and confirmed exit

Functional unit: `terminate_process_checked(pid, linux_start_ticks, force)`.
The monitor exposes `ProcessInfo::linux_start_ticks` from the same
`/proc/PID/stat` read that supplies its counters, rather than reading again
and risking a different process. It is field 22, in clock ticks since
boot, not seconds, nanoseconds, or Windows FILETIME. Preserve the integer
without converting it.

```cpp
// p comes from a recent sample in the same system, boot, and PID namespace.
if (p.linux_start_ticks.value && *p.linux_start_ticks.value != 0) {
    auto result = actions->terminate_process_checked(
        p.pid, *p.linux_start_ticks.value, false); // SIGTERM, no automatic SIGKILL.
    // result.error == none means exit was confirmed, not just that a signal was accepted.
}
```

## Verified sequence

1. Reject PID 0, PID 1, the caller's own PID, numbers above INT_MAX, and a zero token.
2. Check for native procfs and verify that `/proc/self/stat` uses the caller's
   PID. A context or read failure does not permit signaling.
3. Open a pidfd with flags 0 (a process or thread-group leader, not a process group).
4. Read current identity and compare PID plus start_ticks. A mismatch returns
   stale_target without sending a signal. A denied, missing, or invalid read
   returns an explicit error.
5. Poll the pidfd without waiting. A process that already exited produces
   not_found without a signal.
6. Send SIGTERM or SIGKILL with pidfd_send_signal. Never call kill(PID) after the check.
7. Poll that same pidfd for up to 5 seconds to confirm exit. If polling times
   out or fails, the result reports signal acceptance with exit unknown or
   unconfirmed. There is no automatic SIGKILL escalation or rollback promise.

The descriptor is closed on every path after opening, including if an
exception interrupts the query. This does not reap the process with waitpid;
its parent or supervisor retains that responsibility. Confirmed exit does
not establish an exit code, functional success, or cleanup by the application.

## Identity and compatibility limits

The pidfd pins the target **from the time it is opened** and prevents later
PID reuse from redirecting the signal. start_ticks detects an identity
change between sampling and opening, but its resolution is one tick:
a recycled PID with identical start_ticks is indistinguishable using this
token. It is not globally unique, does not survive reboot, and is not valid
on another host or in another PID namespace. Stronger guarantees from
sampling time would require retaining a pidfd in the sample, which this API
does not offer. It also does not pin the executable across exec in the same
process. A real start_ticks of zero can occur very early in boot. The
monitor preserves it, but the checked action rejects it as unusable.

Linux kernel 5.3 or later is required for pidfd_open (pidfd_send_signal
has existed since 5.1), along with suitable permissions and seccomp
policy. Native syscalls use constants from headers without requiring a
new libc wrapper or command-line utility. Headers or a kernel without
support yield not_supported; seccomp can yield permission_denied or another
errno. The library never silently falls back to signaling by PID. There is
no root requirement, helper, or elevation; native signaling permissions apply.

The PID-only `terminate_process` variant also uses a pidfd and confirms
exit, but it has no sample token to protect the interval before opening.
The old implementation passed a uint32 converted to pid_t to kill.
Out-of-range values could become negative and select a process group.
They are now rejected. The change also requires pidfd for this variant;
old kernels do not keep the unsafe fallback. There is no process-tree
termination or universal critical-process exclusion beyond self and
PID 0/1. The host must enforce its own policy.

The stat read is limited to 64 KiB. The 5-second budget applies to the
confirmation poll, not the whole open/read/syscall sequence. A slow
kernel or filesystem may delay other steps. Unmounts or adversarial procfs
changes, descriptor modifications by other threads, and reuse of samples
from different contexts are outside the contract. The consumer must trust
its procfs and coordinate those operations.

## Audit and ABI

AuditedActions records the Linux token in `expected_linux_start_ticks`
for this action. The Windows field stays zero on Linux. The efficiency_checked
token remains a Windows token because that Linux backend is not implemented.
ProcessInfo and AuditEvent are expanded public 0.x structs. Rebuild the library
and consumers together; stable binary ABI is not promised.

## Verification

On 2026-10-14, Linux Release passed 17/17 and Windows Release passed 28/28.
Packages were installed, and external consumers were rebuilt and run on
both platforms. The Linux consumer checks the monitor's new token and the
Windows consumer checks that it is absent.

Injected logic covers a changed identity or PID, exit before signaling,
permission errors, ENOSYS and ESRCH, invalid reads, incomplete APIs,
timeouts, and failure after signal acceptance. It checks for no inappropriate
signal, descriptor closure, and no fallback. Native Linux uses four
temporary test children and its own readiness check. It obtains the token
from Monitor, verifies a mismatch leaves the child alive, and checks correct
SIGTERM/SIGKILL behavior. A child ignoring SIGTERM causes a timeout and
remains alive until an explicit SIGKILL. All children are reaped and no
user process is touched. The real audit includes the Linux token. On Windows
only injected logic runs; that build is not evidence of native Linux coverage.

References: [pidfd_open](https://man7.org/linux/man-pages/man2/pidfd_open.2.html),
[pidfd_send_signal](https://man7.org/linux/man-pages/man2/pidfd_send_signal.2.html),
[proc_pid_stat](https://man7.org/linux/man-pages/man5/proc_pid_stat.5.html).
