# Updating selected network capture while Sampler runs

Revision 77 adds Sampler.update_linux_process_network_selection. It updates
the EXISTING capture without stopping the worker or clearing CPU/memory charts.
It inherits the full list of 1–16 processes, one declared cookie per process,
identity and permission checks, partial coverage, and
[Monitor's fail-closed policy](LINUX_PROCESS_NETWORK_UPDATE.en.md).
There is no automatic discovery.

Revision 79 adds [bucket-mode setup](LINUX_PROCESS_NETWORK_BUCKET_MONITOR.en.md)
through a third argument. Update preserves session mode; to change it, stop
and repeat setup. Two-argument calls stay strict, even after stopping buckets.

```cpp
pcinsight::SampleOptions options;
options.include_linux_process_network = true;
pcinsight::Sampler background(std::chrono::seconds(1), 0, options);
auto started = background.start_linux_process_network(consent, selection);
if (!pcinsight::metric_is_known(started)) return;
background.start();
auto changed = background.update_linux_process_network_selection(consent,
                                                                new_selection);
// On success, wait for samples with selection_epoch == changed.value.
// The first sample in that epoch has no rates; do not connect series across epochs.
background.stop();
```

This is synchronous. It retains no span or borrowed pidfds after returning.
Retained rows use owned pidfds; additions need valid pidfds throughout the
call. Callers and the worker must share a descriptor table, as ordinary
threads do. Do not unshare it or close/replace FDs owned by the library.
Do not use an inherited Sampler/jthread after fork or move or destroy the
object concurrently with its methods. start/stop/update are serialized;
history/latest/running can be read concurrently.

## Ordering and availability

Lock order is lifecycle → Monitor → history. Capture or sampling and
publication finish under the same Monitor lock, so update cannot return
before the preceding sample is committed. The worker does not acquire the
lifecycle lock or need it to finish. Stop does not wait for the worker while
holding Monitor.

A pending update is announced BEFORE waiting for Monitor. The worker yields at
its next boundary and does not start successive samples while update waits.
Resumption is published and notified while holding Monitor, avoiding a lost
notification between checking the predicate and waiting. This avoids repeated
reacquisition ahead of update but provides NO hard deadline: BPF or procfs
work in progress can take time. The sample timestamp starts AFTER acquiring
Monitor, so lock wait is not reported as collection time. Update does not
force an immediate sample or change cadence; the configured interval and
actual collector cost still apply.

False consent returns permission_required without touching capture. The
update is rejected if the flag is off, the worker is stopped, the object was
moved from, or capture is inactive. Update never starts or rearms capture.
Success creates a new epoch and resets ALL baselines. It preserves the
counter origin for retained rows and does not recover earlier bytes for new
ones. An authorized Monitor failure, even an invalid list, closes capture
but does NOT stop the worker from sampling other groups. Later reports are
absent with a reason; recovery requires stop, explicit setup, and start.
Starting the worker alone does not reauthorize capture. Stop, worker exit,
or error closes capture.

## History is not capture state

An update does not clear or rewrite history. latest can still hold the last
sample from another epoch until the next sample, just as it retains historical
data after stop. On receiving changed.value, wait for a known report with that
selection_epoch. On failure, show the returned error without interpreting
the prior latest result as active capture. timed_history retains epochs and
gaps. The GUI must separate series instead of subtracting counters or connecting
rates across them. Other metrics, 60-second windows, capacity, and expiration
are unchanged. No new app persistence or HistoryStore format changes.

## Verification and limits

The existing sampler-lifecycle, sampler-window, and linux-process-network-monitor
CTests cover concurrent start/stop with 40 updates and reads, false consent,
moved-from, stopped, and disabled states, no rearm by a worker without capture,
and continuity of other samples. Checks use require/throw or assertions active
in Release, with bounded waits.

Manual `--authorized-private-worker-update`, NOT CTest: two previously authorized
private children, AB→B→AB with the worker running, updates from another thread,
concurrent history, initial samples without rates, B's counter origin preserved,
no recovery of A's traffic while absent, and four oracles for actor rates and
partial UID rates. An invalid update closes capture while the worker continues.
Update while stopped or restart alone does not rearm it. Stop restores the
original FD count, and both children exit normally. Parent/child watchdogs of
60 s/50 s are not hard BPF syscall limits.

The first native test failed when removing a row. Synchronization changes
removed possible starvation from mutex reacquisition by a busy worker, after
which the native test passed. Clear/notify was also changed to hold the mutex
during resumption. Initial logs were retained. See work/network-worker-final-*
and the consumer/installation logs with the same prefix. No tools were installed,
and no unrelated or global traffic was captured.

This additive API does not change public layouts. Link the current library
for the new method and rebuild all consumers if coming from pre-75 layouts.
At this checkpoint, F03 remains open for policy, automatic discovery, namespace
buckets, and broader coverage. H04/J04 integrations remain pending. No partial
legacy totals or new Windows support.

Final result: Linux Release 126/126 without skips, three focused tests
repeated 20 times each, installed consumer rebuilt, linked, and exiting 0,
and a warning-free build. MSVC passed 3/3 focused tests with the current
library and no new C++ warnings. No new full Windows suite or general
consumer run; the revision 75 consumer failure remains N03. Official revision
77 count: 57/75 = 76.0% closed, 18/75 = 24.0% open, with no partial credit.
