#include "linux_efficiency.hpp"
#include <algorithm>
#include <cerrno>
#include <limits>
#ifdef __linux__
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
constexpr int idle = 3 << 13;
constexpr int normal_io = (2 << 13) | 4;
ActionResult failure(int code, const char* stage) {
    auto kind = ActionError::system_error;
    if (code == EPERM || code == EACCES) kind = ActionError::permission_denied;
    else if (code == ESRCH) kind = ActionError::not_found;
    else if (code == ENOSYS || code == EOPNOTSUPP) kind = ActionError::not_supported;
    else if (code == EINVAL) kind = ActionError::invalid_argument;
    return {kind, std::string(stage) + "; errno " + std::to_string(code)};
}
bool valid_io(int value) {
    if (value < 0 || value > 0xffff) return false;
    const auto cls = value >> 13, data = value & 0x1fff;
    return (cls == 0 && data == 0) || ((cls == 1 || cls == 2) && data <= 7) ||
           (cls == 3 && data <= 7); // older kernels may preserve idle data bits
}
}
ActionResult set_linux_efficiency(std::uint32_t pid, bool enabled, const LinuxEfficiencyApi& api) {
    if (pid < 2 || pid > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) || pid == api.self_pid)
        return {ActionError::invalid_argument, "PID reservado/self/fuera de rango"};
    if (!api.read_nice || !api.read_io || !api.set_nice || !api.set_io)
        return {ActionError::not_supported, "API de prioridades incompleta; sin cambios"};
    const auto nice = api.read_nice(pid);
    if (nice.error) return failure(nice.error, "no se leyó nice; sin cambios");
    const auto io = api.read_io(pid);
    if (io.error) return failure(io.error, "no se leyó prioridad E/S; sin cambios");
    if (nice.value < -20 || nice.value > 19 || !valid_io(io.value))
        return {ActionError::system_error, "prioridad nativa inválida; sin cambios"};
    const auto target_nice = enabled ? std::max(nice.value, 10) : 0;
    const auto target_io = enabled ? idle : normal_io;
    bool io_changed = false;
    if (io.value != target_io) {
        const auto error = api.set_io(pid, target_io);
        if (error) return failure(error, "falló cambio E/S; no se solicitó cambio CPU");
        io_changed = true;
    }
    if (nice.value != target_nice) {
        const auto error = api.set_nice(pid, target_nice);
        if (error) return failure(error, io_changed ?
            "E/S modificada; falló cambio CPU; resultado parcial, sin rollback" :
            "falló cambio CPU; sin cambio E/S en esta llamada");
    }
    const auto final_nice = api.read_nice(pid), final_io = api.read_io(pid);
    if (final_nice.error || final_io.error)
        return failure(final_nice.error ? final_nice.error : final_io.error,
            "cambios solicitados; verificación incompleta, estado final desconocido");
    const auto io_matches = enabled ? ((final_io.value >> 13) == 3 && valid_io(final_io.value)) :
                                      final_io.value == target_io;
    if (final_nice.value != target_nice || !io_matches)
        return {ActionError::system_error, "cambios solicitados; prioridades finales no coinciden, sin rollback"};
    return {ActionError::none, enabled ?
        "preset del hilo principal verificado: nice >= 10 y E/S idle; no garantiza ahorro energético" :
        "preset del hilo principal verificado: nice 0 y E/S best-effort 4; no restaura valores anteriores"};
}
#ifdef __linux__
LinuxEfficiencyApi native_linux_efficiency_api() {
    LinuxEfficiencyApi api;
    api.self_pid = static_cast<std::uint32_t>(getpid());
    api.read_nice = [](std::uint32_t target) {
        errno = 0;
        const auto value = getpriority(PRIO_PROCESS, static_cast<id_t>(target));
        return LinuxPriorityResult{value, errno}; // -1 is a valid nice value
    };
    api.set_nice = [](std::uint32_t target, int value) {
        return setpriority(PRIO_PROCESS, static_cast<id_t>(target), value) < 0 ? errno : 0;
    };
    api.read_io = [](std::uint32_t target) -> LinuxPriorityResult {
#ifdef SYS_ioprio_get
        const auto value = syscall(SYS_ioprio_get, 1, static_cast<int>(target));
        return value < 0 ? LinuxPriorityResult{0, errno} : LinuxPriorityResult{static_cast<int>(value), 0};
#else
        (void)target; return {0, ENOSYS};
#endif
    };
    api.set_io = [](std::uint32_t target, int value) {
#ifdef SYS_ioprio_set
        return syscall(SYS_ioprio_set, 1, static_cast<int>(target), value) < 0 ? errno : 0;
#else
        (void)target; (void)value; return ENOSYS;
#endif
    };
    return api;
}
ActionResult set_linux_efficiency(std::uint32_t pid, bool enabled) {
    return set_linux_efficiency(pid, enabled, native_linux_efficiency_api());
}
#else
LinuxEfficiencyApi native_linux_efficiency_api() { return {}; }
ActionResult set_linux_efficiency(std::uint32_t, bool) {
    return {ActionError::not_supported, "Linux requerido"};
}
#endif
}
