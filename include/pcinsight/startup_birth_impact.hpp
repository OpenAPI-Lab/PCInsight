#pragma once
#include "startup_birth_observer.hpp"
#include "startup_impact.hpp"
namespace pcinsight {
// A single retained child's birth-relative window, NOT a global boot/session
// window, process-tree sum, legacy procfs sample or automatic app attribution.
struct StartupBirthImpactMeasurement {
    StartupBirthProcessKey key;
    Metric<std::string> startup_entry_id; // Existing explicit declaration only.
    std::uint64_t begin_monotonic_ns{},deadline_monotonic_ns{};
    std::optional<std::uint64_t> release_monotonic_ns;
    bool released_within_window{};
    Metric<std::chrono::nanoseconds> cpu_time; // Release-observed lower bound.
    Metric<std::uint64_t> disk_bytes; // Checked read+write; includes waited-child IO.
    Metric<StartupImpact> impact; // High lower bound or UNKNOWN, never low/medium.
    Metric<bool> automatic_startup_origin; // Always unknown.
};
struct StartupBirthImpactResult {
    std::optional<StartupBirthImpactMeasurement> value;
    ActionError error{ActionError::none};
    std::string message;
    explicit operator bool() const {return value.has_value()&&error==ActionError::none;}
};
// Pure, read-only projection. Requires the EXACT retained key and an existing
// explicit startup_entry_id binding. Does not read the OS, bind, sum, save or
// update Monitor/recorder/legacy observers. Window must be >0 and <=1 hour.
// Missing/late release cannot be clipped/prorated into the window: stays unknown.
StartupBirthImpactResult measure_startup_birth_impact(const StartupBirthObserverReport&,
    const StartupBirthProcessKey&,std::chrono::nanoseconds window=std::chrono::seconds(60),
    const StartupImpactPolicy& policy={});
}
