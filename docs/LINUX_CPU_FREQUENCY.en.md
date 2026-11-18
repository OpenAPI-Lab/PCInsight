# Linux CPU frequencies: current and base are not interchangeable

`CpuInfo.current_ghz` and `base_ghz` now use a built-in, bounded cpufreq
reader. It no longer calls `stod` on unvalidated text, averages only CPUs
that happened to respond, or reads base frequency from CPU 0 alone. SMBIOS
Current Speed is no longer reported as base frequency. The SMBIOS parser
retains that internal field, but this provider does not substitute it for
base_frequency. Maximum frequencies, BIOS limits, and CPU retail names
are not used to invent a base frequency either.

## Current speed

The reader enumerates CPUs in `/sys/devices/system/cpu/online`, including
sparse IDs, without assuming CPU 0 exists or is online. Each CPU has three
separate sources under `cpuN/cpufreq`: `cpuinfo_cur_freq`,
`scaling_cur_freq`, and `base_frequency`.

For `current_ghz`, complete `cpuinfo_cur_freq` coverage is preferred.
If it is not available for every online CPU, complete `scaling_cur_freq`
coverage is accepted, with an explicit note that it is a driver value
and may represent a requested P-state rather than the actual hardware
clock. Sources are never mixed across CPUs to fill gaps. If neither
source covers the full set, the average is absent, not partial or zero.

The value is the arithmetic mean per online logical processor, in GHz.
A shared policy is observed through each associated CPU, so its weight
is the number of online CPUs, not one per policy or physical core. This
is not an application's effective frequency, utilization percentage,
work-weighted average, turbo maximum, or simultaneous measurement of
all CPUs. Even readings from the same policy can vary during the sweep.

Present values carry `Support::estimated`, with notes identifying the
source and aggregation. Values are summed in long double before division,
avoiding uint64 sum overflow. The result is checked for finiteness and
positivity. The reader imposes no arbitrary range for commercial CPUs;
valid formatting does not certify the hardware.

## Base speed

`base_ghz` requires a valid, identical `base_frequency` across all online
CPUs, with agreement between two consecutive reads per CPU. Heterogeneous
base frequencies, missing sources, and configuration changes do not
produce an average or the first CPU's base frequency. A missing base
frequency does not clear a valid current speed. A present base frequency
is marked estimated and does not constitute direct BIOS verification or
confirmation of the manufacturer's retail specification.

Base frequency is configuration data, not a dynamic clock measurement.
It does not change merely because `current_ghz` rises or falls. Sampling
reads it again rather than freezing stale data after a profile or hardware
change. No permanent cache guarantees one value for the lifetime of
Monitor. A changed configuration can yield a new base frequency once
it stabilizes.

## Stability and limits

`online` is checked before and after each sweep. Two inventories must have
the same online set and valid, unique rows. Current values can change
between reads: the second complete sample is used. Requiring dynamic
clocks to match would be incorrect. Base frequency does require agreement.
The first inventory validates structure, the CPU set, and base frequency;
it is not an interval used to calculate a frequency rate.

This does not block hotplug or policy/driver changes, is not an atomic
snapshot, and cannot detect every change followed by a reversal. It does
not verify persistent policy identity or prove that CPUs or policies
remain unchanged between syscalls. Hidden or offline CPUs, affinity,
cpusets, and cgroup quotas are outside the average. Each result replaces
both fields and clears earlier values that are no longer valid.

The parser accepts positive decimal uint64 values in kHz with optional
surrounding whitespace. It rejects signs, fractions, exponents, extra units,
NaN/Inf, NUL, overflow, and trailing text. Frequency files are limited to
64 characters. The online list is limited to 64 KiB and 4,096 IDs, with
normalized ranges and no duplicates. Reads are bounded. Per-file errors
yield missing values; an invalid or changing online list invalidates the
whole inventory. These are work limits, not syscall timeouts.

There are no utilities, extra packages, direct register/MSR/BIOS access,
or writes to governors, turbo settings, or frequencies. If the VM or
driver does not expose cpufreq, the fields remain absent. This work does
not implement other sources such as ACPI CPPC nominal_freq or effective
frequency telemetry based on cycles. The public layout and Windows
implementation are unchanged.

## Verification

Portable fixtures cover numeric limits, GHz conversion, sums above uint64,
sparse online sets, duplicate and missing rows, valid clock changes,
complete-source selection, heterogeneous or changing base frequencies,
and clearing previous values. On Linux, a private sysfs tree with CPU 0
offline and a shared policy symlink checks per-CPU weighting, malformed,
oversized, or missing files, CPUs without cpufreq, and rejection of maximum
frequency as base. Release keeps assertions enabled.

Verification on 2026-10-15: Linux Release passed 69/69 and Windows Release
passed 89/89, with installation and external consumers exiting 0. The
focused Linux test was repeated with detailed output. It found 32 online
CPUs and no current or base frequency in WSL without those sources,
verifying absence end to end. No load, turbo, or governor changes were
forced. Positive native readings on physical Linux hardware remain
pending; fixtures verified positive values and their conversions.

Primary sources: [kernel CPUFreq interface and semantics](https://www.kernel.org/doc/html/latest/admin-guide/pm/cpufreq.html)
and [base_frequency in Intel Speed Select](https://www.kernel.org/doc/html/latest/admin-guide/pm/intel-speed-select.html).
Complete coverage, aggregation, and absence when base frequencies
disagree are PCInsight policies, not additional kernel ABI guarantees.
