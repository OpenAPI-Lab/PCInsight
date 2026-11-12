#pragma once
#include "linux_privileged_connection.hpp"
#include <atomic>
namespace pcinsight::detail {
// INTERNAL mechanism switches, never populated from request. OFF by default.
struct PrivilegedExecutionGate {
    std::atomic<bool> enabled{false},permit_interaction{false};
    std::atomic<std::uint32_t> manager_uid{UINT32_MAX};
    std::atomic_flag busy=ATOMIC_FLAG_INIT;
    std::atomic<std::uint64_t> job_timeout_usec{30000000};
    std::atomic<std::uint64_t> request_timeout_usec{5000000};
    std::atomic<std::uint32_t> rpc_limit{96};
};
enum class PrivilegedExecutionState { rejected,unavailable,denied,stale,dispatch_unknown,job_queued };
struct PrivilegedExecutionResult {
    PrivilegedExecutionState state{PrivilegedExecutionState::rejected};
    bool authorization_checked{},dispatch_attempted{};
    Metric<std::string> job_path; // Queued job receipt only, never service completion.
    std::shared_ptr<PrivilegedJobObservation> job; // Data-only async observation.
    std::shared_ptr<PrivilegedRequestBudget> budget; // Diagnostic counters only, not a grant.
    std::string note;
};
// ControlSystemService(uusb) ONLY. Fresh authorization in this handler, no grant
// parameter/cache/retry. Caller serializes connection; busy rejects reentrancy.
PrivilegedExecutionResult execute_owned_privileged_request(void*,const PrivilegedBusConnection&,std::uint32_t authority_uid,const PrivilegedPolicyStore&,PrivilegedExecutionGate&);
}
