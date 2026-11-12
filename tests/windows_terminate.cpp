#include "../src/windows_terminate.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
DWORD open_error, critical_error, terminate_error;
DWORD times_error;
constexpr std::uint64_t expected_creation = 0x12345678abcdef01ULL;
std::uint64_t actual_creation;
BOOL critical_value;
unsigned opens, closes, checks, kills, waits;
std::vector<DWORD> wait_results;
std::vector<char> calls;
HANDLE WINAPI open(DWORD access, BOOL inherit, DWORD pid) {
    assert(access == (PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE));
    assert(!inherit && pid == 1234); ++opens; calls.push_back('O');
    if (open_error) { SetLastError(open_error); return nullptr; }
    return reinterpret_cast<HANDLE>(1);
}
BOOL WINAPI close(HANDLE h) { assert(h == reinterpret_cast<HANDLE>(1)); ++closes; calls.push_back('C'); return TRUE; }
DWORD WINAPI current() { return 5678; }
BOOL WINAPI critical(HANDLE h, PBOOL value) {
    assert(h == reinterpret_cast<HANDLE>(1)); ++checks; calls.push_back('Q');
    if (critical_error) { SetLastError(critical_error); return FALSE; }
    *value = critical_value; return TRUE;
}
BOOL WINAPI terminate(HANDLE h, UINT code) {
    assert(h == reinterpret_cast<HANDLE>(1) && code == 1); ++kills; calls.push_back('T');
    if (terminate_error) { SetLastError(terminate_error); return FALSE; }
    return TRUE;
}
DWORD WINAPI wait(HANDLE h, DWORD ms) {
    assert(h == reinterpret_cast<HANDLE>(1));
    assert(ms == (kills && !terminate_error ? 5000u : 0u));
    assert(waits < wait_results.size()); calls.push_back('W');
    const auto result = wait_results[waits++];
    if (result == WAIT_FAILED) SetLastError(ERROR_INVALID_HANDLE);
    return result;
}
BOOL WINAPI times(HANDLE h, LPFILETIME created, LPFILETIME exited, LPFILETIME kernel, LPFILETIME user) {
    assert(h == reinterpret_cast<HANDLE>(1) && created && exited && kernel && user);
    calls.push_back('I');
    if (times_error) { SetLastError(times_error); return FALSE; }
    created->dwHighDateTime = static_cast<DWORD>(actual_creation >> 32);
    created->dwLowDateTime = static_cast<DWORD>(actual_creation);
    return TRUE;
}
TerminateApi api{open, close, current, critical, terminate, wait, times};
void reset() {
    open_error = critical_error = terminate_error = 0;
    critical_value = FALSE;
    times_error = 0; actual_creation = expected_creation;
    opens = closes = checks = kills = waits = 0;
    wait_results = {WAIT_TIMEOUT, WAIT_OBJECT_0}; calls.clear(); api.critical = critical;
}
ActionResult run(bool force = true, DWORD pid = 1234) {
    auto result = terminate_windows_process(pid, force, api);
    assert(closes == (opens && !open_error ? 1u : 0u));
    return result;
}
ActionResult checked(std::uint64_t expected = expected_creation, bool force = true) {
    auto result = terminate_windows_process_checked(1234, expected, force, api);
    assert(closes == (opens && !open_error ? 1u : 0u));
    return result;
}
}
int main() {
    reset(); assert(run()); assert(kills == 1 && checks == 1 && waits == 2);
    assert((calls == std::vector<char>{'O','W','Q','T','W','C'}));
    for (DWORD pid : {0u,1u,2u,3u,4u,5678u}) {
        reset(); assert(run(true,pid).error == ActionError::invalid_argument); assert(!opens && !kills);
    }
    reset(); assert(run(false).error == ActionError::not_supported); assert(!opens && !kills);
    reset(); api.critical = nullptr; assert(run().error == ActionError::not_supported); assert(!opens && !kills);
    reset(); open_error = ERROR_ACCESS_DENIED; assert(run().error == ActionError::permission_denied); assert(!kills);
    reset(); open_error = ERROR_INVALID_PARAMETER; assert(run().error == ActionError::not_found); assert(!kills);
    reset(); open_error = ERROR_NOT_ENOUGH_MEMORY; assert(run().error == ActionError::system_error);
    reset(); critical_value = TRUE; assert(run().error == ActionError::permission_denied); assert(!kills);
    reset(); critical_error = ERROR_ACCESS_DENIED; assert(run().error == ActionError::permission_denied); assert(!kills);
    reset(); critical_error = ERROR_INVALID_HANDLE; assert(run().error == ActionError::system_error); assert(!kills);
    reset(); wait_results = {WAIT_OBJECT_0}; assert(run()); assert(!checks && !kills);
    reset(); wait_results = {WAIT_FAILED}; assert(!run()); assert(!checks && !kills);
    reset(); wait_results = {WAIT_ABANDONED}; assert(!run()); assert(!checks && !kills);
    reset(); terminate_error = ERROR_ACCESS_DENIED; wait_results = {WAIT_TIMEOUT,WAIT_TIMEOUT};
    assert(run().error == ActionError::permission_denied); assert(kills == 1);
    reset(); terminate_error = ERROR_ACCESS_DENIED; wait_results = {WAIT_TIMEOUT,WAIT_OBJECT_0};
    assert(run()); // Target exited concurrently; same handle is signaled.
    reset(); terminate_error = ERROR_PRIVILEGE_NOT_HELD; wait_results = {WAIT_TIMEOUT,WAIT_FAILED};
    assert(run().error == ActionError::permission_denied); // Preserve original native error.
    reset(); wait_results = {WAIT_TIMEOUT,WAIT_TIMEOUT}; auto result = run();
    assert(!result && result.message.find("no confirmada") != std::string::npos && kills == 1);
    reset(); wait_results = {WAIT_TIMEOUT,WAIT_FAILED}; result = run();
    assert(!result && result.message.find("solicitada") != std::string::npos);
    reset(); wait_results = {WAIT_TIMEOUT,WAIT_ABANDONED}; assert(!run());
    reset(); assert(checked()); assert(kills == 1);
    assert((calls == std::vector<char>{'O','I','W','Q','T','W','C'}));
    for (const auto value : {expected_creation + 1, expected_creation ^ (1ULL << 40), 0ULL}) {
        reset(); actual_creation = value;
        assert(checked().error == ActionError::stale_target);
        assert(!kills && !checks && !waits);
        assert((calls == std::vector<char>{'O','I','C'}));
    }
    reset(); assert(checked(0).error == ActionError::invalid_argument); assert(!opens && !kills);
    reset(); assert(checked(expected_creation,false).error == ActionError::not_supported); assert(!opens);
    reset(); times_error = ERROR_ACCESS_DENIED;
    assert(checked().error == ActionError::permission_denied); assert(!kills && !waits);
    reset(); times_error = ERROR_INVALID_HANDLE;
    assert(checked().error == ActionError::system_error); assert(!kills && !waits);
    reset(); wait_results = {WAIT_OBJECT_0}; assert(checked()); assert(!kills && !checks);
    reset(); actual_creation++; wait_results = {WAIT_OBJECT_0};
    assert(checked().error == ActionError::stale_target); assert(!waits && !kills);
    reset(); critical_value = TRUE; assert(checked().error == ActionError::permission_denied); assert(!kills);
    reset(); api.critical = nullptr; assert(checked().error == ActionError::not_supported); assert(!opens);
    reset(); open_error = ERROR_INVALID_PARAMETER; assert(checked().error == ActionError::not_found); assert(!kills);
    reset(); wait_results = {WAIT_TIMEOUT,WAIT_TIMEOUT}; assert(!checked()); assert(kills == 1);
    // Public dispatch is tested only with rejected inputs; no native termination.
    assert(make_system_actions()->terminate_process_checked(0,expected_creation,true).error == ActionError::invalid_argument);
    assert(make_system_actions()->terminate_process_checked(1234,0,true).error == ActionError::invalid_argument);
    assert(make_system_actions()->terminate_process(0,true).error == ActionError::invalid_argument);
    std::cout << "Windows termination: injected tests passed; no real process terminated\n";
}
