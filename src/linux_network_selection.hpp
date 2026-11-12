#pragma once
#include "pcinsight/linux_process_network_selection.hpp"
#include "linux_network_self.hpp"
#include "linux_network_session.hpp"
namespace pcinsight::detail {
struct NetworkSelectionContext {
    std::array<std::uint64_t,2> boot{};
    LinuxNetworkSelfIdentity observer;
    auto operator<=>(const NetworkSelectionContext&) const = default;
};
struct NetworkSelectionSourceApi {
    void* context=nullptr;
    std::uint64_t (*caller)(void*) noexcept=nullptr;
    Metric<NetworkSelectionContext> (*observe)(void*)=nullptr;
    NetworkSessionFd (*open)(void*,std::uint32_t)=nullptr;
    Metric<LinuxNetworkSelfIdentity> (*inspect)(void*,int,std::uint32_t,std::uint64_t)=nullptr;
    void (*close)(void*,int) noexcept=nullptr;
    explicit operator bool()const{return caller&&observe&&open&&inspect&&close;}
};
NetworkSelectionSourceApi native_network_selection_source_api();
// Injected contexts must outlive the owner. No capture side effects.
class LinuxNetworkSelectionOwner {
    struct OwnedFd {NetworkSelectionSourceApi api;int fd=-1;~OwnedFd(){if(fd>=0)api.close(api.context,fd);}};
    struct Entry {LinuxNetworkProcessCandidate candidate;std::shared_ptr<OwnedFd> handle;LinuxNetworkSelfIdentity process;};
    NetworkSelectionSourceApi api_;
    std::vector<Entry> entries_;
    std::optional<NetworkSelectionContext> context_;
    std::uint64_t creator_{},generation_{};
public:
    explicit LinuxNetworkSelectionOwner(NetworkSelectionSourceApi api=native_network_selection_source_api()):api_(api){}
    ~LinuxNetworkSelectionOwner(){clear();}
    LinuxNetworkSelectionOwner(const LinuxNetworkSelectionOwner&)=delete;
    LinuxNetworkSelectionOwner& operator=(const LinuxNetworkSelectionOwner&)=delete;
    Metric<LinuxNetworkSelectionChange> refresh(bool,std::span<const LinuxNetworkProcessCandidate>);
    Metric<std::vector<LinuxProcessNetworkSelection>> selections();
    void clear() noexcept {entries_.clear();context_.reset();creator_=generation_=0;}
};
}
