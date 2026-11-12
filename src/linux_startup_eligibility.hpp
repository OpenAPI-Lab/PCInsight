#pragma once
#include "pcinsight/types.hpp"
#include <functional>
#include <string_view>
namespace pcinsight::detail {
struct XdgEligibilityContext {
    std::optional<std::string> desktop;
    std::function<Metric<bool>(const std::string&)> executable;
    std::optional<std::string> config_home;
    std::function<Metric<bool>(const std::string&)> file_exists;
    std::function<Metric<bool>(const std::string&,const std::string&)> setting_boolean;
    std::function<Metric<std::string>()> session_name;
};
Metric<bool> xdg_standard_eligibility(std::string_view text, const XdgEligibilityContext& context);
Metric<bool> autostart_condition_satisfied(std::string_view text, const XdgEligibilityContext& context);
XdgEligibilityContext native_xdg_eligibility_context();
}
