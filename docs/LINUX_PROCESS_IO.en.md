# Linux per-process disk I/O

Partial J04 update: [user I/O](LINUX_USER_IO.en.md) adds a real UID observed
around reads and at both interval endpoints to attribute rates to users.
It does not change the D02 per-process calculation. A rate can remain
known even when its user attribution is unknown.

`ProcessInfo.disk_bytes_per_second` uses the sum of separate deltas for
`read_bytes` and `write_bytes` from `/proc/PID/io`, divided by that
process's own monotonic interval. Results are integer bytes/s rounded
down, with `Support::estimated` even for observed zero. They are not
rchar/wchar or a percentage.

Each read pins the directory with O_PATH and reads stat/io/stat through
openat without following final symlinks. PID and start_ticks must match
the expected identity before and after. Files are limited to 64 KiB,
io to 10,000 lines, values to strict uint64 integers, and selected fields
must be unique. No external programs are run.

The first sample, changed identity, decrease in an individual counter,
clock without progress, rate outside uint64, or absent or denied read
yields no value, never an invented zero. Absence clears the baseline.
Omitting process, history, and user collection also resets baselines.
Each observation records its own clock after reading counters, independent
of the global clock or CPU ticks. The tracker accepts up to 100,000 rows
and rejects PID 0 or duplicates while resetting state.

## Semantics and limits

Kernel accounting is not instantaneous physical disk throughput.
write_bytes is charged when pages are dirtied, can differ from completed
writes, and does not arbitrarily subtract cancelled_write_bytes. Shared
resources and accounting for waited-for children limit attribution.
Reads are not atomic transactions. On 32-bit systems, 64-bit counters
can tear. The token does not identify exec, is not global across boots or
namespaces, and has tick resolution. There is no guarantee of consistency
across all row fields, per-device I/O, or per-app networking.
Reference: [kernel procfs documentation, section 3.3, /proc/PID/io](https://www.kernel.org/doc/html/latest/filesystems/proc.html).

## Verification on 2026-10-15

Fixtures cover fields, units, duplicates, NUL, bounds, zeros, overflow,
identity, individual resets despite a growing sum, pauses and permissions,
equal, backward, and extreme clock values, and batch validation. Native
WSL Linux test: PID 877, start_ticks 7767, read delta 0, write delta
131,072 B, and an estimated rate of 66,345,749 B/s during a write and
fsync of a private file. This is not a physical benchmark. Monitor verifies
the first and second samples and opt-out reset. No other processes,
global cache, or permissions are changed. Denial is validated synthetically.
Linux Release passed 74/74 and Windows Release passed 94/94. There is no
new public layout; installation and consumer checks were not repeated
for this work.
