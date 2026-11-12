#pragma once
#include "pcinsight/actions.hpp"
#include "linux_network_selection.hpp"
#include "linux_process_details.hpp"
#include <functional>
namespace pcinsight::detail {
struct LinuxGatedLaunchTarget {
    int borrowed_pidfd{-1},borrowed_executable_fd{-1};
    NetworkSelectionContext owner;
    LinuxNetworkSelfIdentity child;
    LinuxExecutableStamp executable;
};
using LinuxLaunchArm=std::function<ActionResult(const LinuxGatedLaunchTarget&)>;
// Internal, explicit controlled-launch primitive; NOT an app/autostart receipt.
// Parent callback runs before exec, with borrowed stable descriptors. Must not
// close/reuse them or reap the child. Duplicate descriptors if retention is needed;
// originals expire on return. Executable fd is O_PATH. Callback error/exception cancels and reaps
// this still-gated child by pidfd. Successful/unknown released child is caller's
// waitpid responsibility (nonzero process_id). No hidden post-exec kill/reaper.
// Absolute regular ELF path only, no PATH/shell/scripts/elevation; child NNP.
// Inherits environment/non-CLOEXEC fds/cwd. Serialize environment/cwd/credentials,
// external child reapers, fd close/reuse and forks. Uses clone3, not libc atfork.
ActionResult launch_linux_gated(const LaunchRequest&,bool explicitly_authorized,const LinuxLaunchArm&);
}
