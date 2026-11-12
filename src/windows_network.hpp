#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "pcinsight/types.hpp"
#include <map>
namespace pcinsight::detail {
struct NetworkApi {
    decltype(&GetIfTable2) interfaces = &GetIfTable2;
    decltype(&GetUnicastIpAddressTable) addresses = &GetUnicastIpAddressTable;
    decltype(&FreeMibTable) free = &FreeMibTable;
};
class WindowsNetwork {
public:
    explicit WindowsNetwork(NetworkApi api = {}) : api_(api) {}
    std::vector<NetworkAdapter> sample(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
    void reset() { previous_.clear(); }
private:
    struct Previous {
        GUID guid{}; ULONG type{}; IF_OPER_STATUS state{};
        std::uint64_t sent{}, received{};
        std::chrono::steady_clock::time_point captured;
    };
    NetworkApi api_;
    std::map<std::uint64_t,Previous> previous_;
    std::vector<NetworkAdapter> collect(std::chrono::steady_clock::time_point);
};
}
#endif
