#include "../src/windows_service_view.hpp"
#include "../src/windows_services.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
int opens{}, closes{}, manager_calls{}, config_calls{}, mode{};
DWORD state = SERVICE_RUNNING, pid = 42;
SC_HANDLE handle(std::uintptr_t value) { return reinterpret_cast<SC_HANDLE>(value); }
SC_HANDLE WINAPI manager(LPCWSTR machine, LPCWSTR database, DWORD access) {
    assert(!machine && !database && access == SC_MANAGER_CONNECT);
    ++manager_calls;
    if (mode == 1) { SetLastError(ERROR_ACCESS_DENIED); return nullptr; }
    ++opens; return handle(1);
}
SC_HANDLE WINAPI open(SC_HANDLE h, LPCWSTR name, DWORD access) {
    assert(h == handle(1) && std::wstring(name) == L"test");
    assert(access == SERVICE_QUERY_STATUS || access == SERVICE_QUERY_CONFIG);
    if (mode == 2 || mode == 3 || (mode == 4 && access == SERVICE_QUERY_CONFIG)) {
        SetLastError(mode == 3 ? ERROR_SERVICE_DOES_NOT_EXIST : ERROR_ACCESS_DENIED); return nullptr;
    }
    ++opens; return handle(access == SERVICE_QUERY_STATUS ? 2 : 3);
}
BOOL WINAPI close(SC_HANDLE h) { assert(h); ++closes; return TRUE; }
BOOL WINAPI query(SC_HANDLE h, SC_STATUS_TYPE level, LPBYTE data, DWORD size, LPDWORD needed) {
    assert(h == handle(2) && level == SC_STATUS_PROCESS_INFO && size == sizeof(SERVICE_STATUS_PROCESS));
    if (mode == 5) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
    SERVICE_STATUS_PROCESS s{};
    s.dwCurrentState = state; s.dwProcessId = pid;
    s.dwServiceType = mode == 7 ? SERVICE_KERNEL_DRIVER : SERVICE_WIN32_OWN_PROCESS;
    std::memcpy(data, &s, sizeof s); *needed = mode == 6 ? 1 : sizeof s;
    return TRUE;
}
wchar_t* put(BYTE* buffer, std::size_t offset, const std::wstring& s) {
    std::memcpy(buffer + offset, s.c_str(), (s.size() + 1) * sizeof(wchar_t));
    return reinterpret_cast<wchar_t*>(buffer + offset);
}
BOOL WINAPI config(SC_HANDLE h, LPQUERY_SERVICE_CONFIGW data, DWORD size, LPDWORD needed) {
    assert(h == handle(3) && size == 8192); ++config_calls;
    if (mode == 8 || mode == 9) {
        *needed = 16384; SetLastError(mode == 8 ? ERROR_INSUFFICIENT_BUFFER : ERROR_ACCESS_DENIED); return FALSE;
    }
    auto* buffer = reinterpret_cast<BYTE*>(data);
    QUERY_SERVICE_CONFIGW c{};
    c.lpDisplayName = put(buffer, 256, L"Prueba \u00e9");
    c.lpLoadOrderGroup = mode == 10 ? nullptr : put(buffer, 512, L"Group");
    if (mode == 11) c.lpLoadOrderGroup = reinterpret_cast<wchar_t*>(std::uintptr_t(1));
    if (mode == 12) c.lpLoadOrderGroup = reinterpret_cast<wchar_t*>(buffer + 513);
    if (mode == 13) { c.lpLoadOrderGroup = reinterpret_cast<wchar_t*>(buffer + 8190); buffer[8190]=1; buffer[8191]=1; }
    if (mode == 14) c.lpDisplayName = put(buffer, 256, std::wstring(1, static_cast<wchar_t>(0xd800)));
    std::memcpy(data, &c, sizeof c); return TRUE;
}
BOOL WINAPI description(SC_HANDLE h, DWORD level, LPBYTE buffer, DWORD size, LPDWORD) {
    assert(h == handle(3) && level == SERVICE_CONFIG_DESCRIPTION && size == 8192);
    if (mode == 15) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
    if (mode == 16) { SetLastError(ERROR_SERVICE_MARKED_FOR_DELETE); return FALSE; }
    SERVICE_DESCRIPTIONW d{};
    if (mode != 10) d.lpDescription = put(buffer, 128, mode == 17 ? L"@C:\\resource.dll,-1" : L"Description");
    std::memcpy(buffer, &d, sizeof d); return TRUE;
}
WindowsServiceOpenResult run(int next_mode = 0, const std::string& name = "test") {
    mode = next_mode; opens = closes = manager_calls = config_calls = 0;
    auto result = open_windows_service_impl(name, {manager,open,query,config,description,close});
    assert(opens == closes);
    assert(static_cast<bool>(result.result) == result.view.has_value());
    return result;
}
}
int main() {
    auto r = run();
    assert(r.view && r.view->id == "test" && r.view->display_name.value == "Prueba \xc3\xa9");
    assert(r.view->status.value == "running" && r.view->process_id.value->value() == 42);
    assert(r.view->description.value == "Description" && r.view->load_order_group.value == "Group");
    assert(r.view->native_service_type.value == SERVICE_WIN32_OWN_PROCESS && opens == 3);
    for (const auto& name : {std::string{}, std::string("a/b"), std::string("a\\b"), std::string("a\0b",3),
        std::string("\xc0\xaf"), std::string(257,'x'), std::string(1025,'x')}) {
        assert(run(0,name).result.error == ActionError::invalid_argument && !manager_calls);
    }
    assert(run(1).result.error == ActionError::permission_denied);
    assert(run(2).result.error == ActionError::permission_denied);
    assert(run(3).result.error == ActionError::not_found);
    r = run(4); assert(r.view && r.view->status.value && !r.view->description.value && config_calls == 0);
    assert(r.view->description.support == Support::permission_required && !r.view->load_order_group.value);
    assert(run(5).result.error == ActionError::permission_denied);
    assert(run(6).result.error == ActionError::system_error);
    assert(run(7).result.error == ActionError::not_supported);
    r = run(8); assert(!r.view->display_name.value && r.view->description.value);
    assert(r.view->load_order_group.note.find("122") != std::string::npos);
    r = run(9); assert(r.view->display_name.support == Support::permission_required && r.view->description.value);
    r = run(10); assert(r.view->description.value == "" && r.view->load_order_group.value == "");
    assert(metric_is_known(r.view->description) && metric_is_known(r.view->load_order_group));
    for (int m : {11,12,13}) { r = run(m); assert(!r.view->load_order_group.value && r.view->display_name.value); }
    r = run(14); assert(!r.view->display_name.value && r.view->load_order_group.value);
    r = run(15); assert(!r.view->description.value && r.view->description.support == Support::permission_required);
    r = run(16); assert(!r.view->description.value && r.view->description.note.find("1072") != std::string::npos);
    r = run(17); assert(r.view->description.value == "@C:\\resource.dll,-1");
    for (DWORD s = 1; s <= 7; ++s) {
        state = s; r = run(); assert(metric_is_known(r.view->status));
        assert(metric_is_known(r.view->process_id));
        const bool valid = s == SERVICE_RUNNING || s == SERVICE_PAUSED || s == SERVICE_PAUSE_PENDING || s == SERVICE_CONTINUE_PENDING;
        assert(r.view->process_id.value->has_value() == valid);
    }
    state = SERVICE_RUNNING; pid = 0; r = run(); assert(!r.view->process_id.value);
    state = 99; r = run(); assert(!r.view->status.value && !r.view->process_id.value);
    // Native read-only positive: choose an actually visible Win32 service, no
    // hard-coded service dependency and no controls/configuration changes.
    std::size_t opened{}, descriptions{}, groups{};
    for (const auto& service : read_windows_services()) {
        const auto view = open_windows_service(service.id);
        if (!view.view) continue; // races/ACLs are legitimate for individual rows
        ++opened;
        assert(view.result && view.view->id == service.id);
        if (metric_is_known(view.view->description)) ++descriptions;
        if (metric_is_known(view.view->load_order_group)) ++groups;
        if (opened >= 8 && descriptions && groups) break;
    }
    assert(opened && descriptions && groups);
    assert(open_windows_service("").result.error == ActionError::invalid_argument);
    assert(open_windows_service("PCInsight_Absent_" + std::to_string(GetCurrentProcessId())).result.error == ActionError::not_found);
    std::cout << "Native SCM details opened=" << opened << " descriptions=" << descriptions << " groups=" << groups << '\n';
}
