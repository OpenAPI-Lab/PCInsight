#pragma once
#include "startup_impact.hpp"
namespace pcinsight {
struct StartupProcessCandidate {
    std::string startup_entry_id;
    StartupProcessSample baseline;
    Support association_support{Support::estimated};
    Metric<bool> automatic_startup_origin; // unknown: launcher markers are not proof of autostart
};
struct StartupCandidateReport {
    std::vector<StartupProcessCandidate> candidates;
    std::size_t examined{},ignored{},unavailable{};
    bool complete{true}; // all PROVIDED processes inspected/ignored, not entire boot coverage
    ActionError error{ActionError::none};
    std::string message;
};
// Linux, current effective UID only. Caller supplies snapshot with both processes
// and startup_entries enabled. Never matches executable names or Exec substrings.
// Does not automatically register candidates in an observer or update impact.
StartupCandidateReport discover_startup_process_candidates(const SystemSnapshot& snapshot);
}
