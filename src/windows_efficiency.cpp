#include "windows_efficiency.hpp"
#ifdef _WIN32
#include <cstring>
namespace pcinsight::detail {
namespace {
template<class T> T symbol(const char* name) {
    T fn = nullptr;
    if (auto module = GetModuleHandleW(L"kernel32.dll")) {
        auto address = GetProcAddress(module, name);
        static_assert(sizeof fn == sizeof address);
        std::memcpy(&fn, &address, sizeof fn);
    }
    return fn;
}
ActionResult failure(const char* operation, DWORD error) {
    auto kind = ActionError::system_error;
    if (error == ERROR_ACCESS_DENIED || error == ERROR_PRIVILEGE_NOT_HELD) kind = ActionError::permission_denied;
    else if (error == ERROR_NOT_SUPPORTED || error == ERROR_INVALID_PARAMETER || error == ERROR_CALL_NOT_IMPLEMENTED)
        kind = ActionError::not_supported;
    return {kind, std::string(operation) + ": Win32 " + std::to_string(error)};
}
struct Handle {
    HANDLE value; TerminateApi& api;
    Handle(HANDLE v, TerminateApi& a) : value(v), api(a) {}
    Handle(const Handle&) = delete;
    ~Handle() { if (value) api.close(value); }
};
}
GetProcessInfoFn windows_get_process_information() {
    static auto fn = symbol<GetProcessInfoFn>("GetProcessInformation"); return fn;
}
SetProcessInfoFn windows_set_process_information() {
    static auto fn = symbol<SetProcessInfoFn>("SetProcessInformation"); return fn;
}
EfficiencyInfo read_windows_efficiency(HANDLE process, EfficiencyApi api) {
    EfficiencyInfo result;
    if (!api.get || !api.set) {
        result.available.note = result.requested.note = "Native power-throttling API unavailable";
        return result;
    }
    PROCESS_POWER_THROTTLING_STATE state{}; state.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    if (!api.get(process, ProcessPowerThrottling, &state, sizeof state)) {
        const auto error = GetLastError();
        const auto support = error == ERROR_ACCESS_DENIED ? Support::permission_required : Support::unsupported;
        result.available = result.requested = {std::nullopt, support,
            "GetProcessInformation power-throttling failed; Win32 " + std::to_string(error)};
        return result;
    }
    if (state.Version != PROCESS_POWER_THROTTLING_CURRENT_VERSION) {
        result.available.note = result.requested.note = "Unknown power-throttling structure version"; return result;
    }
    constexpr auto bit = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    result.available = {true, Support::available,
        "Native power policy can be queried; write permissions and hardware effects are not guaranteed"};
    result.requested = {bool((state.ControlMask & bit) && (state.StateMask & bit)), Support::available,
        "Explicit execution-speed EcoQoS request; not effective OS QoS or Task Manager's combined mode"};
    return result;
}
ActionResult set_windows_efficiency(std::uint32_t pid, bool enabled, EfficiencyApi api,
                                   std::optional<std::uint64_t> expected) {
    if ((expected && !*expected) || pid <= 4 || pid == api.process.current_pid())
        return {ActionError::invalid_argument, "identidad inválida, PID reservado o propio"};
    if (!api.set || !api.process.critical)
        return {ActionError::not_supported, "API de EcoQoS o verificación de criticidad no disponible"};
    Handle process(api.process.open(PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE,
                                    FALSE, pid), api.process);
    if (!process.value) {
        auto error = GetLastError();
        if (error == ERROR_INVALID_PARAMETER) return {ActionError::not_found, "PID no encontrado"};
        return failure("OpenProcess", error);
    }
    if (expected) {
        FILETIME created{}, exited{}, kernel{}, user{};
        if (!api.process.times(process.value,&created,&exited,&kernel,&user)) return failure("GetProcessTimes", GetLastError());
        if (((std::uint64_t(created.dwHighDateTime)<<32)|created.dwLowDateTime) != *expected)
            return {ActionError::stale_target, "PID reutilizado; no se cambió EcoQoS"};
    }
    const auto wait = api.process.wait(process.value,0);
    if (wait == WAIT_OBJECT_0) return {ActionError::not_found, "proceso ya terminado"};
    if (wait == WAIT_FAILED) return failure("WaitForSingleObject", GetLastError());
    if (wait != WAIT_TIMEOUT) return failure("estado de espera inválido", ERROR_INVALID_DATA);
    BOOL critical = TRUE;
    if (!api.process.critical(process.value,&critical)) return failure("IsProcessCritical", GetLastError());
    if (critical) return {ActionError::permission_denied, "se rechaza cambiar un proceso crítico"};
    PROCESS_POWER_THROTTLING_STATE state{};
    state.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    state.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    state.StateMask = enabled ? PROCESS_POWER_THROTTLING_EXECUTION_SPEED : 0;
    if (!api.set(process.value,ProcessPowerThrottling,&state,sizeof state)) return failure("SetProcessInformation", GetLastError());
    return {ActionError::none, enabled ? "Windows aceptó solicitar EcoQoS" :
        "Windows aceptó desactivar execution-speed throttling explícitamente; no restaura la política anterior"};
}
}
#endif
