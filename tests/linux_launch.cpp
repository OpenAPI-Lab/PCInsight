#include "pcinsight/actions.hpp"
#include "pcinsight/audit.hpp"
#include <cassert>
#ifdef __linux__
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
namespace fs = std::filesystem;
using namespace pcinsight;
namespace {
struct Temp {
    fs::path path;
    Temp() {
        char name[] = "/tmp/pcinsight-launch-XXXXXX";
        auto* p = mkdtemp(name); assert(p); path = p;
    }
    ~Temp() { std::error_code ec; fs::remove_all(path, ec); }
};
struct PathGuard {
    std::optional<std::string> old;
    PathGuard() { if (const auto* p = getenv("PATH")) old = p; }
    ~PathGuard() { if (old) setenv("PATH", old->c_str(), 1); else unsetenv("PATH"); }
};
void reap(const ActionResult& r, int expected = 0) {
    assert(r && r.process_id > 1);
    int status = 0;
    pid_t result;
    do { result = waitpid(static_cast<pid_t>(r.process_id), &status, 0); }
    while (result < 0 && errno == EINTR);
    assert(result == static_cast<pid_t>(r.process_id));
    assert(WIFEXITED(status) && WEXITSTATUS(status) == expected);
}
void rejected(SystemActions& actions, const LaunchRequest& r, ActionError error) {
    const auto result = actions.launch_process(r);
    assert(!result && result.error == error && result.process_id == 0);
}
std::string read(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}
}
int main(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[1]) == "--fixture") {
        std::ofstream output(argv[2], std::ios::binary);
        if (!output) return 8;
        output << fs::current_path().string() << '\n';
        for (int i = 3; i < argc; ++i) {
            const std::string arg = argv[i];
            output << arg.size() << ':' << arg << '\n';
        }
        return output ? 0 : 9;
    }
    if (argc == 2 && std::string(argv[1]) == "--exit7") return 7;
    Temp temp;
    auto actions = make_system_actions();
    const auto self = fs::read_symlink("/proc/self/exe").string();
    const auto original_cwd = fs::current_path();
    const auto out = temp.path / "args";
    std::vector<std::string> literal{"", "two words", "quote\"slash\\", "$(touch NO)",
        "a; touch NO", "$HOME", "*", "line\nbreak", "ñ"};
    LaunchRequest request{self, {"--fixture", out.string()}, temp.path.string(), false};
    request.arguments.insert(request.arguments.end(), literal.begin(), literal.end());
    unsigned events = 0;
    AuditedActions audited(make_system_actions(), [&](const AuditEvent&) { ++events; });
    reap(audited.launch_process(request));
    assert(events == 2); // decorator preserves the child's structured PID
    std::string expected = temp.path.string() + "\n";
    for (const auto& arg : literal) expected += std::to_string(arg.size()) + ":" + arg + "\n";
    assert(read(out) == expected);
    assert(fs::current_path() == original_cwd && !fs::exists(temp.path / "NO"));
    // Relative executable is resolved after the requested cwd, never on parent.
    fs::create_symlink(self, temp.path / "own fixture");
    request.executable = "./own fixture";
    reap(actions->launch_process(request));
    PathGuard path;
    assert(setenv("PATH", temp.path.c_str(), 1) == 0);
    request.executable = "own fixture";
    reap(actions->launch_process(request));
    assert(setenv("PATH", "", 1) == 0);
    reap(actions->launch_process(request)); // empty PATH means child cwd
    assert(unsetenv("PATH") == 0);
    rejected(*actions, request, ActionError::not_found); // _CS_PATH, no guessed cwd

    request = {self, {"--exit7"}, "", false};
    reap(actions->launch_process(request), 7); // spawn success != application's success
    rejected(*actions, {"", {}, "", false}, ActionError::invalid_argument);
    rejected(*actions, {std::string("bad\0path", 8), {}, "", false}, ActionError::invalid_argument);
    rejected(*actions, {self, {std::string("a\0b", 3)}, "", false}, ActionError::invalid_argument);
    rejected(*actions, {self, {}, std::string("a\0b", 3), false}, ActionError::invalid_argument);
    rejected(*actions, {self, {std::string(128 * 1024, 'x')}, "", false}, ActionError::invalid_argument);
    rejected(*actions, {self, std::vector<std::string>(4097), "", false}, ActionError::invalid_argument);
    rejected(*actions, {self, std::vector<std::string>(16, std::string(65536, 'x')), "", false}, ActionError::invalid_argument);
    for (int i = 0; i < 32; ++i)
        rejected(*actions, {(temp.path / "missing").string(), {}, "", false}, ActionError::not_found);
    rejected(*actions, {self, {}, (temp.path / "missing").string(), false}, ActionError::not_found);
    rejected(*actions, {self, {}, out.string(), false}, ActionError::not_found);
    const auto text = temp.path / "not an executable";
    { std::ofstream file(text); file << "touch NO\n"; }
    assert(chmod(text.c_str(), 0600) == 0);
    rejected(*actions, {text.string(), {}, temp.path.string(), false}, ActionError::permission_denied);
    assert(chmod(text.c_str(), 0700) == 0);
    rejected(*actions, {text.string(), {}, temp.path.string(), false}, ActionError::invalid_argument);
    assert(!fs::exists(temp.path / "NO")); // no shell fallback on executable text
    if (geteuid() != 0) rejected(*actions, {self, {}, "", true}, ActionError::permission_denied);

    // EACCES in an earlier PATH candidate doesn't mask a later executable.
    const auto denied = temp.path / "denied";
    fs::create_directory(denied);
    { std::ofstream file(denied / "own fixture"); file << "no"; }
    assert(chmod((denied / "own fixture").c_str(), 0600) == 0);
    const auto search = denied.string() + ":" + temp.path.string();
    assert(setenv("PATH", search.c_str(), 1) == 0);
    reap(actions->launch_process({"own fixture", {"--exit7"}, "", false}), 7);
    assert(setenv("PATH", denied.c_str(), 1) == 0);
    rejected(*actions, {"own fixture", {}, "", false}, ActionError::permission_denied);
    // Multiple caller threads, no child-side C++ allocator or global cwd changes.
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) workers.emplace_back([&, i] {
        const auto file = temp.path / ("parallel-" + std::to_string(i));
        reap(actions->launch_process({self, {"--fixture", file.string(), std::to_string(i)}, temp.path.string(), false}));
        assert(read(file) == temp.path.string() + "\n1:" + std::to_string(i) + "\n");
    });
    for (auto& worker : workers) worker.join();
    assert(fs::current_path() == original_cwd);
    int status = 0;
    assert(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD);
}
#else
int main() {
    // Linux backend intentionally not exercised by Windows.
    return 0;
}
#endif
