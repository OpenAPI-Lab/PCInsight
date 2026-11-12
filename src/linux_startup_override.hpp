#pragma once
#include "linux_startup_inventory.hpp"
#include "pcinsight/actions.hpp"
namespace pcinsight::detail {
struct StartupOverridePlan { std::filesystem::path source, destination; };
std::optional<StartupOverridePlan> plan_startup_override(const std::string&,
    const std::vector<XdgAutostartDirectory>&);
ActionResult set_linux_startup_for_current_user(const std::string&, bool);
ActionResult reset_linux_startup_for_current_user(const std::string&);
ActionResult apply_startup_override(const StartupOverridePlan&, bool);
}
