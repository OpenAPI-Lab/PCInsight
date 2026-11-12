#include "linux_network_rates.hpp"
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#ifdef __linux__
#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstring>
#include <linux/rtnetlink.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace pcinsight::detail {
#ifdef __linux__
namespace {
template<class T> T load_netlink(std::span<const std::byte> bytes) {
    T value{};
    std::memcpy(&value,bytes.data(),sizeof value);
    return value;
}
bool add_address(std::span<const std::byte> payload,LinuxAddressInventory& result) {
    if(payload.size()<sizeof(ifaddrmsg))return false;
    const auto address=load_netlink<ifaddrmsg>(payload);
    if(address.ifa_family!=AF_INET&&address.ifa_family!=AF_INET6)return true;
    if(!address.ifa_index)return false;
    auto attrs=payload.subspan(NLMSG_ALIGN(sizeof(ifaddrmsg)));
    std::span<const std::byte> local, peer;
    const std::size_t width=address.ifa_family==AF_INET?4:16;
    while(!attrs.empty()) {
        if(attrs.size()<sizeof(rtattr))return false;
        const auto attr=load_netlink<rtattr>(attrs);
        const auto step=static_cast<std::size_t>(RTA_ALIGN(attr.rta_len));
        if(attr.rta_len<RTA_LENGTH(0)||step>attrs.size())return false;
        if(attr.rta_type==IFA_LOCAL||attr.rta_type==IFA_ADDRESS) {
            auto& field=attr.rta_type==IFA_LOCAL?local:peer;
            if(!field.empty()||attr.rta_len!=RTA_LENGTH(width))return false;
            field=attrs.subspan(RTA_LENGTH(0),width);
        }
        attrs=attrs.subspan(step);
    }
    // IFA_ADDRESS can be the remote endpoint on point-to-point interfaces.
    const auto selected=local.empty()?peer:local;
    if(selected.empty())return true;
    char text[INET6_ADDRSTRLEN]{};
    if(!inet_ntop(address.ifa_family,selected.data(),text,sizeof text))return false;
    auto& row=result[address.ifa_index];
    (address.ifa_family==AF_INET?row.ipv4:row.ipv6).emplace_back(text);
    return true;
}
}

LinuxAddressDumpState parse_linux_address_datagram(std::span<const std::byte> bytes,
        std::uint32_t sequence,LinuxAddressInventory& result,int* kernel_error) {
    if(kernel_error)*kernel_error=0;
    if(bytes.empty())return LinuxAddressDumpState::invalid;
    while(!bytes.empty()) {
        if(bytes.size()<sizeof(nlmsghdr))return LinuxAddressDumpState::invalid;
        const auto header=load_netlink<nlmsghdr>(bytes);
        if(header.nlmsg_len<NLMSG_HDRLEN||header.nlmsg_len>bytes.size()||
           header.nlmsg_seq!=sequence||(header.nlmsg_flags&NLM_F_DUMP_INTR))return LinuxAddressDumpState::invalid;
        const auto step=static_cast<std::size_t>(NLMSG_ALIGN(header.nlmsg_len));
        if(step>bytes.size())return LinuxAddressDumpState::invalid;
        const auto payload=bytes.subspan(NLMSG_HDRLEN,header.nlmsg_len-NLMSG_HDRLEN);
        if(header.nlmsg_type==NLMSG_ERROR) {
            if(payload.size()>=sizeof(nlmsgerr)) {
                const auto error=load_netlink<nlmsgerr>(payload).error;
                if(kernel_error&&error<0&&error>=-4095)*kernel_error=-error;
            }
            return LinuxAddressDumpState::invalid; // ACK is not a completed dump.
        }
        if(header.nlmsg_type==NLMSG_DONE) {
            if(step==bytes.size()&&payload.size()>=sizeof(int)) {
                const auto error=load_netlink<int>(payload);
                if(kernel_error&&error<0&&error>=-4095)*kernel_error=-error;
            }
            if(step!=bytes.size()||(!payload.empty()&&
               (payload.size()<sizeof(int)||load_netlink<int>(payload)!=0)))return LinuxAddressDumpState::invalid;
            return LinuxAddressDumpState::done;
        }
        if(header.nlmsg_type!=RTM_NEWADDR||!add_address(payload,result))return LinuxAddressDumpState::invalid;
        bytes=bytes.subspan(step);
    }
    return LinuxAddressDumpState::more;
}

Metric<LinuxAddressInventory> linux_address_failure(int error,const std::string& stage) {
    return {{},error==EACCES||error==EPERM?Support::permission_required:Support::unsupported,
        stage+"; errno="+std::to_string(error)+"; no complete address report"};
}
Metric<LinuxAddressInventory> read_linux_interface_address_report() {
    struct Socket {int fd;~Socket(){if(fd>=0)close(fd);}} socket_owner{
        socket(AF_NETLINK,SOCK_RAW|SOCK_CLOEXEC|SOCK_NONBLOCK,NETLINK_ROUTE)};
    const auto fd=socket_owner.fd;
    if(fd<0)return linux_address_failure(errno,"Create rtnetlink socket");
    sockaddr_nl kernel{};kernel.nl_family=AF_NETLINK;
    constexpr std::uint32_t sequence=1; // Dedicated socket, one outstanding request.
    struct Request {nlmsghdr header; ifaddrmsg address;} request{};
    request.header.nlmsg_len=NLMSG_LENGTH(sizeof(ifaddrmsg));
    request.header.nlmsg_type=RTM_GETADDR;
    request.header.nlmsg_flags=NLM_F_REQUEST|NLM_F_DUMP;
    request.header.nlmsg_seq=sequence;
    request.address.ifa_family=AF_UNSPEC;
    const auto sent=sendto(fd,&request,request.header.nlmsg_len,0,reinterpret_cast<sockaddr*>(&kernel),sizeof kernel);
    if(sent!=static_cast<ssize_t>(request.header.nlmsg_len))
        return linux_address_failure(sent<0?errno:0,"Send rtnetlink request");
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(100);
    std::array<std::byte,32768> buffer;
    std::size_t total=0;
    LinuxAddressInventory result;
    while(std::chrono::steady_clock::now()<deadline) {
        const auto left=std::chrono::ceil<std::chrono::milliseconds>(deadline-std::chrono::steady_clock::now());
        if(left.count()<=0)return linux_address_failure(0,"Address dump deadline exceeded");
        pollfd ready{fd,POLLIN,0};
        const auto rc=poll(&ready,1,static_cast<int>(left.count()));
        if(rc<0&&errno==EINTR)continue;
        if(rc==0)return linux_address_failure(0,"Address dump deadline exceeded");
        if(rc!=1||!(ready.revents&POLLIN))return linux_address_failure(rc<0?errno:0,"Wait for address dump");
        sockaddr_nl sender{};
        iovec data{buffer.data(),buffer.size()};
        msghdr message{};message.msg_name=&sender;message.msg_namelen=sizeof sender;
        message.msg_iov=&data;message.msg_iovlen=1;
        const auto received=recvmsg(fd,&message,0);
        if(received<0&&(errno==EAGAIN||errno==EINTR))continue;
        if(received<=0)return linux_address_failure(received<0?errno:0,"Receive address dump");
        if((message.msg_flags&MSG_TRUNC)||message.msg_namelen!=sizeof sender||
           sender.nl_family!=AF_NETLINK||sender.nl_pid!=0||sender.nl_groups!=0)
            return linux_address_failure(0,"Truncated address datagram or invalid sender");
        total+=static_cast<std::size_t>(received);
        if(total>4*1024*1024)return linux_address_failure(0,"Address dump exceeds byte budget");
        int kernel_error{};
        const auto state=parse_linux_address_datagram(std::span(buffer).first(received),sequence,result,&kernel_error);
        if(state==LinuxAddressDumpState::invalid)return linux_address_failure(kernel_error,
            kernel_error?"Kernel rejected address dump":"Invalid or interrupted address dump");
        if(state==LinuxAddressDumpState::done)return {std::move(result),Support::available,"Completed rtnetlink address dump"};
    }
    return linux_address_failure(0,"Address dump deadline exceeded");
}
std::optional<LinuxAddressInventory> read_linux_interface_addresses() {
    auto report=read_linux_interface_address_report();
    return metric_is_known(report)?std::move(report.value):std::nullopt;
}
Metric<std::vector<LinuxInterfaceAddressList>> project_linux_interface_addresses(
        std::optional<LinuxAddressInventory> inventory) {
    if(!inventory)return {{},Support::unsupported,
        "Linux address dump unavailable, incomplete or rejected; not a known empty list"};
    std::vector<LinuxInterfaceAddressList> rows;
    rows.reserve(inventory->size());
    for(auto& [index,addresses]:*inventory) {
        if(!index)return {{},Support::unsupported,"Invalid zero interface index; no partial report"};
        rows.push_back({index,std::move(addresses.ipv4),std::move(addresses.ipv6)});
    }
    return {std::move(rows),Support::available,
        "Completed Linux RTM_GETADDR dump in caller network namespace; no claim of atomicity with other inventories"};
}
#endif
std::optional<std::uint64_t> parse_linux_network_counter(std::string_view text) {
    while (!text.empty() && (text.back()=='\n' || text.back()=='\r' || text.back()==' ' || text.back()=='\t')) text.remove_suffix(1);
    if (text.empty()) return {};
    std::uint64_t value{};
    const auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),value);
    if (error!=std::errc{} || end!=text.data()+text.size()) return {};
    return value;
}
namespace {
Metric<std::uint64_t> rate(std::uint64_t delta,long double seconds) {
    const auto value=static_cast<long double>(delta)/seconds;
    // Exclusive 2^64 bound remains safe even where long double has only 53 bits.
    if (!std::isfinite(value) || value>=std::ldexp(1.0L,64))
        return {{},Support::unsupported,"Interface byte rate exceeds uint64 range"};
    return {static_cast<std::uint64_t>(value),Support::estimated,
        "Interface byte-counter delta / monotonic seconds, rounded down; sysfs reads are not atomic"};
}
}

std::map<std::string,LinuxNetworkRates> LinuxNetworkRateTracker::sample(
    std::span<const LinuxNetworkCounters> rows,std::chrono::steady_clock::time_point now) {
    try {
        if (rows.size()>100000) throw std::invalid_argument("Too many network interfaces");
        std::map<std::string,Previous> next;
        std::map<std::string,LinuxNetworkRates> output;
        for (const auto& row:rows) {
            if (row.name.empty() || row.name.size()>4096 || row.name.find('\0')!=row.name.npos || !row.ifindex || next.contains(row.name))
                throw std::invalid_argument("Invalid/duplicate network interface identity");
            LinuxNetworkRates result;
            result.received.note=result.sent.note="Requires two consecutive samples of the same interface with nondecreasing counters";
            const auto found=previous_.find(row.name);
            if (found!=previous_.end()) {
                const auto& old=found->second;
                // Comparing time_points before subtraction avoids signed duration overflow.
                const auto seconds=std::chrono::duration<long double>(now.time_since_epoch()).count()-
                    std::chrono::duration<long double>(old.at.time_since_epoch()).count();
                if (now>old.at && seconds>0 && std::isfinite(seconds) && row.ifindex==old.counters.ifindex &&
                    row.received>=old.counters.received && row.sent>=old.counters.sent) {
                    result.received=rate(row.received-old.counters.received,seconds);
                    result.sent=rate(row.sent-old.counters.sent,seconds);
                }
            }
            output.emplace(row.name,std::move(result));
            next.emplace(row.name,Previous{row,now});
        }
        previous_=std::move(next);
        return output;
    } catch (...) { reset(); throw; }
}
}

namespace pcinsight {
Metric<std::vector<LinuxInterfaceAddressList>> query_linux_network_addresses() {
#ifdef __linux__
    auto report=detail::read_linux_interface_address_report();
    if(!metric_is_known(report))return {{},report.support,report.note};
    return detail::project_linux_interface_addresses(std::move(report.value));
#else
    return {{},Support::unsupported,"Linux address query requires Linux"};
#endif
}
}
