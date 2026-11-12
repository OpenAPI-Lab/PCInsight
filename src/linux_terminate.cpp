#include "linux_terminate.hpp"
#include "linux_process_stat.hpp"
#include <cerrno>
#include <limits>
#ifdef __linux__
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <string>
#include <sys/syscall.h>
#include <sys/vfs.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
ActionResult failure(int code) {
    auto kind = ActionError::system_error;
    if (code == EPERM || code == EACCES) kind = ActionError::permission_denied;
    else if (code == ESRCH || code == ENOENT) kind = ActionError::not_found;
    else if (code == ENOSYS || code == ENODEV || code == EOPNOTSUPP) kind = ActionError::not_supported;
    else if (code == EINVAL) kind = ActionError::invalid_argument;
    return {kind, "terminación Linux: errno " + std::to_string(code)};
}
struct Guard {
    int fd;
    const LinuxTerminateApi& api;
    ~Guard() { api.close(fd); }
};
}
ActionResult terminate_linux_process(std::uint32_t pid, std::optional<std::uint64_t> expected,
                                    bool force, const LinuxTerminateApi& api) {
    if (pid < 2 || pid > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        pid == api.self_pid || (expected && !*expected))
        return {ActionError::invalid_argument, "PID reservado/self/fuera de rango o identidad cero"};
    if (!api.open || !api.signal || !api.close || !api.wait_exit ||
        (expected && (!api.identity || !api.check_proc_context)))
        return {ActionError::not_supported, "API de terminación incompleta; no se envió señal"};
    if (expected) {
        const auto error = api.check_proc_context();
        if (error) return failure(error);
    }
    const auto opened = api.open(pid);
    if (opened.error) return failure(opened.error);
    if (opened.fd < 0) return {ActionError::system_error, "pidfd inválido"};
    Guard guard{opened.fd, api};
    if (expected) {
        const auto identity = api.identity(pid);
        if (identity.error) return failure(identity.error);
        if (identity.pid != pid || identity.start_ticks != *expected)
            return {ActionError::stale_target, "identidad de proceso cambió; no se envió señal"};
    }
    // Avoid signaling a stale pidfd whose original process exited while /proc was read.
    const auto before = api.wait_exit(opened.fd, 0);
    if (before.error) return failure(before.error);
    if (before.exited) return {ActionError::not_found, "proceso ya terminado; no se envió señal"};
    const auto error = api.signal(opened.fd, force);
    if (error) return failure(error);
    const auto after = api.wait_exit(opened.fd, 5000);
    if (after.error) return {failure(after.error).error,
        "señal aceptada; falló la confirmación de salida, errno " + std::to_string(after.error)};
    if (!after.exited) return {ActionError::system_error,
        "señal aceptada; salida no confirmada en 5s, sin rollback ni escalada automática"};
    return {ActionError::none, "salida del proceso confirmada por pidfd; el padre conserva la recolección"};
}
#ifdef __linux__
namespace {
struct NativeFd { int fd{-1}; ~NativeFd() { if (fd >= 0) ::close(fd); } };
LinuxIdentityResult read_identity(const std::string& path) {
    NativeFd file{::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW)};
    if (file.fd < 0) return {0, 0, errno};
    std::string text;
    char buffer[4096];
    while (true) {
        const auto count = read(file.fd, buffer, sizeof buffer);
        if (count < 0) { if (errno == EINTR) continue; return {0, 0, errno}; }
        if (!count) break;
        if (static_cast<std::size_t>(count) > 65536 - text.size()) return {0, 0, EOVERFLOW};
        text.append(buffer, static_cast<std::size_t>(count));
    }
    const auto parsed = parse_linux_process_stat(text);
    if (!parsed) return {0, 0, EIO};
    return {parsed->pid, parsed->start_ticks, 0};
}
LinuxExitResult wait_exit(int fd, unsigned ms) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    while (true) {
        const auto now = std::chrono::steady_clock::now();
        const auto left = now < deadline ?
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count() + 1 : 0;
        pollfd item{fd, POLLIN, 0};
        const auto result = poll(&item, 1, static_cast<int>(left));
        if (result < 0) {
            if (errno == EINTR && std::chrono::steady_clock::now() < deadline) continue;
            return {false, errno};
        }
        if (!result) return {false, 0};
        if (item.revents & POLLNVAL) return {false, EBADF};
        if (item.revents & POLLERR) return {false, EIO};
        if (item.revents & (POLLIN | POLLHUP)) return {true, 0};
        return {false, EIO};
    }
}
}
ActionResult terminate_linux_process(std::uint32_t pid, std::optional<std::uint64_t> expected, bool force) {
    LinuxTerminateApi api;
    api.self_pid = static_cast<std::uint32_t>(getpid());
    api.open = [](std::uint32_t target) -> LinuxPidfdResult {
#ifdef SYS_pidfd_open
        const auto fd = syscall(SYS_pidfd_open, static_cast<pid_t>(target), 0);
        return fd < 0 ? LinuxPidfdResult{-1, errno} : LinuxPidfdResult{static_cast<int>(fd), 0};
#else
        (void)target; return {-1, ENOSYS};
#endif
    };
    api.check_proc_context = [self = api.self_pid] {
        struct statfs info{};
        if (statfs("/proc", &info) != 0) return errno;
        if (info.f_type != 0x9fa0) return EOPNOTSUPP; // Linux procfs, not an arbitrary directory
        const auto identity = read_identity("/proc/self/stat");
        if (identity.error) return identity.error;
        return identity.pid == self ? 0 : EOPNOTSUPP;
    };
    api.identity = [](std::uint32_t target) {
        return read_identity("/proc/" + std::to_string(target) + "/stat");
    };
    api.signal = [](int fd, bool kill) {
#ifdef SYS_pidfd_send_signal
        return syscall(SYS_pidfd_send_signal, fd, kill ? SIGKILL : SIGTERM, nullptr, 0) < 0 ? errno : 0;
#else
        (void)fd; (void)kill; return ENOSYS;
#endif
    };
    api.wait_exit = wait_exit;
    api.close = [](int fd) { ::close(fd); };
    return terminate_linux_process(pid, expected, force, api);
}
#else
ActionResult terminate_linux_process(std::uint32_t, std::optional<std::uint64_t>, bool) {
    return {ActionError::not_supported, "Linux requerido"};
}
#endif
}
