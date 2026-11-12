#pragma once
#include "pcinsight/types.hpp"
#include "pcinsight/linux_network_addresses.hpp"
#include <map>
#include <span>
#include <string_view>

namespace pcinsight::detail {
std::optional<std::uint64_t> parse_linux_network_counter(std::string_view);
// Identity from AF_PACKET records in the same getifaddrs result as the IPs.
// Zero or conflicting observations permanently invalidate this row.
class LinuxNetworkAddressIdentity {
    std::optional<std::uint64_t> index_;
public:
    void observe(std::uint64_t index) noexcept {
        if (!index_) index_ = index;
        else if (*index_ != index) index_ = 0;
    }
    bool matches(std::uint64_t index) const noexcept { return index != 0 && index_ == index; }
};
#ifdef __linux__
struct LinuxInterfaceAddresses {
    std::vector<std::string> ipv4, ipv6;
};
using LinuxAddressInventory = std::map<std::uint32_t,LinuxInterfaceAddresses>;
inline std::optional<LinuxInterfaceAddresses> take_linux_interface_addresses(
        LinuxAddressInventory& rows,std::uint64_t sysfs_index,std::uint32_t local_index) {
    if(!local_index||sysfs_index!=local_index)return {};
    auto row=rows.extract(local_index);
    if(row.empty())return {};
    return std::move(row.mapped());
}
enum class LinuxAddressDumpState { more, done, invalid };
LinuxAddressDumpState parse_linux_address_datagram(std::span<const std::byte>,std::uint32_t,
                                                   LinuxAddressInventory&,int* kernel_error=nullptr);
std::optional<LinuxAddressInventory> read_linux_interface_addresses();
Metric<LinuxAddressInventory> linux_address_failure(int,const std::string&);
Metric<LinuxAddressInventory> read_linux_interface_address_report();
Metric<std::vector<LinuxInterfaceAddressList>> project_linux_interface_addresses(std::optional<LinuxAddressInventory>);
#endif
struct LinuxNetworkCounters {
    std::string name;
    std::uint64_t ifindex{}, received{}, sent{};
};
struct LinuxNetworkRates {
    Metric<std::uint64_t> received, sent;
};
class LinuxNetworkRateTracker {
    struct Previous {
        LinuxNetworkCounters counters;
        std::chrono::steady_clock::time_point at;
    };
    std::map<std::string,Previous> previous_;
public:
    std::map<std::string,LinuxNetworkRates> sample(std::span<const LinuxNetworkCounters>,
        std::chrono::steady_clock::time_point);
    void reset() { previous_.clear(); }
};
}
