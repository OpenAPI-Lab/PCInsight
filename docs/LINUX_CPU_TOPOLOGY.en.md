# Linux CPU topology: online counts without fabricated fallbacks

`CpuInfo.logical_processors`, `physical_cores`, and `sockets` now use a
built-in sysfs reader. The prior implementation counted `/proc/cpuinfo`
lines, reused a `physical id` across records, and returned as many cores
as threads plus one socket when data was missing. Those fallbacks were
removed. `/proc/cpuinfo` still provides vendor and model but no longer
determines these three counts.

## Sources and public semantics

- `logical_processors`: the number of distinct IDs in
  `/sys/devices/system/cpu/online`, not the highest ID plus one.
- `sockets`: distinct package groups represented by online CPUs, with a
  nonnegative `physical_package_id` that agrees across their online members.
  It is not an inventory of motherboard sockets and excludes packages
  represented only by offline CPUs.
- `physical_cores`: distinct core sibling groups represented by online
  CPUs and fully contained within validated package groups. It is not
  derived from thread counts or counts of numeric core IDs.

The reader prefers `topology/package_cpus_list` and
`topology/core_cpus_list`. It uses `core_siblings_list` and
`thread_siblings_list`, respectively, only when the preferred path is
absent. An unreadable or malformed modern file is not hidden by a legacy
alias. Legacy aliases are not read when the modern file is readable and
valid. `core_id` and `die_id` are not keys; they can repeat without
collapsing distinct groups. Package IDs can be sparse. The highest
ID is not used as a count. An unknown or negative ID is not a measured
socket. Zero can be a valid group ID, not a zero count.

All three present values are `Support::estimated` and carry scope notes:
they are kernel-reported topology, not an independent physical audit.
A kernel can expose default topology and a VM can expose synthetic
topology. Neither certifies SMT or physical hardware. These values are not
installed totals, process affinity, a cpuset, a cgroup quota, or guaranteed
effective CPU availability for the caller.

## Completeness, changes, and limits

Each group must contain the CPU that reports it. Groups must be disjoint
and all their online members must report the same group. Overlap between
distinct groups through offline members is also rejected. Offline members
are part of group identity, but their directories do not have to be read.
Package IDs and groups must map one-to-one. A core that crosses packages
or falls outside its package is not published as valid.

If package validation fails, sockets and cores are absent. If only core
validation fails, valid sockets remain. Missing or invalid per-CPU topology
does not turn a valid online list into zero; the logical count can still
be available. Partial sums are not published for sockets or cores.
Applying a result replaces all three fields and clears earlier values
that can no longer be validated.

`online` is checked before and after each read, and two complete matching
inventories are required. A change between them leaves all three fields
absent. This is not a kernel transaction: intermediate changes and
reversions can go undetected. These fields are not guaranteed to form
an atomic snapshot with independent CPU providers such as utilization
or caches.

Limits: 4,096 IDs per list, uint32 IDs, 64 KiB of text per file, a
nonnegative int32 package ID of at most 64 characters, and up to 1,048,576
total member references per inventory. The reader reuses the strict cache
CPU-list parser for normalization, duplicates, ranges, and overflow.
Validation works on unique groups rather than comparing every CPU's lists
against every other list. Exceeding limits yields absence, not truncation.
These limits are not syscall timeouts.

There are no subprocesses, extra packages, BIOS reads, or kernel writes.
The public layout and Windows backend are unchanged. Restricted or
missing sysfs does not trigger a fallback that invents physical counts.

## Validation

Portable fixtures cover two packages, SMT, single-thread cores, sparse IDs,
offline CPU 0, online subsets, out-of-order rows, repeated IDs, contradictory
or overlapping groups including offline overlap, incomplete coverage,
budgets, sentinels, changes between reads, and clearing prior results.
Linux file fixtures check modern paths and legacy aliases, precedence when
a modern file is invalid, missing or oversized files, and CPUs without a
topology directory. Tests do not perform hotplug or change affinity.

Final run on 2026-10-15: Linux Release passed 68/68 and Windows Release
passed 88/88. Installation and external consumers exited 0. The focused
Linux test was repeated with detailed output: online=32, core groups=16,
and package groups=1 in a stable WSL inventory, matching `Monitor::sample`.
These are values exposed by the VM, not independent physical validation
of the host's sockets or cores. Changes between reads are covered by
fixtures, not a native hotplug experiment.

Primary references: [sysfs CPU topology](https://www.kernel.org/doc/html/latest/admin-guide/cputopology.html)
and the [stable CPU ABI](https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-devices-system-cpu).
They describe the interfaces and their defaults. The completeness policy
and restrictions above are PCInsight decisions.
