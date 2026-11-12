#pragma once

#include "pcinsight/types.hpp"
#include <chrono>
#include <memory>

namespace pcinsight {
namespace detail {struct MonitorAdmissionAccess;}

class Monitor {
public:
    Monitor();
    ~Monitor();
    Monitor(Monitor&&) noexcept;
    Monitor& operator=(Monitor&&) noexcept;
    Monitor(const Monitor&) = delete;
    Monitor& operator=(const Monitor&) = delete;

    // First sample establishes counters. Later samples produce rate metrics.
    SystemSnapshot sample(const SampleOptions& options = {});
    // sample flag never starts BPF. A sample with the flag false stops capture.
    Metric<std::uint64_t> start_linux_process_network(bool explicitly_authorized,std::span<const LinuxProcessNetworkSelection>);
    // Explicit mode for this new session. The two-argument overload ALWAYS uses
    // strict_single_namespace, even after a stopped bucket session. No auto BPF.
    Metric<std::uint64_t> start_linux_process_network(bool explicitly_authorized,std::span<const LinuxProcessNetworkSelection>,LinuxProcessNetworkSelectionMode);
    // Serialize with sample/start/stop. Fresh consent; active capture required.
    // Success resets rates/epoch. ANY authorized failure closes Monitor capture.
    Metric<std::uint64_t> update_linux_process_network_selection(bool explicitly_authorized,std::span<const LinuxProcessNetworkSelection>);
    void stop_linux_process_network() noexcept;
    // Explicit inventory admission setup/refresh. Default strict_all on EVERY
    // call. Cannot mix with an active manually selected session: stop first.
    // Requires fresh consent; sample never enumerates/rearms. Reports coverage
    // in SystemSnapshot.linux_network_admission alongside selected network.
    Metric<LinuxNetworkAdmissionChange> refresh_linux_visible_process_network(bool explicitly_authorized,LinuxNetworkAdmissionPolicy=LinuxNetworkAdmissionPolicy::strict_all);

private:
    friend struct detail::MonitorAdmissionAccess;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace pcinsight
