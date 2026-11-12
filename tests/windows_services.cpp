#include "../src/windows_services.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <system_error>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
int manager_token,service_token;
int closes=0,opens=0,page=0,mode=0;
SC_HANDLE manager_handle() { return reinterpret_cast<SC_HANDLE>(&manager_token); }
SC_HANDLE service_handle() { return reinterpret_cast<SC_HANDLE>(&service_token); }
SC_HANDLE WINAPI manager(LPCWSTR machine,LPCWSTR database,DWORD access) {
    assert(!machine && !database);
    assert(access==(SC_MANAGER_CONNECT|SC_MANAGER_ENUMERATE_SERVICE));
    if (mode==1) { SetLastError(ERROR_ACCESS_DENIED); return nullptr; }
    ++opens; return manager_handle();
}
wchar_t* put_string(unsigned char* buffer,std::size_t offset,const std::wstring& text) {
    std::memcpy(buffer+offset,text.c_str(),(text.size()+1)*sizeof(wchar_t));
    return reinterpret_cast<wchar_t*>(buffer+offset);
}
BOOL WINAPI enumerate(SC_HANDLE h,SC_ENUM_TYPE level,DWORD type,DWORD state,LPBYTE buffer,
    DWORD size,LPDWORD needed,LPDWORD returned,LPDWORD resume,LPCWSTR group) {
    assert(h==manager_handle() && level==SC_ENUM_PROCESS_INFO && type==SERVICE_WIN32);
    assert(state==SERVICE_STATE_ALL && !group && size>=1024);
    if (mode==2 || (mode==7 && page==1)) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
    ENUM_SERVICE_STATUS_PROCESSW entry{};
    entry.lpServiceName=put_string(buffer,128,page==0?L"service-a":L"service-b");
    entry.lpDisplayName=put_string(buffer,256,L"Servicio de prueba");
    entry.ServiceStatusProcess.dwCurrentState=page==0?SERVICE_RUNNING:SERVICE_STOPPED;
    entry.ServiceStatusProcess.dwProcessId=999;
    if (mode==8) entry.ServiceStatusProcess.dwCurrentState=SERVICE_START_PENDING;
    if (mode==4) entry.lpServiceName=reinterpret_cast<wchar_t*>(std::uintptr_t(1));
    std::memcpy(buffer,&entry,sizeof(entry));
    *returned=mode==3?size:1;
    *needed=1024;
    if (mode==5) { SetLastError(ERROR_MORE_DATA); return FALSE; } // unchanged resume
    if (page++==0) { *resume=1; SetLastError(ERROR_MORE_DATA); return FALSE; }
    *resume=0; return TRUE;
}
SC_HANDLE WINAPI service(SC_HANDLE h,LPCWSTR id,DWORD access) {
    assert(h==manager_handle() && id && access==SERVICE_QUERY_CONFIG);
    if (mode==6) { SetLastError(ERROR_ACCESS_DENIED); return nullptr; }
    ++opens; return service_handle();
}
BOOL WINAPI config(SC_HANDLE h,LPQUERY_SERVICE_CONFIGW out,DWORD size,LPDWORD) {
    assert(h==service_handle() && size>=1024);
    auto* buffer=reinterpret_cast<unsigned char*>(out);
    QUERY_SERVICE_CONFIGW record{};
    record.lpLoadOrderGroup=put_string(buffer,128,L"TestGroup");
    std::memcpy(out,&record,sizeof(record)); return TRUE;
}
BOOL WINAPI config2(SC_HANDLE h,DWORD type,LPBYTE buffer,DWORD size,LPDWORD) {
    assert(h==service_handle() && type==SERVICE_CONFIG_DESCRIPTION && size>=1024);
    SERVICE_DESCRIPTIONW record{};
    record.lpDescription=put_string(buffer,128,L"Descripci\u00f3n");
    std::memcpy(buffer,&record,sizeof(record)); return TRUE;
}
BOOL WINAPI close_handle(SC_HANDLE h) {
    assert(h==manager_handle() || h==service_handle()); ++closes; return TRUE;
}
ServicesApi api() { return {manager,enumerate,service,config,config2,close_handle}; }
void start(int next_mode) { mode=next_mode; closes=opens=page=0; }
}
int main() {
    start(0);
    auto result=read_windows_services(api());
    assert(result.size()==2 && result[0].id=="service-a" && result[1].id=="service-b");
    assert(result[0].status=="running" && result[0].pid==999);
    assert(result[1].status=="stopped" && !result[1].pid);
    assert(result[0].group=="TestGroup" && result[0].description=="Descripci\xc3\xb3n");
    assert(opens==closes && opens==3);
    for (int error_mode : {1,2,3,4,5,7}) {
        start(error_mode); bool threw=false;
        try { (void)read_windows_services(api()); } catch (const std::system_error&) { threw=true; }
        assert(threw && opens==closes);
    }
    start(6); result=read_windows_services(api());
    assert(result.size()==2 && result[0].group.empty() && result[0].description.empty());
    assert(opens==closes);
    start(8); result=read_windows_services(api());
    assert(!result[0].pid && result[0].status=="start_pending" && opens==closes);
    const char* states[]{"unknown","stopped","start_pending","stop_pending","running",
        "continue_pending","pause_pending","paused"};
    for (DWORD state=0;state<8;++state) assert(windows_service_state(state)==states[state]);
    assert(windows_service_state(99)=="unknown");
    // Live, read-only integration through the public API.
    Monitor monitor;
    SampleOptions options;
    options.include_processes=false;
    assert(monitor.sample(options).services.empty());
    options.include_services=true;
    const auto snapshot=monitor.sample(options);
    assert(!snapshot.services.empty());
    std::set<std::string> ids;
    for (const auto& s:snapshot.services) {
        assert(!s.id.empty() && ids.insert(s.id).second);
        if (s.status=="stopped") assert(!s.pid);
    }
    std::cout << "Read-only SCM inventory: " << snapshot.services.size() << " services\n";
}
