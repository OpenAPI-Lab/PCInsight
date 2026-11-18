# Explicit buckets in Monitor and Sampler

Revision 79 integrates [declared bucket mode](LINUX_PROCESS_NETWORK_BUCKETS.en.md)
into Monitor and Sampler through a setup overload with a THIRD argument.
It adds no options fields or public layout changes. Mode is opt-in per session,
separate from include_linux_process_network and consent.

```cpp
using Mode = pcinsight::LinuxProcessNetworkSelectionMode;
pcinsight::Monitor monitor;
auto started = monitor.start_linux_process_network(consent, selection,
                                                   Mode::declared_namespace_buckets);
pcinsight::SampleOptions options;
options.include_linux_process_network = true;
if (pcinsight::metric_is_known(started)) {
    auto snapshot = monitor.sample(options);
    // snapshot.linux_process_network: bucket rows, not per-process totals.
}
monitor.stop_linux_process_network();

pcinsight::Sampler background(std::chrono::seconds(1), 0, options);
auto setup = background.start_linux_process_network(consent, selection,
                                                    Mode::declared_namespace_buckets);
if (pcinsight::metric_is_known(setup)) {
    background.start();
    // Update the complete list only with fresh consent:
    // background.update_linux_process_network_selection(consent, another_list);
    background.stop();
}
```

## Per-session mode without implicit authorization

The existing TWO-argument call ALWAYS uses strict_single_namespace, even after
stopping a bucket session. To use buckets again, pass the third argument and
consent again. This differs from the standalone provider constructed with a
fixed mode: previous configuration is not silently inherited. Both existing
setup symbols are retained. The API is additive; link the current library
for the new overload. Rebuild consumers using layouts from before revision 75.
No new CMake requirement for the consumer project.

False consent does not call the provider or change active capture. Unknown
enums are rejected before calling the provider. start does not reconfigure
an active capture; stop it first. An older injected provider without mode
setup can still handle strict mode, but bucket mode is rejected without silently
falling back to strict. Startup errors and exceptions retain the earlier
availability and cleanup behavior. Construction, the flag, sample, and
starting the worker do not by themselves authorize BPF.

Monitor calls must be serialized. Sampler setup requires a stopped worker and
an enabled flag, serializes lifecycle → Monitor, waits for the worker to finish
before setup, and rejects running setup BEFORE touching descriptors. A list
update while running retains the session mode and follows
[revision 77 priority and locking](LINUX_PROCESS_NETWORK_WORKER.en.md); it
cannot change modes. To change mode, stop, run explicit setup, and start.
Do not unshare or manipulate library-owned FDs; share the worker's FD table.
Do not use an inherited Sampler after fork or move/destroy it concurrently.

Disabling the Monitor flag closes capture; true alone does not rearm it.
Stop, worker exit, and worker errors close capture; restart alone does not
rearm. A failed authorized update closes capture without stopping other worker
groups. The latest snapshot and history may still be historical and do not
prove active capture or current mode. Wait for the new selection_epoch and
separate charts across epochs and modes without clearing other metrics.

## Scope and evidence

At this revision the limits and semantics remain: 16 total process-cookie
rows, disjoint cookies per process, consistent local/kernel identities and
UIDs, filtering by the SOCKET cookie with sticky quality failures, a nominal
non-atomic common sweep, and estimated rates.
coverage=declared_tcp_udp_socket_namespace_buckets and processes contains
BUCKETS: multiple rows can repeat PID/start. UID sums cover selected buckets
only, not processes, sessions, or global traffic. Do not fill legacy totals
or infer that a declared namespace exists from a zero counter. No automatic
discovery, setns/unshare, CLI, new tools or dependencies, or automatic elevation.

Controller fixtures cover correct mode, flag without setup, denial or invalid
enum without callback, mode changes rejected during active capture, update
preserving mode, opt-out, explicit rearming, strict mode through the old call,
old callback without buckets, failed startup, epoch zero, estimated results,
exceptions, and cleanup. Public API tests cover the flag, stopped worker,
enabled state, denial, moved-from objects, and invalid inputs. The installed
consumer links both overloads without starting BPF.

Manual `--authorized-private-monitor-buckets` and
`--authorized-private-worker-buckets`, NOT CTest: two authorized private
children; capture of ONE with a real cookie and a deliberately unobserved one.
Four payload/rate oracles per bucket and a UID sum without double counting;
other operations, the parent, and the other child excluded; removal and
re-addition with a new epoch, clean baseline, and no rates across the gap;
closed input FD and no FD leaks. Monitor: fork rejected, opt-out closes capture,
the flag does not rearm, and the default returns to strict. Worker: concurrent
updater and history, bucket coverage, first sample without rates, running mode
change rejected, failed update closes capture but the worker continues, and
stop/default strict/restart without rearming. Only ONE real namespace was
used. The unobserved cookie does not prove two simultaneous real namespaces
or discovery. Parent/child watchdogs are 60 s/50 s, not hard BPF deadlines.
Logs: work/network-bucket-monitor-{native,worker-native}-linux.log.

At this checkpoint F03 stays open for policy, automatic discovery, dynamic
buckets, and broader coverage. H04/J04 remain pending. No new persistence,
GUI, or Windows capture. The general MSVC consumer failure from revision 75
remains in N03. No full Windows compatibility or universal hardware, kernel,
or distro certification.

Final result: Linux Release 126/126 without skips, three focused tests repeated
20 times each, the installed consumer rebuilt, linked, and exiting 0, and a
warning-free build. The library and 3/3 focused MSVC tests pass with no new C++
warnings; expected D9025 keeps assertions active. No new full Windows suite or
consumer run. Logs: work/network-bucket-monitor-*. Official revision 79 count:
57/75 = 76.0% closed, 18/75 = 24.0% open, with no new IDs, closures, or partial
credit.
