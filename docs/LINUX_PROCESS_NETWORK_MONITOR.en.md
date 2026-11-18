# Selected network report in Monitor and Sampler

Revision 75 integrates the [public provider](LINUX_PROCESS_NETWORK.en.md)
through the separate SystemSnapshot.linux_process_network Metric and
SampleOptions.include_linux_process_network (false by default). It does not
populate legacy process, session, or app network fields with partial data.

**0.x layout change:** rebuild the library and ALL consumers. SystemSnapshot
and SampleOptions changed size. Support/Metric retain their types, layouts, and
semantics. They are now defined in metric.hpp to avoid a circular dependency
between general types and the network report. There is no new persisted format,
GUI, or Windows capture component.

## Two explicit steps

```cpp
#include <pcinsight/monitor.hpp>

void observe(pcinsight::Monitor& monitor,
             std::span<const pcinsight::LinuxProcessNetworkSelection> selection,
             bool consent)
{
    const auto started = monitor.start_linux_process_network(consent, selection);
    if (!pcinsight::metric_is_known(started)) return; // Show support/note.

    pcinsight::SampleOptions options;
    options.include_linux_process_network = true;
    const auto snapshot = monitor.sample(options);
    // Show snapshot.linux_process_network and its Metrics; no rates in the first sample.
    monitor.stop_linux_process_network();
}
```

Pass selection and authorization to start_linux_process_network, NOT to the
options or sample. A true flag without explicit start does not load BPF;
it returns absence with a reason. A denied start preserves permission_required
in the report rather than faking an available empty report. Do not start a
new selection while capture is active: stop first or, since revision 76, use
the [explicit update](LINUX_PROCESS_NETWORK_UPDATE.en.md) through
update_linux_process_network_selection. Starting with false does not initiate
capture or revoke an active authorization.

A sample with the flag false, including monitor.sample() with defaults,
CLOSES capture before collecting the general backend and returns a disabled
report. Setting true again does NOT reauthorize capture; another explicit
start is needed. Stop, destruction, and exceptions from the general backend
close references. An ordinary read error is reflected in the Metric without
clearing other groups; a provider exception stops capture. A failed sample
does not reuse an old report.

## Worker and charts

```cpp
#include <pcinsight/sampler.hpp>
// Set options.include_linux_process_network = true BEFORE constructing.
// pcinsight::Sampler background(std::chrono::seconds(1), 0, options);
// background.start_linux_process_network(consent, selection); // Worker stopped.
// background.start();  ... read latest()/timed_history() ...  background.stop();
```

Sampler.start_linux_process_network requires a stopped worker and an enabled
flag in its construction options. It rejects setup while running BEFORE
touching descriptors. Setup uses the same private Monitor. Starting the worker
never authorizes capture; stopping, worker exit, exceptions, or thread-creation
failure close it. To capture on another run, repeat the authorized setup with
valid pidfds. Restarting without setup still reads other groups, but the network
report remains absent.

Revision 77 allows [updating selection with the worker running](LINUX_PROCESS_NETWORK_WORKER.en.md)
through update_linux_process_network_selection, with fresh consent, serialization,
and baseline reset. Do not confuse it with start_linux_process_network, which
still requires a stopped worker. There is no automatic discovery.

After stop, latest remains the last historical sample, NOT live capture
state. Start clears history and errors as before. There is no new app or
network persistence. The requested interval does not guarantee cadence:
BPF sampling and joins are synchronous, and stop can wait for the current
operation without a hard deadline. Do not use a Sampler with an inherited
jthread after fork. That thread restriction is separate from the Monitor
and individual provider identity guard. Monitor on its own and its moved or
forked capture are tested without an inherited running worker.

## Scope and verification

The revision 75 contract retains consent, 1–16 actors with declared cookies,
owned pidfds, separate local and initial identities, nominal brackets,
support/BTF/permissions, PARTIAL UID sums, absence distinct from zero, and
the [coverage limits](LINUX_PROCESS_NETWORK.en.md). It does not start global
capture or automatic discovery, elevate privileges, or install packages,
services, or CLI tools. Callers can link from an IDE and defer CMake setup
until after the GUI without changing this contract.

Revision 79 adds [bucket-mode setup](LINUX_PROCESS_NETWORK_BUCKET_MONITOR.en.md)
through a third argument to start_linux_process_network in Monitor/Sampler.
Two-argument calls ALWAYS remain strict. The flag does not activate the mode
and the mode is not inherited from an earlier session. The rest of this
revision 75 checkpoint is historical.

The internal injected adapter is tested without BPF: flag alone, denied
consent, invalid or throwing startup, denied or throwing reads, recovery,
opt-out, rearming, absence without stale data, unchanged legacy-field
sentinels, and Monitor/Sampler/move lifecycle. A fake callback tests integration,
not the real kernel.

Manual `--authorized-private-monitor-window`, NOT CTest: two private WSL
children, four actor and partial UID oracles, nominal 2.303687773 s interval,
closed input FD, active move, parent exclusion, idle zero, and fork/exit
rejection. Opt-out closes FDs; the flag alone does not rearm capture. An
authorized worker delivers a report, rejects setup while running, closes
capture on stop, and restarts without it. Explicit rearming gives a different
epoch and a first sample without rates. Cleanup leaves no FD leaks. Private
watchdogs are 60 s and 50 s, not hard BPF deadlines.
Log: work/network-monitor-native-linux.log.

The official revision 75 tracker and work/network-monitor-* logs retain
regression, installation, and consumer results. MSVC checks focused compatibility
of the shared change, not new Windows functionality or capture or an updated
full Windows suite. At this checkpoint F03 remains open for dynamic selection,
namespace buckets, and general coverage. A fixed partial list does not complete
the general per-app network request. H04/J04 do not close, and persistence
and sessions are not integrated.

Verified: Linux Release 126/126 without skips, 18 network tests repeated
20 times each, repeated build and suite, and the final installed consumer
exiting 0. See verified-build, verified-suite, and final-consumer-linux logs.
Initial make warnings about DrvFS timestamps milliseconds in the future were
retained, and the rebuild was clean. BTF prototype shadowing detected by MSVC
was fixed. Two of two current focused MSVC tests pass, with no new C++ warnings
on rebuild (initial D9025 is expected to keep assertions active). The general
MSVC consumer builds and links against the current installation but reproduces
0xC0000409. Markers show the new checks passed before the failure later in
the run. This does NOT establish the cause or absence of a regression; it was
recorded in N03 and deferred because Linux is the priority. See
consumer-diagnostic/diagnostic2-msvc logs. Do not claim Windows consumer exit 0.
