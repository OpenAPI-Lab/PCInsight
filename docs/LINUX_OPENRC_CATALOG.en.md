# OpenRC: explicit init.d/runlevels catalog

Revision 131, part of K03. New public function in `pcinsight/linux_openrc.hpp`:

```cpp
auto catalog = pcinsight::query_linux_openrc_catalog(
    "/etc/init.d", "/etc/runlevels");
// Inspect value, complete, diagnostics, and the individual Metrics.
```

One query covers **one** init.d directory and **one** runlevels directory,
both chosen by the caller. It does not discover librc paths, environment
variables, per-user configuration, precedence rules, local directories,
or services started outside that scope. For multiple directories, the host
must query them separately and decide how to show conflicts. The API does
not simulate OpenRC's effective name resolution.

## Returned data

- `scripts`: regular-file and symlink candidates in init.d, excluding hidden
  names and `.sh` files. Ordinary links are resolved only through O_PATH/fstat.
  The provider never reads their bodies, runs scripts, opens FIFOs or devices
  for I/O, or invokes a shell, librc, or CLI. `executable_mode` means an
  execute bit is set, not effective permission, interpreter validity, an
  OpenRC header, or launch capability. A broken link or nonregular target
  is retained with UNKNOWN metadata and makes the inventory incomplete.
- `runlevels`: real directories directly under the selected root. Top-level
  symlinks are not treated as supported aliases and make the inventory partial.
- `references`: links configured directly in each level, excluding hidden
  names and `.sh` files. A regular target has the shape of a service reference.
  `script_index` is available only if **one unique** candidate in this
  complete catalog matches the target's device/inode. The name is not used
  to fabricate an association. If two candidates are hardlinks, neither is
  picked arbitrarily. An external regular target retains the reference
  without an invented local index.
- `stacked_level`: a directory link is associated only with a level in this
  catalog by device/inode. It is a direct edge: no recursive expansion,
  computed inheritance, dependency ordering, or causal inference. Cycles
  and self-links do not trigger infinite traversal. External or ambiguous
  targets remain unassociated. Broken or non-link references yield UNKNOWN
  and a partial inventory, never known absence.

Indices are valid only within their own result. They are NOT persistent
identities or authorization to execute. `complete` means a consistent,
bounded inventory of the selected scope, not that OpenRC is installed
or active. An empty result does not prove there are no services on the
system. This describes filesystem configuration, NOT boot enablement, the
current runlevel, running state/PID, readiness, or descriptions. Even raw
`single` contents are not reported as effective configuration: OpenRC has
special rules that this catalog does not execute.

## Consistency, limits, and safety

Roots must be explicit and absolute, at most 32,768 bytes, and contain no
NUL. max_scripts is 1–4,096 (default 1,024). Up to 256 levels and 16,384
references are supported. Each pass has a global budget of 16,384 visits,
8 MiB of names, and 64 diagnostics, plus final diagnostics. Two passes
use fresh directory descriptions to avoid reusing dup offsets. Sorted
lists and target/directory/root metadata are compared; path selection and
observer context (boot/procfs/PID/start/UID/namespaces/eUID) are revalidated.

openat2 is required and magic links are disallowed. Levels are opened with
beneath/no-symlinks constraints. Configuration links can point outside the
supplied roots but are inspected only through O_PATH. This does not
authenticate configuration or the supervisor, provide an atomic snapshot,
detect all ABA changes, or impose a hard I/O timeout. The host serializes
fork/setns/credential changes. A missing or inaccessible root, changes,
errors, or exceeded bounds yield `complete=false`, estimated support,
and UNKNOWN derived associations and metadata while retaining observed
names. A context change discards the whole result. The query does not
create directories or files.

## Validation and remaining work

The private native `/tmp` fixture covers missing roots without creation,
empty results, ordering, both execute-bit results, hidden and `.sh` files,
directories/FIFOs ignored in init.d, inode-based alias matching, cyclic
stacks, ambiguous hardlinks, external targets, broken and magic links,
nonblocking handling of a FIFO in a level, rejection of a level alias,
bounds, repetitions, and no FD leaks. It does not run scripts or OpenRC.
MSVC covers only the non-Linux fallback. The SDK consumer verifies the new
header and symbols. This does not claim real OpenRC acceptance or tests for
every possible race.

This remains separate from the [marker reader](LINUX_OPENRC.en.md). Names
are not joined to infer liveness or causation. At this revision, K03 remains
open for control/PID/liveness, integration, and real OpenRC acceptance;
real runit testing and s6/SysV remain pending at this checkpoint. Do not
write a state marker to pretend to control a service.

Revision 143 adds [explicit ServiceInfo views](LINUX_OPENRC_SERVICES.en.md):
candidates and markers appear in separate lists with distinct IDs, with no
name-based join or interpretation of markers as liveness. Both sources and
their respective errors are retained.

Primary sources reviewed:
[librc.c](https://raw.githubusercontent.com/OpenRC/openrc/master/src/librc/librc.c)
for enumeration, resolution, and stacks, and the
[public header](https://raw.githubusercontent.com/OpenRC/openrc/master/src/librc/rc.h.in).
This is PCInsight's own catalog API, not an equivalent reimplementation of librc.
