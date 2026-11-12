#pragma once
#include "pcinsight/linux_tcp.hpp"
#include <span>
namespace pcinsight::detail {
Metric<std::uint32_t> parse_linux_tcp_pidfd_pid(std::string_view text);
Metric<LinuxTcpHeldSocketSample> update_linux_tcp_holder(
    std::optional<LinuxTcpHeldSocketSample>& previous, Metric<LinuxTcpHeldSocketSample> current);
class LinuxTcpTracker {
    std::optional<LinuxTcpSnapshot> previous_;
public:
    LinuxTcpSnapshot apply(LinuxTcpSnapshot snapshot);
    void reset(){previous_.reset();}
};
#ifdef __linux__
struct LinuxTcpDumpParser {
    std::uint32_t sequence{}, port{};
    std::uint8_t ip_version{};
    bool done{};
    std::size_t wire_bytes{}, messages{};
    std::vector<LinuxTcpSocketSample> rows;
    void consume(std::span<const std::byte> packet,std::chrono::steady_clock::time_point at);
};
#endif
}
