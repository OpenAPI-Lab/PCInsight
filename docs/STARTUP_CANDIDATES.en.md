# Process candidates from launch metadata

Implemented on 2026-10-14: discover_startup_process_candidates(snapshot)
in startup_candidates.hpp discovers Linux candidates using the two markers
documented by [GIO AppInfo.launch](https://docs.gtk.org/gio/vfunc.AppInfo.launch.html).
It does not depend on GIO or run its launcher or helper. It reads
GIO_LAUNCHED_DESKTOP_FILE and GIO_LAUNCHED_DESKTOP_FILE_PID only through procfs.

```cpp
pcinsight::SampleOptions options;
options.include_processes = options.include_startup_entries = true;
auto snapshot = monitor.sample(options);
auto report = pcinsight::discover_startup_process_candidates(snapshot);
// Check confidence, origin, and complete before accepting an association.
// observer.register_process(candidate.startup_entry_id, candidate.baseline)
// is an explicit caller decision and does not happen automatically.
```

The marker PID must match the process. Children that inherit the variable
are ignored, so their I/O is not added again. The absolute .desktop path
must exactly match an effective ID supplied by the snapshot. The provider
does not compare name or comm, parse Exec, or substitute a basename.
Aliases and lexically equivalent paths are not inferred. XDG precedence
is revalidated before and after. A shadowed snapshot entry or changed
inventory yields stale_target and no candidates. A system source is not
relabeled as personal.

The provider requires genuine procfs and a namespace compatible with the
caller. It opens the /proc/PID descriptor, checks that the directory UID
matches geteuid before and after, and checks PID and start_ticks against
the snapshot's ProcessInfo on both sides of the environment read. It
does not read other UIDs' environments, even when the caller is root.
It copies only the matching evidence path and the baseline from the
impact core's identified counter reader. An inaccessible environment,
a disappeared or changed process, or a missing token yields unavailable
and complete=false. A different UID or nonmatching evidence yields ignored.
Other variables are not emitted, restored, or logged, and environment
secrets are not exposed.

Duplicate markers, malformed or out-of-range PIDs, relative or oversized
paths, and environments without a terminating NUL yield unavailable, not
a fabricated association. Missing one or both markers means no evidence,
not an error or proof that the app did not launch. The provider does not
look for other launch-system paths when markers are absent.

association_support is **always estimated**. automatic_startup_origin
remains unknown or unsupported. A manual launch through the same .desktop
can have these markers, and a process can create or change them. They are
declared metadata, not authenticated evidence of app identity, cause,
or autostart, or identity that is global and resistant to every tick-level
race. The provider does not authorize destructive actions, validate the
file version or contents at launch, or act as a privileged helper.
Observer and inventory are not updated silently, and Monitor.impact
remains not_measured. A currently disabled entry does not establish
previous launch causation either.

This is opt-in: the caller must have requested process and startup data
in the snapshot. Empty lists do not establish global absence. complete
means all supplied inputs were evaluated within limits, not coverage of
the entire session or boot. An error of none can coexist with
unavailable>0 or complete=false; inspect both. Limits: 100,000 objects
per input list, 4,096 visited PIDs, 32 KiB per ID, and 8 MiB of supplied
IDs. Environments are limited to 64 KiB per process and an 8 MiB total
read budget, including failed or oversized reads, plus one excess-probe
byte. stat is bounded to 64 KiB. XDG inventory retains its own limits.
There is no worker or universal procfs/filesystem deadline. The caller
must coordinate its contexts and environments.

Linux Release passed 30/30 and Windows Release passed 41/41. New headers
were installed and external consumers ran. Windows tests the portable
parser and unsupported behavior, not a native equivalent. Linux uses
three private C++ children that exec the same test with markers prepared
before exec, and private XDG files under /tmp. It verifies a direct
exact match, ignored inherited PID markers, ignored shadowed system
paths, rejected stale snapshots or tokens, and a candidate explicitly
accepted by the observer without falsely completing impact. The children
are closed and reaped without executing .desktop Exec, a GIO helper, or
a real app. Parser tests cover limits, duplicates, PIDs, paths, NUL, and
discarded unrelated variables. Actual automatic launches across distros
were not validated; GIO markers are not universal.

Complete automatic impact still needs reliable startup-cause evidence,
early and short-lived observation, other sources such as cgroups or
session managers, aggregation without double counting, persistence, and
conservative inventory integration.

The later [explicit overlay](STARTUP_OVERLAY.en.md) links results.
Candidates remain estimated, are not registered or applied automatically,
and do not prove causation.

[Startup store](STARTUP_STORE.en.md) adds explicit persistence of closed
captures as historical data. It does not implement an early recorder or
certify the candidates.
