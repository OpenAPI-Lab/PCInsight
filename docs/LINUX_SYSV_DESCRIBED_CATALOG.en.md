# SysV: catalog with descriptions tied to candidates

Revision 139/K03. Opt-in extension. It does not change the revision 137
catalog, which still does not read script bodies. It does not run scripts,
a shell, a CLI, status commands, or real services.

```cpp
#include <pcinsight/linux_sysv.hpp>
auto result = pcinsight::query_linux_sysv_described_catalog(
    "/etc/init.d", "/etc", 1024, 8 * 1024 * 1024);
if (result.value) {
    const auto& view = *result.value;
    // view.metadata[i] corresponds to view.catalog.scripts[i] from THIS query.
    // Check each Metric, including short_description and description.
}
```

## Association and identity

Between the two inventory passes, each candidate is selected relative to the
init.d directory pinned with O_PATH/openat2 NO_MAGICLINKS. Before reading, its
device/inode/type/mode/size/mtime/ctime must match the candidate from the first
pass. The reader shared with the [revision 138 API](LINUX_SYSV_LSB.en.md)
reopens only its own descriptor, verifies the file is regular and the identity
matches, reads the prefix twice and requires identical bytes, then revalidates
metadata. It does not reselect a different file by name for I/O.

The name-to-object association is then revalidated within the pinned directory.
The second inventory, absolute roots, and caller context must remain consistent.
A partial or unstable catalog leaves all descriptions UNKNOWN, with the vector
aligned to the second pass's rows. Rows from different snapshots are not joined
by basename. A context change yields no result. There is no atomic snapshot,
guarantee against ABA changes, or guarantee against later updates.

Hardlinks can have valid comments in both rows. This does NOT resolve
script_index ambiguity when a reference matches multiple candidates. The
[revision 137 catalog's](LINUX_SYSV.en.md) reference association policy is unchanged.

## Availability and budget

The default budget is 8 MiB and the maximum is 16 MiB. Zero disables
description reads. Before I/O, each candidate reserves twice min(size, 64 KiB),
with no refund on error or a missing header. The sum reserved does not exceed
the budget. This is not a measure of physical bytes read or disk usage. The
reader follows catalog order; if one candidate does not fit, it is skipped and
a smaller candidate that fits may still be attempted. Descriptions are not
silently truncated. The other inventory and revision 138 parser limits still
apply.

`catalog.complete` and the result's Support describe inventory consistency,
NOT the availability of each description. `all_metadata_reads_attempted`
distinguishes an attempt for every candidate from omissions due to budget,
identity, or a partial catalog. A stable empty catalog yields true; a partial
one yields false. True does not guarantee a header, read permission, fields
being present, or a successful read. `prefix_bytes_reserved` is a conservative
reservation, including when description results are later discarded for
instability. Each Metric retains the reason for absence; permission denials
inside the reader retain their Support.

These are declared comments, not an authenticated description of a live
process, publisher, PID, enablement, effective dependency, or control capability.
The standalone revision 138 API still makes a fresh read of a path, not a token
for linking it retroactively to an old catalog. No cache, worker, persistence,
or hard I/O deadline. The host coordinates fork/setns/credentials and its FDs.

## Evidence and remaining work

`tests/linux_sysv_described.cpp` covers association by index within this query,
a different suffix, ambiguous hardlinks, missing headers, zero/insufficient/
exact budgets, a large-file prefix, a partial catalog without body reads,
file changes before pinning and after reading the pinned file, an incorrect
expected identity, root replacement, and FD counts. Hooks are private to
each call, with no global override. Private trees include a sentinel to
detect execution; an init.d FIFO is ignored. MSVC covers arguments and
fallback only, not the Linux backend. The SDK links the public symbol.

K03 remains E. This association does not yet integrate the views into Monitor
or prove control, liveness, or acceptance with a real SysV init. Revision 140
adds an [explicit ServiceInfo projection](LINUX_SYSV_SERVICES.en.md), keeping
unverified PID, liveness, and control UNKNOWN. At this checkpoint, executing
SysV scripts and the OpenRC revision 132 exception were not authorized. The
I07 observer remains unchanged.
