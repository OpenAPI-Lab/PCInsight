# Linux disk performance: continuity and numeric range

Completed work: correcting DiskInfo read, write, and transfer rates,
active_percent, and average_response_ms. Collection uses native sysfs
without subprocesses, installed utilities, device configuration, or
elevation. Public API layouts are unchanged. This does not add per-app
I/O or automatically identify physical types unknown to the earlier provider.

## What changed

The previous backend retained diskstats counters when include_disks was
disabled, but continued to advance Monitor's global clock. On reenabling,
it could divide activity from a long pause by a short interval. uint64
subtractions, multiplication by 512, and sums of bytes, operations, or
times could also overflow.

LinuxDiskRateTracker now retains only the previous valid sample per disk,
with its own steady_clock. include_disks=false clears baselines. The first
sample after reenabling publishes no rates. An empty sample or omitted
disk breaks continuity. A global fill_disks error clears the tracker
before propagating the exception. A sample rejected for exceeding a limit
or containing a duplicate identity publishes no partial results and also
clears continuity.

Name, dev (major:minor), and optional diskseq must match, time must
advance, and the ten cumulative counters used must not decrease.
in_flight is instantaneous: it may decrease without resetting the tracker.
Equal or backward time, a changed identity, or a decreasing counter
leaves all five metrics absent for that interval and establishes a new
baseline. A change in whether diskseq is present also breaks continuity.
If neither sample has diskseq, name plus dev is accepted without promising
to detect all reuse between polls.

## Source and meaning

Collection reads `/sys/block/<disk>/stat` per device rather than the combined
/proc/diskstats. dev and diskseq are read before and after stat to reject
visible changes. Missing or invalid fields do not remove DiskInfo:
inventory remains, with absent performance metrics and a note. Static
fields do not form an atomic snapshot with stat. The later
[disk properties change](LINUX_DISK_PROPERTIES.en.md) corrects visible
capacity and separates the rotation hint from physical classification.

These counters use 512-byte sectors, not the device's physical sector size.
Rates divide sector deltas by monotonic elapsed seconds and convert to
bytes/s. Conversion and sums avoid intermediate integer arithmetic that
could overflow. Results outside uint64 are absent, not saturated. Each
direction and the aggregate are validated separately. Values are rounded
down. A valid zero delta yields zero. All three rates have Support::estimated.
[Official counter semantics and units](https://docs.kernel.org/block/stat.html).

active_percent estimates delta io_ticks divided by elapsed time and is
capped at 100%. It is not hardware saturation. The kernel documents I/O
time accounting limits, especially with concurrent requests.
[I/O statistics documentation](https://docs.kernel.org/admin-guide/iostats.html).

average_response_ms is (delta read_ms + delta write_ms) divided by
(delta completed reads + delta completed writes) and is marked estimated.
It excludes discard and flush and is not end-to-end application latency.
A zero denominator means an undefined average: the metric is absent with
a note rather than a fabricated 0 ms. Concurrent request times can sum to
more than the elapsed wall-clock time.
[Request timing definitions](https://docs.kernel.org/block/stat.html).

When published, diskseq supplies a device sequence number. It does not
make reads transactional or permanently identify physical media. It can
change when a loop device's backing file is replaced.
[Official diskseq ABI](https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-block).

## Limits

The parser requires 11 to 64 unsigned decimal uint64 fields, without signs
or trailing text, and accepts up to 16,384 B of input. Metrics use the first
11 fields. Additional fields are validated but do not enter throughput or
response calculations. dev requires complete uint32 major and minor values.
The name must be nonempty, NUL-free, and at most 4,096 B. Both tracker and
inventory allow up to 100,000 rows. These are not a total RAM budget or a
syscall deadline. Text is loaded before validation; the limits do not
harden the generic reader against arbitrary files outside sysfs.

Lost activity is not reconstructed, and hidden wraps without a visible
decrease are not corrected. Name plus dev can be reused when diskseq is
inaccessible. Create another Monitor when changing the sysfs or namespace
view. The metrics cover visible devices, including virtual ones; do not
sum layers as independent physical disks. Visual equivalence with Windows
is not guaranteed.

## Verification

On 2026-10-15, Linux Release passed 60/60 and Windows Release passed 80/80;
reinstallation and rebuilt external consumers exited 0. The new
linux-disk-rates test keeps assertions enabled in Release. It covers parser,
overflow, text, length, and column handling; rates, active time and average;
valid decreasing in_flight; reset of each of the ten cumulative fields;
changed or missing name/dev/diskseq, disappearance, opt-out and reenabling;
equal, backward, and full-range clock values; valid zeros, absent average
without operations, out-of-range rates, large sums without wraparound,
duplicates, and row and identity limits.

Native Ubuntu WSL: 28 devices had available rates and active time between
samples, including measured zeros. Opt-out and reenabling were verified
through Monitor. Tests only read: no disk load was generated and no device
was modified. Windows runs portable calculation fixtures, not sysfs
collection. Physical resets and hotplug, other kernels, and other
architectures need more coverage. Reset and identity cases here use
fixtures, not actual disconnections.
