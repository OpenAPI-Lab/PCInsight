#pragma once
#include "linux_network_join.hpp"
#include "pcinsight/startup_birth_observer.hpp"
namespace pcinsight::detail {
// INTERNAL: new process groups CREATED BY selected TGID, not proof of real_parent,
// executable identity, successful exec or automatic startup. Optional local_pid
// is namespace-qualified at birth, NOT a current/live process selection.
struct StartupBirthLocalPid {
    std::uint64_t pid_namespace_inode{};
    std::uint32_t tgid{};
    auto operator<=>(const StartupBirthLocalPid&) const = default;
};
struct StartupBirthPidProfile {LinuxBirthPidBtfLayout layout;std::uint64_t namespace_inode{};};
struct StartupChildBirth {
    PrivateNetworkIdentity kernel;
    std::uint64_t monotonic_ns{},scheduler_runtime_at_birth_ns{},read_bytes_at_birth{},write_bytes_at_birth{};
    std::optional<std::uint64_t> final_monotonic_ns;
    Metric<std::chrono::nanoseconds> cpu_time;
    Metric<std::uint64_t> read_bytes,write_bytes;
    Metric<StartupBirthLocalPid> local_pid; // Birth identity, NOT authority to act on a now-reused PID.
    std::uint32_t exec_state{}; //0pending/disabled,1published,3failed first event (2in-flight is never returned).
    Metric<StartupBirthObservation::Executable> first_executable;
};
using StartupBirthRaw=std::array<std::uint64_t,20>; //15previous fields + first-exec state/time/inode/device/UID.
Metric<std::optional<StartupChildBirth>> decode_startup_birth(const StartupBirthRaw&,bool exec_enabled=false);
Metric<std::vector<NetworkProbeInstruction>> make_startup_birth_probe(int control_map,int records_map,const LinuxStartupResourceBtfLayout&,PrivateNetworkIdentity creator,std::uint32_t capacity,const StartupBirthPidProfile* =nullptr,int exec_allowlist=-1);
Metric<std::vector<NetworkProbeInstruction>> make_startup_birth_exec_probe(int control,int records,int allowlist,const LinuxStartupResourceBtfLayout&,const LinuxBirthExecBtfLayout&,std::uint32_t capacity);
Metric<std::vector<NetworkProbeInstruction>> make_startup_birth_final_probe(int control_map,int records_map,const LinuxStartupResourceBtfLayout&,PrivateNetworkIdentity creator,std::uint32_t capacity);
struct StartupBirthRead {
    PrivatePidfdNetworkJoin creator;
    std::vector<StartupChildBirth> births;
    std::uint64_t quality_flags{}; // Sticky:1 birth,2 capacity,4 duplicate birth,8 final,16 duplicate final,32 PID mapping,64 exec failure.
    std::uint32_t publishing{}; // Point read crossed publication; resample, not invented data.
    bool creator_exited{};
    bool creator_reaped_totals_enabled{};
    bool pid_mapping_enabled{};
    std::string pid_mapping_note;
    bool first_exec_enabled{};
};
class LinuxStartupBirthCapture {
    int pidfd_=-1,control_=-1,records_=-1,program_=-1,link_=-1,final_program_=-1,final_link_=-1,exec_program_=-1,exec_link_=-1,exec_allowlist_=-1;
    std::uint32_t owner_{},euid_{},capacity_{};
    std::optional<PrivatePidfdNetworkJoin> creator_;
    bool totals_{};
    bool pid_mapping_{};
    bool exec_{};
    std::string pid_mapping_note_;
public:
    LinuxStartupBirthCapture()=default;~LinuxStartupBirthCapture(){stop();}
    LinuxStartupBirthCapture(const LinuxStartupBirthCapture&)=delete;
    LinuxStartupBirthCapture& operator=(const LinuxStartupBirthCapture&)=delete;
    Metric<PrivatePidfdNetworkJoin> start(bool explicitly_authorized,int borrowed_pidfd,std::uint32_t pid,std::uint64_t start_ticks,std::uint32_t capacity=16,bool capture_creator_reaped_totals=false,bool observe_first_exec=false);
    Metric<StartupBirthRead> sample();
    void stop() noexcept;
};
}
