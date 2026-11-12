#pragma once
#include "pcinsight/types.hpp"
#include <cstddef>
#include <span>
namespace pcinsight::detail {
// Internal, read-only layout preparation. NOT proof that BPF load/attach works.
struct LinuxSocketNamespaceBtfLayout {
    std::uint32_t sock_net_pointer_offset{},net_cookie_offset{};
};
struct LinuxPidfdIdentityBtfLayout {
    std::uint32_t function_id{},task_tgid_offset{},task_cred_offset{},cred_uid_offset{};
};
struct LinuxNetworkBtfLayout {
    std::uint32_t send_function_id{}, receive_function_id{}, sendto_function_id{};
    std::uint32_t inet_send_function_id{}, inet6_send_function_id{}; // 0 means absent, not a valid hook.
    std::uint32_t socket_sk_offset{}, sock_family_offset{}, sock_protocol_offset{};
    std::uint32_t sock_protocol_width{};
    std::uint32_t task_group_leader_offset{}, task_start_boottime_offset{}, task_flags_offset{};
    Metric<LinuxSocketNamespaceBtfLayout> socket_namespace;
    Metric<LinuxPidfdIdentityBtfLayout> pidfd_identity;
};
Metric<LinuxNetworkBtfLayout> resolve_linux_network_btf(std::span<const std::byte> blob,
    unsigned pointer_bytes=8);
Metric<LinuxNetworkBtfLayout> read_linux_network_btf();
// Shared bounded native BTF reader and independent exit-hook schema query.
Metric<std::vector<std::byte>> read_linux_kernel_btf_blob();
Metric<std::uint32_t> resolve_linux_taskstats_exit_hook(std::span<const std::byte>);
Metric<std::uint32_t> resolve_linux_startup_birth_hook(std::span<const std::byte>);
// Final accounting only when release_task is entered directly for the leader.
// An internal release_task repeat is NOT an entry and is not covered.
struct LinuxStartupResourceBtfLayout {
    std::uint32_t function_id{},tgid{},leader{},start{},cred{},uid{},signal{};
    std::uint32_t nr_threads{},live{},task_runtime{},signal_runtime{};
    std::uint32_t task_read{},task_write{},signal_read{},signal_write{};
};
Metric<LinuxStartupResourceBtfLayout> resolve_linux_startup_resource_btf(std::span<const std::byte>,unsigned pointer_bytes=8);
struct LinuxBirthPidBtfLayout {
    std::uint32_t thread_pid{},level{},numbers{},upid_size{},nr{},ns{},ns_inum{};
};
Metric<LinuxBirthPidBtfLayout> resolve_linux_birth_pid_btf(std::span<const std::byte>,unsigned pointer_bytes=8);
struct LinuxBirthExecBtfLayout {std::uint32_t task_mm{},exe_file{},file_inode{},inode_number{},inode_sb{},sb_device{};};
Metric<LinuxBirthExecBtfLayout> resolve_linux_birth_exec_btf(std::span<const std::byte>,unsigned pointer_bytes=8);
}
