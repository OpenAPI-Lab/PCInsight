#pragma once
#include "pcinsight/types.hpp"
#include <compare>
#include <string_view>
namespace pcinsight::detail {
struct LinuxNetworkSelfIdentity {
    std::uint32_t local_pid{},local_real_uid{};
    std::uint64_t start_ticks{},procfs_device{},procfs_inode{},process_inode{};
    std::uint64_t pid_namespace_device{},pid_namespace_inode{},user_namespace_device{},user_namespace_inode{};
    auto operator<=>(const LinuxNetworkSelfIdentity&) const = default;
};
struct NetworkSelfStatus {std::uint32_t pid{},tgid{},real_uid{};};
Metric<NetworkSelfStatus> parse_network_self_status(std::string_view);
Metric<LinuxNetworkSelfIdentity> read_linux_network_self(); // Read-only, SELF only, no BPF.
// Selected process only. pidfd is borrowed for this call; caller must own it and
// serialize close/reuse. UIDs are procfs observer-domain, not target userns IDs.
Metric<LinuxNetworkSelfIdentity> read_linux_network_pidfd_process(int pidfd,std::uint32_t expected_pid,std::uint64_t expected_start_ticks);
bool valid_network_self(const LinuxNetworkSelfIdentity&);
}
