#include "../src/linux_users.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>
#ifdef __linux__
#include <cstring>
#include <filesystem>
#include <fstream>
#include <paths.h>
#include <pwd.h>
#include <thread>
#include <unistd.h>
#include <utmpx.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
template<class T> Metric<T> value(T number) { return {number, Support::available, "fixture"}; }
ProcessInfo process(std::uint32_t pid, std::string name) {
    ProcessInfo p; p.pid = pid; p.username = std::move(name); return p;
}
template<class F> void invalid(F function) {
    bool threw = false; try { function(); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
}
#ifdef __linux__
struct PrivateUtmp {
    std::filesystem::path dir;
    explicit PrivateUtmp(const std::string& user) {
        char name[] = "/tmp/pcinsight-users-XXXXXX";
        auto* p = mkdtemp(name); assert(p); dir = p;
        const auto path = dir / "utmp";
        std::ofstream file(path, std::ios::binary);
        for (const auto& [line, username] : std::vector<std::pair<std::string, std::string>>{
            {"pcinsight-a", user}, {"pcinsight-b", user}, {"pcinsight-c", "pcinsight-absent-user"}}) {
            utmpx row{}; row.ut_type = USER_PROCESS; row.ut_pid = getpid();
            assert(line.size() < sizeof row.ut_line && username.size() < sizeof row.ut_user);
            memcpy(row.ut_line, line.data(), line.size()); memcpy(row.ut_user, username.data(), username.size());
            file.write(reinterpret_cast<const char*>(&row), sizeof row);
        }
        utmpx ignored{}; ignored.ut_type = DEAD_PROCESS;
        file.write(reinterpret_cast<const char*>(&ignored), sizeof ignored);
        file.close(); assert(file);
        assert(utmpxname(path.c_str()) == 0); // private state of THIS test process
    }
    ~PrivateUtmp() {
        endutxent(); utmpxname(_PATH_UTMP);
        std::error_code ec; std::filesystem::remove_all(dir, ec);
    }
};
#endif
int main() {
    std::vector<LinuxLoginRecord> logins{{"tty1", "alice", "reported_login"},
        {"tty2", "alice", "reported_login"}, {"tty3", "bob", "reported_login"}};
    auto a = process(2, "alice"), b = process(3, "alice"), c = process(4, "bob");
    a.cpu_percent = value(0.0); a.memory_bytes = value<std::uint64_t>(20);
    a.disk_bytes_per_second = value<std::uint64_t>(0);
    b.memory_bytes = value<std::uint64_t>(30);
    b.network_bytes_per_second = {std::nullopt, Support::permission_required, "denied"};
    auto ignored = process(5, "not-logged-in"); ignored.memory_bytes = value<std::uint64_t>(9999);
    std::vector<ProcessInfo> processes{a, b, c, ignored};
    auto rows = aggregate_linux_users(logins, processes);
    assert(rows.size() == 3 && rows[0].id == "tty1" && rows[1].id == "tty2");
    for (unsigned i = 0; i < 2; ++i) {
        assert(rows[i].cpu_percent.value == 0.0 && rows[i].cpu_percent.support == Support::estimated);
        assert(rows[i].cpu_percent.note.find("1 ausentes") != std::string::npos);
        assert(rows[i].memory_bytes.value == 50 && rows[i].memory_bytes.support == Support::estimated);
        // Username-only sessions cannot receive UID-attributed I/O, even a zero.
        assert(!rows[i].disk_bytes_per_second.value&&rows[i].linux_io_details.value);
        assert(!rows[i].linux_io_details.value->real_uid.value&&!rows[i].linux_io_details.value->matching_processes.value);
        assert(!rows[i].network_bytes_per_second.value && rows[i].network_bytes_per_second.support == Support::unsupported);
        assert(rows[i].resource_scope == "linux-visible-processes-by-username");
        assert(!rows[i].disconnect_available.value); // fixture has no provider metadata
    }
    assert(!rows[2].cpu_percent.value && !rows[2].memory_bytes.value);
    auto empty = aggregate_linux_users(logins, {});
    for (const auto& row : empty) assert(!row.cpu_percent.value && !row.memory_bytes.value && !row.disk_bytes_per_second.value && !row.network_bytes_per_second.value);
    assert(aggregate_linux_users({}, processes).empty());
    logins.push_back(logins[0]); assert(aggregate_linux_users(logins, processes).size() == 3);
    auto wrong = logins; wrong.back().username = "bob"; invalid([&] { (void)aggregate_linux_users(wrong, processes); });
    wrong = logins; wrong[0].id.clear(); invalid([&] { (void)aggregate_linux_users(wrong, processes); });
    wrong = logins; wrong[0].username = std::string("a\0b", 3); invalid([&] { (void)aggregate_linux_users(wrong, processes); });
    wrong = logins; wrong[0].status = std::string(4097, 'x'); invalid([&] { (void)aggregate_linux_users(wrong, processes); });
    auto duplicates = processes; duplicates.push_back(a); invalid([&] { (void)aggregate_linux_users(logins, duplicates); });
    duplicates = processes; duplicates[0].pid = 0; invalid([&] { (void)aggregate_linux_users(logins, duplicates); });
    for (const auto bad : {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN(), -1.0, 101.0}) {
        auto values = processes; values[0].cpu_percent = value(bad);
        assert(!aggregate_linux_users(logins, values)[0].cpu_percent.value);
    }
    auto values = processes;
    values[0].cpu_percent = value(60.0); values[1].cpu_percent = value(50.0);
    assert(!aggregate_linux_users(logins, values)[0].cpu_percent.value); // don't clamp >100
    values = processes;
    values[0].memory_bytes = value(std::numeric_limits<std::uint64_t>::max());
    assert(!aggregate_linux_users(logins, values)[0].memory_bytes.value);
    values = processes; values[0].memory_bytes.support = Support::unsupported;
    assert(aggregate_linux_users(logins, values)[0].memory_bytes.value == 30); // unsupported value not trusted
    bool limited = false;
    try { std::vector<LinuxLoginRecord> huge(10001); (void)aggregate_linux_users(huge, {}); }
    catch (const std::length_error&) { limited = true; } assert(limited);
#ifdef __linux__
    const auto* user = getpwuid(getuid()); assert(user);
    const std::string username = user->pw_name;
    {
        PrivateUtmp fixture(username);
        const auto records = read_linux_logins(); assert(records.size() == 3);
        Monitor monitor;
        SampleOptions options;
        options.include_user_sessions = true; options.include_processes = false;
        options.prefer_logind_sessions = false;
        options.include_gpus = options.include_disks = options.include_networks = false;
        const auto first = monitor.sample(options);
        assert(first.processes.empty() && first.user_sessions.size() == 3);
        assert(first.linux_session_inventory_source.value=="utmp");
        for(const auto& row:first.user_sessions){
            assert(row.linux_details.value&&row.linux_details.value->provider=="utmp");
            assert(row.linux_details.value->state.value=="reported_login");
            assert(!row.linux_details.value->active.value&&!row.linux_details.value->remote.value&&!row.linux_details.value->unix_uid.value);
        }
        assert(first.user_sessions[0].memory_bytes.value && *first.user_sessions[0].memory_bytes.value > 0);
        assert(!first.user_sessions[0].cpu_percent.value); // first sample, NOT measured zero
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        const auto second = monitor.sample(options);
        assert(second.processes.empty() && second.user_sessions.size() == 3);
        assert(second.user_sessions[0].cpu_percent.value && second.user_sessions[1].cpu_percent.value == second.user_sessions[0].cpu_percent.value);
        assert(second.user_sessions[0].memory_bytes.value == second.user_sessions[1].memory_bytes.value);
        for (const auto& row : second.user_sessions) assert(row.status == "reported_login" && !row.network_bytes_per_second.value);
        assert(!second.user_sessions[2].memory_bytes.value);
        {std::ofstream empty(fixture.dir/"utmp",std::ios::binary|std::ios::trunc);assert(empty);}
        const auto empty=read_linux_login_records();assert(empty.value&&empty.value->empty());
        const auto empty_snapshot=monitor.sample(options);assert(empty_snapshot.user_sessions.empty()&&empty_snapshot.linux_session_inventory_source.value=="utmp");
        std::filesystem::remove(fixture.dir/"utmp");
        const auto missing=read_linux_login_records();assert(!missing.value);
        const auto missing_snapshot=monitor.sample(options);assert(missing_snapshot.user_sessions.empty()&&!missing_snapshot.linux_session_inventory_source.value);
    }
#endif
}
