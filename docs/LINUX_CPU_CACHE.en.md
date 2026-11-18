# Linux CPU caches: shared instances, not just CPU 0

`CpuInfo.l1_bytes/l2_bytes/l3_bytes` are byte totals for distinct cache
instances exposed by sysfs and reachable by online CPUs. Separate Data
and Instruction caches are added; a cache shared by several threads is
counted once. These are neither per-thread capacities nor a simple sum of
every `index*` row.

The previous reader used only `cpu0/cache`. It could miss other cores or
sockets and failed when CPU 0 was unavailable. It now reads `online` and
`cpuN/cache/index*` for each listed CPU directly through C++ and sysfs.
It does not use lscpu, dmidecode, subprocesses, extra installations,
BIOS access, or kernel writes. No fallback fabricates values when the
kernel or hypervisor hides them.

## Identity and coverage

Each row requires `level`, `type`, `size`, and `shared_cpu_list`.
Identity is the tuple of level, type, and normalized shared CPU list,
not the `index` number or a retail name or model. The optional `id`
attribute is not required.

To publish a level:

- All online CPUs must report the same set of types for that level.
- Unified cannot be mixed with Data or Instruction at the same level.
- Only one row per CPU, level, and type is allowed. Topologies that cannot
  be represented under this conservative policy remain unknown rather
  than being summed blindly.
- Each group must contain the CPU reporting it. All online members must
  report exactly the same group and size for that type and level.
- Sizes and the sum must be positive and fit in uint64. No partial total
  is returned if a member is missing, overlaps conflict, or values disagree.

Offline members can appear in a shared list; their directories do not
have to be read. Their cache is included only if it is also reachable
from an online CPU. A wholly absent level has no value, not zero.
Coverage is validated per level, so an L1 inconsistency does not erase
an intact L2. A read or parse failure that prevents inventory discovery
invalidates the whole read. The reader does not guess which level an
unreadable file might have hidden.

Two consecutive complete inventories must match. The `online` value
must also agree at the beginning and end of each read. This detects
observable changes but is not a kernel transaction; a change followed by
a reversal can go undetected. Present values carry `Support::estimated`
and a scope note. Each call clears all three previous values before
publishing its result.

## Limits and scope

CPU lists accept decimal uint32 ranges with no duplicates or overlap,
up to 4,096 IDs and 64 KiB of text per list. Ordering is normalized.
`size` accepts a positive integer with a K suffix, converts units of
1,024 bytes, and checks for overflow. Signs, trailing text, and guessed
units are not accepted.

Limits are 64 indices and 128 visited directory entries per CPU, and
65,536 rows and 1,048,576 cumulative shared IDs per inventory. Each file
is limited to 64 KiB, with a 64-character limit on `size`. Levels 1–64
are valid, but only L1–L3 are published. Exceeding limits produces absence,
not truncation. These are work and memory limits, not syscall timeouts
or a guarantee of support for every server.

The reader does not count caches belonging exclusively to offline or
hidden CPUs and cannot discover a cache consistently omitted from all
directories. The values do not represent capacity available to a process,
cgroup quotas, affinity, or a cpuset, and do not guarantee physical
hardware facts inside a container or VM. Missing or restricted sysfs
remains absence. Heterogeneous systems without a common signature can
lack a total even if some topology is known. Public fields and layout
are unchanged.

## Verification

Portable fixtures cover two sockets, SMT, split L1, per-core L2, shared
L3, sparse IDs, unified L1, offline CPU 0, online subsets, row order,
missing levels, conflicting groups, sizes and types, duplicates, zeros,
invalid units or numbers, overflow, and limits. On Linux, a private sysfs
tree also checks complete reads, file failures, oversized values, and
clearing previous values. Release keeps assertions enabled.

Final run on 2026-10-15: Windows Release passed 87/87 and Linux Release
passed 67/67. Installation and external consumers exited 0 on both
systems. The focused Linux test was repeated with detailed output and
the final limits and also exited 0.

Native Ubuntu WSL run: a stable inventory with 32 online CPUs and 128
rows reported L1=1,048,576 B, L2=16,777,216 B, and L3=33,554,432 B, all
estimated. These are VM-exposed values, not an independent physical audit.
No hotplug, CPU configuration changes, or affinity changes were performed.

Primary references: the [kernel CPU sysfs ABI](https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-devices-system-cpu)
and [kernel cacheinfo implementation](https://raw.githubusercontent.com/torvalds/linux/master/drivers/base/cacheinfo.c)
for attributes and `size` and list formats. Completeness policy and the
estimated scope are PCInsight decisions, not additional ABI guarantees.
