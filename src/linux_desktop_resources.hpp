#pragma once
#include "linux_desktop_receipt.hpp"
#include "pcinsight/startup_birth_impact.hpp"
namespace pcinsight::detail {
struct LinuxManualLaunchResources {
    LinuxDesktopLaunchOrigin origin{LinuxDesktopLaunchOrigin::manual_request};
    StartupBirthImpactMeasurement measurement; // Single group, birth-to-release lower bounds.
    Metric<bool> controlled_entry_link;
};
Metric<LinuxManualLaunchResources> measure_linux_manual_launch_resources(
    const Metric<LinuxDesktopLaunchReceipt>&,const StartupBirthObserverReport&,
    std::chrono::nanoseconds window=std::chrono::seconds(60),const StartupImpactPolicy& policy={});
struct LinuxDesktopResourceReport {
    Metric<LinuxDesktopLaunchReceipt> receipt;
    StartupBirthObserverReport births; // Original unbound rows, never mutated by the join.
    Metric<LinuxManualLaunchResources> resources;
};
// INTERNAL one-shot: arm selected-creator birth/final/exec BEFORE launching child.
// Serialized: no unrelated forks/child reapers while active. No implicit activation.
class LinuxDesktopResourceSession {
    LinuxStartupBirthObserver births_;
    LinuxDesktopLaunchSession launch_;
    bool attempted_{},active_{};
public:
    LinuxDesktopResourceSession()=default;~LinuxDesktopResourceSession(){stop();}
    LinuxDesktopResourceSession(const LinuxDesktopResourceSession&)=delete;
    LinuxDesktopResourceSession& operator=(const LinuxDesktopResourceSession&)=delete;
    ActionResult start(const std::string& entry_id,bool launch_consent,bool capture_consent,const std::string& capture_id);
    // Explicit last read then detach; caller should reap first for final counters.
    // Never waits/signals/reaps a released child, even if called while alive.
    Metric<LinuxDesktopResourceReport> finish(std::chrono::nanoseconds window=std::chrono::seconds(60),const StartupImpactPolicy& policy={});
    void stop() noexcept; // No hidden final read. Previously returned copies survive.
};
}
