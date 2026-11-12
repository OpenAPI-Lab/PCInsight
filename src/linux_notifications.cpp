#include "pcinsight/linux_notifications.hpp"
#include "linux_notification_ledger.hpp"
#include "linux_service_actions.hpp"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <system_error>
#ifdef __linux__
#include <dlfcn.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
namespace pcinsight {
using namespace detail;
namespace {
#ifdef __linux__
constexpr const char* service = "org.freedesktop.Notifications";
constexpr const char* object = "/org/freedesktop/Notifications";
constexpr const char* driver = "org.freedesktop.DBus";
constexpr const char* driver_path = "/org/freedesktop/DBus";
#endif
}
struct LinuxNotificationObserver::Impl {
    LinuxNotificationLedger ledger;
#ifdef __linux__
    SystemdActionApi api;
    ActionBus* bus{};
    ActionBus* (*close_unref)(ActionBus*){};
    int (*create)(ActionBus**){};
    int (*address)(ActionBus*, const char*){};
    int (*client)(ActionBus*, int){};
    int (*monitor)(ActionBus*, int){};
    int (*start_bus)(ActionBus*){};
    int (*interactive)(ActionBus*, int){};
    int (*socket_fd)(ActionBus*){};
    const char* (*sender)(ActionMessage*){};
    const char* (*destination)(ActionMessage*){};
    const char* (*member)(ActionMessage*){};
    const char* (*interface)(ActionMessage*){};
    const char* (*path)(ActionMessage*){};
    const char* (*signature)(ActionMessage*, int){};
    int (*type)(ActionMessage*, std::uint8_t*){};
    int (*cookie)(ActionMessage*, std::uint64_t*){};
    int (*reply_cookie)(ActionMessage*, std::uint64_t*){};
    int (*expect_reply)(ActionMessage*){};
    int (*enter)(ActionMessage*, char, const char*){};
    int (*exit)(ActionMessage*){};
    int (*skip)(ActionMessage*, const char*){};
    bool load() {
        api = load_systemd_action_api(); if (!api) return false;
#define LOAD(field, name) field = reinterpret_cast<decltype(field)>(dlsym(api.library.get(), name)); if (!field) return false
        LOAD(close_unref, "sd_bus_close_unref"); LOAD(create, "sd_bus_new"); LOAD(address, "sd_bus_set_address");
        LOAD(client, "sd_bus_set_bus_client"); LOAD(monitor, "sd_bus_set_monitor"); LOAD(start_bus, "sd_bus_start");
        LOAD(interactive, "sd_bus_set_allow_interactive_authorization"); LOAD(socket_fd, "sd_bus_get_fd");
        LOAD(sender, "sd_bus_message_get_sender"); LOAD(destination, "sd_bus_message_get_destination");
        LOAD(member, "sd_bus_message_get_member"); LOAD(interface, "sd_bus_message_get_interface");
        LOAD(path, "sd_bus_message_get_path"); LOAD(signature, "sd_bus_message_get_signature");
        LOAD(type, "sd_bus_message_get_type"); LOAD(cookie, "sd_bus_message_get_cookie");
        LOAD(reply_cookie, "sd_bus_message_get_reply_cookie");
        LOAD(expect_reply, "sd_bus_message_get_expect_reply");
        LOAD(enter, "sd_bus_message_enter_container"); LOAD(exit, "sd_bus_message_exit_container");
        LOAD(skip, "sd_bus_message_skip");
#undef LOAD
        return true;
    }
    int connect(ActionBus** out, const std::string& socket, bool is_monitor) {
        auto rc = create(out);
        if (rc >= 0) rc = address(*out, ("unix:path=" + socket).c_str());
        if (rc >= 0) rc = client(*out, 1);
        if (rc >= 0) rc = monitor(*out, is_monitor ? 1 : 0);
        if (rc >= 0) rc = api.timeout(*out, 2000000);
        if (rc >= 0) rc = interactive(*out, 0);
        if (rc >= 0) rc = start_bus(*out);
        if (rc >= 0) {
            const auto fd = socket_fd(*out); struct ucred peer{}; socklen_t size = sizeof(peer);
            if (fd < 0) rc = fd;
            else if (::getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peer, &size) < 0) rc = -errno;
            else if (size != sizeof(peer) || peer.uid != ::geteuid()) rc = -EACCES;
        }
        return rc;
    }
    struct Reply {
        Impl& parent; ActionMessage* message{}; ActionBusError error{};
        ~Reply() { if (message) parent.api.message_unref(message); parent.api.error_free(&error); }
    };
    static std::string text(const char* value, std::size_t limit = 4096) {
        if (!value) return {};
        const auto length = ::strnlen(value, limit + 1);
        if (length > limit) throw std::runtime_error("notification metadata string budget exceeded");
        return {value, length};
    }
    std::string owner(ActionBus* control) {
        Reply result{*this};
        const auto rc = api.call(control, driver, driver_path, driver, "GetNameOwner", &result.error, &result.message, "s", service);
        if (rc < 0) throw std::system_error(-rc, std::generic_category(), "GetNameOwner");
        const char* name{};
        if (!result.message || api.read(result.message, "s", &name) <= 0 || !valid_notification_unique_name(text(name)))
            throw std::runtime_error("notifications service has no verifiable unique owner");
        return text(name);
    }
    void handle(ActionMessage* message) {
        const auto from = text(sender(message), 255), to = text(destination(message), 255);
        std::uint8_t kind{}; if (type(message, &kind) < 0) throw std::runtime_error("notification message type unavailable");
        if (kind == 4 && text(interface(message)) == "org.freedesktop.DBus.Local" && text(member(message)) == "Disconnected") {
            ledger.fail("notification monitor disconnected; explicit restart/new period required"); return;
        }
        if (kind == 4 && from == driver && text(path(message)) == driver_path && text(interface(message)) == driver &&
            text(member(message)) == "NameOwnerChanged") {
            const char *name{}, *old{}, *current{};
            if (api.read(message, "sss", &name, &old, &current) <= 0) throw std::runtime_error("invalid owner change signal");
            if (text(name) == service && text(old) != text(current)) ledger.fail("notification server owner changed; explicit restart/new period required");
            return;
        }
        if (kind == 1 && (to == service || to == ledger.owner) && text(path(message)) == object &&
            text(interface(message)) == service && text(member(message)) == "Notify") {
            if (text(signature(message, 1), 255) != "susssasa{sv}i") throw std::runtime_error("invalid Notify signature");
            const auto expected = expect_reply(message);
            if (expected < 0) throw std::runtime_error("Notify reply expectation unavailable");
            if (!expected) { ledger.unconfirmed(); return; }
            std::uint64_t serial{}; std::uint32_t replaces{};
            if (cookie(message, &serial) < 0 || skip(message, "s") <= 0 || api.read(message, "u", &replaces) <= 0 ||
                skip(message, "sssas") <= 0 || enter(message, 'a', "{sv}") <= 0)
                throw std::runtime_error("invalid Notify arguments");
            std::optional<std::string> hint; bool hint_seen = false; std::size_t count = 0;
            for (;;) {
                const auto rc = enter(message, 'e', "sv");
                if (rc < 0) throw std::runtime_error("invalid Notify hints");
                if (!rc) break;
                if (++count > 256) throw std::runtime_error("Notify hint budget exceeded");
                const char* key{};
                if (api.read(message, "s", &key) <= 0) throw std::runtime_error("invalid Notify hint key");
                if (text(key) == "desktop-entry") {
                    if (hint_seen) throw std::runtime_error("duplicate desktop-entry notification hint");
                    hint_seen = true;
                    if (enter(message, 'v', "s") <= 0) {
                        // A malformed attribution hint must not be mistaken for an app name.
                        throw std::runtime_error("desktop-entry notification hint is not a string");
                    }
                    const char* value{};
                    if (api.read(message, "s", &value) <= 0 || exit(message) < 0) throw std::runtime_error("invalid desktop-entry hint");
                    auto candidate = text(value);
                    if (valid_notification_desktop_hint(candidate)) hint = std::move(candidate);
                } else if (skip(message, "v") <= 0) throw std::runtime_error("invalid Notify hint variant");
                if (exit(message) < 0) throw std::runtime_error("invalid Notify dictionary entry");
            }
            std::int32_t timeout{};
            if (exit(message) < 0 || api.read(message, "i", &timeout) <= 0) throw std::runtime_error("invalid Notify timeout");
            ledger.call(from, serial, std::move(hint), replaces, LinuxNotificationLedger::Clock::now());
        } else if ((kind == 2 || kind == 3) && from == ledger.owner) {
            std::uint64_t serial{};
            if (reply_cookie(message, &serial) < 0) throw std::runtime_error("reply identity unavailable");
            // Ignore replies that cannot correspond to a call observed in this period.
            if (!ledger.pending.contains({to, serial})) return;
            std::uint32_t id{};
            if (kind == 2 && (text(signature(message, 1), 255) != "u" || api.read(message, "u", &id) <= 0))
                throw std::runtime_error("invalid Notify return value");
            ledger.reply(from, to, serial, kind == 2, id);
        }
    }
#endif
    void close() noexcept {
#ifdef __linux__
        if (bus && close_unref) bus = close_unref(bus);
#endif
        ledger.running = false;
    }
    ~Impl() { close(); }
};
LinuxNotificationObserver::LinuxNotificationObserver() : impl_(std::make_unique<Impl>()) {}
LinuxNotificationObserver::~LinuxNotificationObserver() = default;
LinuxNotificationObserver::LinuxNotificationObserver(LinuxNotificationObserver&&) noexcept = default;
LinuxNotificationObserver& LinuxNotificationObserver::operator=(LinuxNotificationObserver&&) noexcept = default;
ActionResult LinuxNotificationObserver::start(const LinuxNotificationOptions& options) {
    if (!impl_) return {ActionError::invalid_argument, "moved-from notification observer"};
    if (impl_->ledger.running) return {ActionError::invalid_argument, "notification observer already running"};
    if (!options.consent_to_monitor_notification_traffic) return {ActionError::invalid_argument, "explicit notification traffic monitoring consent required"};
    auto socket = options.bus_socket_path;
#ifdef __linux__
    if (socket.empty()) socket = "/run/user/" + std::to_string(::geteuid()) + "/bus";
#else
    if (socket.empty()) {
        impl_->ledger.fail("native notification observer requires Linux");
        return {ActionError::not_supported, impl_->ledger.failure};
    }
#endif
    if (!valid_notification_socket(socket)) return {ActionError::invalid_argument, "requires bounded absolute local Unix socket path, no address syntax/transports"};
    impl_->ledger = {};
#ifdef __linux__
    auto& p = *impl_;
    try {
        if (!p.load()) { p.ledger.fail("optional system libsystemd notification APIs unavailable"); return {ActionError::not_supported, p.ledger.failure}; }
        struct Control { Impl& p; ActionBus* bus{}; ~Control() { if (bus) p.close_unref(bus); } } control{p};
        auto rc = p.connect(&control.bus, socket, false);
        if (rc < 0) throw std::system_error(-rc, std::generic_category(), "notification control connection");
        const auto owner = p.owner(control.bus);
        rc = p.connect(&p.bus, socket, true);
        if (rc < 0) throw std::system_error(-rc, std::generic_category(), "notification monitor connection");
        const std::string reply_rule = "type='method_return',sender='" + owner + "'";
        const std::string error_rule = "type='error',sender='" + owner + "'";
        const auto now = LinuxNotificationLedger::Clock::now();
        const auto wall = std::chrono::system_clock::now();
        Impl::Reply reply{p};
        rc = p.api.call(p.bus, driver, driver_path, "org.freedesktop.DBus.Monitoring", "BecomeMonitor", &reply.error, &reply.message,
            "asu", 4,
            "type='method_call',interface='org.freedesktop.Notifications',member='Notify',path='/org/freedesktop/Notifications'",
            reply_rule.c_str(), error_rule.c_str(),
            "type='signal',sender='org.freedesktop.DBus',interface='org.freedesktop.DBus',member='NameOwnerChanged',path='/org/freedesktop/DBus',arg0='org.freedesktop.Notifications'", 0U);
        if (rc < 0) {
            const bool denied = rc == -EACCES || rc == -EPERM || (reply.error.name && std::strcmp(reply.error.name, "org.freedesktop.DBus.Error.AccessDenied") == 0);
            p.close(); p.ledger.fail(denied ? "BecomeMonitor permission denied; no elevation or eavesdrop fallback" : "BecomeMonitor unsupported or failed", denied ? Support::permission_required : Support::unsupported);
            return {denied ? ActionError::permission_denied : ActionError::not_supported, p.ledger.failure};
        }
        if (p.owner(control.bus) != owner) throw std::runtime_error("notification server owner changed during setup");
        p.ledger.begin(owner, now, wall);
        const auto first = poll();
        if (!first.value) return {ActionError::system_error, first.note};
        return {};
    } catch (const std::system_error& e) {
        const bool denied = e.code().value() == EACCES || e.code().value() == EPERM;
        p.close(); p.ledger.fail("notification bus/service unavailable or setup failed", denied ? Support::permission_required : Support::unsupported);
        return {denied ? ActionError::permission_denied : ActionError::not_supported, p.ledger.failure};
    } catch (const std::exception&) {
        p.close(); p.ledger.fail("notification protocol/identity setup failed"); return {ActionError::not_supported, p.ledger.failure};
    }
#else
    impl_->ledger.fail("native notification observer requires Linux");
    return {ActionError::not_supported, impl_->ledger.failure};
#endif
}
Metric<LinuxNotificationReport> LinuxNotificationObserver::poll(std::size_t max_messages) {
    if (!impl_) return {std::nullopt, Support::unsupported, "moved-from notification observer"};
    auto& p = *impl_;
    if (max_messages == 0 || max_messages > 4096) return {std::nullopt, Support::unsupported, "poll message budget must be1..4096"};
#ifdef __linux__
    if (p.ledger.running && p.bus) {
        try {
            p.ledger.tick(LinuxNotificationLedger::Clock::now(), std::chrono::system_clock::now());
            std::size_t processed = 0;
            for (; processed < max_messages && p.ledger.healthy; ++processed) {
                ActionMessage* raw{}; const auto rc = p.api.process(p.bus, &raw);
                struct Message { Impl& p; ActionMessage* value; ~Message() { if (value) p.api.message_unref(value); } } message{p, raw};
                if (rc < 0) throw std::runtime_error("notification monitor disconnected or processing failed");
                if (!rc) break;
                if (raw) p.handle(raw);
            }
            p.ledger.budget = processed == max_messages;
            p.ledger.tick(LinuxNotificationLedger::Clock::now(), std::chrono::system_clock::now());
            if (!p.ledger.healthy) p.close();
        } catch (const std::exception&) { p.close(); p.ledger.fail("notification monitor protocol/budget/connection failure; period invalidated"); }
    }
#else
    (void)max_messages;
#endif
    return p.ledger.report();
}
Metric<LinuxNotificationReport> LinuxNotificationObserver::report() const {
    return impl_ ? impl_->ledger.report() : Metric<LinuxNotificationReport>{std::nullopt, Support::unsupported, "moved-from notification observer"};
}
void LinuxNotificationObserver::stop() noexcept { if (impl_) impl_->close(); }
}
