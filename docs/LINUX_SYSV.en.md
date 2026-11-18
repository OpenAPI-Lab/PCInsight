# SysV: native configuration catalog

Revision 138 adds a separate [LSB comment reader](LINUX_SYSV_LSB.en.md).
It does not enable automatic content reads in this catalog.

Revision 137, K03. Public API: `pcinsight/linux_sysv.hpp`.

```cpp
auto catalog = pcinsight::query_linux_sysv_catalog("/etc/init.d", "/etc");
// For a different layout, explicitly choose /etc/rc.d as the second root.
// Check value/support, complete/diagnostics, and each field's availability.
```

The caller supplies an explicit init.d directory and a parent of rc[0-9S].d.
The API does not discover the current manager, packages, all repositories, or
namespaces. It observes eleven fixed levels, 0–9 and S. An absent rcN.d under
a valid runlevel root yields known false for directory presence. A broken or
inaccessible rcN.d symlink or a non-directory node yields partial/UNKNOWN,
never a fabricated absence. A missing root is not created: the result is
partial with unknown fields, not an empty system-wide inventory.

## Contents and meaning

- `scripts` contains regular-file and symlink candidates from init.d; hidden
  entries and direct non-file nodes are skipped. Names ending in `.sh` are
  retained; the OpenRC catalog's filter does not apply here. Bodies, headers,
  and shebangs are not read. A nonregular or broken target remains an unknown
  candidate and makes the result partial.
- `executable_mode` describes whether any execute bit is set. It does NOT
  establish effective permission, interpreter validity, a confirmed SysV
  script, an installed package, or the ability to execute.
- `runlevels` has eleven rows with directory presence and visible entries.
  Nonstandard names are retained without running them or reporting them as
  active. Ordinary directory aliases are supported.
- `convention` decodes only S/K followed by two digits and a nonempty
  suffix: start/stop, NN from 0 to 99, and the literal suffix. NUL, slash,
  and names over 255 bytes are rejected. Lowercase or unknown names yield
  UNKNOWN. A third digit belongs to the suffix, not an invented three-digit
  priority. This does not establish effective ordering.
- `symlink` and `regular_target` describe observed types, not capability or
  state. Targets are inspected using O_PATH without opening FIFOs or devices
  for I/O. Direct regular files in rcN.d are also described; not all entries
  are assumed to be symlinks.
- `script_index` associates a regular target only with a unique candidate
  matching device/inode in THIS complete catalog. Suffix and basename are
  not identities. Ambiguous hardlinks, external targets, or partial results
  yield an UNKNOWN index.

Indices, names, and paths are not execution tokens or persistent identities.
S/K names describe configuration, NOT global enablement, the current runlevel,
PID, running state, readiness, or proof of an active init. Duplicate aliases
remain visible and are not counted as separate processes. Dependencies and
dispatch/distribution policy are not evaluated; this is not equivalent to
init, update-rc.d, chkconfig, or systemd. No inferred PID is opened in `/proc`,
no pidfile is read, and no signals are sent.

## Consistency and limits

Roots must be absolute, contain no NUL, and be at most 32,768 bytes.
max_entries is 1–4,096 (default 1,024) and counts **scripts plus entries in
all levels**, not the eleven rows. Two passes use fresh directory descriptions
without shared dup offsets, sorted lists, and comparisons of target and
directory metadata. Each pass is bounded to 16,384 visits including dot/hidden
entries, 8 MiB of names, and 64 diagnostics plus final diagnostics. A truncated
subset is not guaranteed to contain the first N entries alphabetically and
is never reported as complete.

The provider uses openat2 with NO_MAGICLINKS for roots and targets, O_PATH/fstat,
and mtime/ctime/size metadata, and revalidates root and runlevel selections.
Boot/procfs/PID/start/UID/namespaces/eUID context is checked before and after;
a context change discards the result. Errors, changes, or bounds yield
complete=false/estimated, with derived fields and indices UNKNOWN. The pure
naming convention can remain known for retained names. There is no atomic
snapshot, guarantee against all ABA changes, or hard I/O deadline. The host
serializes fork/setns/credentials/FD-context changes. Usable openat2 and
procfs are required; there is no unsafe fallback.

The query does NOT execute scripts, status/start/stop/restart, shells, CLIs,
external libraries, or update-rc.d/install_initd. It creates or modifies no
links or directories, enables no apps, and evaluates no content. No worker,
timer, cache, or privilege elevation is added.

## Evidence and remaining work

`tests/linux_sysv.cpp` covers the decoder, bounds, NUL/slash, and the third
digit. Private fixtures cover missing and empty roots without creation,
eleven levels, `.sh` names and both execute-bit results, an rc3.d alias,
ordering, S/K and a different suffix with an inode match, ambiguous hardlinks,
an external target, a broken link, FIFO without blocking, rejected magic
links, a broken versus absent runlevel, partial/bounded results, and no FD
leaks. Scripts are private text and are not executed; no real init or service
is started. MSVC covers decoder, arguments, and fallback only. The SDK
consumer links the new header and symbols. This does not claim deterministic
fixtures for every race or acceptance on a real SysV init.

At this revision, K03 remains open for metadata binding, control/liveness/
integration, real acceptance, and other pending work. Revision 138 adds a
separate LSB reader; revision 139 adds an [opt-in described catalog](LINUX_SYSV_DESCRIBED_CATALOG.en.md)
tied to the pinned candidate, without unsafe name-based joins to earlier
snapshots. Replies of “continue” do not authorize running SysV scripts, nor
does the then-unauthorized OpenRC revision 132 exception. I07 is unchanged.

Primary sources reviewed:
[update-rc.d](https://manpages.debian.org/testing/init-system-helpers/update-rc.d.8.en.html)
for naming and runlevel conventions, and
[LSB Init Script Comments](https://refspecs.linuxfoundation.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/initscrcomconv.html)
to distinguish declared metadata from effective policy. This is PCInsight's
own API, not a replica of the managers or an implementation of LSB evaluation.
