#include "linux_notification_ledger.hpp"
#include <algorithm>
#include <limits>
namespace pcinsight::detail {
bool valid_notification_socket(const std::string& path) {
    return !path.empty() && path.front() == '/' && path.size() <= 100 &&
        std::all_of(path.begin(), path.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '/' || c == '-' || c == '_' || c == '.';
        });
}
bool valid_notification_unique_name(const std::string& name) {
    if (name.size() < 4 || name.size() > 255 || name.front() != ':') return false;
    const auto dot = name.find('.', 1);
    return dot > 1 && dot < name.size() - 1 && name.find_first_not_of("0123456789", 1) == dot &&
        name.find_first_not_of("0123456789", dot + 1) == name.npos;
}
bool valid_notification_desktop_hint(const std::string& name) {
    return !name.empty() && name.size() <= 4096 && name != "." && name != ".." &&
        std::all_of(name.begin(), name.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
        });
}
void LinuxNotificationLedger::begin(std::string server, Clock::time_point now, std::chrono::system_clock::time_point wall) {
    *this = {}; owner = std::move(server); began = ended = now; wall_start = wall_end = wall;
    if (!valid_notification_unique_name(owner)) { fail("invalid notifications server identity"); return; }
    healthy = running = true; failure.clear();
}
void LinuxNotificationLedger::fail(std::string reason, Support support) {
    healthy = running = false; failure = std::move(reason); failure_support = support;
    pending.clear(); apps.clear(); // Never publish a truncated period as complete or stale zeroes.
}
void LinuxNotificationLedger::call(std::string sender, std::uint64_t serial,
    std::optional<std::string> hint, std::uint32_t replaces, Clock::time_point now) {
    if (!healthy || !running) return;
    if (!valid_notification_unique_name(sender) || !serial || serial > UINT32_MAX || now < began ||
        pending.size() >= 1024 || (hint && !valid_notification_desktop_hint(*hint))) {
        fail("invalid/excessive notification call identity or pending budget"); return;
    }
    if (!pending.emplace(std::make_pair(std::move(sender), serial), Pending{std::move(hint), replaces, now}).second)
        fail("duplicate pending notification call identity");
}
void LinuxNotificationLedger::reply(const std::string& source, const std::string& recipient,
    std::uint64_t serial, bool success, std::uint32_t id) {
    if (!healthy || !running || source != owner) return;
    const auto found = pending.find({recipient, serial});
    if (found == pending.end()) return; // Includes duplicates, replies to pre-capture calls and unrelated replies.
    const auto request = found->second; pending.erase(found);
    auto increment = [&](std::uint64_t& value) {
        if (value == std::numeric_limits<std::uint64_t>::max()) { fail("notification counter overflow"); return false; }
        ++value; return true;
    };
    if (!success) { increment(rejected); return; }
    if (!id || (request.replaces && request.replaces != id)) { fail("invalid Notify response identity"); return; }
    const auto key = std::make_pair(recipient, request.hint.value_or(""));
    if (!apps.contains(key) && apps.size() >= 2048) { fail("notification app row budget exceeded"); return; }
    auto& app = apps[key]; app.sender_unique_name = recipient;
    app.declared_desktop_entry = {request.hint, request.hint ? Support::estimated : Support::unsupported,
        request.hint ? "Sender-declared desktop-entry hint, NOT authenticated app/PID/executable attribution" :
                       "No usable desktop-entry hint; connection identity only, app attribution unknown"};
    if (!increment(app.accepted_notify_calls)) return;
    if (request.replaces) increment(app.replacement_calls);
}
void LinuxNotificationLedger::tick(Clock::time_point now, std::chrono::system_clock::time_point wall) {
    if (!healthy || !running) return;
    if (now < ended) { fail("notification monotonic clock regressed"); return; }
    ended = now; wall_end = std::max({wall, wall_start, wall_end});
    for (auto it = pending.begin(); it != pending.end();) {
        if (now - it->second.created >= std::chrono::seconds(30)) {
            if (expired == UINT64_MAX) { fail("notification expiry counter overflow"); return; }
            ++expired; it = pending.erase(it);
        } else ++it;
    }
}
void LinuxNotificationLedger::unconfirmed() {
    if (!healthy || !running) return;
    if (no_reply == UINT64_MAX) { fail("notification unconfirmed counter overflow"); return; }
    ++no_reply;
}
Metric<LinuxNotificationReport> LinuxNotificationLedger::report() const {
    if (!healthy) return {std::nullopt, failure_support, failure};
    LinuxNotificationReport out;
    for (const auto& [key, app] : apps) out.apps.push_back(app);
    out.period_start = wall_start; out.period_end = wall_end; out.observed_elapsed = ended - began;
    out.rejected_calls = rejected; out.expired_unresolved_calls = expired; out.pending_calls = pending.size();
    out.unconfirmed_no_reply_calls = no_reply;
    out.running = running; out.poll_budget_reached = budget; out.server_unique_name = owner;
    return {std::move(out), Support::estimated,
        "Observed successful Notify CALLS only (replacements included), not displayed/read notifications; lower-bound capture, caller-polled, no retroactive history; desktop hints untrusted, no PID/executable or CPU-history join"};
}
}
