#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
namespace pcinsight::detail {
struct GioLaunchEvidence {bool well_formed{true};std::optional<std::string> desktop_file;};
GioLaunchEvidence parse_gio_launch_evidence(std::string_view environment,std::uint32_t pid);
}
