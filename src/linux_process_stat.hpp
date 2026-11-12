#pragma once
#include <cstdint>
#include <optional>
#include <string_view>
#include <span>
#include <string>
namespace pcinsight::detail {
struct LinuxProcessStat {
    std::uint32_t pid{}, parent_pid{};
    std::uint64_t ticks{}, threads{}, start_ticks{};
    std::uint64_t user_ticks{},kernel_ticks{};
};
std::optional<LinuxProcessStat> parse_linux_process_stat(std::string_view);
std::optional<std::uint64_t> linux_process_tick_delta(const LinuxProcessStat&, const LinuxProcessStat&);
std::optional<std::uint64_t> parse_linux_process_io(std::string_view);
std::string linux_elf_architecture(std::span<const unsigned char>);
}
