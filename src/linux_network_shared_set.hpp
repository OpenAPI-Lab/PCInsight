#pragma once
#include "linux_network_shared.hpp"
#include <memory>
namespace pcinsight::detail {
// INTERNAL disjoint admission batches. Never recycle retired slots: new actors
// get fresh producers in chunks of at most64; retained batches keep running.
// At most max_processes nonempty batches, including fragmentation after churn.
// Per-batch coherent reads, bounded common sweep, NOT an atomic cross-batch cut.
class LinuxSharedNetworkSet {
public:
    explicit LinuxSharedNetworkSet(NetworkSessionApi api=native_network_session_api(),NetworkSamplerApi clock=native_network_sampler_api()):api_(api),clock_(clock){}
    ~LinuxSharedNetworkSet(){stop();}
    LinuxSharedNetworkSet(const LinuxSharedNetworkSet&)=delete;
    LinuxSharedNetworkSet& operator=(const LinuxSharedNetworkSet&)=delete;
    Metric<bool> start(bool,std::span<const PrivateNetworkIdentity>);
    Metric<bool> update(bool,std::span<const PrivateNetworkIdentity>);
    Metric<PrivateSharedNetworkRead> read();
    void stop() noexcept {groups_.clear();actors_.clear();creator_=0;}
private:
    struct Group {std::unique_ptr<LinuxSharedNetworkSession> capture;std::vector<PrivateNetworkIdentity> actors;};
    NetworkSessionApi api_;NetworkSamplerApi clock_;
    std::vector<Group> groups_;std::vector<PrivateNetworkIdentity> actors_;
    std::uint64_t creator_{};
};
}
