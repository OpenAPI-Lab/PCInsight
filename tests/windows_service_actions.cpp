#include "../src/windows_service_actions.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
DWORD state, manager_error, open_error, query_error, start_error, stop_error, requested_access;
bool stuck, fail_after_start, short_status;
unsigned opened, closed, starts, stops, queries;
ULONGLONG now;
DWORD sleep_step;
std::vector<char> calls;
SC_HANDLE WINAPI manager(LPCWSTR machine, LPCWSTR db, DWORD flags) {
    assert(!machine && !db && flags == SC_MANAGER_CONNECT);
    if (manager_error) { SetLastError(manager_error); return nullptr; }
    ++opened; return reinterpret_cast<SC_HANDLE>(1);
}
SC_HANDLE WINAPI open(SC_HANDLE h, LPCWSTR name, DWORD flags) {
    assert(h == reinterpret_cast<SC_HANDLE>(1) && std::wstring(name) == L"test");
    requested_access = flags;
    if (open_error) { SetLastError(open_error); return nullptr; }
    ++opened; return reinterpret_cast<SC_HANDLE>(2);
}
BOOL WINAPI close(SC_HANDLE h) { assert(h); ++closed; return TRUE; }
BOOL WINAPI query(SC_HANDLE h, SC_STATUS_TYPE type, LPBYTE data, DWORD capacity, LPDWORD size) {
    assert(h == reinterpret_cast<SC_HANDLE>(2) && type == SC_STATUS_PROCESS_INFO);
    assert(capacity == sizeof(SERVICE_STATUS_PROCESS)); ++queries;
    if (query_error) { SetLastError(query_error); return FALSE; }
    auto& s = *reinterpret_cast<SERVICE_STATUS_PROCESS*>(data);
    s = {}; s.dwCurrentState = state; s.dwWin32ExitCode = 1066;
    *size = short_status ? 1 : sizeof s;
    if (!stuck) {
        if (state == SERVICE_START_PENDING) state = fail_after_start ? SERVICE_STOPPED : SERVICE_RUNNING;
        if (state == SERVICE_STOP_PENDING) state = SERVICE_STOPPED;
    }
    return TRUE;
}
BOOL WINAPI start(SC_HANDLE h, DWORD argc, LPCWSTR* argv) {
    assert(h == reinterpret_cast<SC_HANDLE>(2) && argc == 0 && !argv);
    ++starts; calls.push_back('S');
    if (start_error) {
        if (start_error == ERROR_SERVICE_ALREADY_RUNNING) state = SERVICE_RUNNING;
        SetLastError(start_error); return FALSE;
    }
    state = SERVICE_START_PENDING; return TRUE;
}
BOOL WINAPI control(SC_HANDLE h, DWORD code, LPSERVICE_STATUS output) {
    assert(h == reinterpret_cast<SC_HANDLE>(2) && code == SERVICE_CONTROL_STOP && output);
    ++stops; calls.push_back('T');
    if (stop_error) {
        if (stop_error == ERROR_SERVICE_NOT_ACTIVE) state = SERVICE_STOPPED;
        SetLastError(stop_error); return FALSE;
    }
    state = SERVICE_STOP_PENDING; return TRUE;
}
ULONGLONG WINAPI clock_now() { return now; }
void WINAPI sleep_for(DWORD ms) { now += sleep_step ? sleep_step : ms; }
ServiceActionsApi api{manager, open, query, start, control, close, clock_now, sleep_for};
void reset(DWORD initial = SERVICE_STOPPED) {
    state = initial; manager_error = open_error = query_error = start_error = stop_error = requested_access = 0;
    stuck = fail_after_start = short_status = false;
    opened = closed = starts = stops = queries = 0; now = 0; sleep_step = 0; calls.clear();
}
ActionResult run(ServiceAction action, const std::string& name = "test") {
    auto result = control_windows_service(name, action, api);
    assert(opened == closed); return result;
}
}
int main() {
    reset(); assert(run(ServiceAction::start));
    assert(starts == 1 && stops == 0 && queries >= 3 && state == SERVICE_RUNNING);
    assert(requested_access == (SERVICE_QUERY_STATUS | SERVICE_START));
    reset(SERVICE_RUNNING); assert(run(ServiceAction::start)); assert(!starts && !stops);
    reset(); assert(run(ServiceAction::stop)); assert(!starts && !stops);
    assert(requested_access == (SERVICE_QUERY_STATUS | SERVICE_STOP));
    reset(SERVICE_RUNNING); assert(run(ServiceAction::stop)); assert(stops == 1 && !starts);
    reset(SERVICE_PAUSED); assert(run(ServiceAction::restart));
    assert((calls == std::vector<char>{'T','S'}));
    assert(requested_access == (SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP));
    reset(); assert(run(ServiceAction::restart)); assert(starts == 1 && !stops);
    reset(SERVICE_START_PENDING); assert(run(ServiceAction::stop)); assert(stops == 1);
    reset(SERVICE_STOP_PENDING); assert(run(ServiceAction::start)); assert(starts == 1);
    reset(SERVICE_PAUSED); assert(!run(ServiceAction::start)); assert(!starts);
    for (auto name : {std::string{}, std::string("a/b"), std::string("a\\b"),
                      std::string("a\0b",3), std::string("\xff"), std::string(257,'x')}) {
        reset(); assert(run(ServiceAction::start, name).error == ActionError::invalid_argument); assert(!opened);
    }
    reset(); assert(run(static_cast<ServiceAction>(99)).error == ActionError::invalid_argument); assert(!opened);
    reset(); manager_error = ERROR_ACCESS_DENIED; assert(run(ServiceAction::start).error == ActionError::permission_denied);
    reset(); open_error = ERROR_SERVICE_DOES_NOT_EXIST; assert(run(ServiceAction::start).error == ActionError::not_found);
    reset(); open_error = ERROR_ACCESS_DENIED; assert(run(ServiceAction::start).error == ActionError::permission_denied);
    reset(); query_error = ERROR_ACCESS_DENIED; assert(run(ServiceAction::start).error == ActionError::permission_denied); assert(!starts);
    reset(); short_status = true; assert(!run(ServiceAction::start)); assert(!starts);
    reset(); start_error = ERROR_SERVICE_DISABLED; assert(!run(ServiceAction::start));
    reset(); start_error = ERROR_SERVICE_ALREADY_RUNNING; assert(run(ServiceAction::start));
    reset(SERVICE_RUNNING); stop_error = ERROR_SERVICE_NOT_ACTIVE; assert(run(ServiceAction::stop));
    reset(SERVICE_RUNNING); stop_error = ERROR_DEPENDENT_SERVICES_RUNNING; assert(!run(ServiceAction::restart)); assert(!starts && stops == 1);
    reset(SERVICE_RUNNING); start_error = ERROR_ACCESS_DENIED;
    auto partial = run(ServiceAction::restart);
    assert(partial.error == ActionError::permission_denied && partial.message.find("reinicio parcial") != std::string::npos);
    reset(); fail_after_start = true; assert(!run(ServiceAction::start));
    reset(SERVICE_START_PENDING); stuck = true; assert(!run(ServiceAction::restart));
    assert(now == 30000 && !starts && !stops);
    reset(); stuck = true; assert(!run(ServiceAction::start)); assert(now == 30000 && starts == 1);
    reset(SERVICE_RUNNING); sleep_step = 15000;
    partial = run(ServiceAction::restart);
    assert(!partial && now == 30000 && starts == 1 && stops == 1);
    assert(partial.message.find("reinicio parcial") != std::string::npos);
    reset(999); assert(!run(ServiceAction::stop)); assert(!stops);
    // Public dispatch is exercised only with invalid input: no real SCM mutations.
    assert(make_system_actions()->control_service("",ServiceAction::start).error == ActionError::invalid_argument);
    std::cout << "Windows service actions: injected tests passed; no native mutations\n";
}
