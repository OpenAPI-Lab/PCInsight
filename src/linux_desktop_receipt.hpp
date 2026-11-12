#pragma once
#include "linux_desktop_launch.hpp"
#include "linux_selected_exec.hpp"
namespace pcinsight::detail {
enum class LinuxDesktopLaunchOrigin { manual_request }; // No automatic origin accepted here.
struct LinuxDesktopLaunchReceipt {
    LinuxDesktopLaunchOrigin origin{LinuxDesktopLaunchOrigin::manual_request};
    LinuxDesktopRoute route;
    PrivatePidfdNetworkJoin selected;
    SelectedExecFile pinned_file;
    std::uint32_t caller_owned_pid{};
    ActionError launch_error{ActionError::none};
    SelectedExecEvidence evidence;
    Metric<bool> entry_first_exec_matches; // UNKNOWN until successful exec evidence.
    bool pidfd_exited{};
};
// Pure consistency projection; externally fabricated inputs are NOT authenticated.
Metric<LinuxDesktopLaunchReceipt> project_linux_manual_receipt(const LinuxDesktopRoute&,
    const PrivatePidfdNetworkJoin&,SelectedExecFile,std::uint32_t caller_owned_pid,
    ActionError,const Metric<SelectedExecRead>&);
// INTERNAL one-shot owner. Two independent consents, no fallback without capture.
// Does not start automatically, activate login, signal or reap released children.
class LinuxDesktopLaunchSession {
    LinuxSelectedExecCapture capture_;
    std::optional<LinuxDesktopRoute> route_;
    std::optional<PrivatePidfdNetworkJoin> selected_;
    SelectedExecFile pinned_;
    bool attempted_{};
    std::uint32_t caller_pid_{};
    ActionError error_{ActionError::none};
public:
    LinuxDesktopLaunchSession()=default;~LinuxDesktopLaunchSession(){stop();}
    LinuxDesktopLaunchSession(const LinuxDesktopLaunchSession&)=delete;
    LinuxDesktopLaunchSession& operator=(const LinuxDesktopLaunchSession&)=delete;
    ActionResult start(const std::string& entry_id,bool launch_consent,bool capture_consent);
    Metric<LinuxDesktopLaunchReceipt> sample(); // No wait loop; usable after caller reaps.
    void stop() noexcept; // No implicit sample/kill/wait; instance cannot be relaunched.
};
}
