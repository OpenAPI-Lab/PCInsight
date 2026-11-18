# Linux CPU utilization: validated intervals

This work improves the robustness of CpuInfo.utilization_percent and the
global denominator originally used by ProcessInfo.cpu_percent. It adds no
public API or layout, external utilities, writes, elevation, or affinity
or priority changes. It does not close static model/vendor/topology,
frequency, memory, or app history work.

Update on 2026-10-16: D04 replaces the numeric per-process CPU denominator
with the process's own clock and normalized visible CPU capacity.
linux_cpu_interval remains a context guard, not a tick divisor for rows
sampled at a different instant. See LINUX_PROCESS_CPU.md. The rest of this
document preserves the evidence and design of the original global CPU work;
it does not describe a reversal of D04.

## Reading and parsing

Previously, the reader accepted whatever numeric prefix it could parse from
the cpu line, summed without overflow checks, and subtracted idle without
checking for decreases. It now reads /proc/stat with a 16 MiB limit and
validates the following:

- Exactly one aggregate cpu line, before the other fields.
- Between 4 and 10 complete decimal uint64 counters, with no sign or trailing
  text. cpuN lines must have the same number of columns and unique uint32 IDs.
- At least one visible CPU and at most 100,000. IDs are sorted before samples
  are compared; a different row order does not imply a different CPU set.
- Optional btime must be a complete uint64 and must not appear twice.

The first four fields support older layouts. Missing optional fields are
represented internally as zero, not as readings of fields that are present.
A schema change between samples invalidates the interval. An incomplete
prefix of a known row schema or nonnumeric columns is not accepted. New
schemas with more than 10 fields require a provider update.

CPU and btime lines are limited to 64 KiB each. Other /proc/stat lines are
ignored subject to the total limit; the parser does not validate unrelated
statistics. Open, read, limit, and parse failures return missing counters,
never a partial sample. An invalid CPU read immediately clears its baseline
in Monitor. A failed stream does not identify a specific permission problem.

## Interval and percentage

Compare two valid samples with matching schema, visible CPU set, and btime,
including whether btime is present. Each of the eight aggregate states used
must be nondecreasing. A decrease in any state, a visible CPU, boot, or
schema change, or a sum of deltas outside uint64 leaves the percentage
absent with a note. A total that did not advance is also absence, not a
measured 0%. The baseline is reestablished. After an invalid read, two
valid readings are needed to recover a percentage. Invalid intervals
are not interpolated.

The calculation sums **state deltas**, not absolute totals. An absolute
total outside uint64 does not prevent use of a small representable interval.
Each addition is checked. Idle is the sum of the idle and iowait deltas,
a subset of the total. The subtraction total minus idle is bounded by
construction; neither wrapping nor clamping hides an inconsistency.
The percentage is `100 * (total - idle) / total`, computed with a long double
intermediate and published in the range 0–100 as Support::estimated. A valid
interval containing only idle and iowait yields 0; only busy states yields 100.

The states are user, nice, system, idle, iowait, irq, softirq, and steal.
Guest and guest_nice are not counted twice. The formula treats iowait as
nonbusy and steal as part of the remainder. That is a scheduler accounting
convention, not a measurement of physical execution or frequency-weighted
utilization. The kernel documents that iowait can decrease. Rejecting that
interval does not establish a kernel or hardware fault.
[Official proc_stat fields and iowait uncertainty](https://man7.org/linux/man-pages/man5/proc_stat.5.html).

## Processes and integration

Since D04, linux_cpu_interval is a context guard for per-process CPU, not its
numeric denominator. Those percentages are not published without a valid
global interval. The ratio uses its own clock, CLK_TCK, and the capacity
of all visible CPUs, not one core or a cgroup quota. See LINUX_PROCESS_CPU.md.

Since H01, the separate linux_app_history report accumulates utime and
stime duration with executable identity and preserved fractions. It requires
valid context, boot, and units, but not a positive global delta. A decrease
in global iowait alone does not discard valid process CPU intervals. It is
not a percentage and does not use the global denominator. See
[Linux CPU history](LINUX_APP_HISTORY.en.md); legacy app_history remains empty.

Hiding processes does not stop global CPU sampling. Reenabling processes
requires new baselines without reusing previously hidden vectors. Disks
and network retain independent clocks and providers. Failures elsewhere in
Monitor retain their previous contracts; this work does not make sampling
transactional across the whole snapshot.

Visible IDs and btime are not a global host identity and do not detect all
intermediate hotplug or reboots. CPU rows are not an atomic hardware read.
The procfs view may differ from the cgroup quota or CPUs accessible through
affinity. There is no adjustment for namespaces, frequency, or power limits.
These are data limits, not a total RAM budget or a procfs deadline.

## Verification

### Intermittent failure diagnostics, 2026-10-16 (partial N03)

Historical linux-integration failures did not retain the rejection reason,
so they do not establish an iowait decrease. The calculation and its guard
now share one evaluation that preserves the same rules. The note for an
absent metric distinguishes:

- Missing current counters (the exact read or parse failure is unknown) or
  a missing prior baseline (first sample or reset baseline).
- Invalid previous or current counters, or a change in schema, CPUs, or boot token.
- The first decreasing state, with its name and previous/current uint64 values.
- Sum-of-deltas overflow, with the state where it happens, or lack of progress.

Notes are human diagnostics, not stable codes to parse. These reasons are
not used to infer permissions or an actual host reboot. If several issues
exist, the first in validation order is reported. There is no new public
layout or API, automatic library logging, or extra system read.
linux-integration prints the note from the same failing Monitor before its
original assertion. It does not infer a cause from parallel reads, remove
the assertion, or retry until favorable.

Fixtures verify each reason, all eight decreasing states and their values,
and that no percentage is produced or absence support changed. Valid
0/50/75/100 calculations and recovery remain covered. Windows runs these
portable fixtures, not a native Linux provider.

In this bounded run, all 100 linux-integration repetitions passed. The
historical failure **did not reproduce**. This validates the normal path,
not the cause or a fix for the intermittency. More precise evidence can now
be collected if it recurs. N03 remains open. This instrumentation does not
create a closed subtask or enlarge the denominator. The complete Release
suites passed on Linux (83/83) and Windows (103/103). Packages were reinstalled,
and relinked external consumers exited 0.

On 2026-10-15, Linux Release passed 65/65 and Windows Release passed 85/85.
Packages were reinstalled and rebuilt external consumers exited 0. The new
linux-cpu-usage test keeps assertions enabled and covers parsing, schemas,
duplicates, columns, ranges, limits, no double-counted guest time, decreases
in every state including iowait, boot/ID/schema changes, 0/100, no-progress
intervals, large absolute sums with valid deltas, delta-sum overflow, and
baseline loss and recovery.

Native Ubuntu WSL: 32 visible CPUs. The recorded run reported 3.52113%
globally and 3.52113% for the test process itself. Only the test ran a brief
loop; no other processes were launched or modified. Missing first samples
and process opt-out and reenabling were verified. Windows runs portable
calculation fixtures, not a native Linux procfs read. Real hotplug/reset,
denied procfs reads, and real decreasing iowait were not induced; fixtures
cover them. Other kernels, architectures, and C libraries need further
coverage. This does not establish equivalence with Task Manager.
