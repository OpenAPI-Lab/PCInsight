#include "linux_startup_session.hpp"
#include "linux_service_actions.hpp"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#ifdef __linux__
#include <dlfcn.h>
#endif
namespace pcinsight::detail {
namespace {
Metric<std::string> unknown(const char* note) { return {std::nullopt,Support::unsupported,note}; }
#ifdef __linux__
Metric<std::string> error(int rc,const ActionBusError& e) {
    if(rc==-EACCES || rc==-EPERM || (e.name && std::strcmp(e.name,"org.freedesktop.DBus.Error.AccessDenied")==0))
        return {std::nullopt,Support::permission_required,"GNOME SessionName access denied"};
    return unknown("GNOME SessionName/bus/service unavailable or query failed");
}
bool owner_valid(const char* s) {
    if(!s || strnlen(s,256)>255 || s[0]!=':') return false;
    bool dot=false,digit=false;
    for(++s;*s;++s) {
        if(*s=='.' && digit && !dot) {dot=true;digit=false;}
        else if(*s>='0' && *s<='9') digit=true;
        else return false;
    }
    return dot && digit;
}
#endif
}
Metric<std::string> read_native_gnome_session_name(const std::string& service) {
#ifdef __linux__
    if(service.empty() || service.size()>255 || service.find('\0')!=service.npos)
        return unknown("invalid session manager service name");
    // Never allow an exec/autolaunch transport supplied through environment.
    if(const auto* address=std::getenv("DBUS_SESSION_BUS_ADDRESS")) {
        if(strnlen(address,32769)>32768 || !std::string_view(address).starts_with("unix:") || std::strchr(address,';'))
            return unknown("session bus requires a single local unix address; no autolaunch/exec transport");
    }
    auto api=load_systemd_action_api();
    if(!api) return unknown("libsystemd session bus API unavailable");
    const auto open_user=reinterpret_cast<int(*)(ActionBus**)>(dlsym(api.library.get(),"sd_bus_open_user"));
    const auto enter=reinterpret_cast<int(*)(ActionMessage*,char,const char*)>(dlsym(api.library.get(),"sd_bus_message_enter_container"));
    const auto exit=reinterpret_cast<int(*)(ActionMessage*)>(dlsym(api.library.get(),"sd_bus_message_exit_container"));
    if(!open_user || !enter || !exit) return unknown("session bus symbols unavailable");
    struct Bus {const SystemdActionApi& api;ActionBus* p{};~Bus(){if(p)api.unref(p);}} bus{api};
    const auto rc=open_user(&bus.p);
    if(rc<0 || !bus.p) return error(rc,{});
    if(api.timeout(bus.p,2000000)<0) return unknown("session bus timeout setup failed");
    struct Reply {
        const SystemdActionApi& api;ActionMessage* p{};ActionBusError e{};
        ~Reply(){if(p)api.message_unref(p);api.error_free(&e);}
    } owner{api};
    auto status=api.call(bus.p,"org.freedesktop.DBus","/org/freedesktop/DBus","org.freedesktop.DBus",
        "GetNameOwner",&owner.e,&owner.p,"s",service.c_str());
    if(status<0) return error(status,owner.e);
    const char* unique{};
    if(!owner.p || api.read(owner.p,"s",&unique)<=0 || !owner_valid(unique)) return unknown("invalid session manager owner reply");
    // A unique destination cannot activate a missing well-known service.
    Reply reply{api};
    status=api.call(bus.p,unique,"/org/gnome/SessionManager","org.freedesktop.DBus.Properties",
        "Get",&reply.e,&reply.p,"ss","org.gnome.SessionManager","SessionName");
    if(status<0) return error(status,reply.e);
    const char* name{};
    if(!reply.p || enter(reply.p,'v',"s")<=0 || api.read(reply.p,"s",&name)<=0 || exit(reply.p)<0 || !name)
        return unknown("invalid SessionName property reply");
    const auto length=strnlen(name,1025);
    if(!length || length>1024) return unknown("empty/excessive SessionName");
    for(std::size_t i=0;i<length;++i) if(static_cast<unsigned char>(name[i])<32 || name[i]==127)
        return unknown("SessionName contains control characters");
    return {std::string(name,length),Support::available,"SessionName reported by current caller bus manager; sampled, not launch guarantee"};
#else
    (void)service;return unknown("native GNOME SessionName requires Linux");
#endif
}
}
