#pragma once
#include "linux_network_sampler.hpp"
#include "linux_network_join.hpp"
namespace pcinsight::detail {
struct SelectedNetworkSourceApi {
    void* context=nullptr;
    NetworkSessionFd (*hold)(void*,int)=nullptr;
    void (*close)(void*,int) noexcept=nullptr;
    Metric<PrivatePidfdNetworkJoin> (*join)(void*,int,std::uint32_t,std::uint64_t)=nullptr;
    // Optional optimization: reuse immutable kernel schema already owned by the
    // capture session. Never cache identity/liveness results or skip checks.
    Metric<PrivatePidfdNetworkJoin> (*join_layout)(void*,int,std::uint32_t,std::uint64_t,const LinuxNetworkBtfLayout&)=nullptr;
    // Optional native optimization: per-sampler query resources. Session link
    // is gated off outside queries; both kernel queries of each join execute.
    Metric<PrivatePidfdNetworkJoin> (*join_query)(void*,int,std::uint32_t,std::uint64_t,const LinuxNetworkBtfLayout&,LinuxPidfdIdentityQuery&)=nullptr;
};
SelectedNetworkSourceApi native_selected_network_source_api();
struct PrivateSelectedNetworkWindow {PrivateNetworkWindow window;PrivateNetworkProcContext local;};
struct PrivateSelectedDiscoveredNetworkWindow {
    NetworkAccountingOutput accounting;
    std::vector<PrivateDiscoveredNetworkBucket> buckets;
    std::uint64_t completed_updates{},read_started_ns{},read_finished_ns{};
    PrivateNetworkProcContext local;
};
// Internal single selected actor, declared cookie or observed-cookie registry.
// Serialize calls, contexts must outlive sampler; no raw reads, public publication,
// automatic process selection or global coverage.
// Calls must use the same descriptor table (normal shared-thread tables work);
// do not unshare/replace its owned descriptors between calls or asynchronously.
class LinuxSelectedNetworkSampler {
public:
    explicit LinuxSelectedNetworkSampler(NetworkSessionApi capture=native_network_session_api(),
        NetworkSamplerApi clock=native_network_sampler_api(),SelectedNetworkSourceApi target=native_selected_network_source_api()):
        capture_api_(capture),capture_(capture),clock_(clock),target_(target),query_(capture,clock.context,clock.local_process){}
    ~LinuxSelectedNetworkSampler(){stop();}
    LinuxSelectedNetworkSampler(const LinuxSelectedNetworkSampler&)=delete;
    LinuxSelectedNetworkSampler& operator=(const LinuxSelectedNetworkSampler&)=delete;
    Metric<PrivatePidfdNetworkJoin> start(bool authorized,int pidfd,std::uint32_t pid,std::uint64_t start_ticks,std::uint64_t cookie,bool filter_other_namespaces=false);
    Metric<PrivateSelectedNetworkWindow> sample();
    // Internal observed-cookie mode: owns pidfd and brackets each complete read
    // with authenticated joins. No caller cookie, public publication or global scope.
    Metric<PrivatePidfdNetworkJoin> start_discovering(bool authorized,int pidfd,std::uint32_t pid,std::uint64_t start_ticks);
    Metric<PrivateSelectedDiscoveredNetworkWindow> sample_discovered();
    void discard_window() noexcept {reset_window();} // Group rollback; retains capture/pidfd.
    void stop() noexcept;
private:
    friend class LinuxMultiNetworkSampler;
    Metric<PrivatePidfdNetworkJoin> start_impl(bool,int,std::uint32_t,std::uint64_t,std::uint64_t,bool,bool);
    Metric<PrivatePidfdNetworkJoin> join_current();
    Metric<PrivateNetworkSessionRead> read_current();
    Metric<PrivateDiscoveredNetworkSessionRead> read_discovered_current(); // Validates, does not commit registry.
    void reset_window() noexcept;
    NetworkSessionApi capture_api_;
    LinuxPrivateNetworkSession capture_;
    NetworkSamplerApi clock_;
    SelectedNetworkSourceApi target_;
    LinuxPidfdIdentityQuery query_;
    LinuxNetworkAccounting reducer_;
    std::optional<NetworkAccountingScope> scope_;
    std::optional<LinuxNetworkBtfLayout> verified_layout_;
    std::optional<PrivateNetworkProcContext> last_context_;
    PrivateNetworkIdentity identity_{};
    int pidfd_=-1;
    std::uint32_t pid_{};
    std::uint64_t start_{},cookie_{},creator_{},last_end_{};
    bool filter_other_namespaces_=false;
    bool discover_=false;
    std::optional<PrivateDiscoveredNetworkSessionRead> last_discovered_;
};
}
