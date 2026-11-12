#pragma once
#include "actions.hpp"
#include "metric.hpp"
#include <array>
#include <chrono>
#include <memory>
#include <span>
namespace pcinsight {
namespace detail {struct RunitTransitionAccess;}
enum class LinuxRunitState { down,run,finish };
struct LinuxRunitStatus {
    std::array<std::uint8_t,12> changed_at_tai64n{}; // Raw TAI64N, NOT Unix/uptime.
    std::uint32_t reported_pid{}; // Supervisor's PID namespace, NOT a process identity token.
    LinuxRunitState state{};
    bool paused{},want_up{},term_sent{};
    bool operator==(const LinuxRunitStatus&)const=default;
};
Metric<LinuxRunitStatus> decode_linux_runit_status(std::span<const std::uint8_t>);
struct LinuxRunitRequestResult {
    ActionError error{ActionError::none};
    std::uint32_t requested_bytes{},written_bytes{};
    Metric<bool> completed; // Always UNKNOWN: delivery is not readiness/job completion.
    std::string message;
    explicit operator bool()const{return error==ActionError::none;}
};
struct LinuxRunitServiceInfo {
    std::string name,directory;
    Metric<LinuxRunitStatus> status;
    Metric<std::string> description,group; // No standard runit metadata: unknown.
};
struct LinuxRunitInventory {
    std::string root;
    bool complete{true};
    std::vector<LinuxRunitServiceInfo> services;
    std::vector<std::string> diagnostics;
};
// Explicit caller-trusted runit tree only. No auto-detect, SVDIR, PATH or shell.
// Pins selected directory/endpoints, not a supervisor PID/authenticated peer.
// Serialized calls, coordinate forks/FD ownership; no hidden polling or worker.
class LinuxRunitService {
public:
    static Metric<std::shared_ptr<LinuxRunitService>> open(const std::string& absolute_directory);
    ~LinuxRunitService();
    LinuxRunitService(const LinuxRunitService&)=delete;
    LinuxRunitService& operator=(const LinuxRunitService&)=delete;
    Metric<LinuxRunitStatus> read() const;
    // start=u, stop=d, restart=tcu; writes only. No sv/check/script execution or escalation.
    LinuxRunitRequestResult request(ServiceAction,bool explicitly_authorized);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit LinuxRunitService(std::unique_ptr<Impl>);
    static Metric<std::shared_ptr<LinuxRunitService>> open_at(int,const std::string&);
    friend Metric<LinuxRunitInventory> query_linux_runit_inventory(const std::string&,std::uint32_t);
};
Metric<LinuxRunitInventory> query_linux_runit_inventory(const std::string& absolute_root,std::uint32_t max_entries=1024);
// A state predicate ONLY, not attribution to a request, readiness or job success.
Metric<bool> observe_linux_runit_target(ServiceAction,const LinuxRunitStatus& before,const LinuxRunitStatus& after);
enum class LinuxRunitTransitionState { idle,pending,observed,timed_out,failed,abandoned };
struct LinuxRunitTransitionReport {
    LinuxRunitTransitionState state{LinuxRunitTransitionState::idle};
    ServiceAction action{ServiceAction::start};
    bool request_attempted{};
    LinuxRunitRequestResult delivery;
    Metric<LinuxRunitStatus> before,latest;
    Metric<bool> baseline_satisfied,target_state_observed;
    std::chrono::steady_clock::time_point started_at{},deadline{};
    std::optional<std::chrono::steady_clock::time_point> sampled_at;
    ActionError error{ActionError::none};
};
// One request, caller-driven polls, no thread/sleep/retries. Observed is historical,
// NOT completed/readiness/causal job proof. close does not undo a delivered request.
class LinuxRunitTransition {
public:
    explicit LinuxRunitTransition(std::shared_ptr<LinuxRunitService>);
    ~LinuxRunitTransition();
    LinuxRunitTransition(const LinuxRunitTransition&)=delete;
    LinuxRunitTransition& operator=(const LinuxRunitTransition&)=delete;
    ActionResult start(ServiceAction,bool explicitly_authorized,std::chrono::milliseconds timeout=std::chrono::seconds(7));
    ActionResult poll();
    LinuxRunitTransitionReport report()const; // Copy, no IO.
    void close(); // Release observation only, no final IO/control.
private:
    friend struct detail::RunitTransitionAccess;
    struct Impl;std::unique_ptr<Impl> impl_;
    LinuxRunitTransition(std::unique_ptr<Impl>,int);
};
}
