#include "../src/windows_process_snapshot.hpp"
#include <cassert>
#include <cwchar>
#include <iostream>
#include <system_error>
using namespace pcinsight::detail;
namespace {
DWORD create_error, first_error, final_error;
bool null_handle;
unsigned creates, closes, next_calls;
std::size_t cursor;
std::vector<PROCESSENTRY32W> entries;
HANDLE WINAPI create(DWORD flags, DWORD pid) {
    assert(flags == TH32CS_SNAPPROCESS && pid == 0); ++creates;
    if (create_error) { SetLastError(create_error); return INVALID_HANDLE_VALUE; }
    return null_handle ? nullptr : reinterpret_cast<HANDLE>(1);
}
BOOL WINAPI first(HANDLE h, LPPROCESSENTRY32W out) {
    assert(h == reinterpret_cast<HANDLE>(1) && out->dwSize == sizeof *out);
    if (first_error || entries.empty()) {
        SetLastError(first_error ? first_error : ERROR_NO_MORE_FILES); return FALSE;
    }
    *out = entries[0]; cursor = 1; return TRUE;
}
BOOL WINAPI next(HANDLE h, LPPROCESSENTRY32W out) {
    assert(h == reinterpret_cast<HANDLE>(1) && out->dwSize == sizeof *out);
    assert(out->th32ProcessID == 0 && out->szExeFile[0] == L'\0'); ++next_calls;
    if (cursor == entries.size()) { SetLastError(final_error); return FALSE; }
    *out = entries[cursor++]; return TRUE;
}
BOOL WINAPI close(HANDLE h) { assert(h == reinterpret_cast<HANDLE>(1)); ++closes; return TRUE; }
ProcessSnapshotApi api{create, first, next, close};
PROCESSENTRY32W entry(DWORD pid, const wchar_t* name) {
    PROCESSENTRY32W result{}; result.dwSize = sizeof result;
    result.th32ProcessID = pid; result.th32ParentProcessID = 12; result.cntThreads = 3;
    std::wcscpy(result.szExeFile, name); return result;
}
void reset() {
    create_error = first_error = 0; final_error = ERROR_NO_MORE_FILES; null_handle = false;
    creates = closes = next_calls = 0; cursor = 0;
    entries = {entry(0,L"System Idle Process"), entry(1234,L"test\u00e9.exe")};
}
void fails(DWORD expected, std::size_t limit = 100000) {
    try { (void)read_windows_process_snapshot(api, limit); assert(false); }
    catch (const std::system_error& e) { assert(e.code().value() == static_cast<int>(expected)); }
    assert(closes == (create_error || null_handle ? 0u : 1u));
}
}
int main() {
    reset(); const auto result = read_windows_process_snapshot(api);
    assert(result.size() == 2 && result[1].th32ProcessID == 1234);
    assert(std::wstring(result[1].szExeFile) == L"test\u00e9.exe");
    assert(result[1].cntThreads == 3 && result[1].th32ParentProcessID == 12);
    assert(closes == 1 && next_calls == 2);
    reset(); entries.clear(); assert(read_windows_process_snapshot(api).empty()); assert(closes == 1);
    reset(); create_error = ERROR_ACCESS_DENIED; fails(ERROR_ACCESS_DENIED);
    reset(); null_handle = true; fails(ERROR_INVALID_HANDLE);
    reset(); first_error = ERROR_BAD_LENGTH; fails(ERROR_BAD_LENGTH); assert(!next_calls);
    reset(); final_error = ERROR_ACCESS_DENIED; fails(ERROR_ACCESS_DENIED); assert(next_calls == 2);
    reset(); entries[1].dwSize = 1; fails(ERROR_INVALID_DATA);
    reset(); std::wmemset(entries[1].szExeFile, L'x', MAX_PATH); fails(ERROR_INVALID_DATA);
    reset(); entries[1].th32ProcessID = 0; fails(ERROR_INVALID_DATA);
    reset(); fails(ERROR_MORE_DATA, 1); assert(next_calls == 1);
    reset(); assert(read_windows_process_snapshot(api, 2).size() == 2); assert(closes == 1);
    for (auto limit : {std::size_t(0), std::size_t(100001)}) {
        reset(); try { (void)read_windows_process_snapshot(api,limit); assert(false); }
        catch (const std::invalid_argument&) {} assert(!creates && !closes);
    }
    // A new independent inventory recovers after an earlier complete failure.
    reset(); final_error = ERROR_BAD_LENGTH; fails(ERROR_BAD_LENGTH);
    reset(); assert(read_windows_process_snapshot(api).size() == 2);
    std::cout << "Process snapshot: injected failure and bounds tests passed\n";
}
