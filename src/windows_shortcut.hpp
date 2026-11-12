#pragma once
#include "pcinsight/types.hpp"
#include <functional>
#include <span>
namespace pcinsight::detail {
Metric<WindowsShortcutInfo> parse_windows_shortcut(std::span<const std::uint8_t>);
Metric<WindowsShortcutInfo> parse_windows_internet_shortcut(std::span<const std::uint8_t>);
using ShortcutRead=std::function<Metric<std::vector<std::uint8_t>>(const std::string&)>;
void fill_windows_startup_shortcuts(std::vector<StartupEntry>&,const ShortcutRead&);
void fill_windows_startup_shortcuts(std::vector<StartupEntry>&);
Metric<std::vector<std::uint8_t>> read_windows_shortcut_file(const std::string&);
}
