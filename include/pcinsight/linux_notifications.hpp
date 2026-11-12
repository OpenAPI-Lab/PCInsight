#pragma once
#include "actions.hpp"
#include "types.hpp"
#include <memory>
namespace pcinsight {
struct LinuxNotificationOptions {
    bool consent_to_monitor_notification_traffic{false};
    // Empty -> /run/user/<effective uid>/bus. Local filesystem Unix socket only.
    std::string bus_socket_path;
};
struct LinuxNotificationApp {
    std::string sender_unique_name;
    Metric<std::string> declared_desktop_entry; // Untrusted sender hint, not PID/executable identity.
    std::uint64_t accepted_notify_calls{}, replacement_calls{};
};
struct LinuxNotificationReport {
    std::vector<LinuxNotificationApp> apps;
    std::chrono::system_clock::time_point period_start, period_end;
    std::chrono::steady_clock::duration observed_elapsed{};
    std::uint64_t rejected_calls{}, expired_unresolved_calls{}, pending_calls{};
    std::uint64_t unconfirmed_no_reply_calls{}; // Observed requests explicitly not asking for acknowledgement.
    bool running{}, poll_budget_reached{};
    std::string server_unique_name;
};
// Explicit, single-thread-confined observer; construction never connects.
// No titles, body, actions, icons or notification content are retained/logged.
// Successful Notify replies are counted, NOT notifications displayed or read.
class LinuxNotificationObserver {
public:
    LinuxNotificationObserver();
    ~LinuxNotificationObserver();
    LinuxNotificationObserver(LinuxNotificationObserver&&) noexcept;
    LinuxNotificationObserver& operator=(LinuxNotificationObserver&&) noexcept;
    LinuxNotificationObserver(const LinuxNotificationObserver&) = delete;
    LinuxNotificationObserver& operator=(const LinuxNotificationObserver&) = delete;
    ActionResult start(const LinuxNotificationOptions& = {});
    // Non-waiting, at most max_messages (1..4096) processed per call.
    Metric<LinuxNotificationReport> poll(std::size_t max_messages = 256);
    Metric<LinuxNotificationReport> report() const;
    // Closes only this observer's connection. Keeps last finite observed report.
    void stop() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
