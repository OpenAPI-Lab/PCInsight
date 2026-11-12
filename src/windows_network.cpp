#include "windows_network.hpp"
#ifdef _WIN32
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cwchar>
#include <limits>
#include <system_error>
namespace pcinsight::detail {
namespace {
struct Table {
    void* value = nullptr;
    NetworkApi& api;
    explicit Table(NetworkApi& a) : api(a) {}
    ~Table() { if (value) api.free(value); }
    Table(const Table&) = delete;
};
[[noreturn]] void failure(const char* what, DWORD error) {
    throw std::system_error(static_cast<int>(error),std::system_category(),what);
}
template<std::size_t N> std::string utf8(const wchar_t (&text)[N]) {
    const auto end = std::wmemchr(text,L'\0',N);
    if (!end) failure("unterminated adapter name",ERROR_INVALID_DATA);
    const auto length = static_cast<int>(end-text);
    if (!length) return {};
    const auto count = WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text,length,nullptr,0,nullptr,nullptr);
    if (!count) failure("invalid adapter Unicode",ERROR_INVALID_DATA);
    std::string result(count,'\0');
    if (!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text,length,result.data(),count,nullptr,nullptr))
        failure("adapter Unicode conversion",GetLastError());
    return result;
}
std::string type_name(ULONG type) {
    switch (type) {
        case IF_TYPE_ETHERNET_CSMACD: return "Ethernet";
        case IF_TYPE_IEEE80211: return "Wi-Fi";
        case IF_TYPE_SOFTWARE_LOOPBACK: return "Loopback";
        case IF_TYPE_TUNNEL: return "Tunnel";
        case IF_TYPE_PPP: return "PPP";
        default: return "IANA ifType " + std::to_string(type);
    }
}
Metric<std::uint64_t> rate(std::uint64_t delta,double seconds) {
    const auto value = static_cast<long double>(delta)/seconds;
    if (!std::isfinite(value) || value >= std::numeric_limits<std::uint64_t>::max())
        return {std::nullopt,Support::unsupported,"Byte rate out of range"};
    return {static_cast<std::uint64_t>(value),Support::available,
        "Interface octet counter delta / monotonic seconds; bytes/s rounded down"};
}
}
std::vector<NetworkAdapter> WindowsNetwork::sample(std::chrono::steady_clock::time_point now) {
    try { return collect(now); }
    catch (...) { reset(); throw; }
}
std::vector<NetworkAdapter> WindowsNetwork::collect(std::chrono::steady_clock::time_point now) {
    PMIB_IF_TABLE2 raw = nullptr;
    const auto rc = api_.interfaces(&raw);
    Table interfaces(api_); interfaces.value = raw;
    if (rc != NO_ERROR) failure("GetIfTable2",rc);
    if (!raw || raw->NumEntries > 100000) failure("invalid interface table",ERROR_INVALID_DATA);
    PMIB_UNICASTIPADDRESS_TABLE ipraw = nullptr;
    const auto iprc = api_.addresses(AF_UNSPEC,&ipraw);
    Table ips(api_); ips.value = ipraw;
    if (iprc != NO_ERROR) failure("GetUnicastIpAddressTable",iprc);
    if (!ipraw || ipraw->NumEntries > 100000) failure("invalid address table",ERROR_INVALID_DATA);
    std::vector<NetworkAdapter> result;
    std::map<std::uint64_t,Previous> next;
    std::map<std::uint64_t,std::size_t> index;
    for (ULONG i = 0; i < raw->NumEntries; ++i) {
        const auto& row = raw->Table[i]; const auto id = row.InterfaceLuid.Value;
        if (!id || index.contains(id)) failure("invalid or duplicate interface LUID",ERROR_INVALID_DATA);
        NetworkAdapter adapter;
        adapter.id = "luid:" + std::to_string(id);
        adapter.name = utf8(row.Alias); adapter.model = utf8(row.Description);
        adapter.connection_type = type_name(row.Type);
        adapter.total_sent_bytes = row.OutOctets; adapter.total_received_bytes = row.InOctets;
        adapter.send_bytes_per_second.note = adapter.receive_bytes_per_second.note =
            "Requires consecutive samples with same identity/state and nondecreasing counters";
        const auto found = previous_.find(id);
        if (found != previous_.end()) {
            const auto& previous = found->second;
            const auto seconds = std::chrono::duration<double>(now-previous.captured).count();
            if (seconds > 0 && std::isfinite(seconds) && previous.type == row.Type && previous.state == row.OperStatus &&
                std::memcmp(&previous.guid,&row.InterfaceGuid,sizeof(GUID)) == 0 &&
                row.OutOctets >= previous.sent && row.InOctets >= previous.received) {
                adapter.send_bytes_per_second = rate(row.OutOctets-previous.sent,seconds);
                adapter.receive_bytes_per_second = rate(row.InOctets-previous.received,seconds);
            }
        }
        next.emplace(id,Previous{row.InterfaceGuid,row.Type,row.OperStatus,row.OutOctets,row.InOctets,now});
        index.emplace(id,result.size()); result.push_back(std::move(adapter));
    }
    for (ULONG i = 0; i < ipraw->NumEntries; ++i) {
        const auto& row = ipraw->Table[i]; const auto found = index.find(row.InterfaceLuid.Value);
        if (found == index.end()) continue;
        char buffer[INET6_ADDRSTRLEN]{};
        std::string address;
        if (row.Address.si_family == AF_INET) {
            if (!InetNtopA(AF_INET,&row.Address.Ipv4.sin_addr,buffer,sizeof buffer)) failure("InetNtop IPv4",WSAGetLastError());
            address = buffer;
        } else if (row.Address.si_family == AF_INET6) {
            if (!InetNtopA(AF_INET6,&row.Address.Ipv6.sin6_addr,buffer,sizeof buffer)) failure("InetNtop IPv6",WSAGetLastError());
            address = buffer;
            if (row.Address.Ipv6.sin6_scope_id) address += "%" + std::to_string(row.Address.Ipv6.sin6_scope_id);
        } else continue;
        auto& target = row.Address.si_family == AF_INET ? result[found->second].ipv4 : result[found->second].ipv6;
        if (std::find(target.begin(),target.end(),address) == target.end()) target.push_back(std::move(address));
    }
    previous_ = std::move(next);
    return result;
}
}
#endif
