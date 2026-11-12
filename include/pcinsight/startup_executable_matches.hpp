#pragma once
#include "startup_birth_observer.hpp"
#include "types.hpp"
namespace pcinsight {
struct StartupExecutableCandidate {
    std::string entry_id,name;
    StartupStatus current_status{StartupStatus::unknown};
    Metric<std::string> direct_executable;
    Metric<bool> same_file_identifiers; // CURRENT path vs historical first exec, not app/cause proof.
};
struct StartupExecutableMatchReport {
    std::vector<StartupExecutableCandidate> candidates;
    Metric<std::string> unique_candidate; // Estimated, only within the COMPLETE current caller catalog.
    Metric<bool> automatic_startup_origin;
    std::size_t matched{},unresolved{};
    bool complete{false};
    ActionError error{ActionError::none};
    std::string message;
};
// Read-only Linux CURRENT CALLER effective XDG catalog, not target-UID or
// launch-time inventory. No PATH guessing, shell execution, launch, binding,
// privilege elevation, writes or impact classification. Native I/O may block.
// Caller must serialize environment changes. Partial/ambiguous never yields a
// unique candidate; even a unique file match is NOT automatic attribution.
StartupExecutableMatchReport discover_startup_executable_matches(
    const StartupBirthObserverReport&,const StartupBirthProcessKey&);
}
