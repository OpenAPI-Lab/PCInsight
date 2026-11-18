# DRM engine classification by driver

classify_linux_drm_engine(driver, engine) returns Metric<LinuxDrmEngineClass>.
It is a pure query in linux_drm.hpp. The table can be used without Linux
and does not load drivers, perform I/O, or discover hardware. Raw reads
and aggregates include engine_class, also visible in snapshot.linux_drm
when Monitor integration is enabled. The native name is unchanged.

The category describes an engine class or capability, **not** the type
of work it is executing or an exclusive 3D measurement. The percentage
still represents observed client activity; classification does not make
it global GPU utilization.

## Exact mappings

| Driver | Native name | LinuxDrmEngineClass |
|---|---|---|
| amdgpu | gfx | graphics_compute |
| amdgpu | compute | compute |
| amdgpu | dma | copy |
| amdgpu | dec | video_decode |
| amdgpu | enc, enc_1 | video_encode |
| amdgpu | jpeg | jpeg |
| amdgpu | vpe | video_processing |
| i915 | render | graphics_compute |
| i915 | copy | copy |
| i915 | compute | compute |
| i915 | video | video_codec |
| i915 | video-enhance | video_processing |
| xe | rcs | graphics_compute |
| xe | bcs | copy |
| xe | ccs | compute |
| xe | vcs | video_codec |
| xe | vecs | video_processing |

Matching is exact and case-sensitive. It does not infer prefixes, suffixes,
or numbers, or normalize whitespace. Empty inputs or inputs over 128
bytes, unknown drivers, undocumented variants, and Xe other yield no
value with Support::unsupported. Aliases such as sdma, dma0, or compute0
are not invented, and raw counters are not discarded if the parser
accepts them. NVIDIA and nouveau do not inherit Intel or AMD categories
merely because names match.

A known mapping has Support::available and a provenance and scope note.
That is availability of the taxonomy, not evidence that the machine
has the engine. Aggregates recalculate the class from driver and name
instead of trusting categories manually supplied by consumers.

Xe normally reports busy and total cycles. The [cycle provider](LINUX_DRM_CYCLES.en.md) implements
that basis without converting to nanoseconds. Classification remains
independent of rates.

## Verification and limits

The tests cover 18 exact pairs, including classification, synthetic
nanosecond parsing, aggregation on first and second samples, preservation
of name and rate, and rejection of tampered classes. Negative cases
cover similar names or drivers, spaces, case, NUL, limits, unknown names,
and a cycles-only Xe record. A Monitor fixture verifies graphics_compute
without touching GpuInfo.utilization_percent or its global engines.

There is no runtime database or SDK installation or download. Research
was checked against kernel sources, and driver code is not redistributed.
The expanded 0.x API requires rebuilding the library and consumers
together. Positive physical AMD/Intel and NVIDIA validation, new
taxonomies, and global hardware coverage remain pending.
The [maximum-frequency provider](LINUX_DRM_MAXFREQ.en.md) adds the cycles
and maximum-frequency alternative.
Linux Release passed 47/47 and Windows Release passed 60/60. Packages
were installed and rebuilt external consumers ran against the public
classification API.

Sources:

- [AMDGPU fdinfo, Linux v6.12](https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/gpu/drm/amd/amdgpu/amdgpu_fdinfo.c).
- [AMDGPU block semantics](https://www.kernel.org/doc/html/v6.10/gpu/amdgpu/driver-core.html).
- [Intel classes and i915 fdinfo](https://www.kernel.org/doc/html/latest/gpu/i915.html).
- [i915 uabi_class_names table](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux/+/69050f8d6d075dc01af7a5f2f550a8067510366f/drivers/gpu/drm/i915/i915_drm_client.c).
- [Xe statistics names](https://www.kernel.org/doc/html/latest/gpu/xe/xe-drm-usage-stats.html).
