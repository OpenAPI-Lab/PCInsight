#pragma once
#include "pcinsight/actions.hpp"
#include "pcinsight/types.hpp"
#include <optional>
#include <string_view>
namespace pcinsight::detail {
std::optional<std::string> rewrite_xdg_startup(std::string_view text, bool enabled);
std::optional<StartupEntry> read_xdg_startup(std::string_view text);
ActionResult set_linux_startup_enabled(const std::string& path, bool enabled);
}
