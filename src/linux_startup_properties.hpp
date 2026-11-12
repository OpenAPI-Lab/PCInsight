#pragma once
#include "pcinsight/startup.hpp"
#include <string_view>
namespace pcinsight::detail {
std::optional<StartupProperties> parse_startup_properties(std::string_view);
}
