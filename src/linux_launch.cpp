#include "linux_launch.hpp"
#ifdef __linux__
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
extern char** environ;
namespace pcinsight::detail {
namespace {
constexpr std::size_t text_limit = 128 * 1024 - 1;
constexpr std::size_t request_limit = 1024 * 1024;
constexpr std::size_t environment_limit = 2 * 1024 * 1024;
bool text_ok(const std::string& s) {
    return s.size() <= text_limit && s.find('\0') == std::string::npos;
}
ActionResult failure(int code) {
    auto kind = ActionError::system_error;
    if (code == EACCES || code == EPERM) kind = ActionError::permission_denied;
    else if (code == ENOENT || code == ENOTDIR) kind = ActionError::not_found;
    else if (code == EINVAL || code == E2BIG || code == ENAMETOOLONG || code == ENOEXEC)
        kind = ActionError::invalid_argument;
    return {kind, "lanzamiento Linux: errno " + std::to_string(code)};
}
struct Fd {
    int value{-1};
    ~Fd() { if (value >= 0) close(value); }
};
}
ActionResult launch_linux_process(const LaunchRequest& r) {
    auto invalid = [] { return ActionResult{ActionError::invalid_argument,
        "executable/cwd/argumentos inválidos o límites de lanzamiento excedidos"}; };
    if (r.executable.empty() || !text_ok(r.executable) || !text_ok(r.working_directory) ||
        r.arguments.size() > 4096) return invalid();
    std::size_t bytes = r.executable.size() + r.working_directory.size() + 2;
    for (const auto& s : r.arguments) {
        if (!text_ok(s) || s.size() + 1 > request_limit - bytes) return invalid();
        bytes += s.size() + 1;
    }
    if (r.elevated && geteuid() != 0) return {ActionError::permission_denied,
        "elevación requiere un helper autorizado; no se inicia sin ella"};
    // Snapshot before spawning. Host must serialize external setenv/unsetenv.
    std::vector<std::string> env_storage;
    std::string path;
    bool has_path = false;
    std::size_t env_bytes = 0;
    if (environ) for (char** p = environ; *p; ++p) {
        const auto length = strnlen(*p, text_limit + 1);
        if (length > text_limit || length + 1 > environment_limit - env_bytes ||
            env_storage.size() >= 32768) return invalid();
        env_bytes += length + 1;
        env_storage.emplace_back(*p, length);
        if (env_storage.back().starts_with("PATH=")) {
            if (has_path) return invalid();
            has_path = true; path = env_storage.back().substr(5);
        }
    }
    if (!has_path && r.executable.find('/') == std::string::npos) {
        const auto size = confstr(_CS_PATH, nullptr, 0);
        if (!size || size > text_limit) return {ActionError::not_supported,
            "PATH ausente y ruta predeterminada del sistema no disponible"};
        std::vector<char> buffer(size);
        if (confstr(_CS_PATH, buffer.data(), buffer.size()) != size)
            return {ActionError::system_error, "ruta predeterminada cambió durante la consulta"};
        path.assign(buffer.data());
    }
    std::vector<std::string> candidates;
    if (r.executable.find('/') != std::string::npos) candidates.push_back(r.executable);
    else {
        std::size_t begin = 0, candidate_bytes = 0;
        do {
            const auto end = path.find(':', begin);
            auto dir = path.substr(begin, end == std::string::npos ? end : end - begin);
            auto candidate = dir.empty() ? r.executable : dir + "/" + r.executable;
            if (candidate.size() > text_limit || candidates.size() >= 4096 ||
                candidate.size() + 1 > request_limit - candidate_bytes) return invalid();
            candidate_bytes += candidate.size() + 1;
            candidates.push_back(std::move(candidate));
            if (end == std::string::npos) break;
            begin = end + 1;
        } while (true);
    }
    std::vector<std::string> storage{r.executable};
    storage.insert(storage.end(), r.arguments.begin(), r.arguments.end());
    std::vector<char*> argv, env;
    for (auto& s : storage) argv.push_back(s.data());
    argv.push_back(nullptr);
    for (auto& s : env_storage) env.push_back(s.data());
    env.push_back(nullptr);
    std::vector<const char*> paths;
    for (const auto& s : candidates) paths.push_back(s.c_str());
    // Everything the child needs is prepared before fork. No C++ allocation,
    // stream, environment lookup, or mutex in the child's pre-exec path.
    const char* cwd = r.working_directory.empty() ? nullptr : r.working_directory.c_str();
    const auto path_count = paths.size();
    const char* const* path_data = paths.data();
    char* const* arg_data = argv.data();
    char* const* env_data = env.data();
    int pipe_fds[2];
    if (pipe2(pipe_fds, O_CLOEXEC) != 0) return failure(errno);
    Fd input{pipe_fds[0]}, output{pipe_fds[1]};
    const auto pid = fork();
    if (pid < 0) return failure(errno);
    if (pid == 0) {
        close(input.value);
        int error = 0;
        if (cwd && chdir(cwd) != 0) error = errno;
        if (!error) {
            bool denied = false;
            for (std::size_t i = 0; i < path_count; ++i) {
                // No execvp: executable text must NOT fall back to /bin/sh.
                execve(path_data[i], arg_data, env_data);
                error = errno;
                if (error == EACCES) denied = true;
                else if (error != ENOENT && error != ENOTDIR) break;
                if (i + 1 == path_count) error = denied ? EACCES : ENOENT;
            }
        }
        const char* data = reinterpret_cast<const char*>(&error);
        std::size_t sent = 0;
        while (sent < sizeof error) {
            const auto count = write(output.value, data + sent, sizeof error - sent);
            if (count > 0) sent += static_cast<std::size_t>(count);
            else if (count < 0 && errno == EINTR) continue;
            else break;
        }
        _exit(127);
    }
    close(output.value); output.value = -1;
    int error = 0;
    std::size_t received = 0;
    auto* data = reinterpret_cast<char*>(&error);
    while (received < sizeof error) {
        const auto count = read(input.value, data + received, sizeof error - received);
        if (count > 0) received += static_cast<std::size_t>(count);
        else if (count == 0) break;
        else if (errno != EINTR) return {ActionError::system_error,
            "falló el canal de lanzamiento; estado del hijo desconocido", static_cast<std::uint32_t>(pid)};
    }
    if (!received) return {ActionError::none,
        "proceso creado con PID " + std::to_string(pid) +
        "; sin error pre-exec comunicado, inicialización no verificada", static_cast<std::uint32_t>(pid)};
    // Child reported an error and will _exit; reap only OUR failed child.
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    if (received != sizeof error || !error) return {ActionError::system_error,
        "respuesta incompleta del canal de lanzamiento"};
    return failure(error);
}
}
#else
namespace pcinsight::detail {
ActionResult launch_linux_process(const LaunchRequest&) {
    return {ActionError::not_supported, "Linux requerido"};
}
}
#endif
