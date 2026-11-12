#pragma once
#include "pcinsight/windows_startup_file_control.hpp"
#include <functional>
namespace pcinsight::detail {
using WindowsStartupFileBackupProvider=std::function<Metric<std::vector<WindowsStartupFileBackup>>() >;
// Internal test seam: disabled options must never invoke a provider.
void fill_windows_startup_file_backups(SystemSnapshot&,const SampleOptions&,
    const WindowsStartupFileBackupProvider& provider=pcinsight::list_windows_startup_file_backups);
}
