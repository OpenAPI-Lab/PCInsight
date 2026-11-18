# Linux: process identity and counters

Update 161: the historical CPU calculation using global ticks below was
replaced by [D04](LINUX_PROCESS_CPU.en.md), which uses its own interval and
visible CPUs, with global context only as a guard. For fields, actions,
units, support, and a current SDK example, see the
[consumer contract](LINUX_PROCESS_REQUIREMENTS.en.md). The initial evidence
below is historical, not the current formula.

Fixes verified on 2026-10-13. The reader uses its own parser for
`/proc/PID/stat`: PID, PPID, user and kernel time, threads, and starttime.
No external utility is required. comm names with spaces, parentheses, or
line breaks are delimited by the last closing parenthesis. Truncated entries,
invalid or negative numbers, and overflowing sums are rejected.

An accumulated CPU time of zero is valid, so these processes are now kept
in the inventory. Previously zero was mistaken for a read failure and the
processes were omitted. Consecutive samples are linked by PID **and kernel
starttime**. PID reuse, a decreasing counter, or a first sample yields no
delta. A valid zero delta does yield 0%. In this historical formula, CPU
was normalized against total system ticks; a decreasing global counter or
a result above 100% was not published. Utilization was not weighted by frequency.

The same identity check governs disk deltas and CPU accumulation in app
history. Disabling processes and history clears baselines. After re-enabling
them, the first sample again has no deltas.

I/O reads require both read_bytes and write_bytes to be valid. Missing or
inaccessible reads, duplicates, and overflow do not become zero. A pair of
successfully read zeros remains valid. An inaccessible sample drops its
baseline, so two samples are required once access is restored. These counters
can include waited-for children, and their storage accounting is not the
same as syscall byte counts or instantaneous physical disk activity.

Architecture uses ELF class, byte order, and e_machine. It recognizes x86/x64,
ARM32/ARM64, RISC-V32/64, and PowerPC32/64. Unknown machines and invalid
combinations remain empty. A 64-bit binary is not assumed to be x64. This
reader identifies the header; it does not validate the entire executable.

Parser and delta tests cover zero, PID reuse, decreasing counters, special
names, truncated fields, invalid numbers, and overflow. Additional fixtures
cover absent, zero, or duplicate I/O; ARM, RISC-V, and big-endian PowerPC ELF;
invalid fields; and truncated headers. The Linux suite ran on Ubuntu WSL:
6/6. Windows also runs the portable parser, with a 13/13 suite; that does
not validate the Linux backend. Linux integration verifies that disabling
and re-enabling processes invalidates samples.

Limits at this checkpoint: `/proc` files are read separately, not in an
atomic transaction. Checking starttime across samples does not eliminate
every race within a sample or enable verified Linux process termination.
Absent I/O did not yet distinguish permission failure from process
disappearance. New architectures require extending the ELF map.
Per-process and per-user network accounting remained pending.

Additional fix on 2026-10-13: CPU panel counts use currently visible PIDs,
not the cumulative number of processes ever created. Inaccessible thread or
FD counts are marked partial or absent. See [Linux counts](LINUX_COUNTS.en.md).
After this change: Linux WSL 7/7 and Windows Release 20/20.

References: [proc_pid_stat fields](https://man7.org/linux/man-pages/man5/proc_pid_stat.5.html),
[I/O counters](https://man7.org/linux/man-pages/man5/proc_pid_io.5.html),
[ELF header](https://man7.org/linux/man-pages/man5/elf.5.html).
