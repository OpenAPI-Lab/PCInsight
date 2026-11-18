# CPU vendor and model: coverage, provenance, and SMBIOS fallback

The new `CpuInfo.reported_vendor` and `reported_model` fields are
`Metric<string>` values with availability, support, and a provenance note.
Legacy `vendor` and `model` remain and mirror the published values. If no
data is available, they are empty rather than filled with an invented name.
The new members expand CpuInfo and SystemSnapshot in the 0.x API. Rebuild
the library and all consumers, including on Windows. Do not mix layouts.

## Linux kernel text

The previous reader picked the first vendor_id/model name and imposed no
size limit. The reader now recognizes records that start with a unique
numeric uint32 `processor` ID, including sparse IDs. It considers
`vendor_id` for vendor and `model name`, `cpu model`, or `cpu` for model.
The last two also capture MIPS/PowerPC text alongside the usual x86/ARM32
format, without running utilities or using an external name database.

A string is published only if every observed processor record has the same
normalized value and source key. Duplicate fields or simultaneous aliases
within a record, incomplete coverage, and contradictory values leave that
Metric without a value. The reader does not pick the first result or hide
disagreement behind one firmware string. Vendor and model are independent:
a common vendor does not establish a common model.

The available note identifies the key and scope: text reported in visible
procfs records, not certification of physical hardware, retail model, or
uniformity among hidden CPUs. The result is not atomically correlated with
sysfs online CPUs or CPU topology, and not all hotplug is detected. A kernel
or hypervisor can supply generic or synthetic names; PCInsight does not
reinterpret them as a different brand.

Hardware, Model, machine, system type, board or SoC names, and numeric CPU
implementer or CPU part values are not converted into vendor/model text.
On ARM64 those IDs may exist without a product string; without another source
the name remains unknown. This work does not implement a MIDR catalog, CPU
device-tree lookup, RISC-V uarch/ID lookup, or universal architecture decoding.

Procfs limits: 16 MiB total, 250,000 lines, 64 KiB per line, and 4,096 CPU
records. Reads are chunked without reserving 16 MiB for every small sample.
Duplicate or invalid IDs, NUL, or exceeded limits invalidate the inventory;
partial rows are not passed off as complete. Unknown lines and fields are
ignored. Only selected keys within a CPU record supply names. The reader
does not fabricate a record from a global platform description or an
unrecognized architecture schema.

## Firmware names

If all records lack a selected textual name, or procfs cannot be read or
parsed, an independent SMBIOS source may be used and marked estimated.
If procfs reports mixed values, partial coverage, or an invalid field,
firmware is not used to hide it. Firmware notes do not claim correlation
with online CPUs in the process namespace.

The built-in parser preserves each structure's string-set. For a central
Type 4 processor in a populated socket, it extracts Manufacturer and
Processor Version using their one-based indices. Index zero, an out-of-range
index, or an invalid string leaves that field unknown. The parser does not
read strings from another structure or use Socket Designation or Processor
Family as a model. A scalar summary is available only when all reported
populated central processor structures agree and contain that field. Empty
sockets are excluded. A too-short Type 4 structure invalidates the CPU summary
so a subtotal is not published. The three exact placeholders Unknown,
Not Specified, and To Be Filled By O.E.M. are omitted. This is not a universal
filter for unknown OEM text or a guarantee of truthful, complete firmware.

SMBIOS keeps the 16 MiB table limit and checks string-set termination.
Selected strings are limited to 1,024 bytes. Existing RAM interpretation is
unchanged. Current Speed is not used as identity or Linux base frequency.
Firmware is read when Monitor is constructed. Recreate it after hardware
or firmware changes to refresh this static fallback.

If procfs has no name and firmware access is denied, the result is
permission_required, not an empty string presented as a measurement. Each
sample replaces all four fields and clears earlier values if sources are
unavailable. A missing table is not downloaded and does not require
dmidecode. Both providers rely on what the environment exposes, not the
physical BIOS of a VM's host.

## Text and Windows compatibility

Normalized CPU text must be valid UTF-8, with no overlong encodings,
surrogates, out-of-range scalar values, or ASCII/C1 controls. Spaces and
tabs are collapsed and trimmed. The reader does not transliterate, translate,
or infer a retail name. This is not a universal sanitizer for HTML, terminals,
or bidirectional text. Consumers must escape output for their GUI.

Windows fills the new Metrics from its existing CentralProcessor/0 registry
strings and marks them estimated. They represent one CPU, not proof of
uniformity across all packages. The registry reader did not change and was
not given the same validation as the Linux parser. Consumers can use the
new Metrics on both backends without losing the legacy strings.

## Tests

Fixtures cover x86/ARM32/MIPS/PowerPC text, ARM64 without a text name and with
SMBIOS fallback, sparse IDs, empty/duplicate/heterogeneous fields, partial
coverage, UTF-8, controls, limits, firmware permissions, and stale-value
clearing. Type 4 tables test indices, missing fields, disagreements, empty
sockets, placeholders, truncation at each byte, and short structures.
Non-x86 cases and positive firmware cases are fixtures, not runs on physical
ARM, MIPS, or PowerPC hardware.

On 2026-10-15, Linux Release passed 72/72 and Windows Release passed 92/92;
installation and rebuilt external consumers exited 0. The focused Linux
test reported AuthenticAMD / AMD Ryzen 9 7950X 16-Core Processor across 32 WSL
records, consistent with Monitor. Positive firmware vendor/model and
non-x86 architecture cases are fixtures only, not native validation.

Primary references: [SMBIOS 3.8, Type 4](https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.8.0.pdf),
[ARM64 cpuinfo](https://raw.githubusercontent.com/torvalds/linux/master/arch/arm64/kernel/cpuinfo.c),
[MIPS cpuinfo](https://raw.githubusercontent.com/torvalds/linux/v6.12/arch/mips/kernel/proc.c),
and [PowerPC cpuinfo](https://raw.githubusercontent.com/torvalds/linux/master/arch/powerpc/kernel/setup-common.c).
Coverage, limits, precedence, and availability are PCInsight policy.
