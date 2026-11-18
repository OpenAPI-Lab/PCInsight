# Linux per-process CPU (D04)

`ProcessInfo.cpu_percent` estimates a process's CPU time relative to the
capacity of all CPUs visible in procfs. It is not a percentage of one core
or a Windows-equivalent measure of utilization weighted by frequency.

## Calculation

```
100 * (delta utime + delta stime) / CLK_TCK / process_elapsed_seconds / visible_CPUs
```

utime and stime are kept separately. Their uint64 sum is checked, and a
decrease in either is rejected even if the total increases. In addition to
its existing limits, the parser requires a positive PID, an alphabetic state,
and NUL-free text. Guest time is already included in utime and is not added
again. Child cutime and cstime are not included.
Source and units: [proc_pid_stat fields 14 and 15](https://man7.org/linux/man-pages/man5/proc_pid_stat.5.html).

The process's steady_clock timestamp is recorded immediately after reading
and parsing stat, before RSS, status, NSS, or ELF. The rate does not divide
that row's ticks by another row's interval or Monitor's global interval.
The calculation uses long double seconds from both clock epochs to avoid
overflow from subtracting integer durations across their full range.
CLK_TCK must be positive; a failed sysconf is not replaced with 1. One
fully busy CPU on a host with 32 visible CPUs represents about 3.125% under
this convention. /proc/PID/stat already accounts for all threads in the
process, so the result is not multiplied by thread_count.

## Continuity and validation

- Two accessible observations of the same process, with matching PID and
  start_ticks and increasing time.
- Valid global proc_stat context in both samples: same schema, CPU IDs, and
  boot time including its availability; nondecreasing states and a total
  that advances. linux_cpu_interval is reused as a guard, not the numeric
  denominator.
- CLK_TCK must match between samples. Context or unit changes reset the
  baseline. An invalid global source clears all process baselines.
- Invalid, absent, or unreliable process data clears that process's baseline.
  Permission denial stays distinct. A disappeared or omitted PID is forgotten.
- The first sample, or recovery after missing data, has no percentage rather
  than a fabricated zero. Two valid samples with unchanged ticks and valid
  context do yield zero.
- The result must be finite, between 0 and 100, and marked Support::estimated.
  Out-of-range or invalid values are not clamped or converted to 100. The
  interval is discarded and a new baseline established.
- Disabling all internal process collection resets the tracker. Enabled user
  or history reports still require internal collection even when process rows
  are not returned. Hiding rows does not mean sampling stopped.
- At most 100,000 observations. PID 0, duplicate PIDs, or an oversized batch
  throw invalid_argument and reset the tracker. Native enumeration limits
  numeric candidates to 100,000 before accumulating more rows. Exceeding
  that limit aborts Monitor explicitly instead of silently truncating.

## Relationship to global CPU and history

CpuInfo.utilization_percent retains its scheduler-state formula and its
own window. D04 estimates capacity using each process's own elapsed time
and CPU count. The row sum is **not** guaranteed to equal global CPU
utilization or 100, because the windows and coverage differ. It is not IRQ
or steal time caused by each app. A decrease in global iowait can invalidate
the conservative context without establishing a kernel fault. See
[global CPU utilization](LINUX_CPU_USAGE.en.md).

Normalization does not represent cgroup quotas or affinity, hidden offline
CPUs, frequency or turbo, power, or physical performance. It does not turn
that view into a measurement of the limit assigned to a container. CPU IDs
come from proc_stat and are not certified physical inventory. Intermediate
hotplug or changes from A to B and back to A can go undetected. start_ticks
is local to the boot and namespace, not an exec generation or universal
identity. Sequential sampling, tick quantization, and read latency prevent
atomicity. Very short intervals can have no ratio because of quantization
or range. There is no overall procfs or collector deadline.

H01 update: `linux_app_history` accumulates observed duration by executable
path with an exact fractional remainder and availability. It shares the
utime/stime helper but not the percentage denominator or positive global
delta requirement. It requires valid context, boot, CLK_TCK, and executable
continuity. See [Linux CPU history](LINUX_APP_HISTORY.en.md). Legacy app_history remains empty,
and HistoryStore does not save the new report. D04 did not change layout;
H01 expands the 0.x SystemSnapshot and requires consumer rebuilds. A01 later
closed with a common view and an explicit legacy alias; see the
[availability contract](METRIC_AVAILABILITY.en.md). A05 and N03 retain
their own scope.

## Evidence

The linux-process-cpu test keeps assertions enabled in Release. It covers
0/25/50/100%, different row intervals in one batch, an individual counter
reset while the sum increases, PID reuse, missing data and permissions,
invalid sums, overflow and range, CPU/boot/schema/hz context changes,
equal/backward/extreme clock values, invalid batches, and recovery.
Mismatched PID and counter data are not attributed.

Native WSL run: test PID 1495, CLK_TCK 100, 32 visible CPUs, and a positive
ratio of 2.87677%. A separate workload interval for that process observed
12 ticks and 0.120041 seconds in CLOCK_PROCESS_CPUTIME_ID. The windows differ
and are quantized, so exact numeric equality is not required. Monitor
checks the first sample, opt-out, and resumption. The test ran only its
own brief load, with no new processes, affinity or priority changes,
containers, or other users' tasks. Hotplug, reset, reuse, and permission
cases are fixtures, not forced physical events.

Final Release regression on 2026-10-16: Linux passed 77/77 and Windows
97/97. Packages were reinstalled, and relinked external consumers exited 0.
Windows runs portable calculations and fixtures, not native procfs collection.
