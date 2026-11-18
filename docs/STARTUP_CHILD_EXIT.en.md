# Final sampling of unreaped children

read_startup_child_exit_sample(pid,expected_linux_start_ticks) can
recover a final sample when a child of the caller exits between polls.
It does not run utilities, install components, or alter SIGCHLD. It
requires Linux 5.4 or later (pidfd_open and waitid P_PIDFD), genuine
procfs in the same PID namespace, and a child the consumer can still
wait for. Windows returns not_supported.

The result's bool indicates a valid operation, not exit. If exited=false,
the child has no observable exit and sample, exit_code, and
termination_signal are empty. If exited=true and the operation succeeded,
sample contains final counters and either exit_code or termination_signal
describes the exit. Errors return no sample; exited can remain true if
exit was observed before a later failure. Missing metrics do not become
fabricated zeros.

The provider opens a pidfd, validates PID, start_ticks, and context with
the counter reader, queries WEXITED|WNOHANG|WNOWAIT, obtains the final
sample, and rechecks exit through that same pidfd. It neither waits by
bare PID nor consumes the exit status. A wrong PID or token is rejected.
A non-child, kernel without the API, or already-reaped child does not
become success or use an unsafe fallback.

The caller must coordinate waitpid, supervisors, and SIGCHLD handlers.
Another thread can reap the child during checks, and the sample is rejected
if that is detected. WNOWAIT does not block other reapers or give exclusive
ownership. SIG_IGN or SA_NOCLDWAIT can prevent retaining the zombie.
After sampling, the caller must still reap its child. PCInsight does not
automatically leave zombies or assume that responsibility.

CPU and I/O use the procfs counters documented in
[startup impact](STARTUP_IMPACT.en.md): quantized utime+stime and
read_bytes+write_bytes when accessible. They are not mixed with wait4
rusage, I/O blocks are not converted to bytes, and descendant times are
not added. Permission denial and missing counters remain unknown.

## Applying the final sample to the observer

```cpp
// Requires an earlier baseline explicitly associated with the observer.
auto result = pcinsight::read_startup_child_exit_sample(pid, start_ticks);
if (result && result.exited && result.sample)
    observer.finish_process(*result.sample);
// The caller's supervisor calls waitpid(pid, ...) in its own flow.
```

finish_process validates and applies the last sample and closes **only**
that identity. Other records remain open. An invalid sample is not applied
and does not close the record. It does not perform I/O, check exit, or
authenticate sample provenance. A closed identity does not accept another
final sample or count twice. Short or gapped coverage still cannot
establish low or medium; observed high can remain a lower bound.
The explicit overlay can then be used.

This fills the gap between baseline and exit for unreaped children, not
universal short-lived capture. Without a baseline, when another program
was the parent, or after disappearance, lost CPU and I/O cannot be
reconstructed. It does not prove automatic launch or observe descendants.
The supervisor must take the final sample **before** reaping or globally
closing the observer. This neither creates an autostart launcher nor
enables any user startup file.

## Tests

A private Linux C++ fixture covers a blocked child, a baseline, 50 ms
of CPU, exit code 7 before the second sample, final and repeated
sampling, finishing one identity, and subsequent waitpid with the
same exit code. It also covers SIGTERM exit, no capture after reaping,
rejection of a wrong token or non-child, and invalid input. No real
apps are used. Portable tests cover individual closure with other
rows open, invalid chronology, no double counting, global closure,
and the Windows unsupported contract. The installed consumer calls
both APIs with invalid inputs without creating processes.

Verified on 2026-10-14: Linux Release passed 32/32 and Windows Release
passed 43/43. Installation, rebuild, and external consumer execution
were verified on both systems.

Sources: [waitid, WNOWAIT, and P_PIDFD](https://man7.org/linux/man-pages/man2/waitpid.2.html)
and [pidfd_open and child retention conditions](https://man7.org/linux/man-pages/man2/pidfd_open.2.html).
