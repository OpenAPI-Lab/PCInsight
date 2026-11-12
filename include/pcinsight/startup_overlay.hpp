#pragma once
#include "startup_observer.hpp"
namespace pcinsight {
struct StartupImpactOverlayResult {
    ActionError error{ActionError::none};
    std::size_t applied_entries{},not_present_observations{};
    std::string message;
    explicit operator bool() const {return error==ActionError::none;}
};
// Pure, explicit GUI overlay. No native I/O or automatic registration/cause proof.
// expected_native_context is the context of the caller's accepted observation run.
// Validation errors leave snapshot unchanged. Unmatched entries stay untouched.
// Multi-process results are NOT summed; only a known high lower bound survives.
// For native birth keys use startup_birth_overlay.hpp instead; never fabricate
// legacy PID/start_ticks/steady-clock samples to call this API.
StartupImpactOverlayResult apply_startup_impact_observations(SystemSnapshot& snapshot,
    const std::vector<StartupImpactObservation>& observations,const std::string& expected_native_context);
}
