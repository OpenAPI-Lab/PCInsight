#pragma once
#include "pcinsight/actions.hpp"
#include <functional>
#include <optional>
namespace pcinsight::detail {
struct LinuxPidfdResult { int fd{-1}, error{}; };
struct LinuxIdentityResult { std::uint32_t pid{}; std::uint64_t start_ticks{}; int error{}; };
struct LinuxExitResult { bool exited{}; int error{}; };
struct LinuxTerminateApi {
    std::uint32_t self_pid{};
    std::function<LinuxPidfdResult(std::uint32_t)> open;
    std::function<int()> check_proc_context;
    std::function<LinuxIdentityResult(std::uint32_t)> identity;
    std::function<int(int, bool)> signal; // errno or 0, force selects SIGKILL
    std::function<LinuxExitResult(int, unsigned)> wait_exit; // milliseconds
    std::function<void(int)> close;
};
ActionResult terminate_linux_process(std::uint32_t, std::optional<std::uint64_t>, bool,
    const LinuxTerminateApi&);
ActionResult terminate_linux_process(std::uint32_t, std::optional<std::uint64_t>, bool);
}
