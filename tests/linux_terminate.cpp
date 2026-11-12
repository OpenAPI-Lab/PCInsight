#include "../src/linux_terminate.hpp"
#include "pcinsight/monitor.hpp"
#include "pcinsight/audit.hpp"
#include <cassert>
#include <cerrno>
#include <limits>
#include <vector>
#ifdef __linux__
#include <csignal>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
struct Fake {
    LinuxTerminateApi api;
    unsigned opens{}, signals{}, closes{}, waits{}, reads{};
    int open_error{}, signal_error{}, read_error{}, context_error{}, wait_error{}, after_error{};
    bool exited_before{}, exited_after{true}, forced{};
    std::uint64_t ticks{77};
    std::uint32_t identity_pid{123};
    Fake() {
        api.self_pid = 10;
        api.open = [&](std::uint32_t pid) { assert(pid == 123); ++opens; return LinuxPidfdResult{open_error ? -1 : 9, open_error}; };
        api.check_proc_context = [&] { return context_error; };
        api.identity = [&](std::uint32_t pid) { assert(pid == 123); ++reads; return LinuxIdentityResult{identity_pid, ticks, read_error}; };
        api.signal = [&](int fd, bool force) { assert(fd == 9); ++signals; forced = force; return signal_error; };
        api.wait_exit = [&](int fd, unsigned ms) { assert(fd == 9); ++waits; assert(ms == 0 || ms == 5000); return LinuxExitResult{ms ? exited_after : exited_before, ms ? after_error : wait_error}; };
        api.close = [&](int fd) { assert(fd == 9); ++closes; };
    }
    ActionResult run(std::optional<std::uint64_t> expected = 77, bool force = false) {
        return terminate_linux_process(123, expected, force, api);
    }
};
#ifdef __linux__
struct Child {
    pid_t pid{-1};
    explicit Child(bool ignore = false) {
        int fds[2]; assert(pipe2(fds, O_CLOEXEC) == 0);
        pid = fork(); assert(pid >= 0);
        if (pid == 0) {
            close(fds[0]);
            struct sigaction action{}; action.sa_handler = ignore ? SIG_IGN : SIG_DFL;
            sigemptyset(&action.sa_mask);
            if (sigaction(SIGTERM, &action, nullptr) != 0) _exit(8);
            sigset_t mask; sigemptyset(&mask); sigaddset(&mask, SIGTERM);
            if (sigprocmask(SIG_UNBLOCK, &mask, nullptr) != 0) _exit(9);
            char ready = 'R'; if (write(fds[1], &ready, 1) != 1) _exit(10);
            close(fds[1]);
            while (true) pause();
        }
        close(fds[1]); char ready = 0; ssize_t count;
        do { count = read(fds[0], &ready, 1); } while (count < 0 && errno == EINTR);
        close(fds[0]); assert(count == 1 && ready == 'R');
    }
    ~Child() {
        if (pid > 0) { kill(pid, SIGKILL); int status; while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {} }
    }
    void reap(int signal) {
        int status; pid_t result;
        do { result = waitpid(pid, &status, 0); } while (result < 0 && errno == EINTR);
        assert(result == pid && WIFSIGNALED(status) && WTERMSIG(status) == signal);
        pid = -1;
    }
};
std::uint64_t identity(pid_t pid) {
    Monitor monitor;
    SampleOptions options; options.include_app_history = false;
    const auto snapshot = monitor.sample(options);
    for (const auto& process : snapshot.processes) if (process.pid == static_cast<std::uint32_t>(pid)) {
        assert(process.linux_start_ticks.value && *process.linux_start_ticks.value > 0);
        return *process.linux_start_ticks.value;
    }
    assert(false); return 0;
}
#endif
int main() {
    for (auto pid : {0u, 1u, 10u, std::numeric_limits<std::uint32_t>::max()}) {
        Fake f; assert(terminate_linux_process(pid, 77, true, f.api).error == ActionError::invalid_argument);
        assert(!f.opens && !f.signals);
    }
    { Fake f; assert(f.run(0).error == ActionError::invalid_argument); assert(!f.opens); }
    { Fake f; assert(f.run()); assert(f.opens == 1 && f.reads == 1 && f.signals == 1 && f.closes == 1 && !f.forced); }
    { Fake f; assert(f.run(std::nullopt, true)); assert(!f.reads && f.forced && f.closes == 1); }
    for (auto code : {ENOSYS, ENODEV, EACCES, ESRCH, EMFILE}) {
        Fake f; f.open_error = code;
        auto result = f.run(); assert(!result && !f.signals && !f.closes);
        if (code == ENOSYS || code == ENODEV) assert(result.error == ActionError::not_supported);
        if (code == EACCES) assert(result.error == ActionError::permission_denied);
        if (code == ESRCH) assert(result.error == ActionError::not_found);
    }
    { Fake f; f.context_error = EOPNOTSUPP; assert(f.run().error == ActionError::not_supported); assert(!f.opens); }
    { Fake f; f.ticks = 78; assert(f.run().error == ActionError::stale_target); assert(!f.signals && f.closes == 1); }
    { Fake f; f.identity_pid = 124; assert(f.run().error == ActionError::stale_target); assert(!f.signals && f.closes == 1); }
    for (auto code : {EACCES, ENOENT, EIO, EOVERFLOW}) {
        Fake f; f.read_error = code; assert(!f.run()); assert(!f.signals && f.closes == 1);
    }
    { Fake f; f.exited_before = true; assert(f.run().error == ActionError::not_found); assert(!f.signals && f.closes == 1); }
    { Fake f; f.wait_error = EBADF; assert(!f.run()); assert(!f.signals && f.closes == 1); }
    for (auto code : {ESRCH, EPERM, ENOSYS}) {
        Fake f; f.signal_error = code; assert(!f.run()); assert(f.signals == 1 && f.closes == 1 && f.waits == 1);
    }
    { Fake f; f.exited_after = false; auto result = f.run(); assert(result.error == ActionError::system_error); assert(result.message.find("aceptada") != std::string::npos && f.signals == 1 && f.closes == 1); }
    { Fake f; f.after_error = EIO; auto result = f.run(); assert(result.error == ActionError::system_error); assert(result.message.find("aceptada") != std::string::npos && f.signals == 1 && f.closes == 1); }
    { Fake f; f.api.open = {}; assert(f.run().error == ActionError::not_supported); }
#ifdef __linux__
    auto actions = make_system_actions();
    assert(actions->terminate_process(static_cast<std::uint32_t>(getpid()), true).error == ActionError::invalid_argument);
    {
        Child child; const auto ticks = identity(child.pid);
        assert(actions->terminate_process_checked(child.pid, ticks + 1, true).error == ActionError::stale_target);
        assert(kill(child.pid, 0) == 0);
        std::vector<AuditEvent> events;
        AuditedActions audited(make_system_actions(), [&](const AuditEvent& e) { events.push_back(e); });
        assert(audited.terminate_process_checked(child.pid, ticks, false));
        assert(events.size() == 2 && events[0].expected_linux_start_ticks == ticks && !events[0].expected_windows_creation_time_100ns);
        child.reap(SIGTERM);
    }
    { Child child; const auto ticks = identity(child.pid); assert(actions->terminate_process_checked(child.pid, ticks, true)); child.reap(SIGKILL); }
    {
        Child child(true); const auto ticks = identity(child.pid);
        const auto result = actions->terminate_process_checked(child.pid, ticks, false);
        assert(!result && result.error == ActionError::system_error && kill(child.pid, 0) == 0);
        assert(actions->terminate_process_checked(child.pid, ticks, true)); child.reap(SIGKILL);
    }
    { Child child; assert(actions->terminate_process(child.pid, true)); child.reap(SIGKILL); }
    int status; assert(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD);
#endif
}
