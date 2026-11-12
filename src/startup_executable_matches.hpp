#pragma once
#include "pcinsight/startup_executable_matches.hpp"
#include "pcinsight/startup.hpp"
#include "linux_startup_inventory.hpp"
#include "linux_network_selection.hpp"
#include <functional>
namespace pcinsight::detail {
Metric<std::string> direct_xdg_exec_path(const StartupProperties&);
struct StartupExecutableFileId {std::uint64_t inode{};std::uint32_t major{},minor{};};
struct StartupExecutableMatchApi {
    std::function<Metric<NetworkSelectionContext>()> context;
    std::function<XdgStartupInventory()> inventory;
    std::function<StartupPropertiesResult(const std::string&)> properties;
    std::function<Metric<StartupExecutableFileId>(const std::string&)> file;
};
StartupExecutableMatchReport discover_startup_executable_matches(const StartupBirthObserverReport&,const StartupBirthProcessKey&,const StartupExecutableMatchApi&);
}
