#include "windows_service_actions.hpp"
#ifdef _WIN32
namespace pcinsight::detail {
namespace {
struct Handle {
    SC_HANDLE value;
    ServiceActionsApi& api;
    ~Handle() { if (value) api.close(value); }
};
ActionResult failure(const std::string& operation, DWORD error) {
    auto kind = ActionError::system_error;
    if (error == ERROR_ACCESS_DENIED || error == ERROR_PRIVILEGE_NOT_HELD)
        kind = ActionError::permission_denied;
    if (error == ERROR_SERVICE_DOES_NOT_EXIST) kind = ActionError::not_found;
    return {kind, std::string(operation) + ": Win32 " + std::to_string(error)};
}
bool pending(DWORD state) {
    return state == SERVICE_START_PENDING || state == SERVICE_STOP_PENDING ||
           state == SERVICE_PAUSE_PENDING || state == SERVICE_CONTINUE_PENDING;
}
}
ActionResult control_windows_service(const std::string& name, ServiceAction action, ServiceActionsApi api) {
    if (name.empty() || name.size() > 1024 || name.find('\0') != std::string::npos ||
        name.find_first_of("/\\") != std::string::npos ||
        (action != ServiceAction::start && action != ServiceAction::stop && action != ServiceAction::restart))
        return {ActionError::invalid_argument, "nombre interno o acción de servicio inválido"};
    const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name.data(),
                                          static_cast<int>(name.size()), nullptr, 0);
    if (count <= 0 || count > 256) return {ActionError::invalid_argument, "nombre UTF-8 inválido o demasiado largo"};
    std::wstring wide(count, L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name.data(),
                            static_cast<int>(name.size()), wide.data(), count))
        return {ActionError::invalid_argument, "nombre UTF-8 inválido"};
    Handle manager{api.manager(nullptr, nullptr, SC_MANAGER_CONNECT), api};
    if (!manager.value) return failure("OpenSCManager", GetLastError());
    DWORD access = SERVICE_QUERY_STATUS;
    if (action != ServiceAction::stop) access |= SERVICE_START;
    if (action != ServiceAction::start) access |= SERVICE_STOP;
    Handle service{api.open(manager.value, wide.c_str(), access), api};
    if (!service.value) return failure("OpenService", GetLastError());
    const auto began = api.clock();
    SERVICE_STATUS_PROCESS status{};
    // One shared polling budget for the entire operation, including both restart phases.
    auto settle = [&]() -> ActionResult {
        for (;;) {
            DWORD size = 0;
            if (!api.query(service.value, SC_STATUS_PROCESS_INFO,
                           reinterpret_cast<BYTE*>(&status), sizeof status, &size))
                return failure("QueryServiceStatusEx", GetLastError());
            if (size < sizeof status) return failure("estado incompleto", ERROR_INVALID_DATA);
            if (api.clock() - began >= 30000) return failure("espera agotada", ERROR_TIMEOUT);
            if (!pending(status.dwCurrentState)) return {};
            api.sleep(100);
        }
    };
    auto result = settle();
    if (!result) return result;
    bool stopped_for_restart = false;
    if (action != ServiceAction::start && status.dwCurrentState != SERVICE_STOPPED) {
        if (status.dwCurrentState != SERVICE_RUNNING && status.dwCurrentState != SERVICE_PAUSED)
            return failure("estado incompatible con stop", ERROR_SERVICE_CANNOT_ACCEPT_CTRL);
        SERVICE_STATUS ignored{};
        if (!api.control(service.value, SERVICE_CONTROL_STOP, &ignored)) {
            const auto error = GetLastError();
            if (error != ERROR_SERVICE_NOT_ACTIVE) return failure("ControlService STOP", error);
        }
        result = settle();
        if (!result) return result;
        if (status.dwCurrentState != SERVICE_STOPPED)
            return failure("servicio no detenido", ERROR_SERVICE_CANNOT_ACCEPT_CTRL);
        stopped_for_restart = action == ServiceAction::restart;
    }
    if (action == ServiceAction::stop) return {};
    auto start_phase = [&]() -> ActionResult {
        if (status.dwCurrentState == SERVICE_RUNNING) return {};
        if (status.dwCurrentState != SERVICE_STOPPED)
            return failure("estado incompatible con start", ERROR_SERVICE_CANNOT_ACCEPT_CTRL);
        if (!api.start(service.value, 0, nullptr)) {
            const auto error = GetLastError();
            if (error != ERROR_SERVICE_ALREADY_RUNNING) return failure("StartService", error);
        }
        auto started = settle();
        if (!started) return started;
        if (status.dwCurrentState != SERVICE_RUNNING)
            return failure("servicio no llegó a running (exit=" + std::to_string(status.dwWin32ExitCode) + ")", ERROR_SERVICE_NOT_ACTIVE);
        return {};
    };
    result = start_phase();
    if (!result && stopped_for_restart)
        result.message = "reinicio parcial: servicio detenido; inicio no confirmado. " + result.message;
    return result;
}
}
#endif
