#pragma once
#include "linux_network_probe.hpp"
#include <span>
namespace pcinsight::detail {
struct NetworkSessionFd {int fd=-1,error=0;};
struct NetworkSessionRun {int error=0;std::uint32_t returned=0;};
struct NetworkSessionApi {
    void* context=nullptr; // Borrowed; must outlive the session.
    Metric<LinuxNetworkBtfLayout> (*layout)(void*)=nullptr;
    NetworkSessionFd (*map)(void*,unsigned)=nullptr;
    NetworkSessionFd (*load)(void*,std::span<const NetworkProbeInstruction>,std::uint32_t,bool)=nullptr;
    NetworkSessionRun (*run)(void*,int)=nullptr;
    Metric<std::uint64_t> (*lookup)(void*,int,std::uint32_t)=nullptr;
    NetworkSessionFd (*attach)(void*,int)=nullptr;
    void (*close)(void*,int) noexcept=nullptr;
    int (*update)(void*,int,std::uint32_t,std::uint64_t)=nullptr; // Optional; errno, zero on success.
    NetworkSessionFd (*actor_map)(void*,unsigned)=nullptr; // Optional HASH u32->u64, no eviction.
    int (*erase)(void*,int,std::uint32_t)=nullptr; // Optional HASH deletion; errno, zero on success.
    explicit operator bool() const {return layout&&map&&load&&run&&lookup&&attach&&close;}
};
NetworkSessionApi native_network_session_api();
// Explicit privileged, unattached caller-only discovery; not an external PID lookup.
Metric<PrivateNetworkIdentity> read_private_network_caller_identity(const NetworkSessionApi&,const LinuxNetworkBtfLayout&);
enum class PrivateNetworkSessionState { stopped, active, detached };
enum class PrivateNetworkTargetKind { self, selected_actor };
struct PrivateNetworkSessionRead {
    PrivateNetworkIdentity identity;
    PrivateNetworkQualityCounters counters;
    PrivateNetworkSessionState state=PrivateNetworkSessionState::stopped;
    std::optional<std::uint64_t> expected_socket_network_namespace_cookie;
    PrivateNetworkTargetKind target_kind=PrivateNetworkTargetKind::self;
    bool filters_other_socket_namespaces=false;
};
struct PrivateDiscoveredNetworkSessionRead {
    PrivateNetworkIdentity identity;
    std::vector<PrivateDiscoveredNetworkBucket> buckets;
    std::uint64_t completed_updates{};
    PrivateNetworkSessionState state=PrivateNetworkSessionState::stopped;
};
// Internal opt-in single-actor capture. No Monitor activation, automatic target discovery,
// namespace join, reducer/history publication or claim of complete network I/O.
class LinuxPrivateNetworkSession {
public:
    explicit LinuxPrivateNetworkSession(NetworkSessionApi api=native_network_session_api()):api_(api){}
    ~LinuxPrivateNetworkSession(){stop();}
    LinuxPrivateNetworkSession(const LinuxPrivateNetworkSession&)=delete;
    LinuxPrivateNetworkSession& operator=(const LinuxPrivateNetworkSession&)=delete;
    Metric<PrivateNetworkIdentity> start(bool explicitly_authorized,std::uint64_t expected_socket_netns_cookie=0);
    // Caller-declared kernel-domain token, NOT authenticated procfs/PID mapping.
    Metric<PrivateNetworkIdentity> start_selected(bool explicitly_authorized,PrivateNetworkIdentity,std::uint64_t socket_netns_cookie,bool filter_other_namespaces=false);
    // INTERNAL actor token only, not an authenticated public PID selection.
    // Discovers up to16 observed socket cookies; quality loss is sticky.
    Metric<PrivateNetworkIdentity> start_discovering_selected(bool explicitly_authorized,PrivateNetworkIdentity);
    Metric<PrivateNetworkSessionRead> read();
    Metric<PrivateDiscoveredNetworkSessionRead> read_discovered();
    Metric<PrivateNetworkIdentity> read_caller_identity(); // Unattached TEST_RUN; never read another PID.
    void detach() noexcept; // Close links/programs; preserve map, still validate read quality.
    void stop() noexcept; // Close every owned FD. Safe in a forked child's copy.
    PrivateNetworkSessionState state() const {return state_;}
private:
    Metric<PrivateNetworkIdentity> start_impl(bool,std::uint64_t,const PrivateNetworkIdentity*,bool=false,bool=false);
    void close(int&) noexcept;
    NetworkSessionApi api_;
    PrivateNetworkSessionState state_=PrivateNetworkSessionState::stopped;
    PrivateNetworkIdentity identity_{};
    PrivateNetworkTargetKind target_kind_=PrivateNetworkTargetKind::self;
    std::optional<LinuxNetworkBtfLayout> layout_;
    std::uint64_t expected_netns_cookie_{};
    bool filter_other_namespaces_=false;
    bool discover_=false;
    int map_=-1;
    std::array<int,3> programs_{-1,-1,-1},links_{-1,-1,-1};
};
}
