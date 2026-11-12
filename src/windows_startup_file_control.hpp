#pragma once
#include "pcinsight/windows_startup_file_control.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <functional>
#include <string>
namespace pcinsight::detail {
// Test seam redirects only the known-folder root; native file operations remain real.
using WindowsStartupRoot = std::function<std::wstring()>;
enum class WindowsStartupFileScope { current_user, common };
Metric<WindowsStartupFileRegistration> read_windows_startup_file_registration(const std::string&, const WindowsStartupRoot&,
    WindowsStartupFileScope scope=WindowsStartupFileScope::current_user);
ActionResult set_windows_startup_file_registered(const WindowsStartupFileRegistration&, bool, const WindowsStartupRoot&,
    WindowsStartupFileScope scope=WindowsStartupFileScope::current_user);
Metric<std::vector<WindowsStartupFileBackup>> list_windows_startup_file_backups(const WindowsStartupRoot&,
    decltype(&GetFileInformationByHandleEx) query=&GetFileInformationByHandleEx,
    WindowsStartupFileScope scope=WindowsStartupFileScope::current_user);
}
#endif
