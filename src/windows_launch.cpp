#include "windows_launch.hpp"
#ifdef _WIN32
#include <algorithm>
#include <string_view>
namespace pcinsight::detail {
namespace {
constexpr std::size_t command_limit = 32766; // Windows limit includes the terminating NUL.
bool decode(const std::string& input, std::wstring& output) {
    // UTF-8 needs at most four bytes per UTF-16 code unit; bound before conversion.
    if (input.size() > 4 * command_limit || input.find('\0') != std::string::npos) return false;
    if (input.empty()) { output.clear(); return true; }
    const auto n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(),
                                      static_cast<int>(input.size()), nullptr, 0);
    if (n <= 0 || static_cast<std::size_t>(n) > command_limit) return false;
    output.resize(n);
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(),
                              static_cast<int>(input.size()), output.data(), n) == n;
}
bool absolute_path(const std::wstring& input) {
    if (input.empty() || input.find(L'"') != std::wstring::npos) return false;
    auto path = input;
    std::replace(path.begin(), path.end(), L'/', L'\\');
    const auto letter = [](wchar_t c) { return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z'); };
    if (path.size() >= 3 && letter(path[0]) && path[1] == L':' && path[2] == L'\\') return true;
    // Conventional UNC only; device/extended namespaces are outside this API's initial scope.
    if (path.size() < 5 || path.substr(0, 2) != L"\\\\" || path[2] == L'?' || path[2] == L'.') return false;
    const auto server_end = path.find(L'\\', 2);
    return server_end != std::wstring::npos && server_end > 2 &&
           server_end + 1 < path.size() && path[server_end + 1] != L'\\';
}
// Encode one argument using Microsoft C runtime rules, not shell syntax.
std::wstring quote(std::wstring_view argument) {
    std::wstring result(1, L'"');
    std::size_t slashes = 0;
    for (const auto ch : argument) {
        if (ch == L'\\') { ++slashes; continue; }
        result.append(ch == L'"' ? 2 * slashes + 1 : slashes, L'\\');
        result.push_back(ch); slashes = 0;
    }
    result.append(2 * slashes, L'\\'); result.push_back(L'"');
    return result;
}
struct Handle {
    HANDLE value;
    LaunchApi& api;
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) api.close(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(HANDLE v, LaunchApi& a) : value(v), api(a) {}
};
}
ActionResult launch_windows_process(const LaunchRequest& request, LaunchApi api) {
    auto invalid = [] { return ActionResult{ActionError::invalid_argument,
        "ruta absoluta/UTF-8 inválido o línea de argumentos demasiado larga"}; };
    std::wstring executable, directory;
    if (!decode(request.executable, executable) || !absolute_path(executable) ||
        executable.back() == L'\\' || executable.back() == L'/' ||
        !decode(request.working_directory, directory) ||
        (!directory.empty() && !absolute_path(directory)) || request.arguments.size() > command_limit / 3)
        return invalid();
    auto command = quote(executable);
    if (command.size() > command_limit) return invalid();
    for (const auto& arg : request.arguments) {
        std::wstring decoded;
        if (!decode(arg, decoded)) return invalid();
        auto encoded = quote(decoded);
        if (encoded.size() + 1 > command_limit - command.size()) return invalid();
        command.push_back(L' '); command += encoded;
    }
    if (request.elevated) return {ActionError::permission_denied,
        "elevación explícita requiere un helper UAC autorizado; no se inicia sin ella"};
    STARTUPINFOW startup{}; startup.cb = sizeof startup;
    PROCESS_INFORMATION info{};
    if (!api.create(executable.c_str(), command.data(), nullptr, nullptr, FALSE,
                    0, nullptr, directory.empty() ? nullptr : directory.c_str(), &startup, &info)) {
        const auto error = GetLastError();
        auto kind = ActionError::system_error;
        if (error == ERROR_ACCESS_DENIED || error == ERROR_PRIVILEGE_NOT_HELD || error == ERROR_ELEVATION_REQUIRED)
            kind = ActionError::permission_denied;
        else if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) kind = ActionError::not_found;
        else if (error == ERROR_INVALID_PARAMETER || error == ERROR_INVALID_NAME || error == ERROR_DIRECTORY ||
                 error == ERROR_BAD_EXE_FORMAT || error == ERROR_FILENAME_EXCED_RANGE) kind = ActionError::invalid_argument;
        return {kind, "CreateProcessW: Win32 " + std::to_string(error)};
    }
    Handle process(info.hProcess, api), thread(info.hThread, api);
    return {ActionError::none, "proceso creado con PID " + std::to_string(info.dwProcessId) +
        "; inicialización de la aplicación no verificada", info.dwProcessId};
}
}
#endif
