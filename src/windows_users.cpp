#include "windows_users.hpp"
#ifdef _WIN32
#include <algorithm>
#include <set>
#include <system_error>
namespace pcinsight::detail {
namespace {
[[noreturn]] void fail(DWORD error,const char* stage){
    throw std::system_error(static_cast<int>(error?error:ERROR_INVALID_DATA),std::system_category(),stage);
}
struct Buffer {
    void* data;decltype(&WTSFreeMemory) free;
    ~Buffer(){if(data)free(data);}
};
std::string query_text(DWORD id,WTS_INFO_CLASS kind,const WindowsUsersApi& api){
    LPWSTR text=nullptr;DWORD bytes=0;
    const auto ok=api.query(WTS_CURRENT_SERVER_HANDLE,id,kind,&text,&bytes);
    const auto error=ok?ERROR_SUCCESS:GetLastError();Buffer cleanup{text,api.free};
    if(!ok)fail(error,"WTS session identity query failed; no partial inventory");
    if(!text || bytes<sizeof(wchar_t) || bytes>4097*sizeof(wchar_t) || bytes%sizeof(wchar_t))fail(ERROR_INVALID_DATA,"WTS invalid identity buffer");
    const auto end=std::find(text,text+bytes/sizeof(wchar_t),L'\0');
    if(end==text+bytes/sizeof(wchar_t))fail(ERROR_INVALID_DATA,"WTS unterminated identity");
    const auto length=static_cast<int>(end-text);if(!length)return {};
    const auto size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text,length,nullptr,0,nullptr,nullptr);
    if(!size)fail(GetLastError(),"WTS invalid UTF-16 identity");
    std::string out(size,'\0');
    if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text,length,out.data(),size,nullptr,nullptr)!=size)fail(GetLastError(),"WTS identity conversion failed");
    return out;
}
}
std::string windows_session_state(WTS_CONNECTSTATE_CLASS state){
    switch(state){
    case WTSActive:return "active";
    case WTSConnected:return "connected";
    case WTSConnectQuery:return "connect_query";
    case WTSShadow:return "shadow";
    case WTSDisconnected:return "disconnected";
    case WTSIdle:return "idle";
    case WTSListen:return "listen";
    case WTSReset:return "reset";
    case WTSDown:return "down";
    case WTSInit:return "init";
    default:return "unknown("+std::to_string(static_cast<unsigned>(state))+")";
    }
}
std::vector<UserSession> read_windows_users(WindowsUsersApi api){
    if(!api.enumerate || !api.query || !api.free)fail(ERROR_NOT_SUPPORTED,"Incomplete WTS session API");
    PWTS_SESSION_INFOW entries=nullptr;DWORD count=0;
    const auto ok=api.enumerate(WTS_CURRENT_SERVER_HANDLE,0,1,&entries,&count);
    const auto error=ok?ERROR_SUCCESS:GetLastError();Buffer cleanup{entries,api.free};
    if(!ok)fail(error,"WTS session enumeration failed; no partial inventory");
    if(count>10000 || (count && !entries))fail(ERROR_INVALID_DATA,"WTS excessive or invalid session inventory");
    std::vector<UserSession> result;std::set<DWORD> ids;std::size_t payload=0;
    for(DWORD i=0;i<count;++i){
        const auto& entry=entries[i];
        if(entry.SessionId==WTS_CURRENT_SESSION || !ids.insert(entry.SessionId).second)fail(ERROR_INVALID_DATA,"WTS duplicate/reserved session ID");
        // A listener is not a login. Other states are retained if an account is reported.
        if(entry.State==WTSListen)continue;
        const auto user=query_text(entry.SessionId,WTSUserName,api);
        if(user.empty())continue; // No reported account, not an invented anonymous user.
        const auto domain=query_text(entry.SessionId,WTSDomainName,api);
        const auto account=domain.empty()?user:domain+"\\"+user;
        if(account.size()+user.size()>16*1024*1024-payload)fail(ERROR_INVALID_DATA,"WTS identity payload exceeds 16 MiB");
        payload+=account.size()+user.size();
        UserSession row;row.id="wts:"+std::to_string(entry.SessionId);row.username=account;
        row.display_name=user;row.status=windows_session_state(entry.State);
        row.resource_scope="windows-wts-inventory-only";
        row.cpu_percent.note="WTS inventory has no process/session CPU accounting";
        row.memory_bytes.note="WTS inventory has no process/session memory accounting";
        row.disk_bytes_per_second.note="WTS inventory has no process/session disk accounting";
        row.network_bytes_per_second.note="WTS inventory has no process/session network accounting";
        row.disconnect_available={false,Support::available,"WTS inventory only; no disconnect action provider implemented"};
        row.logout_available={false,Support::available,"WTS inventory only; no logout action provider or stable logout token implemented"};
        result.push_back(std::move(row));
    }
    std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.id<b.id;});
    return result;
}
}
#endif
