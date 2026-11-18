# Sampler: 60-second monotonic window (A03)

`Sampler::history()` and `timed_history()` return samples whose collection
began in the inclusive interval `[steady_now-60s, steady_now]`. Age is
checked both when adding a sample and when querying history, even when the
sampler is stopped. The check relies neither on the calendar clock nor on
counting 60 elements. The time limit applies equally on Linux and Windows.

## API and charting

```cpp
#include <pcinsight/sampler.hpp>
using namespace std::chrono_literals;
pcinsight::Sampler sampler(100ms); // Automatic capacity: up to 601 points.
sampler.start();
// On a GUI tick, after samples are available:
auto points = sampler.timed_history();
auto now = std::chrono::steady_clock::now();
for (const auto& point : points) {
    auto age_seconds = std::chrono::duration<double>(now-point.started_at).count();
    const auto& cpu = point.snapshot.cpu.utilization_percent;
    // Plot at x=-age_seconds. If cpu.value is missing, leave a gap, not a zero.
    // Do not interpolate unless the consumer has an explicit policy.
}
sampler.stop();
```

`SamplerSample` holds `started_at`, `finished_at` (steady_clock), and the
original `SystemSnapshot`. `latest_timed()` returns the latest result in
that envelope; `latest()` retains the snapshot API. Both can return a result
older than 60 seconds: they mean *latest successful collection*, not fresh
data. History can be empty while the latest result remains present. At most
one expired sample is retained for latest, without duplicating all its data.

Points are ordered by monotonic start time. Start must be at or before finish,
and the next collection cannot start before the previous one finishes. The
timestamps bracket collection; they are **not** a common atomic instant for
all metrics. Each provider retains its own rate or percentage window.
`captured_at` remains the calendar time supplied by Monitor. Calendar-clock
jumps do not affect retention or ordering. Do not persist steady-clock values
across processes, boots, or different clock domains. Suspend semantics depend
on the platform clock; elapsed civil time during suspend is not guaranteed.

`now` is read under the query lock, before results are copied. If the GUI
takes time to receive or draw them, the points continue to age. Reapply the
cutoff while rendering if the chart needs an exact edge at that moment.
Separate calls to history, timed_history, and latest do not share a single
transaction or necessarily the same window.

## Interval, cost, and capacity

The constructor retains three arguments: `(interval, history_capacity, options)`.
The default `history_capacity` changes from 60 to 0. Zero now means automatic:
`floor(60000ms/interval_ms)+1`. At 1 second, it holds up to 61 points
(including endpoints 0 and 60); at 100 ms, 601; at 1 ms, 60,001. The space is
not reserved in advance. The interval must be positive and representable
in steady_clock. Unrepresentable, zero, or negative inputs cause
`invalid_argument` before the worker starts.

A positive explicit capacity remains an additional limit. A capacity of 2
retains at most the two most recent samples within 60 seconds. It can shorten
the covered period and does not guarantee a full 60 seconds. Automatic capacity
is sufficient for an uninterrupted run with the configured minimum spacing.
The sampler neither fills missing seconds nor creates synthetic samples.

The worker reads the start time once and waits until start plus interval,
with bounded arithmetic to avoid clock overflow. Collections do not overlap
or catch up on missed ticks in bursts. If collection takes longer than the
interval, the next starts when the previous one finishes. Exact frequency
and maximum latency are therefore not guaranteed. A collection taking more
than 60 seconds can become latest while already outside history at publication.
This avoids drawing it as a fresh point taken at the end when it began more
than a minute earlier.

Copying full snapshots and collecting data consume CPU and memory. Enabling
many inventories at 1 ms can be expensive. Reduce groups with SampleOptions
or set an explicit capacity if the consumer needs a lower limit. There is no
global byte limit or real-time guarantee. The GUI still draws the charts.

## Lifecycle, errors, and compatibility

`start()` is a no-op while active. A new start, including after failure,
clears chart history and the last error. It does not mix points from separate
runs. It does not reconstruct Monitor: its counters and app history keep
their own lifecycle. Values in the first sample after a pause can cover
backend intervals different from the GUI's point window.

`stop()` requests shutdown and wakes the wait; it does not interrupt
collection in progress. The worker can publish its final result before
exiting. A blocked kernel or driver call can delay stopping or destruction;
there is no forced cancellation of those APIs. Worker errors are retained
in `last_error()`, end that run, and do not escape the thread. Earlier
successful results do not become zeros. Failure to create the thread
propagates to the caller. When moving the sampler, the caller must not use
the source object concurrently. Start, stop, and queries use their own locks.

0.x behavior change: capacity 0 is now valid and automatic, the default at
1 second is 61, the window is time-based, and each new start clears history.
There are new methods and the new SamplerSample type. A03 does not change
the SystemSnapshot layout. Rebuild the library and consumers to pick up the
new defaults and use the new symbols. An older caller that explicitly
passes 60 remains limited to 60 points. This does not promise a stable ABI
or remove the documented metric limitations.

## Tests and evidence

`tests/sampler_window.cpp` exercises the same internal buffer as the worker
with synthetic time: 601 points at 100 ms over the inclusive 0–60 second
interval, the edge plus 1 ns, variable collection costs, sparse sampling,
a 61-second collection, expiration on reads while stopped, an expired
latest result, calendar jumps in both directions, explicit capacity,
clearing, rejection of backward or overlapping collections, and arithmetic
at steady_clock extremes. Synthetic time covers more than 60 seconds
without slowing the suite or changing the system clock. It does not test
real suspension or real 60-second latency.

Native integration on Linux WSL and Windows used a worker at 10 ms, at
least three samples with ordered envelopes, concurrent queries while
running, group options, and stop and restart with new points.
`sampler_lifecycle` covers moving a worker sleeping for one hour, waking it,
and concurrent start/stop. Assertions remain active in Release, and CTest
bounds test duration. No actions target other processes, services, or sessions.

Closing regression on 2026-10-16: Linux Release 79/79 and Windows Release
99/99. Packages were reinstalled, and rebuilt external consumers exited 0.
This does not close CI, pending hardware validation, or the isolated global
CPU failure previously recorded in N03.
