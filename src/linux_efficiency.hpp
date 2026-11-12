#pragma once
#include "pcinsight/actions.hpp"
#include "pcinsight/linux_efficiency.hpp"
#include "linux_terminate.hpp"
#include <functional>
namespace pcinsight::detail {
struct LinuxPriorityResult { int value{}, error{}; };
struct LinuxEfficiencyApi {
    std::uint32_t self_pid{};
    std::function<LinuxPriorityResult(std::uint32_t)> read_nice, read_io;
    std::function<int(std::uint32_t, int)> set_nice, set_io; // errno or 0
};
ActionResult set_linux_efficiency(std::uint32_t, bool, const LinuxEfficiencyApi&);
ActionResult set_linux_efficiency(std::uint32_t, bool);
LinuxEfficiencyApi native_linux_efficiency_api();
struct LinuxThreadList { std::vector<std::uint32_t> tids; int error{}; };
struct LinuxThreadEfficiencyApi {
    LinuxEfficiencyApi priorities;
    std::function<int()> check_context;
    std::function<LinuxThreadList(std::uint32_t)> list;
    // tid=pid reads the leader; otherwise /proc/pid/task/tid/stat.
    std::function<LinuxIdentityResult(std::uint32_t, std::uint32_t)> identity;
};
LinuxEfficiencyUpdate apply_linux_efficiency_unchecked(std::uint32_t, std::uint64_t,
    bool, const LinuxThreadEfficiencyApi&);
LinuxEfficiencyUpdate restore_linux_efficiency_unchecked(const LinuxEfficiencyReceipt&,
    bool, const LinuxThreadEfficiencyApi&);
}
