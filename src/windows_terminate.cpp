#include "windows_terminate.hpp"
#ifdef _WIN32
#include <cstring>
#include <optional>
namespace pcinsight::detail {
IsCriticalFn windows_is_critical() {
    static const auto fn = [] {
        IsCriticalFn value = nullptr;
        const auto module = GetModuleHandleW(L"kernel32.dll");
        if (module) {
            const auto symbol = GetProcAddress(module, "IsProcessCritical");
            static_assert(sizeof value == sizeof symbol);
            std::memcpy(&value, &symbol, sizeof value);
        }
        return value;
    }();
    return fn;
}
namespace {
struct Handle {
    HANDLE value;
    TerminateApi& api;
    Handle(HANDLE v, TerminateApi& a) : value(v), api(a) {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    ~Handle() { if (value) api.close(value); }
};
ActionResult failure(const char* operation, DWORD error) {
    auto kind = ActionError::system_error;
    if (error == ERROR_ACCESS_DENIED || error == ERROR_PRIVILEGE_NOT_HELD)
        kind = ActionError::permission_denied;
    return {kind, std::string(operation) + ": Win32 " + std::to_string(error)};
}
ActionResult terminate_impl(std::uint32_t pid, bool force, TerminateApi api,
                            std::optional<std::uint64_t> expected) {
    if (expected && !*expected)
        return {ActionError::invalid_argument, "fecha de creación esperada no puede ser cero"};
    if (pid <= 4 || pid == api.current_pid())
        return {ActionError::invalid_argument, "se rechaza PID reservado o el propio proceso"};
    if (!force) return {ActionError::not_supported,
        "Windows no ofrece cierre cooperativo universal; no se fuerza sin force=true"};
    if (!api.critical) return {ActionError::not_supported,
        "IsProcessCritical no disponible; se rechaza terminar sin verificar"};
    Handle process(api.open(PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid), api);
    if (!process.value) {
        const auto error = GetLastError();
        if (error == ERROR_INVALID_PARAMETER)
            return {ActionError::not_found, "OpenProcess: PID no encontrado"};
        return failure("OpenProcess", error);
    }
    // Keep the same process handle for all checks and the destructive operation.
    if (expected) {
        FILETIME created{}, exited{}, kernel{}, user{};
        if (!api.times(process.value, &created, &exited, &kernel, &user))
            return failure("GetProcessTimes (verificación de identidad)", GetLastError());
        const auto actual = (std::uint64_t(created.dwHighDateTime) << 32) | created.dwLowDateTime;
        if (actual != *expected)
            return {ActionError::stale_target, "PID reutilizado o identidad distinta; no se solicitó terminación"};
    }
    const auto initial = api.wait(process.value, 0);
    if (initial == WAIT_OBJECT_0) return {ActionError::none, "proceso ya terminado"};
    if (initial == WAIT_FAILED) return failure("WaitForSingleObject", GetLastError());
    if (initial != WAIT_TIMEOUT) return failure("estado de espera inválido", ERROR_INVALID_DATA);
    BOOL critical = TRUE; // Fail closed if the native query fails.
    if (!api.critical(process.value, &critical)) return failure("IsProcessCritical", GetLastError());
    if (critical) return {ActionError::permission_denied, "se rechaza terminar un proceso crítico"};
    if (!api.terminate(process.value, 1)) {
        const auto error = GetLastError();
        // The target can exit between our check and TerminateProcess.
        if (api.wait(process.value, 0) == WAIT_OBJECT_0)
            return {ActionError::none, "proceso terminó concurrentemente"};
        return failure("TerminateProcess", error);
    }
    // TerminateProcess accepts an asynchronous request, not a completed exit.
    const auto final = api.wait(process.value, 5000);
    if (final == WAIT_OBJECT_0) return {ActionError::none, "terminación del proceso confirmada"};
    if (final == WAIT_TIMEOUT) return {ActionError::system_error,
        "terminación solicitada, no confirmada en 5000 ms; no se cancela la petición"};
    if (final == WAIT_FAILED) {
        auto result = failure("WaitForSingleObject", GetLastError());
        result.message = "terminación solicitada, resultado no confirmado. " + result.message;
        return result;
    }
    return failure("terminación solicitada, estado de espera inválido", ERROR_INVALID_DATA);
}
}
ActionResult terminate_windows_process(std::uint32_t pid, bool force, TerminateApi api) {
    return terminate_impl(pid, force, api, std::nullopt);
}
ActionResult terminate_windows_process_checked(std::uint32_t pid, std::uint64_t expected, bool force, TerminateApi api) {
    return terminate_impl(pid, force, api, expected);
}
}
#endif
