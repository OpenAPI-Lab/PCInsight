#include "windows_services.hpp"
#ifdef _WIN32
#include <algorithm>
#include <cstring>
#include <set>
#include <system_error>

namespace pcinsight::detail {
namespace {
struct ServiceHandle {
    SC_HANDLE value;
    decltype(&CloseServiceHandle) close;
    ServiceHandle(SC_HANDLE v,decltype(close) c) : value(v),close(c) {}
    ~ServiceHandle() { if (value) close(value); }
    ServiceHandle(const ServiceHandle&)=delete;
    ServiceHandle& operator=(const ServiceHandle&)=delete;
};
[[noreturn]] void fail(DWORD error,const char* operation) {
    throw std::system_error(static_cast<int>(error),std::system_category(),operation);
}
std::optional<std::wstring> bounded_string(const std::vector<unsigned char>& buffer,const wchar_t* ptr) {
    if (!ptr) return std::wstring{};
    const auto begin=reinterpret_cast<std::uintptr_t>(buffer.data());
    const auto address=reinterpret_cast<std::uintptr_t>(ptr);
    if (address<begin || address-begin>=buffer.size()) return std::nullopt;
    auto offset=static_cast<std::size_t>(address-begin);
    std::wstring result;
    for (;offset+sizeof(wchar_t)<=buffer.size();offset+=sizeof(wchar_t)) {
        wchar_t ch;
        std::memcpy(&ch,buffer.data()+offset,sizeof(ch));
        if (!ch) return result;
        if (result.size()>=32768) return std::nullopt;
        result.push_back(ch);
    }
    return std::nullopt;
}
std::string utf8(const std::wstring& text) {
    if (text.empty()) return {};
    const auto size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
    if (!size) return {};
    std::string result(size,'\0');
    WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),result.data(),size,nullptr,nullptr);
    return result;
}
void metadata(SC_HANDLE manager,const std::wstring& id,ServiceInfo& out,const ServicesApi& api) {
    ServiceHandle handle(api.service(manager,id.c_str(),SERVICE_QUERY_CONFIG),api.close);
    if (!handle.value) return; // preserve enumerated status when config is inaccessible
    std::vector<unsigned char> buffer(8192);
    DWORD needed=0;
    if (api.config(handle.value,reinterpret_cast<QUERY_SERVICE_CONFIGW*>(buffer.data()),
                   static_cast<DWORD>(buffer.size()),&needed)) {
        QUERY_SERVICE_CONFIGW config{};
        std::memcpy(&config,buffer.data(),sizeof(config));
        const auto group=bounded_string(buffer,config.lpLoadOrderGroup);
        if (group) out.group=utf8(*group);
    }
    std::fill(buffer.begin(),buffer.end(),std::uint8_t{0});
    if (api.config2(handle.value,SERVICE_CONFIG_DESCRIPTION,buffer.data(),
                    static_cast<DWORD>(buffer.size()),&needed)) {
        SERVICE_DESCRIPTIONW description{};
        std::memcpy(&description,buffer.data(),sizeof(description));
        const auto text=bounded_string(buffer,description.lpDescription);
        if (text) out.description=utf8(*text);
    }
}
}
std::string windows_service_state(DWORD state) {
    switch(state) {
        case SERVICE_STOPPED: return "stopped";
        case SERVICE_START_PENDING: return "start_pending";
        case SERVICE_STOP_PENDING: return "stop_pending";
        case SERVICE_RUNNING: return "running";
        case SERVICE_CONTINUE_PENDING: return "continue_pending";
        case SERVICE_PAUSE_PENDING: return "pause_pending";
        case SERVICE_PAUSED: return "paused";
        default: return "unknown";
    }
}
std::vector<ServiceInfo> read_windows_services(ServicesApi api) {
    ServiceHandle manager(api.manager(nullptr,nullptr,SC_MANAGER_CONNECT|SC_MANAGER_ENUMERATE_SERVICE),api.close);
    if (!manager.value) fail(GetLastError(),"OpenSCManagerW");
    std::vector<unsigned char> buffer(256*1024);
    std::vector<ServiceInfo> out;
    std::set<std::string> seen;
    DWORD resume=0;
    for (unsigned page=0;page<1024;++page) {
        DWORD needed=0,returned=0;
        const auto previous_resume=resume;
        const bool success=api.enumerate(manager.value,SC_ENUM_PROCESS_INFO,SERVICE_WIN32,
            SERVICE_STATE_ALL,buffer.data(),static_cast<DWORD>(buffer.size()),&needed,&returned,&resume,nullptr)!=FALSE;
        const auto error=success?ERROR_SUCCESS:GetLastError();
        if (!success && error!=ERROR_MORE_DATA) fail(error,"EnumServicesStatusExW");
        if (returned>buffer.size()/sizeof(ENUM_SERVICE_STATUS_PROCESSW)) fail(ERROR_INVALID_DATA,"Service count exceeds buffer");
        for (DWORD i=0;i<returned;++i) {
            ENUM_SERVICE_STATUS_PROCESSW record{};
            std::memcpy(&record,buffer.data()+i*sizeof(record),sizeof(record));
            const auto id=bounded_string(buffer,record.lpServiceName);
            const auto name=bounded_string(buffer,record.lpDisplayName);
            if (!id || id->empty() || !name) fail(ERROR_INVALID_DATA,"Invalid service name buffer");
            ServiceInfo s;
            s.id=utf8(*id); s.name=utf8(*name);
            if (s.id.empty()) fail(ERROR_INVALID_DATA,"Invalid service identifier encoding");
            if (!seen.insert(s.id).second) continue;
            s.status=windows_service_state(record.ServiceStatusProcess.dwCurrentState);
            const auto state=record.ServiceStatusProcess.dwCurrentState;
            // Pending start/stop PIDs are not guaranteed valid by QueryServiceStatusEx.
            if ((state==SERVICE_RUNNING || state==SERVICE_PAUSED || state==SERVICE_PAUSE_PENDING ||
                 state==SERVICE_CONTINUE_PENDING) && record.ServiceStatusProcess.dwProcessId)
                s.pid=record.ServiceStatusProcess.dwProcessId;
            metadata(manager.value,*id,s,api);
            out.push_back(std::move(s));
            if (out.size()>100000) fail(ERROR_INVALID_DATA,"Service inventory limit exceeded");
        }
        if (success) return out;
        if (!returned || resume==previous_resume) fail(ERROR_INVALID_DATA,"Service enumeration made no progress");
    }
    fail(ERROR_INVALID_DATA,"Service enumeration page limit exceeded");
}
}
#endif
