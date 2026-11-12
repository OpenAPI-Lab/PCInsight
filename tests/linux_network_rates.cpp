#include "../src/linux_network_rates.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#ifdef __linux__
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <linux/rtnetlink.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
using namespace std::chrono_literals;
using Clock=std::chrono::steady_clock;
int main() {
#ifdef __linux__
    for(const auto error:{EACCES,EPERM,EIO,ETIMEDOUT}) {
        const auto failed=linux_address_failure(error,"fixture stage");
        assert(!failed.value&&failed.support==((error==EACCES||error==EPERM)?Support::permission_required:Support::unsupported));
        assert(failed.note.find("fixture stage")!=std::string::npos);
    }
    assert(!metric_is_known(project_linux_interface_addresses(std::nullopt)));
    const auto empty_addresses=project_linux_interface_addresses(LinuxAddressInventory{});
    assert(metric_is_known(empty_addresses)&&empty_addresses.value->empty());
    LinuxAddressInventory invalid_addresses;
    invalid_addresses[0].ipv4={"192.0.2.1"};
    assert(!metric_is_known(project_linux_interface_addresses(std::move(invalid_addresses))));
    // A labelled point-to-point address must use the kernel index and local IP.
    struct AddressFixture {
        nlmsghdr header;
        ifaddrmsg address;
        rtattr label;
        char label_text[8];
        rtattr peer;
        unsigned char peer_ip[4];
        rtattr local;
        unsigned char local_ip[4];
    } packet{{sizeof(AddressFixture),RTM_NEWADDR,NLM_F_MULTI,7,0},
        {AF_INET,24,0,0,23},{RTA_LENGTH(8),IFA_LABEL},"lo:test",
        {RTA_LENGTH(4),IFA_ADDRESS},{192,0,2,2},{RTA_LENGTH(4),IFA_LOCAL},{192,0,2,1}};
    LinuxAddressInventory indexed;
    const auto bytes=std::as_bytes(std::span(&packet,1));
    assert(parse_linux_address_datagram(bytes,7,indexed)==LinuxAddressDumpState::more);
    assert(indexed.at(23).ipv4==std::vector<std::string>{"192.0.2.1"});
    const auto validate_mutation=[](std::span<const std::byte> input) {
        LinuxAddressInventory output;
        (void)parse_linux_address_datagram(input,7,output);
        for(const auto& [index,row]:output) {
            assert(index!=0);
            in_addr ipv4{};in6_addr ipv6{};
            for(const auto& ip:row.ipv4)assert(inet_pton(AF_INET,ip.c_str(),&ipv4)==1);
            for(const auto& ip:row.ipv6)assert(inet_pton(AF_INET6,ip.c_str(),&ipv6)==1);
        }
    };
    // Bounded deterministic mutations, including unaligned input buffers.
    for(std::size_t offset=0;offset<bytes.size();++offset)for(unsigned bit=0;bit<8;++bit) {
        std::vector<std::byte> mutated(bytes.size()+1);
        std::copy(bytes.begin(),bytes.end(),mutated.begin()+1);
        mutated[offset+1]^=std::byte{static_cast<unsigned char>(1u<<bit)};
        validate_mutation(std::span(mutated).subspan(1));
    }
    std::uint32_t seed=0x5043494e;
    for(unsigned iteration=0;iteration<4096;++iteration) {
        seed=seed*1664525u+1013904223u;
        std::vector<std::byte> arbitrary(seed%257);
        for(auto& byte:arbitrary){seed=seed*1664525u+1013904223u;byte=std::byte(seed>>24);}
        validate_mutation(arbitrary);
    }
    assert(parse_linux_address_datagram(bytes,8,indexed)==LinuxAddressDumpState::invalid);
    for(std::size_t length=0;length<bytes.size();++length) {
        LinuxAddressInventory truncated;
        assert(parse_linux_address_datagram(bytes.first(length),7,truncated)==LinuxAddressDumpState::invalid);
        assert(truncated.empty());
    }
    packet.local.rta_len=1;
    assert(parse_linux_address_datagram(bytes,7,indexed)==LinuxAddressDumpState::invalid);
    nlmsghdr done{sizeof(nlmsghdr),NLMSG_DONE,NLM_F_MULTI,7,0};
    const auto done_bytes=std::as_bytes(std::span(&done,1));
    assert(parse_linux_address_datagram(done_bytes,7,indexed)==LinuxAddressDumpState::done);
    done.nlmsg_flags|=NLM_F_DUMP_INTR;
    assert(parse_linux_address_datagram(done_bytes,7,indexed)==LinuxAddressDumpState::invalid);
    struct DumpError { nlmsghdr header; int code; } failed{
        {NLMSG_LENGTH(sizeof(int)),NLMSG_DONE,NLM_F_MULTI,7,0},-ENOBUFS};
    const auto failed_bytes=std::as_bytes(std::span(&failed,1));
    assert(parse_linux_address_datagram(failed_bytes,7,indexed)==LinuxAddressDumpState::invalid);
    failed.header.nlmsg_type=NLMSG_ERROR;
    assert(parse_linux_address_datagram(failed_bytes,7,indexed)==LinuxAddressDumpState::invalid);
    struct KernelError {nlmsghdr header;nlmsgerr error;} denial{
        {NLMSG_LENGTH(sizeof(nlmsgerr)),NLMSG_ERROR,0,7,0},{-EPERM,{}}};
    const auto denial_bytes=std::as_bytes(std::span(&denial,1));
    int kernel_error=123;
    assert(parse_linux_address_datagram(denial_bytes,7,indexed,&kernel_error)==LinuxAddressDumpState::invalid&&kernel_error==EPERM);
    assert(linux_address_failure(kernel_error,"kernel rejection").support==Support::permission_required);
    denial.error.error=0;
    assert(parse_linux_address_datagram(denial_bytes,7,indexed,&kernel_error)==LinuxAddressDumpState::invalid&&kernel_error==0);
    denial.error.error=std::numeric_limits<int>::min();
    assert(parse_linux_address_datagram(denial_bytes,7,indexed,&kernel_error)==LinuxAddressDumpState::invalid&&kernel_error==0);
    assert(!take_linux_interface_addresses(indexed,23,0));
    assert(!take_linux_interface_addresses(indexed,23,24));
    const auto matching_addresses=take_linux_interface_addresses(indexed,23,23);
    assert(matching_addresses&&matching_addresses->ipv4==std::vector<std::string>{"192.0.2.1"});
    assert(!take_linux_interface_addresses(indexed,23,23));
#endif
    LinuxNetworkAddressIdentity address_identity;
    assert(!address_identity.matches(7));
    address_identity.observe(7);address_identity.observe(7);
    assert(address_identity.matches(7)&&!address_identity.matches(8));
    address_identity.observe(8);address_identity.observe(7);
    assert(!address_identity.matches(7)&&!address_identity.matches(8)&&!address_identity.matches(0));
    LinuxNetworkAddressIdentity invalid_address_identity;
    invalid_address_identity.observe(0);invalid_address_identity.observe(7);
    assert(!invalid_address_identity.matches(7));
    assert(parse_linux_network_counter("0\n")==0);
    assert(parse_linux_network_counter("18446744073709551615\n")==std::numeric_limits<std::uint64_t>::max());
    for (auto text:{"","-1","+1"," 1","1x","1 2","18446744073709551616"}) assert(!parse_linux_network_counter(text));
    LinuxNetworkRateTracker tracker;
    std::vector<LinuxNetworkCounters> rows{{"test0",1,100,200}};
    const Clock::time_point start(10s);
    auto sample=[&](auto at) { return tracker.sample(rows,at).at("test0"); };
    assert(!sample(start).received.value);
    rows[0].received+=1000;rows[0].sent+=500;
    auto rate=sample(start+2s);
    assert(rate.received.value==500 && rate.sent.value==250);
    assert(rate.received.support==Support::estimated);
    rate=sample(start+3s);assert(rate.received.value==0 && rate.sent.value==0);
    // Reset either counter invalidates both directions, not unsigned wraparound.
    rows[0].received=1;
    assert(!sample(start+4s).sent.value);
    rows[0].received+=100;rows[0].sent+=200;
    rate=sample(start+5s);assert(rate.received.value==100 && rate.sent.value==200);
    rows[0].sent=0;assert(!sample(start+6s).received.value);
    rows[0].ifindex=2;assert(!sample(start+7s).received.value);
    tracker.sample({},start+8s);assert(!sample(start+9s).received.value);
    tracker.reset();assert(!sample(start+10s).received.value);
    assert(!sample(start+10s).received.value); // no interval
    assert(!sample(start+9s).received.value); // backwards, re-baseline
    assert(sample(start+10s).received.value==0);
    rows[0].name="renamed";
    assert(!tracker.sample(rows,start+11s).at("renamed").received.value);
    rows[0].name="test0";tracker.reset();rows[0].received=0;rows[0].sent=0;
    sample(start);
    rows[0].received=std::numeric_limits<std::uint64_t>::max();rows[0].sent=1;
    rate=sample(start+1ns);
    assert(!rate.received.value && rate.sent.value && *rate.sent.value>0);
    assert(rate.received.support==Support::unsupported);
    // Rejected samples clear continuity, but never publish a partial vector.
    rows.push_back(rows[0]);bool threw=false;
    try { tracker.sample(rows,start+1s); } catch(const std::invalid_argument&) { threw=true; }
    assert(threw);rows.pop_back();assert(!sample(start+2s).received.value);
    for (const auto& bad:std::vector<LinuxNetworkCounters>{{"",1,0,0},{"x",0,0,0},{std::string("x\0y",3),1,0,0},{std::string(4097,'x'),1,0,0}}) {
        threw=false;try { tracker.sample(std::span(&bad,1),start); } catch(const std::invalid_argument&) { threw=true; }
        assert(threw);
    }
    {
        std::vector<LinuxNetworkCounters> too_many(100001);
        threw=false;try { tracker.sample(too_many,start); } catch(const std::invalid_argument&) { threw=true; }
        assert(threw);
    }
    rows[0].received=0;rows[0].sent=0;tracker.reset();sample(start);
    rows[0].received=5;rows[0].sent=3;
    rate=sample(start+2s);assert(rate.received.value==2 && rate.sent.value==1);
    tracker.reset();sample(Clock::time_point::min());
    // Does not overflow a signed duration even across the clock's full range.
    assert(sample(Clock::time_point::max()).received.value==0);
#ifdef __linux__
    SampleOptions options;
    options.include_processes=false;options.include_app_history=false;
    options.include_user_sessions=false;options.include_services=false;
    options.include_startup_entries=false;options.include_disks=false;options.include_gpus=false;
    Monitor monitor;
    auto loopback=[](const SystemSnapshot& snapshot)->const NetworkAdapter& {
        for (const auto& adapter:snapshot.networks) if (adapter.name=="lo") return adapter;
        throw std::runtime_error("loopback missing from native inventory");
    };
    const auto first=monitor.sample(options);
    assert(!loopback(first).receive_bytes_per_second.value);
    const auto fd=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);assert(fd>=0);
    struct Socket { int fd;~Socket(){if(fd>=0)close(fd);} } socket_owner{fd};
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(fd,reinterpret_cast<sockaddr*>(&address),sizeof address)==0);
    socklen_t length=sizeof address;assert(getsockname(fd,reinterpret_cast<sockaddr*>(&address),&length)==0);
    const std::string payload(1024,'x');
    for (int i=0;i<16;++i) {
        assert(sendto(fd,payload.data(),payload.size(),0,reinterpret_cast<sockaddr*>(&address),length)==1024);
        pollfd ready{fd,POLLIN,0};assert(poll(&ready,1,1000)==1);
        char buffer[2048];assert(recv(fd,buffer,sizeof buffer,MSG_DONTWAIT)==1024);
    }
    const int ipv6_fd=socket(AF_INET6,SOCK_DGRAM|SOCK_CLOEXEC,0);
    Socket ipv6_owner{ipv6_fd};
    bool ipv6_available=false;
    if(ipv6_fd<0){
        assert(errno==EAFNOSUPPORT||errno==EPROTONOSUPPORT);
    }else{
        sockaddr_in6 ipv6_address{};
        ipv6_address.sin6_family=AF_INET6;
        ipv6_address.sin6_addr=in6addr_loopback;
        ipv6_available=bind(ipv6_fd,reinterpret_cast<sockaddr*>(&ipv6_address),sizeof ipv6_address)==0;
        if(!ipv6_available)assert(errno==EADDRNOTAVAIL);
    }
    std::this_thread::sleep_for(20ms);
    const auto second=monitor.sample(options);const auto& lo=loopback(second);
    assert(std::find(lo.ipv4.begin(),lo.ipv4.end(),"127.0.0.1")!=lo.ipv4.end());
    if(const auto direct=read_linux_interface_addresses()) {
        assert(lo.linux_details.value);
        const auto public_addresses=query_linux_network_addresses();
        assert(metric_is_known(public_addresses));
        bool public_loopback=false;
        for(const auto& row:*public_addresses.value)if(row.interface_index==lo.linux_details.value->interface_index)
            public_loopback=std::find(row.ipv4.begin(),row.ipv4.end(),"127.0.0.1")!=row.ipv4.end();
        assert(public_loopback);
        const auto& row=direct->at(lo.linux_details.value->interface_index);
        assert(std::find(row.ipv4.begin(),row.ipv4.end(),"127.0.0.1")!=row.ipv4.end());
        if(ipv6_available)assert(std::find(row.ipv6.begin(),row.ipv6.end(),"::1")!=row.ipv6.end());
        std::cout<<"Native rtnetlink: loopback addresses verified by index\n";
    }
    if(ipv6_available){
        assert(std::find(lo.ipv6.begin(),lo.ipv6.end(),"::1")!=lo.ipv6.end());
        std::cout<<"Native IPv6 loopback: ::1 associated with lo\n";
    }else{
        std::cout<<"Native IPv6 loopback unavailable; IPv6 assertion not exercised\n";
    }
    assert(lo.total_received_bytes>loopback(first).total_received_bytes);
    assert(lo.receive_bytes_per_second.value && *lo.receive_bytes_per_second.value>0);
    assert(lo.send_bytes_per_second.value && *lo.send_bytes_per_second.value>0);
    options.include_networks=false;assert(monitor.sample(options).networks.empty());
    options.include_networks=true;
    const auto resumed=monitor.sample(options);
    assert(!loopback(resumed).receive_bytes_per_second.value);
    std::cout<<"Native Linux loopback: positive send/receive rates, opt-out/reset verified\n";
#endif
    std::cout<<"Linux network rates: interval/reset/reuse/gaps/zero/overflow/parser checks passed\n";
}
