#pragma once
#include "linux_network_pidfd.hpp"
#include "linux_network_self.hpp"
namespace pcinsight::detail {
struct PrivateNetworkProcContext {
    std::array<std::uint64_t,2> boot{};
    LinuxNetworkSelfIdentity observer,process;
    auto operator<=>(const PrivateNetworkProcContext&) const = default;
};
struct PrivatePidfdNetworkJoin {PrivateNetworkIdentity kernel;PrivateNetworkProcContext local;};
// Classification only, AFTER native pidfd/procfs verification. PID equality alone
// is never proof. The native observer is read from /proc/self at each endpoint.
inline bool private_network_join_is_self(const PrivateNetworkProcContext& c){
    return (c.boot[0]||c.boot[1])&&valid_network_self(c.observer)&&c.process==c.observer;
}
Metric<PrivatePidfdNetworkJoin> validate_private_pidfd_network_join(
    std::uint32_t expected_pid,std::uint64_t expected_start,
    const PrivateNetworkProcContext& before,const PrivateNetworkIdentity& first,
    const PrivateNetworkIdentity& last,const PrivateNetworkProcContext& after);
// Internal opt-in point observation; owns duplicate for entire transaction.
// No UID/tick conversion, lifetime guarantee, total coverage or public provider.
Metric<PrivatePidfdNetworkJoin> read_private_pidfd_network_join(
    bool authorized,int pidfd,std::uint32_t expected_pid,std::uint64_t expected_start,const LinuxNetworkBtfLayout* session_layout=nullptr,LinuxPidfdIdentityQuery* reusable=nullptr);
}
