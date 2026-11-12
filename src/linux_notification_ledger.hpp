#pragma once
#include "pcinsight/linux_notifications.hpp"
#include <map>
namespace pcinsight::detail {
bool valid_notification_socket(const std::string&);
bool valid_notification_unique_name(const std::string&);
bool valid_notification_desktop_hint(const std::string&);
struct LinuxNotificationLedger {
    using Clock = std::chrono::steady_clock;
    struct Pending { std::optional<std::string> hint; std::uint32_t replaces{}; Clock::time_point created; };
    std::map<std::pair<std::string, std::uint64_t>, Pending> pending;
    std::map<std::pair<std::string, std::string>, LinuxNotificationApp> apps;
    std::string owner, failure{"notification observation not started"};
    Support failure_support{Support::unsupported};
    std::uint64_t rejected{}, expired{}, no_reply{};
    bool healthy{}, running{}, budget{};
    std::chrono::system_clock::time_point wall_start, wall_end;
    Clock::time_point began, ended;
    void begin(std::string, Clock::time_point, std::chrono::system_clock::time_point);
    void fail(std::string, Support = Support::unsupported);
    void call(std::string, std::uint64_t, std::optional<std::string>, std::uint32_t, Clock::time_point);
    void reply(const std::string&, const std::string&, std::uint64_t, bool, std::uint32_t);
    void tick(Clock::time_point, std::chrono::system_clock::time_point);
    void unconfirmed();
    Metric<LinuxNotificationReport> report() const;
};
}
