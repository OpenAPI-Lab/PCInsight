# Linux process, thread, and descriptor counts

The CPU panel enumerates numeric PID directories visible in `/proc`.
It does not use `processes` from `/proc/stat`: that counter records processes
and threads created since boot, not the current population.

- `processes`: observed PID directories, including processes that are not
  runnable at that moment, such as sleeping processes or zombies. This is
  not `procs_running`.
- `threads`: sum of valid `Threads` fields from `/proc/PID/status`.
  Missing, negative, zero, duplicate, truncated, or overflowing values
  do not become a valid zero. A complete empty enumeration yields zero.
- `handles`: sum of numeric FD entries under `/proc/PID/fd` for readable
  processes. Always `estimated`: it does not count Windows objects or
  unique files. Descriptor tables shared between processes can be counted
  more than once.

The scope is the caller's procfs view. Namespaces and `hidepid` can hide
processes. `available` means a complete read of that view, not guaranteed
access to the whole host. Enumeration is not transactional: processes,
threads, and FDs can change during sampling. A disappearing entry or read
error yields an `estimated` subtotal if any reads succeeded. With no valid
reads, the result is absent (`permission_required` if access denial was
observed, `unsupported` for other errors). Overflow discards the total.
Global enumeration errors propagate to all three counters.

The reader does not silently skip permission failures with
`skip_permission_denied`. It uses iterators with `error_code`, a maximum
of one million entries per enumeration, and status files smaller than
64 KiB. Reaching a limit counts as an incomplete read. No external programs
run and privileges are not elevated. Disabling `include_processes` removes
process details but retains these CPU-panel counts.

Verification on 2026-10-13: portable fixtures covered current versus
cumulative PID counts, non-PID entries, empty and missing directories,
zero and absent FD counts, invalid fields, duplicates, limits, and overflow.
Linux tests covered permissions as UID 1000 and native Ubuntu WSL sampling.
Release suites passed 7/7 on Linux and 20/20 on Windows. The Windows fixtures
test only the portable reader, not the Linux backend.

Reference: [kernel procfs documentation](https://www.kernel.org/doc/html/latest/filesystems/proc.html).
