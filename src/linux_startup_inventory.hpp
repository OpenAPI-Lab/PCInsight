#pragma once
#include "pcinsight/types.hpp"
#include "linux_startup_eligibility.hpp"
#include <filesystem>
#include <string_view>
namespace pcinsight::detail {
struct XdgAutostartDirectory { std::filesystem::path path; bool personal{}; };
struct XdgStartupInventory {
    std::vector<StartupEntry> entries;
    bool complete{true};
    std::vector<std::string> diagnostics;
};
std::vector<XdgAutostartDirectory> resolve_xdg_autostart(
    std::string_view home, std::string_view config_home, std::string_view config_dirs);
XdgStartupInventory read_xdg_startup_inventory(const std::vector<XdgAutostartDirectory>& directories,
    std::size_t payload_limit = 64 * 1024 * 1024, const XdgEligibilityContext* eligibility = nullptr);
XdgStartupInventory read_native_xdg_startup_inventory();
std::vector<XdgAutostartDirectory> native_xdg_autostart_directories();
}
