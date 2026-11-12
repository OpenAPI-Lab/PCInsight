#pragma once
#include "linux_desktop_resources.hpp"
#include "pcinsight/startup_store.hpp"
#include <string_view>
namespace pcinsight::detail {
// Historical ONLY: numeric caller_owned_pid is NOT live child ownership on load.
// No handles/session/authentication restored. Derived links/resources are recomputed.
struct LinuxManualLaunchArchive {
    StartupBirthArchive births;
    Metric<LinuxDesktopLaunchReceipt> receipt;
};
struct LinuxManualLaunchLoad {
    std::optional<LinuxManualLaunchArchive> archive;
    ActionError error{ActionError::none};std::string message;
    explicit operator bool()const{return archive.has_value()&&error==ActionError::none;}
};
Metric<std::string> encode_linux_manual_launch(const LinuxManualLaunchArchive&);
LinuxManualLaunchLoad decode_linux_manual_launch(std::string_view);
StartupArchiveIoResult save_linux_manual_launch(const std::filesystem::path&,const LinuxManualLaunchArchive&);
LinuxManualLaunchLoad load_linux_manual_launch(const std::filesystem::path&);
}
