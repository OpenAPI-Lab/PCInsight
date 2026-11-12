#include "../src/linux_efficiency.hpp"
#include "pcinsight/audit.hpp"
#include "../src/linux_process_details.hpp"
#include <cassert>
#include <cerrno>
#include <limits>
#include <iostream>
#include <vector>
#include <map>
#include <atomic>
#include <thread>
#ifdef __linux__
#include <csignal>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
struct Fake {
    LinuxEfficiencyApi api;
    int nice{}, io{}, nice_error{}, io_error{}, set_nice_error{}, set_io_error{};
    int final_nice_error{}, final_io_error{};
    unsigned nice_reads{}, io_reads{}, nice_sets{}, io_sets{};
    bool ignore_nice{}, ignore_io{};
    std::vector<int> order;
    Fake() {
        api.self_pid = 10;
        api.read_nice = [&](std::uint32_t pid) { assert(pid == 123); ++nice_reads; return LinuxPriorityResult{nice, nice_reads > 1 ? final_nice_error : nice_error}; };
        api.read_io = [&](std::uint32_t pid) { assert(pid == 123); ++io_reads; return LinuxPriorityResult{io, io_reads > 1 ? final_io_error : io_error}; };
        api.set_nice = [&](std::uint32_t pid, int value) { assert(pid == 123); ++nice_sets; order.push_back(2); if (!set_nice_error && !ignore_nice) nice = value; return set_nice_error; };
        api.set_io = [&](std::uint32_t pid, int value) { assert(pid == 123); ++io_sets; order.push_back(1); if (!set_io_error && !ignore_io) io = value; return set_io_error; };
    }
    ActionResult run(bool enabled = true) { return set_linux_efficiency(123, enabled, api); }
};
struct ThreadFake {
    struct Row { int nice{}, io{}; std::uint64_t ticks{10}; };
    std::map<std::uint32_t, Row> rows{{123, {5, 0, 10}}, {124, {19, (2 << 13) | 7, 11}}};
    LinuxThreadEfficiencyApi api;
    unsigned writes{}, lists{};
    int write_error{}, io_write_error{}, context_error{};
    bool add_thread{}, duplicate{}, fail_capture{}, reuse_after_write{}, ignore_write{}, read_back_error{};
    ThreadFake() {
        api.priorities.self_pid = 10;
        api.check_context = [&] { return context_error; };
        api.identity = [&](std::uint32_t pid, std::uint32_t tid) {
            assert(pid == 123);
            const auto it = rows.find(tid);
            return it == rows.end() ? LinuxIdentityResult{0, 0, ESRCH} :
                LinuxIdentityResult{tid, it->second.ticks + (reuse_after_write && writes ? 1 : 0), 0};
        };
        api.list = [&](std::uint32_t pid) {
            assert(pid == 123); ++lists;
            std::vector<std::uint32_t> tids{124, 123};
            if (add_thread && lists > 1) tids.push_back(125);
            if (duplicate) tids.push_back(124);
            return LinuxThreadList{tids, 0};
        };
        api.priorities.read_nice = [&](std::uint32_t tid) { return LinuxPriorityResult{rows.at(tid).nice, fail_capture && tid == 124 ? EACCES : 0}; };
        api.priorities.read_io = [&](std::uint32_t tid) { return LinuxPriorityResult{rows.at(tid).io, read_back_error && writes ? EIO : 0}; };
        api.priorities.set_nice = [&](std::uint32_t tid, int value) {
            ++writes; if (write_error) return write_error;
            if (!ignore_write) rows.at(tid).nice = value;
            return 0;
        };
        api.priorities.set_io = [&](std::uint32_t tid, int value) {
            ++writes; if (io_write_error) return io_write_error;
            if (!ignore_write) rows.at(tid).io = value; return 0;
        };
    }
    LinuxEfficiencyUpdate apply(bool ack = true, std::uint64_t ticks = 10) {
        return pcinsight::detail::apply_linux_efficiency_unchecked(123, ticks, ack, api);
    }
    LinuxEfficiencyUpdate restore(const LinuxEfficiencyReceipt& receipt, bool ack = true) {
        return pcinsight::detail::restore_linux_efficiency_unchecked(receipt, ack, api);
    }
};
void thread_tests() {
    { ThreadFake f; assert(!f.apply(false).result && !f.writes && !f.lists); }
    { ThreadFake f; assert(f.apply(true, 9).result.error == ActionError::stale_target && !f.writes); }
    { ThreadFake f; f.context_error = EOPNOTSUPP; assert(!f.apply().result && !f.writes); }
    { ThreadFake f; f.api.identity = {}; assert(f.apply().result.error == ActionError::not_supported && !f.writes); }
    { ThreadFake f; f.add_thread = true; assert(!f.apply().result && !f.writes); }
    { ThreadFake f; f.duplicate = true; assert(!f.apply().result && !f.writes); }
    { ThreadFake f; f.fail_capture = true; assert(f.apply().result.error == ActionError::permission_denied && !f.writes); }
    { ThreadFake f; f.rows.at(124).io = 8; assert(!f.apply().result && !f.writes); }
    { ThreadFake f; f.rows.at(123).nice = -1; assert(f.apply().result && f.rows.at(123).nice == 10); }
    { ThreadFake f; f.api.list = [](auto) { return LinuxThreadList{std::vector<std::uint32_t>(4097, 123), 0}; }; assert(!f.apply().result && !f.writes); }
    { ThreadFake f; f.io_write_error = ENOSYS; auto r = f.apply(); assert(r.result.error == ActionError::not_supported && f.writes == 1 && !r.receipt.threads[0].io_pending); }
    { ThreadFake f; f.read_back_error = true; auto r = f.apply(); assert(!r.result && r.receipt.threads[0].io_pending && f.writes == 1); f.read_back_error = false; assert(f.restore(r.receipt).result); }
    {
        ThreadFake f; auto applied = f.apply(); assert(applied.result);
        assert(applied.receipt.threads.size() == 2 && f.writes == 3);
        assert(f.rows.at(123).nice == 10 && f.rows.at(124).nice == 19);
        assert(f.rows.at(123).io == (3 << 13) && f.rows.at(124).io == (3 << 13));
        const auto count = f.writes;
        assert(!f.restore(applied.receipt, false).result && f.writes == count);
        auto restored = f.restore(applied.receipt); assert(restored.result && f.writes == 6);
        assert(f.rows.at(123).nice == 5 && f.rows.at(123).io == 0);
        assert(f.rows.at(124).nice == 19 && f.rows.at(124).io == ((2 << 13) | 7));
        for (const auto& row : restored.receipt.threads) assert(!row.nice_pending && !row.io_pending);
        assert(f.restore(restored.receipt).result && f.writes == 6);
    }
    {
        ThreadFake f; f.write_error = EACCES; auto applied = f.apply();
        assert(applied.result.error == ActionError::permission_denied);
        assert(applied.receipt.threads[0].io_pending && !applied.receipt.threads[0].nice_pending);
        auto restored = f.restore(applied.receipt); assert(restored.result);
        assert(!restored.receipt.threads[0].io_pending && f.rows.at(123).io == 0);
    }
    {
        ThreadFake f; auto applied = f.apply(); assert(applied.result);
        f.write_error = EPERM; auto restored = f.restore(applied.receipt);
        assert(restored.result.error == ActionError::permission_denied);
        assert(restored.receipt.threads[0].nice_pending && !restored.receipt.threads[0].io_pending);
        const auto writes = f.writes; f.write_error = 0;
        auto retried = f.restore(restored.receipt); assert(retried.result && f.writes == writes + 2);
    }
    {
        ThreadFake f; auto applied = f.apply(); assert(applied.result);
        f.rows.at(123).ticks++; const auto count = f.writes;
        assert(f.restore(applied.receipt).result.error == ActionError::stale_target && f.writes == count);
    }
    {
        ThreadFake f; auto applied = f.apply(); assert(applied.result);
        f.rows.at(123).io = (2 << 13) | 1; const auto count = f.writes;
        assert(f.restore(applied.receipt).result.error == ActionError::stale_target && f.writes == count);
    }
    {
        ThreadFake f; auto applied = f.apply(); assert(applied.result);
        f.rows.at(123).nice = 15; const auto count = f.writes;
        auto r = f.restore(applied.receipt);
        assert(r.result.error == ActionError::stale_target && f.writes == count + 1);
        assert(!r.receipt.threads[0].io_pending && r.receipt.threads[0].nice_pending && f.rows.at(123).nice == 15);
    }
    {
        ThreadFake f; auto applied = f.apply(); assert(applied.result);
        f.rows.at(124).ticks++; auto r = f.restore(applied.receipt);
        assert(r.result.error == ActionError::stale_target && r.receipt.threads[1].io_pending);
        assert(!r.receipt.threads[0].io_pending && !r.receipt.threads[0].nice_pending);
    }
    {
        ThreadFake f; auto applied = f.apply(); assert(applied.result);
        auto malformed = applied.receipt; malformed.threads.push_back(malformed.threads.front());
        const auto count = f.writes; assert(f.restore(malformed).result.error == ActionError::invalid_argument && f.writes == count);
        malformed = applied.receipt; malformed.threads.back().original_nice = -21;
        assert(f.restore(malformed).result.error == ActionError::invalid_argument && f.writes == count);
        malformed = applied.receipt; malformed.threads.resize(4097);
        assert(f.restore(malformed).result.error == ActionError::invalid_argument && f.writes == count);
    }
    { ThreadFake f; f.reuse_after_write = true; auto r = f.apply(); assert(!r.result && f.writes == 1 && r.receipt.threads[0].io_pending); }
    { ThreadFake f; f.ignore_write = true; auto r = f.apply(); assert(!r.result && f.writes == 1 && r.receipt.threads[0].io_pending); }
    // A newly observed thread after writes must report partial coverage.
    { ThreadFake f; const auto base = f.api.list; f.api.list = [&](auto pid) { auto r = base(pid); if (f.lists == 3) r.tids.push_back(125); return r; }; auto r = f.apply(); assert(!r.result && f.writes == 3); }
    // Shared I/O context: one write affects both threads; restore its recorded
    // write once, rather than resetting every thread to best-effort defaults.
    { ThreadFake f; f.rows.at(124).io = 0; f.api.priorities.set_io = [&](auto, int v) { ++f.writes; for (auto& [id, row] : f.rows) { (void)id; row.io = v; } return 0; }; auto r = f.apply(); assert(r.result && f.writes == 2); assert(f.restore(r.receipt).result && f.writes == 4); assert(f.rows.at(124).io == 0); }
}
#ifdef __linux__
struct Child {
    pid_t pid{-1};
    explicit Child(int nice, bool two_threads = false) {
        int fds[2]; assert(pipe2(fds, O_CLOEXEC) == 0);
        pid = fork(); assert(pid >= 0);
        if (pid == 0) {
            close(fds[0]);
            if (setpriority(PRIO_PROCESS, 0, nice) != 0) _exit(8);
            std::atomic<bool> worker_ready{false};
            std::thread worker;
            if (two_threads) {
                worker = std::thread([&] {
                    if (setpriority(PRIO_PROCESS, 0, 19) != 0) _exit(10);
                    worker_ready.store(true);
                    while (true) pause();
                });
                while (!worker_ready.load()) std::this_thread::yield();
            }
            char ready = 'R'; if (write(fds[1], &ready, 1) != 1) _exit(9);
            close(fds[1]); while (true) pause();
        }
        close(fds[1]); char ready = 0; ssize_t count;
        do { count = read(fds[0], &ready, 1); } while (count < 0 && errno == EINTR);
        close(fds[0]); assert(count == 1 && ready == 'R');
    }
    ~Child() {
        if (pid > 0) { kill(pid, SIGKILL); int status; while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {} }
    }
};
int nice_of(pid_t pid) { errno = 0; auto value = getpriority(PRIO_PROCESS, pid); assert(!errno); return value; }
int io_of(pid_t pid) {
#ifdef SYS_ioprio_get
    const auto value = syscall(SYS_ioprio_get, 1, pid); assert(value >= 0); return static_cast<int>(value);
#else
    (void)pid; assert(false); return 0;
#endif
}
#endif
int main() {
    thread_tests();
    for (auto pid : {0u, 1u, 10u, std::numeric_limits<std::uint32_t>::max()}) {
        Fake f; assert(set_linux_efficiency(pid, true, f.api).error == ActionError::invalid_argument);
        assert(!f.nice_reads && !f.io_sets);
    }
    { Fake f; assert(f.run()); assert(f.nice == 10 && f.io == (3 << 13)); assert((f.order == std::vector<int>{1, 2})); }
    { Fake f; f.nice = 19; assert(f.run()); assert(f.nice == 19 && !f.nice_sets); }
    { Fake f; f.nice = -1; assert(f.run()); assert(f.nice == 10); } // -1 is not an error
    { Fake f; f.nice = 10; f.io = 3 << 13; assert(f.run()); assert(!f.nice_sets && !f.io_sets); }
    { Fake f; f.nice = 15; f.io = 3 << 13; assert(f.run(false)); assert(f.nice == 0 && f.io == ((2 << 13) | 4)); }
    for (auto error : {EPERM, EACCES, ESRCH, ENOSYS, EINVAL, EIO}) {
        Fake f; f.io_error = error; assert(!f.run()); assert(!f.io_sets && !f.nice_sets);
        Fake g; g.nice_error = error; assert(!g.run()); assert(!g.io_reads && !g.nice_sets);
    }
    for (auto error : {EPERM, ESRCH, ENOSYS}) {
        Fake f; f.set_io_error = error; assert(!f.run()); assert(f.io_sets == 1 && !f.nice_sets && f.nice == 0);
        Fake g; g.set_nice_error = error; auto r = g.run(); assert(!r && r.message.find("parcial") != std::string::npos);
        assert(g.io == (3 << 13) && g.nice == 0 && g.io_sets == 1 && g.nice_sets == 1);
    }
    { Fake f; f.io = 3 << 13; f.set_nice_error = EPERM; auto r = f.run(); assert(!r && !f.io_sets && r.message.find("sin cambio E/S") != std::string::npos); }
    { Fake f; f.ignore_nice = true; assert(!f.run()); }
    { Fake f; f.ignore_io = true; assert(!f.run()); }
    { Fake f; f.final_nice_error = ESRCH; auto r = f.run(); assert(!r && r.message.find("desconocido") != std::string::npos); }
    { Fake f; f.final_io_error = EIO; assert(!f.run()); }
    for (auto nice : {-21, 20}) { Fake f; f.nice = nice; assert(!f.run() && !f.io_sets); }
    for (auto io : {-1, 0x10000, 8, (4 << 13), (2 << 13) | 8}) { Fake f; f.io = io; assert(!f.run() && !f.io_sets); }
    { Fake f; f.api.read_io = {}; assert(f.run().error == ActionError::not_supported); }
#ifdef __linux__
    const auto own_nice = nice_of(getpid()), own_io = io_of(getpid());
    auto actions = make_system_actions();
    assert(actions->set_efficiency_mode(getpid(), true).error == ActionError::not_supported);
    assert(actions->set_efficiency_mode_checked(0, 1234, true).error == ActionError::not_supported);
    {
        Child child(0); std::vector<AuditEvent> events;
        const auto original_io=io_of(child.pid);
        AuditedActions audited(make_system_actions(), [&](const AuditEvent& e) { events.push_back(e); });
        assert(audited.set_efficiency_mode(child.pid, true).error == ActionError::not_supported);
        assert(events.size() == 2 && events[0].enabled && events[1].result == ActionError::not_supported);
        assert(actions->set_efficiency_mode(child.pid, false).error == ActionError::not_supported);
        assert(actions->set_efficiency_mode_checked(child.pid, 1234, true).error == ActionError::not_supported);
        assert(nice_of(child.pid) == 0 && io_of(child.pid) == original_io);
        std::cout << "Linux generic/checked actions rejected with unchanged private-child priorities; audit rejection recorded\n";
        // The old algorithm is retained internally for regression coverage,
        // not exposed as a bypass through SystemActions or installed headers.
        assert(detail::set_linux_efficiency(child.pid, true));
        assert(nice_of(child.pid) == 10 && (io_of(child.pid) >> 13) == 3);
        assert(detail::set_linux_efficiency(child.pid, true));
        const auto disabled = detail::set_linux_efficiency(child.pid, false);
        std::cout << "Linux preset disable: " << disabled.message << '\n';
        if (disabled) assert(nice_of(child.pid) == 0 && io_of(child.pid) == ((2 << 13) | 4));
        else {
            assert(disabled.error == ActionError::permission_denied);
            assert(nice_of(child.pid) == 10 && io_of(child.pid) == ((2 << 13) | 4));
            assert(disabled.message.find("parcial") != std::string::npos);
        }
    }
    { Child child(19); assert(detail::set_linux_efficiency(child.pid, true)); assert(nice_of(child.pid) == 19 && (io_of(child.pid) >> 13) == 3); }
    {
        Child child(19, true);
        const auto id = read_linux_process_identity(std::filesystem::path("/proc") / std::to_string(child.pid));
        assert(id.value && id.value->pid == static_cast<std::uint32_t>(child.pid));
        const auto before_io=io_of(child.pid);
        assert(pcinsight::apply_linux_efficiency_unchecked(child.pid, id.value->start_ticks).result.error == ActionError::invalid_argument);
        assert(nice_of(child.pid)==19&&io_of(child.pid)==before_io);
        auto applied = pcinsight::apply_linux_efficiency_unchecked(child.pid, id.value->start_ticks, true);
        assert(applied.result && applied.receipt.threads.size() == 2);
        for (const auto& row : applied.receipt.threads) assert(row.original_nice == 19 && !row.nice_pending && (io_of(row.tid) >> 13) == 3);
        auto unacknowledged=pcinsight::restore_linux_efficiency_unchecked(applied.receipt);
        assert(unacknowledged.result.error==ActionError::invalid_argument);
        assert(unacknowledged.receipt.threads.size()==applied.receipt.threads.size());
        for(std::size_t i=0;i<unacknowledged.receipt.threads.size();++i){
            const auto& row=unacknowledged.receipt.threads[i];
            const auto& before=applied.receipt.threads[i];
            assert(row.io_pending==before.io_pending&&row.nice_pending==before.nice_pending);
            assert(nice_of(row.tid)==19&&(io_of(row.tid)>>13)==3);
        }
        auto restored = pcinsight::restore_linux_efficiency_unchecked(applied.receipt, true);
        std::cout << "Linux thread restore: " << restored.result.message << '\n';
        assert(restored.result);
        for (const auto& row : restored.receipt.threads) assert(!row.nice_pending && !row.io_pending && nice_of(row.tid) == row.original_nice && io_of(row.tid) == row.original_io);
        assert(pcinsight::restore_linux_efficiency_unchecked(restored.receipt, true).result);
    }
    {
        Child child(5, true);
        const auto id = read_linux_process_identity(std::filesystem::path("/proc") / std::to_string(child.pid));
        assert(id.value);
        auto applied = pcinsight::apply_linux_efficiency_unchecked(child.pid, id.value->start_ticks, true);
        assert(applied.result && applied.receipt.threads.size() == 2 && nice_of(child.pid) == 10);
        auto restored = pcinsight::restore_linux_efficiency_unchecked(applied.receipt, true);
        std::cout << "Linux mixed thread restore: " << restored.result.message << '\n';
        if (restored.result) {
            for (const auto& row : restored.receipt.threads) assert(!row.io_pending && !row.nice_pending && nice_of(row.tid) == row.original_nice && io_of(row.tid) == row.original_io);
        } else {
            assert(restored.result.error == ActionError::permission_denied);
            const auto& leader = restored.receipt.threads.front();
            assert(leader.tid == static_cast<std::uint32_t>(child.pid) && leader.nice_pending && !leader.io_pending);
            assert(nice_of(child.pid) == 10 && io_of(child.pid) == leader.original_io);
        }
    }
    assert(nice_of(getpid()) == own_nice && io_of(getpid()) == own_io);
    int status; assert(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD);
#else
    assert(pcinsight::apply_linux_efficiency_unchecked(123, 10, true).result.error == ActionError::not_supported);
    assert(pcinsight::restore_linux_efficiency_unchecked({}, true).result.error == ActionError::not_supported);
#endif
}
