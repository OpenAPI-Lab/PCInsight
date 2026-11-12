#include "../src/windows_users.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <system_error>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
int mode=0,queries=0,enumerations=0;std::set<void*> buffers;
void* allocate(std::size_t bytes){auto* p=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,bytes);assert(p);buffers.insert(p);return p;}
void WINAPI release(PVOID p){assert(buffers.erase(p)==1);assert(HeapFree(GetProcessHeap(),0,p));}
BOOL WINAPI enumerate(HANDLE server,DWORD reserved,DWORD version,PWTS_SESSION_INFOW* out,DWORD* count){
    assert(server==WTS_CURRENT_SERVER_HANDLE && reserved==0 && version==1);++enumerations;
    if(mode==1){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    if(mode==2){*count=10001;return TRUE;}
    if(mode==3){*count=1;return TRUE;}
    if(mode==4){*count=0;return TRUE;}
    *out=static_cast<PWTS_SESSION_INFOW>(allocate(5*sizeof(WTS_SESSION_INFOW)));*count=5;
    (*out)[0].SessionId=1;(*out)[0].State=WTSActive;
    (*out)[1].SessionId=2;(*out)[1].State=WTSDisconnected;
    (*out)[2].SessionId=3;(*out)[2].State=WTSConnected; // Empty account.
    (*out)[3].SessionId=4;(*out)[3].State=WTSListen; // Never query a listener.
    (*out)[4].SessionId=5;(*out)[4].State=WTSIdle;
    if(mode==5)(*out)[1].SessionId=1;
    if(mode==6)(*out)[1].SessionId=WTS_CURRENT_SESSION;
    if(mode==7){SetLastError(ERROR_ACCESS_DENIED);return FALSE;} // Free even on failure.
    return TRUE;
}
BOOL WINAPI query(HANDLE server,DWORD id,WTS_INFO_CLASS kind,LPWSTR* out,DWORD* bytes){
    assert(server==WTS_CURRENT_SERVER_HANDLE && id!=4 && (kind==WTSUserName || kind==WTSDomainName));++queries;
    if(mode==8 && id==2){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    if(mode==9 && kind==WTSDomainName){SetLastError(ERROR_CTX_WINSTATION_NOT_FOUND);return FALSE;}
    if(mode==10){*bytes=sizeof(wchar_t);return TRUE;}
    std::wstring value=kind==WTSDomainName?(id==5?L"":L"DOMAIN"):(id==3?L"":id==5?L"Jos\u00e9":L"alice");
    *bytes=static_cast<DWORD>((value.size()+1)*sizeof(wchar_t));
    *out=static_cast<LPWSTR>(allocate(*bytes));std::memcpy(*out,value.c_str(),*bytes);
    if(mode==11)*bytes=1;
    if(mode==12){(*out)[value.size()]=L'x';}
    if(mode==13)*bytes=4098*sizeof(wchar_t);
    if(mode==14){(*out)[0]=static_cast<wchar_t>(0xd800);(*out)[1]=0;*bytes=2*sizeof(wchar_t);}
    if(mode==15){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    return TRUE;
}
WindowsUsersApi api(){return {enumerate,query,release};}
void start(int m){assert(buffers.empty());mode=m;queries=enumerations=0;}
}
int main(){
    start(0);const auto rows=read_windows_users(api());assert(buffers.empty() && enumerations==1 && queries==7 && rows.size()==3);
    assert(rows[0].id=="wts:1" && rows[0].username=="DOMAIN\\alice" && rows[0].display_name=="alice" && rows[0].status=="active");
    assert(rows[1].username==rows[0].username && rows[1].status=="disconnected"); // Keep both sessions.
    assert(rows[2].username=="Jos\xc3\xa9" && rows[2].status=="idle");
    for(const auto& row:rows){assert(row.resource_scope=="windows-wts-inventory-only" && !row.cpu_percent.value && !row.memory_bytes.value && !row.disk_bytes_per_second.value && !row.network_bytes_per_second.value && row.logout_token.empty() && row.logout_available.value==false && row.disconnect_available.value==false);}
    const char* states[]{"active","connected","connect_query","shadow","disconnected","idle","listen","reset","down","init"};
    for(unsigned k=0;k<10;++k)assert(windows_session_state(static_cast<WTS_CONNECTSTATE_CLASS>(k))==states[k]);
    assert(windows_session_state(static_cast<WTS_CONNECTSTATE_CLASS>(99))=="unknown(99)");
    for(int m:{1,2,3,5,6,7,8,9,10,11,12,13,14,15}){
        start(m);bool failed=false;try{(void)read_windows_users(api());}catch(const std::system_error& error){failed=true;assert(error.code().value()!=0);}
        assert(failed && buffers.empty()); // No partial list or leaked WTS allocations.
    }
    start(4);assert(read_windows_users(api()).empty() && queries==0 && buffers.empty());
    start(0);auto incomplete=api();incomplete.query=nullptr;bool failed=false;
    try{(void)read_windows_users(incomplete);}catch(const std::system_error&){failed=true;}assert(failed && enumerations==0 && buffers.empty());
    // Live read-only inventory and opt-in public Monitor path. No disconnect/logout.
    const auto native=read_windows_users();DWORD session=0;assert(ProcessIdToSessionId(GetCurrentProcessId(),&session));
    const auto own="wts:"+std::to_string(session);std::set<std::string> ids;bool own_seen=false;
    for(const auto& row:native){assert(!row.username.empty() && ids.insert(row.id).second);own_seen|=row.id==own;}
    assert(own_seen);
    Monitor monitor;SampleOptions options;options.include_processes=false;options.include_gpus=false;options.include_disks=false;options.include_networks=false;
    assert(monitor.sample(options).user_sessions.empty());options.include_user_sessions=true;
    auto snapshot=monitor.sample(options);assert(snapshot.processes.empty() && !snapshot.user_sessions.empty());
    own_seen=false;for(const auto& row:snapshot.user_sessions){if(row.id==own){own_seen=true;assert(!row.cpu_percent.value && row.memory_bytes.value && row.memory_bytes.support==Support::estimated);}assert(row.resource_scope=="windows-visible-processes-by-wts-session-and-account" && !row.disk_bytes_per_second.value && !row.network_bytes_per_second.value);}
    assert(own_seen);
    snapshot=monitor.sample(options);own_seen=false;for(const auto& row:snapshot.user_sessions)if(row.id==own){own_seen=true;assert(row.cpu_percent.value && row.cpu_percent.support==Support::estimated);}
    assert(own_seen && snapshot.processes.empty());
    options.include_user_sessions=false;assert(monitor.sample(options).user_sessions.empty());
    options.include_user_sessions=true;snapshot=monitor.sample(options);for(const auto& row:snapshot.user_sessions)assert(!row.cpu_percent.value); // Reset after opt-out without processes.
    options.include_processes=true;snapshot=monitor.sample(options);bool process_seen=false;
    for(const auto& p:snapshot.processes)if(p.pid==GetCurrentProcessId()){process_seen=true;assert(p.windows_session_id.value==session && p.windows_session_id.support==Support::available);}
    assert(process_seen);
    std::cout<<"WTS users: injected errors/cleanup, native current session and Monitor opt-in passed\n";
}
