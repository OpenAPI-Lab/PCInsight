#include "../src/linux_service_actions.hpp"
#include "pcinsight/linux_notifications.hpp"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <csignal>
#include <dlfcn.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs = std::filesystem;
namespace {
void require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
struct PrivateBus {
    fs::path directory; pid_t child{-1};
    ~PrivateBus() {
        if (child > 0) {
            ::kill(child, SIGTERM);
            for (unsigned i = 0; i < 100; ++i) {
                if (::waitpid(child, nullptr, WNOHANG) == child) { child = -1; break; }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            if (child > 0) { ::kill(child, SIGKILL); ::waitpid(child, nullptr, 0); }
        }
        if (!directory.empty()) { std::error_code error; fs::remove_all(directory, error); }
    }
    std::string launch(bool deny_monitor = false) {
        char pattern[] = "/tmp/pcinsight-notifications-XXXXXX";
        const auto* made = ::mkdtemp(pattern); require(made, "private bus directory unavailable"); directory = made;
        const auto socket = (directory / "bus").string(), config = (directory / "bus.conf").string();
        std::ofstream file(config);
        file << "<busconfig><type>session</type><auth>EXTERNAL</auth><listen>unix:path=" << socket
             << "</listen><policy context=\"default\"><allow send_destination=\"*\"/><allow receive_sender=\"*\"/><allow own=\"*\"/>";
        if (deny_monitor) file << "<deny send_destination=\"org.freedesktop.DBus\" send_interface=\"org.freedesktop.DBus.Monitoring\" send_member=\"BecomeMonitor\"/>";
        file << "</policy></busconfig>\n";
        file.close(); require(bool(file), "private bus configuration failed");
        int ready[2]; require(::pipe2(ready, O_CLOEXEC) == 0, "private bus pipe failed");
        child = ::fork();
        if (child == 0) {
            ::dup2(ready[1], STDOUT_FILENO); ::close(ready[0]); ::close(ready[1]);
            ::execl(PCINSIGHT_TEST_DBUS_DAEMON, PCINSIGHT_TEST_DBUS_DAEMON, "--nofork", "--config-file", config.c_str(), "--print-address", static_cast<char*>(nullptr));
            ::_exit(127);
        }
        ::close(ready[1]);
        if (child < 0) { ::close(ready[0]); throw std::runtime_error("private bus fork failed"); }
        struct pollfd input{ready[0], POLLIN, 0}; const auto available = ::poll(&input, 1, 3000);
        char address[1024]{}; const auto n = available > 0 ? ::read(ready[0], address, sizeof(address) - 1) : -1;
        ::close(ready[0]); require(n > 0 && std::string(address, n).starts_with("unix:path=" + socket), "private bus did not become ready");
        return socket;
    }
};
struct Native {
    SystemdActionApi api{load_systemd_action_api()};
    int (*create)(ActionBus**){}; int (*address)(ActionBus*, const char*){}; int (*client)(ActionBus*, int){};
    int (*start)(ActionBus*){}; ActionBus* (*close)(ActionBus*){};
    int (*request)(ActionBus*, const char*, std::uint64_t){}; int (*release)(ActionBus*, const char*){};
    int (*new_call)(ActionBus*, ActionMessage**, const char*, const char*, const char*, const char*){};
    int (*append)(ActionMessage*, const char*, ...){}; int (*send)(ActionBus*, ActionMessage*, std::uint64_t*){};
    int (*flush)(ActionBus*){}; const char* (*member)(ActionMessage*){};
    int (*reply)(ActionMessage*, const char*, ...){};
    int (*error)(ActionMessage*, const char*, const char*, ...){};
    Native() {
        require(bool(api), "optional system bus API missing");
#define LOAD(field, symbol) field = reinterpret_cast<decltype(field)>(dlsym(api.library.get(), symbol)); require(field, symbol)
        LOAD(create,"sd_bus_new"); LOAD(address,"sd_bus_set_address"); LOAD(client,"sd_bus_set_bus_client"); LOAD(start,"sd_bus_start");
        LOAD(close,"sd_bus_close_unref"); LOAD(request,"sd_bus_request_name"); LOAD(release,"sd_bus_release_name");
        LOAD(new_call,"sd_bus_message_new_method_call"); LOAD(append,"sd_bus_message_append"); LOAD(send,"sd_bus_send");
        LOAD(flush,"sd_bus_flush"); LOAD(member,"sd_bus_message_get_member"); LOAD(reply,"sd_bus_reply_method_return");
        LOAD(error,"sd_bus_reply_method_errorf");
#undef LOAD
    }
    ActionBus* open(const std::string& socket) {
        ActionBus* bus{};
        require(create(&bus) >= 0 && bus, "test bus allocation failed");
        if (address(bus, ("unix:path=" + socket).c_str()) < 0 || client(bus, 1) < 0 || api.timeout(bus, 2000000) < 0 || start(bus) < 0) {
            close(bus); throw std::runtime_error("test bus connection failed");
        }
        // sd_bus_start is asynchronous: finish Hello before queued test sends.
        ActionBusError error{}; ActionMessage* reply{};
        const auto ready = api.call(bus, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetId", &error, &reply, "");
        if (reply) api.message_unref(reply);
        api.error_free(&error);
        if (ready < 0) { close(bus); throw std::runtime_error("private client handshake failed"); }
        return bus;
    }
};
}
int main() {
    try {
        PrivateBus fixture; const auto socket = fixture.launch(); Native native;
        struct Bus { Native& native; ActionBus* bus; ~Bus() { native.close(bus); } } server{native, native.open(socket)}, client{native, native.open(socket)};
        require(native.request(server.bus, "org.freedesktop.Notifications", 0) >= 0, "private notifications name unavailable");
        LinuxNotificationObserver observer; LinuxNotificationOptions options;
        options.consent_to_monitor_notification_traffic = true; options.bus_socket_path = socket;
        const auto started = observer.start(options);
        if (!started) throw std::runtime_error("observer start failed: " + started.message);
        require(observer.report().value.has_value(), "no initial observed period");
        require(!observer.poll(0).value && observer.report().value.has_value(), "bad poll argument damaged healthy capture");
        require(observer.start(options).error == ActionError::invalid_argument, "double start not rejected");
        auto emit = [&](const char* hint, std::uint32_t replacement, bool no_reply = false) {
            ActionMessage* message{};
            require(native.new_call(client.bus, &message, "org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify") >= 0, "private Notify allocation failed");
            struct Message { Native& native; ActionMessage* message; ~Message() { native.api.message_unref(message); } } cleanup{native, message};
            const auto rc = hint ? native.append(message, "susssasa{sv}i", "Private test", replacement, "", "PRIVATE TITLE NOT RETAINED", "PRIVATE BODY NOT RETAINED", 0, 1, "desktop-entry", "s", hint, -1) :
                native.append(message, "susssasa{sv}i", "Private test", replacement, "", "PRIVATE TITLE NOT RETAINED", "PRIVATE BODY NOT RETAINED", 0, 0, -1);
            std::uint64_t cookie{}; // A null cookie tells sd-bus to set NO_REPLY_EXPECTED.
            require(rc >= 0 && native.send(client.bus, message, no_reply ? nullptr : &cookie) >= 0 && native.flush(client.bus) >= 0, "private Notify send failed");
        };
        emit("org.test.App", 0); emit("org.test.App", 17); emit(nullptr, 0); emit("org.test.Rejected", 0);
        emit("org.test.Unconfirmed", 0, true);
        unsigned handled = 0;
        Metric<LinuxNotificationReport> report;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
        do {
            for (unsigned i = 0; i < 16; ++i) {
                ActionMessage* message{}; const auto rc = native.api.process(server.bus, &message);
                require(rc >= 0, "private server processing failed");
                if (message) {
                    const auto* name = native.member(message);
                    if (name && std::strcmp(name, "Notify") == 0) {
                        ++handled;
                        const auto replied = handled == 4 ? native.error(message, "org.test.Rejected", "private rejection") :
                            native.reply(message, "u", handled <= 2 ? 17U : 18U);
                        require(replied >= 0, "private server reply failed");
                    }
                    native.api.message_unref(message);
                }
                if (!rc) break;
            }
            require(native.flush(server.bus) >= 0, "private reply flush failed");
            report = observer.poll(1); require(report.value.has_value(), "monitor lost healthy period");
            if (handled == 5 && report.value->rejected_calls == 1 && report.value->apps.size() == 2 && !report.value->pending_calls && report.value->unconfirmed_no_reply_calls == 1) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } while (std::chrono::steady_clock::now() < deadline);
        if (!(handled == 5 && report.value && report.value->apps.size() == 2 && report.value->rejected_calls == 1 && report.value->pending_calls == 0 && report.value->unconfirmed_no_reply_calls == 1)) {
            std::cerr << "diagnostic handled=" << handled << " rows=" << (report.value ? report.value->apps.size() : 0)
                      << " rejected=" << (report.value ? report.value->rejected_calls : 0)
                      << " pending=" << (report.value ? report.value->pending_calls : 0) << '\n';
            throw std::runtime_error("private call/reply accounting incomplete");
        }
        std::uint64_t accepted = 0, replacements = 0, unattributed = 0;
        for (const auto& app : report.value->apps) {
            accepted += app.accepted_notify_calls; replacements += app.replacement_calls;
            if (!app.declared_desktop_entry.value) unattributed += app.accepted_notify_calls;
            else require(app.declared_desktop_entry.value == "org.test.App" && app.declared_desktop_entry.support == Support::estimated, "unexpected app attribution");
        }
        require(accepted == 3 && replacements == 1 && unattributed == 1, "wrong accepted/replacement/unknown app counters");
        observer.stop(); const auto stopped = observer.report();
        require(stopped.value && !stopped.value->running && stopped.value->apps.size() == 2, "stop did not retain finite observed period");
        require(bool(observer.start(options)) && observer.report().value->apps.empty(), "restart did not begin a new empty period");
        require(native.release(server.bus, "org.freedesktop.Notifications") >= 0, "private server name release failed");
        const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        do { report = observer.poll(); if (!report.value) break; std::this_thread::sleep_for(std::chrono::milliseconds(5)); } while (std::chrono::steady_clock::now() < until);
        require(!report.value && !observer.report().value, "server owner change not invalidated");
        PrivateBus denied_fixture; const auto denied_socket = denied_fixture.launch(true);
        Bus denied_server{native, native.open(denied_socket)};
        require(native.request(denied_server.bus, "org.freedesktop.Notifications", 0) >= 0, "denial fixture server unavailable");
        options.bus_socket_path = denied_socket;
        const auto denied = observer.start(options);
        require(denied.error == ActionError::permission_denied && !observer.report().value &&
                observer.report().support == Support::permission_required, "monitor permission denial not preserved");
        std::cout << "PASS private native D-Bus monitor: accepted3/replacements1/rejected1/unknown-app1/unconfirmed1; explicit stop/new period, owner-loss and permission denial; no notification content retained\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
