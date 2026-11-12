#pragma once

#include "pcinsight/monitor.hpp"
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace pcinsight {
namespace detail {struct SamplerAdmissionAccess;}

// Monotonic collection envelope, not an atomic timestamp for every metric.
// Only meaningful in this process/steady-clock domain; never persist as wall time.
struct SamplerSample {
    std::chrono::steady_clock::time_point started_at;
    std::chrono::steady_clock::time_point finished_at;
    SystemSnapshot snapshot;
};

// Background sampler intended for a Task-Manager-style 0..60 second graph.
class Sampler {
public:
    // capacity=0 selects floor(60s/interval)+1; an explicit smaller capacity
    // keeps only the newest subset of the time window. Interval must fit steady_clock.
    explicit Sampler(std::chrono::milliseconds interval = std::chrono::seconds(1),
                     std::size_t history_capacity = 0,
                     SampleOptions options = {});
    ~Sampler();
    Sampler(Sampler&&) noexcept;
    Sampler& operator=(Sampler&&) noexcept;
    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    // A new run clears graph history/error, but preserves Monitor's own counters.
    void start();
    void stop();
    // Only while stopped with include_linux_process_network=true. Worker exit/
    // stop closes capture; start() never silently reauthorizes it.
    Metric<std::uint64_t> start_linux_process_network(bool explicitly_authorized,std::span<const LinuxProcessNetworkSelection>);
    // Same stopped-worker/flag/consent rules, explicit mode for this session.
    // Two-argument setup remains strict; worker restart never reauthorizes.
    Metric<std::uint64_t> start_linux_process_network(bool explicitly_authorized,std::span<const LinuxProcessNetworkSelection>,LinuxProcessNetworkSelectionMode);
    // Synchronous update of an ALREADY active capture while the worker runs.
    // Serializes with collection and start/stop; may wait, no hard deadline.
    // Fresh consent on every call. Monitor's fail-closed update policy applies.
    // History is retained: wait for the returned epoch, never bridge epochs.
    Metric<std::uint64_t> update_linux_process_network_selection(bool explicitly_authorized,std::span<const LinuxProcessNetworkSelection>);
    // Setup only while stopped and include_linux_process_network=true. No
    // worker-start/flag authorization. Default remains strict_all each call.
    Metric<LinuxNetworkAdmissionChange> start_linux_visible_process_network(bool explicitly_authorized,LinuxNetworkAdmissionPolicy=LinuxNetworkAdmissionPolicy::strict_all);
    // While running with a previously prepared visible route. Serializes with
    // sample/history commit; fresh consent EACH refresh, no automatic polling.
    // Unchanged eligible set preserves epoch/rates. Failed refresh stops capture,
    // not the worker; another explicit refresh may retry that configured route.
    Metric<LinuxNetworkAdmissionChange> refresh_linux_visible_process_network(bool explicitly_authorized,LinuxNetworkAdmissionPolicy=LinuxNetworkAdmissionPolicy::strict_all);
    bool running() const;
    // Last collection error; cleared when a new run starts. No exception escapes
    // the worker thread. A failed run stops and can be restarted.
    std::string last_error() const;
    // Most recent successful collection, even if older than the graph window.
    std::optional<SystemSnapshot> latest() const;
    std::optional<SamplerSample> latest_timed() const;
    // Samples whose collection start is in [steady_now-60s, steady_now].
    // Expiration also applies while stopped; no interpolation/fake zeroes.
    std::vector<SystemSnapshot> history() const;
    std::vector<SamplerSample> timed_history() const;

private:
    friend struct detail::SamplerAdmissionAccess;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace pcinsight
