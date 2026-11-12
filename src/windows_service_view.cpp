#include "windows_service_view.hpp"
#ifdef _WIN32
#include "windows_services.hpp"
#include <algorithm>
#include <cstring>

namespace pcinsight::detail {
namespace {
struct Handle {
    SC_HANDLE value;
    decltype(&CloseServiceHandle) close;
    ~Handle() { if (value) close(value); }
};
ActionResult error(const char* operation, DWORD code) {
    auto kind = ActionError::system_error;
    if (code == ERROR_ACCESS_DENIED || code == ERROR_PRIVILEGE_NOT_HELD) kind = ActionError::permission_denied;
    if (code == ERROR_SERVICE_DOES_NOT_EXIST) kind = ActionError::not_found;
    return {kind, std::string(operation) + ": Win32=" + std::to_string(code)};
}
template<class T> Metric<T> missing(const char* operation, DWORD code) {
    const auto result = error(operation, code);
    return {std::nullopt, result.error == ActionError::permission_denied ? Support::permission_required : Support::unsupported,
        result.message};
}
Metric<std::string> text(const std::vector<unsigned char>& buffer, const wchar_t* pointer) {
    if (!pointer) return {std::string{}, Support::available, {}};
    const auto base = reinterpret_cast<std::uintptr_t>(buffer.data());
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    if (address < base || address - base >= buffer.size() || (address - base) % sizeof(wchar_t))
        return missing<std::string>("invalid SCM string pointer", ERROR_INVALID_DATA);
    std::wstring wide;
    auto offset = static_cast<std::size_t>(address - base);
    for (; offset + sizeof(wchar_t) <= buffer.size(); offset += sizeof(wchar_t)) {
        wchar_t ch{};
        std::memcpy(&ch, buffer.data() + offset, sizeof ch);
        if (!ch) {
            if (wide.empty()) return {std::string{}, Support::available, {}};
            const auto size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(),
                static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
            if (!size) return missing<std::string>("invalid SCM UTF-16", ERROR_INVALID_DATA);
            std::string result(size, '\0');
            if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), static_cast<int>(wide.size()),
                result.data(), size, nullptr, nullptr))
                return missing<std::string>("SCM UTF-8 conversion", GetLastError());
            return {std::move(result), Support::available, {}};
        }
        wide.push_back(ch);
    }
    return missing<std::string>("unterminated SCM string", ERROR_INVALID_DATA);
}
}
WindowsServiceOpenResult open_windows_service_impl(const std::string& name, WindowsServiceViewApi api) {
    if (name.empty() || name.size() > 1024 || name.find('\0') != std::string::npos || name.find_first_of("/\\") != std::string::npos)
        return {{ActionError::invalid_argument, "invalid internal service name"}, std::nullopt};
    const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name.data(), static_cast<int>(name.size()), nullptr, 0);
    if (size <= 0 || size > 256)
        return {{ActionError::invalid_argument, "invalid UTF-8 or internal name exceeds 256 UTF-16 code units"}, std::nullopt};
    std::wstring wide(size, L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name.data(), static_cast<int>(name.size()), wide.data(), size))
        return {{ActionError::invalid_argument, "invalid UTF-8"}, std::nullopt};
    Handle manager{api.manager(nullptr, nullptr, SC_MANAGER_CONNECT), api.close};
    if (!manager.value) return {error("OpenSCManager", GetLastError()), std::nullopt};
    // Pin the SCM object while a separate config handle is requested. Do not
    // require config rights to return readable status, or elevate on denial.
    Handle service{api.open(manager.value, wide.c_str(), SERVICE_QUERY_STATUS), api.close};
    if (!service.value) return {error("OpenService status", GetLastError()), std::nullopt};
    SERVICE_STATUS_PROCESS status{};
    DWORD needed{};
    if (!api.query(service.value, SC_STATUS_PROCESS_INFO, reinterpret_cast<BYTE*>(&status), sizeof status, &needed))
        return {error("QueryServiceStatusEx", GetLastError()), std::nullopt};
    if (needed != sizeof status) return {error("incomplete SCM status", ERROR_INVALID_DATA), std::nullopt};
    const auto type = status.dwServiceType & SERVICE_WIN32;
    if (type != SERVICE_WIN32_OWN_PROCESS && type != SERVICE_WIN32_SHARE_PROCESS)
        return {{ActionError::not_supported, "only Win32 services, not kernel drivers, are supported"}, std::nullopt};
    WindowsServiceView out;
    out.id = name;
    out.native_service_type = {status.dwServiceType, Support::available, {}};
    const auto state = windows_service_state(status.dwCurrentState);
    if (state != "unknown") out.status = {state, Support::available, {}};
    else out.status = missing<std::string>("unrecognized SCM state", ERROR_INVALID_DATA);
    if (status.dwCurrentState == SERVICE_RUNNING || status.dwCurrentState == SERVICE_PAUSED ||
        status.dwCurrentState == SERVICE_PAUSE_PENDING || status.dwCurrentState == SERVICE_CONTINUE_PENDING) {
        if (status.dwProcessId) out.process_id = {std::optional<std::uint32_t>{status.dwProcessId}, Support::available, {}};
        else out.process_id = missing<std::optional<std::uint32_t>>("no valid PID reported", ERROR_INVALID_DATA);
    } else if (status.dwCurrentState == SERVICE_STOPPED || status.dwCurrentState == SERVICE_START_PENDING ||
        status.dwCurrentState == SERVICE_STOP_PENDING) {
        out.process_id.value.emplace(std::nullopt);
        out.process_id.support = Support::available;
        out.process_id.note = "SCM state does not provide a publishable PID";
    } else out.process_id = missing<std::optional<std::uint32_t>>("unrecognized SCM state", ERROR_INVALID_DATA);
    Handle config{api.open(manager.value, wide.c_str(), SERVICE_QUERY_CONFIG), api.close};
    if (!config.value) {
        const auto code = GetLastError();
        out.display_name = out.description = out.load_order_group = missing<std::string>("OpenService config", code);
        return {{}, std::move(out)};
    }
    std::vector<unsigned char> buffer(8192, 0xff);
    if (api.config(config.value, reinterpret_cast<QUERY_SERVICE_CONFIGW*>(buffer.data()),
        static_cast<DWORD>(buffer.size()), &needed)) {
        QUERY_SERVICE_CONFIGW data{};
        std::memcpy(&data, buffer.data(), sizeof data);
        out.display_name = text(buffer, data.lpDisplayName);
        out.load_order_group = text(buffer, data.lpLoadOrderGroup);
    } else {
        const auto code = GetLastError();
        out.display_name = out.load_order_group = missing<std::string>("QueryServiceConfig", code);
    }
    std::fill(buffer.begin(), buffer.end(), std::uint8_t{0xff});
    if (api.description(config.value, SERVICE_CONFIG_DESCRIPTION, buffer.data(), static_cast<DWORD>(buffer.size()), &needed)) {
        SERVICE_DESCRIPTIONW data{};
        std::memcpy(&data, buffer.data(), sizeof data);
        out.description = text(buffer, data.lpDescription);
    } else out.description = missing<std::string>("QueryServiceConfig2 description", GetLastError());
    return {{}, std::move(out)};
}
}
#endif
namespace pcinsight {
WindowsServiceOpenResult open_windows_service(const std::string& name) {
#ifdef _WIN32
    return detail::open_windows_service_impl(name);
#else
    (void)name;
    return {{ActionError::not_supported, "Windows SCM is unavailable on this platform"}, std::nullopt};
#endif
}
}
