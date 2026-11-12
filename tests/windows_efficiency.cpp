#include "../src/windows_efficiency.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
unsigned opens, closes, sets;
DWORD open_error, get_error, set_error, critical_error, times_error, wait_state;
BOOL is_critical;
std::uint64_t creation;
PROCESS_POWER_THROTTLING_STATE queried, written;
HANDLE WINAPI open(DWORD rights, BOOL inherit, DWORD pid) {
    assert(rights == (PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE));
    assert(!inherit && pid == 1234); ++opens;
    if (open_error) { SetLastError(open_error); return nullptr; }
    return reinterpret_cast<HANDLE>(1);
}
BOOL WINAPI close(HANDLE h) { assert(h == reinterpret_cast<HANDLE>(1)); ++closes; return TRUE; }
DWORD WINAPI self() { return 5678; }
DWORD WINAPI wait(HANDLE h, DWORD time) { assert(h == reinterpret_cast<HANDLE>(1) && !time); SetLastError(ERROR_INVALID_HANDLE); return wait_state; }
BOOL WINAPI critical(HANDLE h, PBOOL value) {
    assert(h == reinterpret_cast<HANDLE>(1));
    if (critical_error) { SetLastError(critical_error); return FALSE; }
    *value = is_critical; return TRUE;
}
BOOL WINAPI times(HANDLE h, LPFILETIME c, LPFILETIME, LPFILETIME, LPFILETIME) {
    assert(h == reinterpret_cast<HANDLE>(1));
    if (times_error) { SetLastError(times_error); return FALSE; }
    c->dwHighDateTime = static_cast<DWORD>(creation>>32); c->dwLowDateTime = static_cast<DWORD>(creation); return TRUE;
}
BOOL WINAPI get(HANDLE h, PROCESS_INFORMATION_CLASS type, LPVOID out, DWORD size) {
    assert(h == reinterpret_cast<HANDLE>(1) && type == ProcessPowerThrottling && size == sizeof queried);
    assert(static_cast<PROCESS_POWER_THROTTLING_STATE*>(out)->Version == PROCESS_POWER_THROTTLING_CURRENT_VERSION);
    if (get_error) { SetLastError(get_error); return FALSE; }
    *static_cast<PROCESS_POWER_THROTTLING_STATE*>(out) = queried; return TRUE;
}
BOOL WINAPI set(HANDLE h, PROCESS_INFORMATION_CLASS type, LPVOID data, DWORD size) {
    assert(h == reinterpret_cast<HANDLE>(1) && type == ProcessPowerThrottling && size == sizeof written);
    ++sets; written = *static_cast<PROCESS_POWER_THROTTLING_STATE*>(data);
    assert(written.Version == PROCESS_POWER_THROTTLING_CURRENT_VERSION);
    assert(written.ControlMask == PROCESS_POWER_THROTTLING_EXECUTION_SPEED);
    if (set_error) { SetLastError(set_error); return FALSE; }
    return TRUE;
}
EfficiencyApi api;
void reset() {
    opens = closes = sets = 0; open_error = get_error = set_error = critical_error = times_error = 0;
    creation = 0x12345678abcdef01ULL; wait_state = WAIT_TIMEOUT; is_critical = FALSE;
    queried = {PROCESS_POWER_THROTTLING_CURRENT_VERSION,0,0}; written = {};
    api = {}; api.get=get; api.set=set;
    api.process.open=open; api.process.close=close; api.process.current_pid=self;
    api.process.critical=critical; api.process.times=times; api.process.wait=wait;
}
ActionResult run(bool enable = true, std::optional<std::uint64_t> expected = std::nullopt) {
    auto result=set_windows_efficiency(1234,enable,api,expected);
    assert(closes==(opens && !open_error ? 1u : 0u)); return result;
}
EfficiencyInfo read() { return read_windows_efficiency(reinterpret_cast<HANDLE>(1),api); }
}
int main() {
    reset(); assert(run()); assert(sets==1 && written.StateMask==PROCESS_POWER_THROTTLING_EXECUTION_SPEED);
    reset(); assert(run(false)); assert(sets==1 && written.StateMask==0);
    reset(); assert(run(true,creation)); assert(sets==1);
    reset(); assert(run(true,creation+1).error==ActionError::stale_target); assert(!sets);
    reset(); assert(run(true,0).error==ActionError::invalid_argument); assert(!opens && !sets);
    reset(); times_error=ERROR_ACCESS_DENIED;
    assert(run(true,creation).error==ActionError::permission_denied); assert(!sets);
    reset(); is_critical=TRUE; assert(run().error==ActionError::permission_denied); assert(!sets);
    reset(); critical_error=ERROR_INVALID_HANDLE; assert(!run()); assert(!sets);
    reset(); api.process.critical=nullptr; assert(run().error==ActionError::not_supported); assert(!opens);
    reset(); api.set=nullptr; assert(run().error==ActionError::not_supported); assert(!opens);
    reset(); open_error=ERROR_ACCESS_DENIED; assert(run().error==ActionError::permission_denied);
    reset(); open_error=ERROR_INVALID_PARAMETER; assert(run().error==ActionError::not_found);
    reset(); wait_state=WAIT_OBJECT_0; assert(run().error==ActionError::not_found); assert(!sets);
    reset(); wait_state=WAIT_FAILED; assert(!run()); assert(!sets);
    reset(); wait_state=WAIT_ABANDONED; assert(!run()); assert(!sets);
    for (DWORD error : {ERROR_INVALID_PARAMETER,ERROR_NOT_SUPPORTED,ERROR_CALL_NOT_IMPLEMENTED,
                        ERROR_ACCESS_DENIED,ERROR_PRIVILEGE_NOT_HELD,ERROR_INVALID_HANDLE}) {
        reset(); set_error=error; const auto result=run();
        const auto kind=error==ERROR_ACCESS_DENIED||error==ERROR_PRIVILEGE_NOT_HELD ? ActionError::permission_denied :
            (error==ERROR_INVALID_HANDLE ? ActionError::system_error : ActionError::not_supported);
        assert(result.error==kind && sets==1);
    }
    reset(); auto info=read(); assert(info.available.value==true && info.requested.value==false);
    queried.ControlMask=queried.StateMask=PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    assert(read().requested.value==true);
    queried.ControlMask=0; assert(read().requested.value==false);
    queried.ControlMask=PROCESS_POWER_THROTTLING_EXECUTION_SPEED; queried.StateMask=0;
    assert(read().requested.value==false);
    reset(); api.get=nullptr; assert(!read().available.value);
    reset(); get_error=ERROR_ACCESS_DENIED;
    assert(read().available.support==Support::permission_required && !read().requested.value);
    reset(); get_error=ERROR_INVALID_PARAMETER; assert(!read().available.value);
    reset(); queried.Version=999; assert(!read().available.value);
    reset(); assert(set_windows_efficiency(0,true,api).error==ActionError::invalid_argument && !opens);
    assert(set_windows_efficiency(5678,true,api).error==ActionError::invalid_argument && !opens);
    assert(make_system_actions()->set_efficiency_mode_checked(0,0,true).error==ActionError::invalid_argument);
    const auto native=read_windows_efficiency(GetCurrentProcess()); // Read-only; may be unsupported.
    if (native.available.value) assert(native.requested.value.has_value());
    std::cout << "EcoQoS tests passed; no native policy changed; query support="
              << native.available.value.value_or(false) << '\n';
}
