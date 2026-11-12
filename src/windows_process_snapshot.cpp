#include "windows_process_snapshot.hpp"
#ifdef _WIN32
#include <cwchar>
#include <set>
#include <stdexcept>
#include <system_error>
namespace pcinsight::detail {
namespace {
struct Handle {
    HANDLE value;
    ProcessSnapshotApi& api;
    Handle(HANDLE v, ProcessSnapshotApi& a) : value(v), api(a) {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) api.close(value); }
};
[[noreturn]] void failure(const char* operation, DWORD error) {
    throw std::system_error(static_cast<int>(error), std::system_category(), operation);
}
}
std::vector<PROCESSENTRY32W> read_windows_process_snapshot(ProcessSnapshotApi api, std::size_t limit) {
    if (!limit || limit > 100000) throw std::invalid_argument("invalid process snapshot bound");
    Handle snapshot(api.create(TH32CS_SNAPPROCESS, 0), api);
    if (snapshot.value == INVALID_HANDLE_VALUE) failure("CreateToolhelp32Snapshot", GetLastError());
    if (!snapshot.value) failure("CreateToolhelp32Snapshot returned null", ERROR_INVALID_HANDLE);
    std::vector<PROCESSENTRY32W> result;
    std::set<DWORD> pids;
    PROCESSENTRY32W entry{}; entry.dwSize = sizeof entry;
    if (!api.first(snapshot.value, &entry)) {
        const auto error = GetLastError();
        if (error == ERROR_NO_MORE_FILES) return result;
        failure("Process32FirstW", error);
    }
    for (;;) {
        if (entry.dwSize != sizeof entry ||
            !std::wmemchr(entry.szExeFile, L'\0', sizeof(entry.szExeFile) / sizeof(wchar_t)))
            failure("invalid PROCESSENTRY32W", ERROR_INVALID_DATA);
        if (!pids.insert(entry.th32ProcessID).second)
            failure("duplicate PID in process snapshot", ERROR_INVALID_DATA);
        if (result.size() == limit) failure("process snapshot record limit", ERROR_MORE_DATA);
        result.push_back(entry);
        // Reinitialize each output buffer rather than relying on previous output.
        entry = {}; entry.dwSize = sizeof entry;
        if (api.next(snapshot.value, &entry)) continue;
        const auto error = GetLastError();
        if (error != ERROR_NO_MORE_FILES) failure("Process32NextW", error);
        return result;
    }
}
}
#endif
