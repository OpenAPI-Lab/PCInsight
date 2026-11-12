#include "../src/linux_session_actions.hpp"
#include "../src/linux_users.hpp"
#include "pcinsight/audit.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#ifdef __linux__
#include <cerrno>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <paths.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utmpx.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
#ifdef __linux__
volatile sig_atomic_t hup = 0;
void caught(int) { hup = 1; }
char bounded_read(int fd) {
    pollfd descriptor{fd, POLLIN, 0};
    int rc;
    do { rc = poll(&descriptor, 1, 2000); } while (rc < 0 && errno == EINTR);
    assert(rc == 1 && (descriptor.revents & POLLIN));
    char value; ssize_t count;
    do { count = read(fd, &value, 1); } while (count < 0 && errno == EINTR);
    assert(count == 1); return value;
}
struct PrivateSession {
    pid_t child{-1};
    int request{-1}, response{-1};
    std::filesystem::path dir;
    PrivateSession() {
        int in[2], out[2]; assert(pipe(in) == 0 && pipe(out) == 0);
        child = fork(); assert(child >= 0);
        if (child == 0) {
            close(in[1]); close(out[0]);
            if (setsid() < 0) _exit(8);
            struct sigaction action{}; action.sa_handler = caught; sigemptyset(&action.sa_mask);
            if (sigaction(SIGHUP, &action, nullptr) != 0) _exit(9);
            sigset_t mask; sigemptyset(&mask); sigaddset(&mask, SIGHUP);
            if (sigprocmask(SIG_UNBLOCK, &mask, nullptr) != 0) _exit(10);
            char ready = 'R'; if (write(out[1], &ready, 1) != 1) _exit(11);
            while (true) {
                char command; auto count = read(in[0], &command, 1);
                if (count < 0 && errno == EINTR) continue;
                if (count != 1) _exit(0);
                const char state = hup ? 'H' : 'N';
                if (write(out[1], &state, 1) != 1) _exit(12);
            }
        }
        close(in[0]); close(out[1]); request = in[1]; response = out[0];
        assert(bounded_read(response) == 'R');
        char name[] = "/tmp/pcinsight-disconnect-XXXXXX";
        auto* path = mkdtemp(name); assert(path); dir = path;
        const auto file = dir / "utmp";
        std::ofstream stream(file, std::ios::binary);
        utmpx row{}; row.ut_type = USER_PROCESS; row.ut_pid = child;
        std::strcpy(row.ut_line, "pcinsight-disconnect"); std::strcpy(row.ut_user, "pcinsight-fixture");
        stream.write(reinterpret_cast<const char*>(&row), sizeof row);
        stream.close(); assert(stream);
        assert(utmpxname(file.c_str()) == 0);
    }
    void untouched() {
        const char question = 'Q'; assert(write(request, &question, 1) == 1);
        assert(bounded_read(response) == 'N');
        int status; assert(waitpid(child, &status, WNOHANG) == 0);
    }
    ~PrivateSession() {
        endutxent(); utmpxname(_PATH_UTMP);
        if (request >= 0) close(request);
        if (response >= 0) close(response);
        if (child > 0) { kill(child, SIGKILL); int status; while (waitpid(child, &status, 0) < 0 && errno == EINTR) {} }
        std::error_code ec;
        assert(std::filesystem::remove(dir / "utmp", ec) && !ec);
        assert(std::filesystem::remove(dir, ec) && !ec); // exact owned file/directory only
    }
};
#endif
int main() {
    // Static capability is usable without a running session/bus/remote connection.
    for (const auto provider : {LinuxDisconnectProvider::utmp, LinuxDisconnectProvider::login1}) {
        const auto cap = query_linux_disconnect_capability(provider);
#ifdef __linux__
        assert(cap.value == false && cap.support == Support::available);
        assert(cap.note.find(provider == LinuxDisconnectProvider::utmp ? "login records" : "no detach operation") != cap.note.npos);
#else
        assert(!cap.value && cap.support == Support::unsupported);
#endif
    }
    for (const auto raw : {-1, 2, 99}) {
        const auto unknown = query_linux_disconnect_capability(static_cast<LinuxDisconnectProvider>(raw));
        assert(!unknown.value && unknown.support == Support::unsupported);
    }
    for (const auto provider : {"utmp", "login1"}) {
        LinuxSessionDetails details; details.provider = provider;
        for (const auto state : {"active", "online", "closing", "unknown"}) {
            details.state = {state, Support::available, "fixture"};
            for (const bool remote : {false, true}) for (const bool active : {false, true}) {
                details.remote = {remote, Support::available, "fixture"};
                details.active = {active, Support::available, "fixture"};
                LinuxLoginRecord login{"fixture", "fixture-user", state, "not-a-detach-token",
                    {details, Support::available, "fixture"}};
                const auto rows = aggregate_linux_users(std::vector{login}, {});
                assert(rows.size() == 1);
                const auto& cap = rows[0].disconnect_available;
#ifdef __linux__
                assert(cap.value == false && cap.support == Support::available);
                const auto expected = query_linux_disconnect_capability(provider == std::string("utmp") ? LinuxDisconnectProvider::utmp : LinuxDisconnectProvider::login1);
                assert(cap.note == expected.note);
#else
                assert(!cap.value && cap.support == Support::unsupported);
#endif
            }
        }
    }
    for (unsigned mode = 0; mode < 6; ++mode) {
        LinuxLoginRecord login{"login1:1", "fixture-user", "active"};
        LinuxSessionDetails details; details.provider = mode == 5 ? std::string("utmp\0other", 10) : mode == 4 ? "LOGIN1" : mode == 3 ? "future" : "login1";
        if (mode != 0) login.details = {details, mode == 1 ? Support::permission_required : mode == 2 ? Support::unsupported : Support::available, "fixture"};
        if (mode == 5) {
            bool rejected = false;
            try { (void)aggregate_linux_users(std::vector{login}, {}); }
            catch (const std::invalid_argument&) { rejected = true; }
            assert(rejected); // malformed NUL metadata is rejected before capability selection
            continue;
        }
        const auto rows = aggregate_linux_users(std::vector{login}, {});
        assert(rows.size() == 1 && !rows[0].disconnect_available.value && rows[0].disconnect_available.support == Support::unsupported);
    }
    for (const auto& id : std::vector<std::string>{"", std::string("a\0b", 3), "tty\n1", "\t", std::string(1, '\x7f'), std::string(4097, 'x')})
        assert(disconnect_linux_session(id).error == ActionError::invalid_argument);
    for (const auto& id : {"pts/0", "pcinsight-disconnect", "42", "logind:session-1", "用户", "login1:2", "login1@:1.42/org/freedesktop/login1/session/_32#12345"}) {
        const auto result = disconnect_linux_session(id);
        assert(result.error == ActionError::not_supported && !result.process_id);
    }
#ifdef __linux__
    PrivateSession session;
    std::vector<AuditEvent> events;
    AuditedActions audited(make_system_actions(), [&](const AuditEvent& e) { events.push_back(e); });
    assert(audited.disconnect_user("pcinsight-disconnect").error == ActionError::not_supported);
    assert(events.size() == 2 && events[1].result == ActionError::not_supported && events[1].backend_invoked);
    session.untouched(); // a matching utmp row MUST NOT authorize guessed group SIGHUP
    // Even a plausible destructive-logout token must not select the logout provider.
    for (const auto& id : {"login1:2", "login1@:1.42/org/freedesktop/login1/session/_32#12345", "pts/0"}) {
        events.clear();
        assert(audited.disconnect_user(id).error == ActionError::not_supported);
        assert(events.size() == 2 && events[0].operation == AuditOperation::disconnect_user && events[1].operation == AuditOperation::disconnect_user);
        assert(events[1].result == ActionError::not_supported && !events[1].pid);
        session.untouched();
    }
    Monitor monitor; SampleOptions options;
    options.include_user_sessions = true; options.include_processes = false;
    options.prefer_logind_sessions = false;
    options.include_gpus = options.include_disks = options.include_networks = false;
    const auto snapshot = monitor.sample(options);
    assert(snapshot.user_sessions.size() == 1);
    const auto& available = snapshot.user_sessions[0].disconnect_available;
    assert(available.value == false && available.support == Support::available);
    assert(available.note == query_linux_disconnect_capability(LinuxDisconnectProvider::utmp).note);
    assert(make_system_actions()->disconnect_user("").error == ActionError::invalid_argument);
    session.untouched();
    std::cout << "Native Linux private POSIX session preserved: child responsive/no SIGHUP; utmp capability reason propagated, logout tokens never substituted\n";
#endif
    std::cout << "Disconnect capability: recognized adapters, unknown providers, active/remote flags and no false success checked\n";
}
