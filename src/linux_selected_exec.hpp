#pragma once
#include "linux_gated_launch.hpp"
#include "pcinsight/startup_birth_observer.hpp"
#include "linux_network_join.hpp"
#include <array>
#include <cstdint>
#include <optional>
namespace pcinsight::detail {
struct SelectedExecFile {
    std::uint64_t inode{};std::uint32_t device_major{},device_minor{};
    auto operator<=>(const SelectedExecFile&) const = default;
};
// Shared exec-probe ABI ONLY:0=armed,1..3=selected kernel key,4..14=UNUSED ZERO,
//15=event state,16..19=event time/inode/kernel-dev/UID. NOT a birth record.
using SelectedExecRaw=std::array<std::uint64_t,20>;
struct SelectedExecEvidence {
    Metric<StartupBirthObservation::Executable> first_exec;
    Metric<bool> pinned_file_matches; // Tuple/object comparison, never content/app/cause proof.
    std::uint64_t quality_flags{};bool publishing{};
};
Metric<SelectedExecEvidence> decode_selected_exec(const SelectedExecRaw&,PrivateNetworkIdentity,
    SelectedExecFile,std::uint64_t quality_flags);
struct SelectedExecRead {PrivatePidfdNetworkJoin selected;SelectedExecEvidence evidence;bool pidfd_exited{};};
// INTERNAL explicit privileged provider, call ONLY from a still-gated launch's
// arming callback. Owns duplicates; no launch/signal/reap or global census.
class LinuxSelectedExecCapture {
    int pidfd_=-1,image_=-1,control_=-1,records_=-1,allowlist_=-1,program_=-1,link_=-1;
    std::uint32_t owner_{},euid_{};
    std::optional<PrivatePidfdNetworkJoin> selected_;
    SelectedExecFile expected_;
    std::optional<StartupBirthObservation::Executable> retained_;
public:
    LinuxSelectedExecCapture()=default;~LinuxSelectedExecCapture(){stop();}
    LinuxSelectedExecCapture(const LinuxSelectedExecCapture&)=delete;
    LinuxSelectedExecCapture& operator=(const LinuxSelectedExecCapture&)=delete;
    Metric<PrivatePidfdNetworkJoin> start(bool explicitly_authorized,const LinuxGatedLaunchTarget&);
    Metric<SelectedExecRead> sample(); // Works after reap; no PID-only or /proc/exe inference.
    void stop() noexcept; // No hidden final sample; copies already returned remain historical.
};
}
