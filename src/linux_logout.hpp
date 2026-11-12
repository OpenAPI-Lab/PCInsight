#pragma once
#include "linux_users.hpp"
#include "pcinsight/actions.hpp"
#include <functional>
#include <optional>
namespace pcinsight::detail {
struct LogoutToken { std::string owner, path; std::uint64_t timestamp{}; };
std::string logout_token(const LogoutToken&);
std::optional<LogoutToken> parse_logout_token(const std::string&);
struct LogoutQuery { std::uint64_t timestamp{}; ActionError error{ActionError::none}; };
struct LogindSessionObservation {
    std::string id, username;
    std::uint32_t uid{};
    LogoutToken identity;
    LogoutQuery before, after;
    Metric<std::string> state;
    Metric<bool> active, remote;
    Metric<std::string> object_id, object_username;
    Metric<std::uint32_t> object_uid;
};
LinuxLoginRecord make_logind_login(const LogindSessionObservation&);
struct LogoutApi {
    std::function<std::pair<std::string, ActionError>()> owner;
    std::function<LogoutQuery(const LogoutToken&)> query;
    std::function<ActionResult(const LogoutToken&)> terminate;
    // false means deadline/budget exhausted; no cancellation or repeated Terminate.
    std::function<bool()> next;
};
ActionResult logout_linux_session(const std::string&, const LogoutApi&);
ActionResult logout_linux_session(const std::string&);
std::optional<std::vector<LinuxLoginRecord>> read_logind_logins();
}
