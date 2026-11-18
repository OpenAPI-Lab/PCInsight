# Linux uptime: bounded reads and validated integer conversion

`CpuInfo.uptime` remains a `Metric<std::chrono::seconds>` obtained from
the first field of `/proc/uptime`. The previous `stod` followed by a direct
integer conversion was removed. It could accept an incomplete numeric
prefix, throw, or convert nonfinite or out-of-range values without proper
validation.

The new provider does not run `uptime`, `cat`, or other utilities,
download packages, or change clocks. It reads at most 256 bytes and
validates exactly two nonnegative decimal fields with two fractional
digits, as emitted by procfs. Whitespace around and between fields is
allowed. Extra fields, signs, exponents, NaN/Inf, NUL, incomplete decimals,
and suffixes are rejected.

Each integer part is parsed as uint64; the first must also fit in
`chrono::seconds::rep`. There is no multiplication by 100 or conversion
to nanoseconds or double, so the largest representable value and its
fractions avoid intermediate rounding and overflow. The fraction is
validated and discarded. 1.99 means 1 whole second, not 2. Both `0.00`
and `0.99` produce a valid zero with `Support::available`.

The second field is validated but is not used as uptime or subtracted
from the first. Idle time accumulated across CPUs can legitimately
exceed uptime. No idle <= uptime constraint is imposed, and no CPU
percentage is derived from these fields.

A missing or unreadable file, read failure, exceeded size, or invalid
format or range leaves the Metric without a value and with a note.
The provider neither invents zero nor retains the last valid sample.
The conversion does not throw for invalid numeric text. This does not
rule out resource exceptions such as bad_alloc elsewhere in the program.
A stream error without a reliable errno is not enough to infer
permission_required.

## Scope

This is the boot time duration the kernel exposes to the current
environment, including suspend and possible time namespace offsets.
It is not the lifetime of Monitor, a user session, or a process, and
does not establish when the physical host outside a VM or container
powered on. The API reports whole seconds, not hundredths or a civil
boot date and time. CLOCK_MONOTONIC is not used as a substitute because
its suspend semantics differ.

The provider is stateless. It does not hide a reboot or change of view
by forcing monotonicity with a historical maximum. The nondecreasing
check covers consecutive reads in the same stable WSL environment, not
a guarantee across reboots, VM restores, namespace changes, or altered
procfs sources. No fallback is implemented here if procfs is absent.
The read is bounded in bytes, not by a syscall timeout. The public
layout and Windows backend are unchanged.

## Verification

Portable fixtures cover zero, fractional truncation, idle greater than
uptime, the seconds::rep maximum, uint64 overflow, negatives, NaN/Inf,
trailing text, truncated records, and field and file limits. Linux
fixtures cover a private file that is missing, a directory, empty,
invalid, or oversized; an exact accepted size; and clearing prior
values. Release keeps assertions enabled.

The native test reads `/proc/uptime`, takes two Monitor samples, and
reads procfs again. When CLOCK_BOOTTIME is available, a pair of clock
reads brackets the sequence in whole seconds. This is read-only:
it does not force suspend/resume, reboot, or namespace or clock changes.
The installed consumer checks that uptime is present and nonnegative
on both hosts.

Run on 2026-10-15: Linux Release passed 70/70 and Windows Release passed
90/90; installation and external consumers exited 0. The focused Linux
test was repeated with detailed output: 52 seconds of uptime in WSL,
within the CLOCK_BOOTTIME range 52–52. The proc/Monitor/Monitor/proc
sequence was nondecreasing. This is the value from that run, not current
uptime or a measurement of when the Windows host powered on.

Primary sources: the [kernel proc uptime implementation](https://raw.githubusercontent.com/torvalds/linux/master/fs/proc/uptime.c),
[procfs documentation](https://www.kernel.org/doc/html/latest/filesystems/proc.html),
and the [Linux clock_gettime manual](https://man7.org/linux/man-pages/man2/clock_gettime.2.html).
Validation and limits are PCInsight policy, not extensions of kernel guarantees.
