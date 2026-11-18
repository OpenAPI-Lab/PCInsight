# Explicit persistence for observed Linux history

Revision 99, 2026-10-24. `pcinsight/linux_history_store.hpp` provides
`LinuxHistoryStore::{load,save,merge,retain_since,prune,reset}` for
`LinuxAppHistoryEntry` rows. It is the persistence component of H04, NOT a
network collector by itself. Since revision 100, F03 feeds the separate
[`linux_network_app_history` report](LINUX_NETWORK_APP_HISTORY.en.md).
CPU and network use independent periods and files. H02, legacy HistoryStore,
and WindowsHistoryStore are unchanged.

It preserves the optional value, `Support`, and note for CPU, network, and
notifications. CPU is in nanoseconds, network in accumulated bytes, and
notifications in counts, not rates. A known zero stays zero; an absent value
stays absent. Do not save an absent outer report as an empty vector: first
check `metric_is_known(snapshot.linux_app_history)`. Only ROWS are stored, not
the outer Metric's Support/note or F03 snapshots, brackets, pidfds, or baselines.
Persistence does not reconstruct earlier resource usage, identities, or permissions.

## C++20 usage

```cpp
#include <pcinsight/linux_history_store.hpp>
#include <stdexcept>

pcinsight::HistoryIoResult save_report(
    const std::filesystem::path& path,
    const pcinsight::Metric<std::vector<pcinsight::LinuxAppHistoryEntry>>& report)
{
    if (!pcinsight::metric_is_known(report))
        return {false, "Report absent: do not replace with empty history"};
    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    if (ec) return {false, ec.message()};
    pcinsight::LinuxHistoryLoadResult previous;
    if (exists) {
        previous = pcinsight::LinuxHistoryStore::load(path);
        if (!previous) return {false, previous.error};
    }
    try {
        const auto merged = pcinsight::LinuxHistoryStore::merge(
            previous.entries, *report.value);
        return pcinsight::LinuxHistoryStore::save(path, merged);
    } catch (const std::invalid_argument& e) {
        return {false, e.what()};
    }
}
```

The caller chooses the file, timing, and retention and serializes load/merge/save.
Monitor and Sampler do not write; there is no hidden maintenance, CLI, daemon,
or new dependency. The GUI can include the header and link the library from
the IDE during development; no consumer CMake setup is needed ahead of time.

## Identity and cumulative merging

The key is `(id, period_start)` with times normalized to milliseconds.
Snapshots from the same period REPLACE counters; they are not added. Older
snapshots are ignored. An equally recent or newer snapshot cannot lower a
counter or discard a known value. Fields from conflicting rows are not
combined to imply broader coverage. `merge` throws invalid_argument without
changing its inputs; save/load return an error. `save` replaces the validated
document; it does NOT merge or verify progression against the previous file.
For cumulative updates, use the example's load/merge/save sequence under a
caller-owned lock.

Each input to merge/save and each file must contain unique keys; two starts in
the same millisecond collide. `merge` also normalizes its output timestamps.
Different periods remain separate. When a counter resets, start a new period
rather than continuing the same one with a smaller value. Concurrent or
independent captures must not share an id/period pair or a file without
coordination. Do not mix CPU and network from separate collectors just because
paths match: the network report has an independent period and its own brackets.

## File format and safety

Linux-specific v1 format: 8-byte magic `PCILHST\0`, u32 version/count, rows
with length-prefixed strings, u64 wall times in ms, and metrics encoded as
support/presence/optional value/note. Integers are little-endian. A 64-bit FNV-1a
trailer covers the whole body to detect accidental corruption; it is NOT a
signature or encryption. No automatic conversion from legacy or Windows or
promotion of their raw counters is performed.

Limits: 64 MiB per file, 100,000 rows, and 1 MiB per string or note. Invalid
checksum/version/flags, truncation/trailing data, support inconsistent with
presence, negative or out-of-range CPU, backwards times, and duplicate keys
are rejected. A failed load exposes no partial rows. Save refuses to replace
an existing corrupt or foreign-format destination. It uses an exclusive
sibling temporary file, flush/close, and replacement through the shared H02
primitive. Failure before replacement preserves the destination and leaves
unrelated backups untouched. There is no guarantee of power-loss durability
or Linux directory metadata fsync; it depends on the filesystem. Concurrent
saves are last-writer-wins, so the caller must lock.

Retention keeps COMPLETE periods ending at or after cutoff; it does not
prorate bytes. `prune` validates before replacing; errors or corruption preserve
the file. `reset` removes ONLY the selected file, returns errors, and accepts
absence. It is explicitly destructive and does not check the format before
deleting. It unlinks the directory entry without following a symlink and
rejects even empty directories, with no recursive removal or separate racy
type check. No network content is retained, but paths, names, and times can
also be private: callers are responsible for destination, permissions, and
encryption.

## Verification

The Release fixture keeps checks enabled. It covers round trips for presence,
support, notes, zero, CPU nanoseconds, and uint64_max network bytes; 50 replays
without double counting; stale snapshots; decreases and lost known metrics;
distinct periods; ms normalization; duplicate keys even with a valid checksum;
every truncation of a sample file; corruption, flags, and bounds; preservation
of Windows/legacy formats, a directory destination, and unrelated backups;
temporary cleanup; and prune/reset. On Linux it also round-trips the REAL Monitor
CPU report with network/notifications still absent; it does NOT capture eBPF.
Final results are in the TXT and logs under work/linux-history-store-*.
No existing layouts or CPU semantics were changed.

Revision 99 results: Linux 134/134 without skips plus 30 focused runs (three
stores ten times each). The installed consumer includes and links the new
header/API and exits 0. All 20 focused MSVC runs outside the sandbox (new and
old stores ten times each) pass. Inside the sandbox, both fail temporary file
replacement with AccessDenied 5. No Windows code was changed and no checks
were relaxed. Failure logs are retained: the Linux fixture reused a file
corrupted by another case (now uses a separate native.bin); the chrono fixture
changed from 100 ns to 1 µs representable by both clocks; and sandbox replacement
was denied. No new C++ warnings; D9025 expected. N03 for the older Windows
consumer remains open and is NOT considered fixed. H04 moves from P to E,
not C: 77.3% complete/22.7% remaining, with no partial credit. The TXT is
the official tracker. Revision 99 mirror confirmed: comment 90140272938633;
remote history was left intact.
