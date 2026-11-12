#include "../src/linux_logout.hpp"
#include "../src/linux_service_actions.hpp"
#include "../src/linux_systemd.hpp"
#include "pcinsight/audit.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <thread>
#ifdef __linux__
#include <csignal>
#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
#include <poll.h>
#include <pwd.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
#ifdef __linux__
namespace fs = std::filesystem;
void require(bool condition, const std::string& reason) { if (!condition) throw std::runtime_error(reason); }
struct PamHandle; struct PamMessage; struct PamResponse;
struct PamConv { int (*callback)(int, const PamMessage**, PamResponse**, void*); void* data; };
int no_conversation(int, const PamMessage**, PamResponse**, void*) { return 19; }
int pam_fixture(const std::string& directory, const std::string& service, const std::string& username) {
    require(geteuid() == 0, "PAM fixture requires explicit root context");
    auto* library = dlopen("libpam.so.0", RTLD_NOW | RTLD_LOCAL);
    require(library, "libpam unavailable");
    auto start = reinterpret_cast<int(*)(const char*, const char*, const PamConv*, const char*, PamHandle**)>(dlsym(library, "pam_start_confdir"));
    auto open = reinterpret_cast<int(*)(PamHandle*, int)>(dlsym(library, "pam_open_session"));
    auto close = reinterpret_cast<int(*)(PamHandle*, int)>(dlsym(library, "pam_close_session"));
    auto end = reinterpret_cast<int(*)(PamHandle*, int)>(dlsym(library, "pam_end"));
    auto environment = reinterpret_cast<const char*(*)(PamHandle*, const char*)>(dlsym(library, "pam_getenv"));
    auto item = reinterpret_cast<int(*)(PamHandle*, int, const void*)>(dlsym(library, "pam_set_item"));
    require(start && open && close && end && environment && item, "private PAM API unavailable");
    PamConv conversation{no_conversation, nullptr}; PamHandle* handle{};
    require(start(service.c_str(), username.c_str(), &conversation, directory.c_str(), &handle) == 0, "private pam_start failed");
    std::ofstream ready(fs::path(directory) / "ready"); require(bool(ready), "ready file unavailable");
    require(item(handle, 3, service.c_str()) == 0, "private PAM_TTY failed");
    const auto rc = open(handle, 0x8000); // PAM_SILENT; session only, no authentication/config edits
    if (rc) { end(handle, rc); throw std::runtime_error("private pam_open_session failed: " + std::to_string(rc)); }
    const auto* id = environment(handle, "XDG_SESSION_ID");
    if (!id || !*id) { close(handle, 0x8000); end(handle, 0); throw std::runtime_error("PAM supplied no session identity"); }
    const std::string session_id = id;
    const auto* user = getpwnam(username.c_str()); require(user, "fixture user disappeared");
    require(setgid(user->pw_gid) == 0 && setuid(user->pw_uid) == 0, "fixture privilege drop failed");
    ready << session_id << '\n' << getpid() << '\n'; ready.close();
    for (;;) pause(); // logind's explicit Terminate is expected to kill THIS fixture
}
struct Fixture {
    SystemdActionApi api{load_systemd_action_api()};
    ActionBus* bus{};
    fs::path directory;
    std::string name, unit, token;
    bool owned{}, referenced{};
    int pidfd{-1};
    Fixture() {
        require(geteuid() == 0, "native fixture must be explicitly run as root");
        require(api && api.open(&bus) >= 0 && bus, "fixture system bus unavailable");
        require(api.timeout(bus, 5000000) >= 0, "fixture timeout unavailable");
        char folder[] = "/tmp/pcinsight-logout-XXXXXX";
        auto* dir = mkdtemp(folder); require(dir, "private PAM directory unavailable"); directory=dir;
        name = "pcinsight-logout-" + std::to_string(getpid()) + "-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        unit = name + ".service";
        // Only this private file; no /etc/pam.d change and no pam_exec/external command.
        std::ofstream config(directory / name);
        config << "session required pam_systemd.so\n"; config.close(); require(bool(config), "PAM config write failed");
    }
    template<class... Args> void invoke(const char* method, const char* signature, Args... args) {
        ActionMessage* reply{}; ActionBusError error{};
        const auto rc = api.call(bus, "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
            "org.freedesktop.systemd1.Manager", method, &error, &reply, signature, args...);
        const std::string reason = error.message ? error.message : "";
        if (reply) api.message_unref(reply); api.error_free(&error);
        require(rc >= 0, std::string(method) + ": " + reason + " / " + std::to_string(rc));
    }
    ServiceInfo inspect() {
        const auto rows = read_native_systemd_services(); require(bool(rows), "fixture service inventory unavailable");
        for (const auto& row : *rows) if (row.id == unit) {
            require(row.description == "PCInsight private PAM logout fixture " + name, "owned service description mismatch");
            return row;
        }
        throw std::runtime_error("owned fixture unit not found");
    }
    void create(const std::string& username) {
        const auto executable = fs::read_symlink("/proc/self/exe").string();
        const auto dir = directory.string();
        invoke("StartTransientUnit", "ssa(sv)a(sa(sv))", unit.c_str(), "fail", 3,
            "Description", "s", ("PCInsight private PAM logout fixture " + name).c_str(),
            "ExecStart", "a(sasb)", 1, executable.c_str(), 5,
                executable.c_str(), "--pam-fixture", dir.c_str(), name.c_str(), username.c_str(), 0,
            "Type", "s", "simple", 0);
        owned=true; invoke("RefUnit", "s", unit.c_str()); referenced=true;
        std::cout << "Owned PAM fixture unit: " << unit << std::endl;
    }
    void cleanup() {
        if (pidfd >= 0) {
#ifdef SYS_pidfd_send_signal
            syscall(SYS_pidfd_send_signal, pidfd, SIGKILL, nullptr, 0); // owned fixture only; stable reference
#endif
            ::close(pidfd); pidfd=-1;
        }
        if (owned) {
            (void)inspect(); // don't operate on a changed/unrelated unit
            const auto result=make_system_actions()->control_service(unit, ServiceAction::stop);
            require(bool(result), "own fixture cleanup: " + result.message);
            invoke("ResetFailedUnit", "s", unit.c_str());
            if (referenced) { invoke("UnrefUnit", "s", unit.c_str()); referenced=false; }
            owned=false;
        }
        if (!directory.empty()) { fs::remove_all(directory); directory.clear(); }
    }
    ~Fixture() {
        try { cleanup(); } catch (const std::exception& e) { std::cerr << "Owned fixture cleanup pending: " << unit << " / " << e.what() << '\n'; }
        if (bus) api.unref(bus);
    }
};
void verify_properties(Fixture& fixture, const LogoutToken& identity, unsigned leader) {
    using Number=int(*)(ActionBus*, const char*, const char*, const char*, const char*, ActionBusError*, char, void*);
    using String=int(*)(ActionBus*, const char*, const char*, const char*, const char*, ActionBusError*, char**);
    const auto number = reinterpret_cast<Number>(dlsym(fixture.api.library.get(), "sd_bus_get_property_trivial"));
    const auto string = reinterpret_cast<String>(dlsym(fixture.api.library.get(), "sd_bus_get_property_string"));
    require(number && string, "fixture property API unavailable");
    unsigned actual{};
    require(number(fixture.bus, identity.owner.c_str(), identity.path.c_str(), "org.freedesktop.login1.Session",
        "Leader", nullptr, 'u', &actual) >= 0 && actual == leader, "session leader is not our fixture; refusing logout");
    char* service{};
    const auto rc=string(fixture.bus, identity.owner.c_str(), identity.path.c_str(), "org.freedesktop.login1.Session",
        "Service", nullptr, &service);
    const auto matches=rc >= 0 && service && std::string(service) == fixture.name;
    free(service); require(matches, "session service is not our private PAM service; refusing logout");
}
void native_fixture() {
    const auto before=read_logind_logins(); require(bool(before), "logind is not available");
    std::set<std::string> baseline;
    for (const auto& row : *before) baseline.insert(row.id);
    const auto* pw=getpwuid(1000); require(pw, "fixture requires existing Linux UID1000; no account is created");
    const std::string username=pw->pw_name;
    Fixture fixture; fixture.create(username);
    unsigned leader{};
    std::string session_id;
    for (unsigned attempt=0; attempt<100; ++attempt) {
        const auto info=fixture.inspect();
        if (info.pid && fixture.pidfd < 0) {
#ifdef SYS_pidfd_open
            fixture.pidfd=static_cast<int>(syscall(SYS_pidfd_open, *info.pid, 0));
#endif
            require(fixture.pidfd >= 0, "cannot pin owned fixture process"); leader=*info.pid;
        }
        std::ifstream ready(fixture.directory / "ready");
        unsigned reported{};
        if (ready >> session_id >> reported) { require(reported==leader, "ready PID doesn't match owned unit"); break; }
        require(info.active_state != "failed", "private PAM fixture failed before ready");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    require(!session_id.empty() && leader, "private PAM readiness timeout");
    require(!baseline.contains("login1:" + session_id), "PAM reused a pre-existing session; refusing logout");
    const auto current=read_logind_logins(); require(bool(current), "login1 inventory disappeared");
    for (const auto& row : *current) if (row.id == "login1:" + session_id) fixture.token=row.logout_token;
    const auto identity=parse_logout_token(fixture.token); require(bool(identity), "owned session has no verified logout token");
    verify_properties(fixture, *identity, leader);
    pollfd alive{fixture.pidfd, POLLIN, 0}; require(poll(&alive, 1, 0)==0, "fixture already exited before logout");
    auto wrong=*identity; ++wrong.timestamp;
    require(make_system_actions()->terminate_user_session(logout_token(wrong)).error==ActionError::stale_target,
        "native stale identity was not rejected");
    require(poll(&alive, 1, 0)==0, "stale request altered owned fixture");
    std::vector<AuditEvent> events;
    AuditedActions actions(make_system_actions(), [&](const AuditEvent& e) { events.push_back(e); });
    const auto result=actions.terminate_user_session(fixture.token);
    require(bool(result), "owned logout failed: " + result.message);
    require(events.size()==2 && events[1].result==ActionError::none, "native logout audit missing");
    pollfd exited{fixture.pidfd, POLLIN, 0};
    require(poll(&exited, 1, 5000)>0 && (exited.revents & (POLLIN|POLLHUP)), "owned session leader did not exit");
    const auto after=read_logind_logins(); require(bool(after), "post-logout inventory unavailable");
    std::set<std::string> remaining;
    for (const auto& row : *after) remaining.insert(row.id);
    require(!remaining.contains("login1:" + session_id), "owned session still listed");
    for (const auto& id : baseline) require(remaining.contains(id), "pre-existing session changed during test");
    std::cout << "Native logout confirmed for owned session " << session_id << "; leader exited; baseline sessions retained\n";
    const auto temporary=fixture.directory;
    fixture.cleanup();
    require(!fs::exists(temporary), "private PAM files remain");
    bool absent=false;
    for (unsigned attempt=0; attempt<20; ++attempt) {
        const auto services=read_native_systemd_services(); require(bool(services), "cleanup inventory unavailable");
        absent=true;
        for (const auto& row : *services) if (row.id==fixture.unit) absent=false;
        if (absent) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    require(absent, "owned transient service still registered after cleanup");
    std::cout << "Owned transient service absent; private PAM files removed\n";
}
#endif
int main(int argc, char** argv) {
    try {
#ifdef __linux__
        if (argc==5 && std::string(argv[1])=="--pam-fixture") return pam_fixture(argv[2], argv[3], argv[4]);
        if (argc==2 && std::string(argv[1])=="--native-fixture") { native_fixture(); return 0; }
#else
        (void)argc; (void)argv;
#endif
        std::cout << "Explicit --native-fixture required; no mutation performed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
