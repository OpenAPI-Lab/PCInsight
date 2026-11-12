#pragma once
#include "pcinsight/types.hpp"
#include <span>
namespace pcinsight::detail {
struct LinuxLoginRecord { std::string id, username, status; std::string logout_token{}; Metric<LinuxSessionDetails> details{}; };
std::vector<UserSession> aggregate_linux_users(std::span<const LinuxLoginRecord>,
    std::span<const ProcessInfo>);
std::vector<LinuxLoginRecord> read_linux_logins();
Metric<std::vector<LinuxLoginRecord>> read_linux_login_records();
}
