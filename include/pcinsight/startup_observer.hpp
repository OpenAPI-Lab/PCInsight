#pragma once
#include "startup_impact.hpp"
#include <functional>
#include <memory>
namespace pcinsight {
using StartupImpactSampleReader=std::function<StartupProcessSampleResult(std::uint32_t,std::uint64_t)>;
struct StartupImpactObservation {
    std::string startup_entry_id;
    std::uint32_t pid{};
    std::uint64_t linux_start_ticks{};
    std::optional<StartupImpactMeasurement> measurement;
    std::size_t sample_count{};
    bool closed{},window_complete{},sampling_gap{};
    ActionError last_error{ActionError::none};
    std::string native_context{};
};
struct StartupImpactPollResult {std::size_t attempted{},updated{},failed{};};
// Poll-driven, explicit association. No thread, timer, launch or auto boot tracking.
// Thread-safe; injected readers must not reenter this observer (serialized calls).
class StartupImpactObserver {
public:
    explicit StartupImpactObserver(std::chrono::milliseconds window=std::chrono::seconds(60),
        StartupImpactPolicy policy={},StartupImpactSampleReader reader=read_startup_process_sample);
    ~StartupImpactObserver();
    StartupImpactObserver(const StartupImpactObserver&)=delete;
    StartupImpactObserver& operator=(const StartupImpactObserver&)=delete;
    ActionResult register_process(const std::string& startup_entry_id,const StartupProcessSample& baseline);
    ActionResult observe_sample(const StartupProcessSample& sample);
    // Explicit final sample + close of only this registered identity. Does not
    // verify exit itself, reap, or certify a short/gapped observation window.
    ActionResult finish_process(const StartupProcessSample& final_sample);
    // Retain a partial/lower-bound final sample and close. Forces sampling_gap
    // even if elapsed time spans the window; low/medium cannot be certified.
    ActionResult finish_partial_process(const StartupProcessSample& final_sample);
    StartupImpactPollResult poll();
    // Same polling lifecycle with caller-supplied steady-clock time (tests or
    // replay). At/past each deadline, attempt one final read, then close even
    // on failure. Expiration alone does not certify window_complete.
    StartupImpactPollResult poll(std::chrono::steady_clock::time_point now);
    // Closes retained observations without a hidden final read. Short windows are
    // incomplete; high observed lower bounds survive, low/medium do not.
    void close();
    std::vector<StartupImpactObservation> observations() const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
