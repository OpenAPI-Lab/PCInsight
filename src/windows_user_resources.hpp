#pragma once
#include "pcinsight/types.hpp"
#include <span>
namespace pcinsight::detail {
// Pure bounded aggregation; transactional output on malformed input.
void fill_windows_user_resources(std::vector<UserSession>&,std::span<const ProcessInfo>);
}
