#pragma once
#include "linux_privileged_preflight.hpp"
#include <mutex>
#include <string_view>
#include <vector>
namespace pcinsight::detail {
// INTERNAL: anchor/owner/path come from trusted bootstrap, never client or env.
// This verifies descendants and the anchor itself, NOT ancestors of an arbitrary FD.
Metric<std::vector<PrivilegedServiceRule>> parse_privileged_policy(std::string_view);
Metric<std::vector<PrivilegedServiceRule>> load_privileged_policy(int trusted_directory_fd,std::string_view relative_path,std::uint32_t trusted_owner_uid=0);
struct PrivilegedPolicySnapshot {std::vector<PrivilegedServiceRule> rules;std::uint64_t generation{};};
class PrivilegedPolicyStore {
public:
    Metric<PrivilegedPolicySnapshot> refresh(int trusted_directory_fd,std::string_view relative_path,std::uint32_t trusted_owner_uid=0);
    void invalidate(); // Revokes pending observations, no last-known-good fallback.
    Metric<PrivilegedPolicySnapshot> snapshot()const;
    bool is_current(std::uint64_t generation)const;
private:
    void advance_locked();
    mutable std::mutex state_mutex_;
    std::mutex refresh_mutex_;
    std::uint64_t generation_{};
    bool exhausted_{};
    Metric<PrivilegedPolicySnapshot> current_{{},Support::unsupported,"No policy loaded"};
};
}
