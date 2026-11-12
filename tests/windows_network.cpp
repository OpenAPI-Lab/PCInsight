#include "../src/windows_network.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cwchar>
#include <iostream>
#include <system_error>
#include <thread>
using namespace pcinsight::detail;
namespace {
struct IfTable { ULONG NumEntries; MIB_IF_ROW2 Table[2]; } interfaces;
struct IpTable { ULONG NumEntries; MIB_UNICASTIPADDRESS_ROW Table[3]; } addresses;
static_assert(offsetof(IfTable,Table)==offsetof(MIB_IF_TABLE2,Table));
static_assert(offsetof(IpTable,Table)==offsetof(MIB_UNICASTIPADDRESS_TABLE,Table));
DWORD if_error, ip_error;
unsigned freed;
BOOL null_interfaces, null_addresses;
NETIO_STATUS WINAPI get_interfaces(PMIB_IF_TABLE2* result) {
    *result=null_interfaces ? nullptr : reinterpret_cast<PMIB_IF_TABLE2>(&interfaces); return if_error;
}
NETIO_STATUS WINAPI get_addresses(ADDRESS_FAMILY family, PMIB_UNICASTIPADDRESS_TABLE* result) {
    assert(family==AF_UNSPEC);
    *result=null_addresses ? nullptr : reinterpret_cast<PMIB_UNICASTIPADDRESS_TABLE>(&addresses); return ip_error;
}
void WINAPI free_table(PVOID p) { assert(p==&interfaces || p==&addresses); ++freed; }
NetworkApi api{get_interfaces,get_addresses,free_table};
void reset() {
    interfaces={}; addresses={}; if_error=ip_error=freed=0; null_interfaces=null_addresses=FALSE;
    interfaces.NumEntries=2; addresses.NumEntries=3;
    for (int i=0;i<2;++i) {
        auto& row=interfaces.Table[i]; row.InterfaceLuid.Value=i+1;
        row.InterfaceGuid.Data1=i+20; row.Type=i ? IF_TYPE_IEEE80211 : IF_TYPE_ETHERNET_CSMACD;
        row.OperStatus=IfOperStatusUp;
        std::wcscpy(row.Alias,i ? L"Wi-Fi" : L"Ethernet\u00e9");
        std::wcscpy(row.Description,L"Native adapter description");
        row.OutOctets=100; row.InOctets=200;
    }
    auto& v4=addresses.Table[0]; v4.InterfaceLuid.Value=1; v4.Address.Ipv4.sin_family=AF_INET;
    auto* bytes=reinterpret_cast<unsigned char*>(&v4.Address.Ipv4.sin_addr);
    bytes[0]=192; bytes[1]=0; bytes[2]=2; bytes[3]=1;
    addresses.Table[1]=v4; // Duplicate address must not duplicate output.
    auto& v6=addresses.Table[2]; v6.InterfaceLuid.Value=2; v6.Address.Ipv6.sin6_family=AF_INET6;
    v6.Address.Ipv6.sin6_addr.u.Byte[0]=0xfe; v6.Address.Ipv6.sin6_addr.u.Byte[1]=0x80;
    v6.Address.Ipv6.sin6_addr.u.Byte[15]=1; v6.Address.Ipv6.sin6_scope_id=7;
}
void fails(WindowsNetwork& network, DWORD expected) {
    try { (void)network.sample(); assert(false); }
    catch(const std::system_error& e) { assert(e.code().value()==static_cast<int>(expected)); }
}
}
int main() {
    const auto now=std::chrono::steady_clock::now();
    reset(); WindowsNetwork network(api); auto first=network.sample(now);
    assert(freed==2 && first.size()==2 && first[0].id=="luid:1");
    assert(first[0].name=="Ethernet\xc3\xa9" && first[0].connection_type=="Ethernet");
    assert(first[0].ipv4==std::vector<std::string>{"192.0.2.1"});
    assert(first[1].ipv6==std::vector<std::string>{"fe80::1%7"});
    assert(!first[0].send_bytes_per_second.value && first[0].total_sent_bytes==100);
    interfaces.Table[0].OutOctets+=1000; interfaces.Table[0].InOctets+=2000;
    auto next=network.sample(now+std::chrono::seconds(2));
    assert(next[0].send_bytes_per_second.value==500 && next[0].receive_bytes_per_second.value==1000);
    assert(next[1].send_bytes_per_second.value==0);
    interfaces.Table[0].OutOctets=0;
    next=network.sample(now+std::chrono::seconds(3)); assert(!next[0].send_bytes_per_second.value);
    next=network.sample(now+std::chrono::seconds(4)); assert(next[0].send_bytes_per_second.value==0);
    interfaces.Table[0].InterfaceGuid.Data1++;
    next=network.sample(now+std::chrono::seconds(5)); assert(!next[0].receive_bytes_per_second.value);
    interfaces.Table[0].OperStatus=IfOperStatusDown;
    next=network.sample(now+std::chrono::seconds(6)); assert(!next[0].send_bytes_per_second.value);
    next=network.sample(now+std::chrono::seconds(6)); assert(!next[1].send_bytes_per_second.value);
    network.reset(); assert(!network.sample(now+std::chrono::seconds(7))[0].send_bytes_per_second.value);
    if_error=ERROR_ACCESS_DENIED; freed=0; fails(network,ERROR_ACCESS_DENIED); assert(freed==1);
    if_error=0; assert(!network.sample(now+std::chrono::seconds(8))[0].send_bytes_per_second.value);
    ip_error=ERROR_NOT_ENOUGH_MEMORY; freed=0; fails(network,ip_error); assert(freed==2);
    reset(); null_interfaces=TRUE; fails(network,ERROR_INVALID_DATA);
    reset(); null_addresses=TRUE; fails(network,ERROR_INVALID_DATA);
    reset(); interfaces.NumEntries=100001; fails(network,ERROR_INVALID_DATA);
    reset(); addresses.NumEntries=100001; fails(network,ERROR_INVALID_DATA);
    reset(); interfaces.Table[1].InterfaceLuid.Value=1; fails(network,ERROR_INVALID_DATA);
    reset(); std::wmemset(interfaces.Table[0].Alias,L'x',IF_MAX_STRING_SIZE+1); fails(network,ERROR_INVALID_DATA);
    reset(); interfaces.NumEntries=addresses.NumEntries=0; assert(network.sample().empty());
    reset(); assert(!network.sample()[0].send_bytes_per_second.value);
    // Native integration reads only; it does not create network traffic.
    pcinsight::Monitor monitor; pcinsight::SampleOptions options;
    options.include_processes=false; options.include_gpus=false; options.include_disks=false;
    auto native=monitor.sample(options); assert(!native.networks.empty());
    for(const auto& adapter:native.networks) assert(!adapter.id.empty() && !adapter.send_bytes_per_second.value);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const auto measured=monitor.sample(options); unsigned native_rates=0;
    for(const auto& adapter:measured.networks) {
        if(adapter.send_bytes_per_second.value&&adapter.receive_bytes_per_second.value)++native_rates;
    }
    assert(native_rates>0);
    options.include_networks=false; assert(monitor.sample(options).networks.empty());
    options.include_networks=true;
    for(const auto& adapter:monitor.sample(options).networks) assert(!adapter.send_bytes_per_second.value);
    std::cout << "Native network interfaces: " << native.networks.size() << "; measured send/receive rates=" << native_rates << '\n';
}
