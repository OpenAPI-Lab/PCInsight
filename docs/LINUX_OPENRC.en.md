# OpenRC: native state marker reader

Revision 131 also adds the separate [init.d/runlevels catalog](LINUX_OPENRC_CATALOG.en.md).
It does not establish control, effective state, or acceptance on a real system.

Revision 130, part of K03. Public API: `pcinsight/linux_openrc.hpp`.

```cpp
auto inventory = pcinsight::query_linux_openrc_markers("/run/openrc");
if (!inventory.value) { /* show support/note; do not invent zero */ }
else {
    // Check complete, diagnostics, and per-entry availability.
    // entries describes markers, NOT all installed services or live PIDs.
}
```

The caller explicitly selects a trusted runtime tree. The provider does not
autodetect OpenRC or change Monitor or the systemd provider. The query is
read-only through syscalls. It does not run rc-status, rc-service, a shell,
scripts, or librc, and installs no dependencies. It does not create missing
roots or categories or choose a tree through environment variables. The host
serializes fork/setns/credential changes.

Seven categories are observed: started, stopping, starting, inactive, failed,
hotplugged, and wasinactive. The internal mask uses bits 1, 2, 4, 8, 16, 32,
and 64 respectively. These are **NOT librc RC_SERVICE bits**. Only symlink
entries are accepted. Their targets are not read, opened, validated, or
followed, so a marker with a broken target can appear. A regular file,
directory, FIFO, or device is rejected without opening it, makes the
inventory incomplete, and leaves derived states unknown. Hidden names are
skipped. Categories must be real directories, not links. Ordinary links
are allowed for the root, but magic links are not. openat2 is required;
there is no unsafe fallback. The path does not authenticate OpenRC.

`marked_state` describes only an unambiguous primary marker among the
first four categories. If all are absent, there is a conflict, bits are
unrecognized, or the query is partial or unstable, it remains UNKNOWN.
`decode_linux_openrc_markers` exposes this pure rule without querying the
system. Supplementary flags remain in the mask; they do not prove process
failure or readiness.

**A started marker does NOT establish that the process is running.** There
is no verified PID or process identity, uptime, description, Windows group,
or scheduled/crashed classification. Absence does not imply stopped:
stopped or unmarked apps are outside this inventory's scope. An empty
result means no markers were observed, not zero installed services, and
does not prove OpenRC is installed or active.

Two passes over directory descriptors compare name/mask sets, category
metadata, and root selection. Boot/procfs/PID/start/UID/namespaces/eUID
context is checked before and after. A context change discards the result.
Tree changes, errors, and bounds retain raw observations with
`complete=false`, estimated support, and UNKNOWN derived states. This is
not an atomic snapshot, does not detect every ABA change or change between
samples, and sets no hard I/O deadline. Do not present these observations
as the rc_service_state API.

Per-pass limits: 16,384 visited entries including hidden and dot entries,
a requested maximum of 1–4,096 names (default 1,024), and an 8 MiB name
payload. Two passes bound the work and diagnostics. Results are sorted by
name; truncated or errored results are never reported as complete. A
missing category is observed as empty; an inaccessible or non-directory
category is not equivalent to an empty one.

## Evidence and remaining work

`tests/linux_openrc.cpp` covers the decoder's four states, flags, conflicts,
and unknown cases; arguments and bounds; a missing root without creation;
an empty root without changes; broken and hidden symlinks; ordering and flags;
reported limits; file/FIFO rejection without blocking; rejection of
symlink/FIFO categories and magic links; and no FD leaks. The data and tree
are PRIVATE under /tmp. The tests do not run OpenRC or touch real services.
MSVC checks pure contracts and non-Linux fallback only, not new Windows
support.

At this revision, K03 remains open. This reader does not implement installed
service/runlevel inventory, PID/liveness, real OpenRC control, or integration,
and is not acceptance on a real OpenRC system. Real runit testing and
s6/SysV adapters are still pending at this checkpoint. Do not write a
marker to simulate starting or stopping a service.

## Primary sources reviewed

The [librc code](https://raw.githubusercontent.com/OpenRC/openrc/master/src/librc/librc.c)
creates markers as symlinks and checks additional conditions when querying
state. Its description lookup uses a shell, so it is NOT invoked here.
Some librc open operations also create directories; this observer does not
replicate that behavior. The
[public header](https://raw.githubusercontent.com/OpenRC/openrc/master/src/librc/rc.h.in)
defines states, flags, and the Linux runtime location. Categories were
cross-checked against the
[internal header](https://raw.githubusercontent.com/OpenRC/openrc/master/src/librc/librc.h).
The implementation is not copied, and API equivalence is not assumed.
