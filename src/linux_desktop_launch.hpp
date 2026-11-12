#pragma once
#include "linux_gated_launch.hpp"
#include "pcinsight/startup.hpp"
namespace pcinsight::detail {
// A resolved snapshot/request, not proof of current file/app/automatic cause.
struct LinuxDesktopRoute {std::string entry_id;LaunchRequest request;};
Metric<LinuxDesktopRoute> resolve_linux_desktop_route(const StartupProperties&);
using LinuxDesktopArm=std::function<ActionResult(const LinuxDesktopRoute&,const LinuxGatedLaunchTarget&)>;
// Explicit manual launch of caller's effective XDG entry, NOT login activation.
// Same child ownership/privileges/FD/serialization contract as launch_linux_gated.
ActionResult launch_linux_desktop_entry(const std::string& entry_id,bool authorized,const LinuxDesktopArm&);
}
