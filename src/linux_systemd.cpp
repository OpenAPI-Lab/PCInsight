#include "linux_systemd.hpp"
#include "linux_service_actions.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <set>
#include <limits>
#ifdef __linux__
#include <dlfcn.h>
#include <unistd.h>
#include <sys/socket.h>
#endif

namespace pcinsight::detail {
namespace {
constexpr std::size_t max_units = 10000;
constexpr std::size_t max_payload=16*1024*1024;
void valid_text(const std::string& value) {
    if (value.size() > 65536 || value.find('\0') != std::string::npos)
        throw std::runtime_error("systemd: cadena inválida o excesiva");
}
bool service_name(const std::string& value) {
    valid_text(value);
    if (!value.ends_with(".service")) return false;
    if (!valid_systemd_service_name(value))
        throw std::runtime_error("systemd: identificador de servicio inválido");
    return true;
}
void account(std::size_t& payload,const std::string& s){valid_text(s);if(s.size()>max_payload-payload)throw std::runtime_error("systemd: payload exceeds16MiB");payload+=s.size();}
bool object_path(const std::string& s){
    constexpr std::string_view prefix="/org/freedesktop/systemd1/unit/";
    if(!s.starts_with(prefix)||s.size()<=prefix.size()||s.size()>4096||s.substr(prefix.size())=="self")return false;
    return s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_",prefix.size())==s.npos;
}
bool state_token(const std::string& s){return !s.empty()&&s.size()<=128&&s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")==s.npos;}
bool slice_name(const std::string& s){
    if(s.size()<=6||s.size()>255||!s.ends_with(".slice"))return false;
    const auto hex=[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');};
    for(std::size_t i=0;i<s.size();++i){const auto c=s[i];if(c=='\\'){
        if(i+3>=s.size()||s[i+1]!='x'||!hex(s[i+2])||!hex(s[i+3]))return false;
        i+=3;
    }else if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'||c==':'))return false;}
    return true;
}
template<class T> Metric<T> absent(std::string note,Support s=Support::unsupported){return {{},s==Support::permission_required?s:Support::unsupported,std::move(note)};}
Metric<std::string> token_metric(const std::string& s,const char* source){return state_token(s)?Metric<std::string>{s,Support::available,source}:absent<std::string>(std::string(source)+": unavailable/invalid token");}
LinuxServiceDetails defaults(){
    LinuxServiceDetails d;d.provider="systemd-system-manager";d.group_kind="systemd-slice";
    d.name=d.description=d.status=d.load_state=d.active_state=d.sub_state=d.unit_file_state=d.slice=absent<std::string>("No corresponding systemd metadata observed");
    d.main_pid=absent<std::uint32_t>("No verified MainPID property available");return d;
}
}
std::vector<ServiceInfo> read_systemd_services(const SystemdServiceReader& reader) {
    if (!reader.loaded || !reader.installed) throw std::invalid_argument("systemd: lector incompleto");
    // Complete both enumerations before exposing rows; failed queries don't masquerade as success.
    const auto loaded = reader.loaded();
    const auto installed = reader.installed();
    if (loaded.size() > max_units || installed.size() > max_units)
        throw std::runtime_error("systemd: inventario excesivo");
    std::map<std::string, ServiceInfo> rows;
    std::set<std::string> paths;std::map<std::string,std::string> file_states;std::size_t payload=0;
    for (const auto& unit : loaded) {
        for(const auto* s:{&unit.name,&unit.description,&unit.load,&unit.active,&unit.sub,&unit.object_path})account(payload,*s);
        if (!service_name(unit.name)) continue;
        for (const auto* s : {&unit.description, &unit.load, &unit.active, &unit.sub, &unit.object_path}) valid_text(*s);
        if (!object_path(unit.object_path)||!paths.insert(unit.object_path).second)
            throw std::runtime_error("systemd: ruta de unidad inválida");
        ServiceInfo row;
        row.id = row.name = unit.name;
        row.description = unit.description;
        auto d=defaults();d.name={unit.name,Support::available,"ListUnits primary unit name"};
        d.description={unit.description,Support::available,"ListUnits description, including a known empty description"};
        d.load_state=token_metric(unit.load,"ListUnits LoadState");d.active_state=token_metric(unit.active,"ListUnits ActiveState");d.sub_state=token_metric(unit.sub,"ListUnits SubState");
        row.load_state=d.load_state.value.value_or("");row.active_state=d.active_state.value.value_or("");row.sub_state=d.sub_state.value.value_or("");
        row.status="unknown";
        if(metric_is_known(d.active_state)&&metric_is_known(d.sub_state)){
            row.status=unit.active=="active"&&unit.sub=="running"?"running":unit.active+"/"+unit.sub;
            d.status={row.status,Support::estimated,"Display mapping of ListUnits ActiveState/SubState, not a live process check; enabled does not mean running"};
        }
        const auto before=reader.unit_id?reader.unit_id(unit.object_path):absent<std::string>("No Unit.Id reader");
        if(metric_is_known(before)&&before.value==unit.name){
            if(reader.main_pid)d.main_pid=reader.main_pid(unit.object_path);
            if(reader.slice)d.slice=reader.slice(unit.object_path);
            const auto after=reader.unit_id(unit.object_path);
            if(!metric_is_known(after)||after.value!=unit.name){
                d.main_pid=absent<std::uint32_t>("Unit.Id unavailable/changed during property queries",after.support);
                d.slice=absent<std::string>("Unit.Id unavailable/changed during property queries",after.support);
            }
        }else{
            d.main_pid=absent<std::uint32_t>("Unit.Id unavailable/mismatched before property queries",before.support);
            d.slice=absent<std::string>("Unit.Id unavailable/mismatched before property queries",before.support);
        }
        if(!metric_is_known(d.main_pid))d.main_pid=absent<std::uint32_t>(d.main_pid.note,d.main_pid.support);
        else if(*d.main_pid.value>static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))d.main_pid=absent<std::uint32_t>("MainPID exceeds Linux signed PID range");
        if(!metric_is_known(d.slice)||(!d.slice.value->empty()&&!slice_name(*d.slice.value)))d.slice=absent<std::string>("Slice unavailable/invalid; not a provider label or Unix credential Group",d.slice.support);
        else d.slice.note=d.slice.value->empty()?"Service.Slice returned an empty string: no named Slice reported; not an inaccessible property":
            "Service.Slice resource-control grouping; not a Windows load-order/svchost group or Unix credential Group";
        if(d.main_pid.value&&*d.main_pid.value)row.pid=*d.main_pid.value;
        row.group=d.slice.value.value_or("");
        row.linux_details={std::move(d),Support::available,"System manager enumeration with per-field availability; ListUnits and property reads are not atomic"};
        if (!rows.emplace(row.id, std::move(row)).second)
            throw std::runtime_error("systemd: identidad cargada duplicada");
    }
    for (const auto& file : installed) {
        account(payload,file.path);account(payload,file.state);
        const auto slash = file.path.find_last_of('/');
        auto name = slash == std::string::npos ? file.path : file.path.substr(slash + 1);
        if (!service_name(name)) continue;
        const auto [state_it,state_added]=file_states.emplace(name,file.state);
        if(!state_added&&state_it->second!=file.state)throw std::runtime_error("systemd: contradictory installed state");
        auto [it, added] = rows.try_emplace(name);
        auto& row = it->second;
        if (!row.unit_file_state.empty() && row.unit_file_state != file.state)
            throw std::runtime_error("systemd: estado de archivo contradictorio");
        if (added) {
            row.id = row.name = name;
            row.status = "unknown"; // enabled/disabled/masked is NOT an execution state
            row.load_state = "not_loaded";
            auto d=defaults();d.name={name,Support::available,"ListUnitFiles basename; aliases/templates not resolved or loaded"};
            d.load_state={"not_loaded",Support::estimated,"Not observed in earlier ListUnits response; not a native LoadState property or proof of stopped"};
            row.linux_details={std::move(d),Support::available,"Installed unit file only; no queries that deliberately load/start this unit"};
        }
        auto& d=*row.linux_details.value;d.unit_file_state=token_metric(file.state,"ListUnitFiles state; installation policy, not execution state");
        row.unit_file_state=d.unit_file_state.value.value_or("");
    }
    std::vector<ServiceInfo> result;
    for (auto& [id, row] : rows) result.push_back(std::move(row));
    return result;
}
void apply_linux_service_file_metadata(ServiceInfo& row,bool sysv){
    auto d=defaults();d.provider=sysv?"sysv-file-scan":"systemd-file-scan";d.group_kind.clear();
    d.name={row.name,Support::estimated,"Filename in partial init-file scan, not verified by a running manager"};
    d.description=absent<std::string>("Effective description not resolved by this partial file scan");
    d.status=absent<std::string>("Execution state cannot be inferred from an init/unit file");
    d.slice=absent<std::string>("File scan does not resolve effective Slice grouping");
    row.group.clear();row.description.clear();row.status="unknown";row.pid.reset();
    row.load_state.clear();row.active_state.clear();row.sub_state.clear();row.unit_file_state.clear();
    row.linux_details={std::move(d),Support::estimated,"Partial file inventory only; not an init-manager enumeration or control provider"};
}

#ifdef __linux__
namespace {
struct Bus; struct Message;
struct BusError {const char* name{};const char* message{};int need_free{};};
class NativeBus {
    void* library_{};
public:
    Bus* bus{};
    std::string owner;
    int error{-ENOSYS};
    int (*open)(Bus**){};
    Bus* (*unref)(Bus*){};
    Message* (*message_unref)(Message*){};
    int (*call)(Bus*, const char*, const char*, const char*, const char*, BusError*, Message**, const char*, ...){};
    int (*enter)(Message*, char, const char*){};
    int (*exit)(Message*){};
    int (*read)(Message*, const char*, ...){};
    int (*property)(Bus*, const char*, const char*, const char*, const char*, BusError*, char, void*){};
    int (*timeout)(Bus*, std::uint64_t){};
    int (*interactive)(Bus*,int){};
    void (*error_free)(BusError*){};
    NativeBus(LinuxServiceScope scope) {
        library_ = dlopen("libsystemd.so.0", RTLD_NOW | RTLD_LOCAL);
        if (!library_) return;
#define PCINSIGHT_BUS_SYMBOL(member, symbol) member = reinterpret_cast<decltype(member)>(dlsym(library_, symbol))
        PCINSIGHT_BUS_SYMBOL(open, "sd_bus_open_system");
        PCINSIGHT_BUS_SYMBOL(unref, "sd_bus_unref");
        PCINSIGHT_BUS_SYMBOL(message_unref, "sd_bus_message_unref");
        PCINSIGHT_BUS_SYMBOL(call, "sd_bus_call_method");
        PCINSIGHT_BUS_SYMBOL(enter, "sd_bus_message_enter_container");
        PCINSIGHT_BUS_SYMBOL(exit, "sd_bus_message_exit_container");
        PCINSIGHT_BUS_SYMBOL(read, "sd_bus_message_read");
        PCINSIGHT_BUS_SYMBOL(property, "sd_bus_get_property_trivial");
        PCINSIGHT_BUS_SYMBOL(timeout, "sd_bus_set_method_call_timeout");
        PCINSIGHT_BUS_SYMBOL(error_free,"sd_bus_error_free");
        PCINSIGHT_BUS_SYMBOL(interactive,"sd_bus_set_allow_interactive_authorization");
#undef PCINSIGHT_BUS_SYMBOL
        if (!open || !unref || !message_unref || !call || !enter || !exit || !read || !property || !timeout||!error_free||!interactive) return;
        if(scope==LinuxServiceScope::current_user){
            // Never honor DBUS_SESSION_BUS_ADDRESS/XDG_RUNTIME_DIR: no remote bus,
            // arbitrary UID or autolaunch fallback is part of this API.
            auto create=reinterpret_cast<int(*)(Bus**)>(dlsym(library_,"sd_bus_new"));
            auto address=reinterpret_cast<int(*)(Bus*,const char*)>(dlsym(library_,"sd_bus_set_address"));
            auto client=reinterpret_cast<int(*)(Bus*,int)>(dlsym(library_,"sd_bus_set_bus_client"));
            auto start=reinterpret_cast<int(*)(Bus*)>(dlsym(library_,"sd_bus_start"));
            auto fd=reinterpret_cast<int(*)(Bus*)>(dlsym(library_,"sd_bus_get_fd"));
            if(!create||!address||!client||!start||!fd)return;
            const auto destination="unix:path=/run/user/"+std::to_string(geteuid())+"/bus";
            error=create(&bus);
            if(error>=0)error=address(bus,destination.c_str());
            if(error>=0)error=client(bus,1);
            if(error>=0)error=timeout(bus,5000000);
            if(error>=0)error=interactive(bus,0);
            if(error>=0)error=start(bus);
            if(error>=0){
                const int socket=fd(bus);struct ucred peer{};socklen_t size=sizeof(peer);
                if(socket<0)error=socket;
                else if(getsockopt(socket,SOL_SOCKET,SO_PEERCRED,&peer,&size)<0)error=-errno;
                else if(size!=sizeof(peer)||peer.uid!=geteuid())error=-EACCES;
            }
        }else error=open(&bus);
        if (error < 0) { if (bus) unref(bus); bus = nullptr; return; }
        if (timeout(bus, 5000000) < 0||interactive(bus,0)<0) { unref(bus); bus = nullptr; }
    }
    ~NativeBus() { if (bus) unref(bus); if (library_) dlclose(library_); }
    NativeBus(const NativeBus&) = delete;
    NativeBus& operator=(const NativeBus&) = delete;
};
void check(int result, const char* operation) {
    if (result < 0) throw std::system_error(-result, std::generic_category(), operation);
}
std::string bounded(const char* value) {
    if (!value) throw std::runtime_error("systemd: cadena nula");
    const auto size = strnlen(value, 65537);
    if (size > 65536) throw std::runtime_error("systemd: cadena excesiva");
    return {value, size};
}
struct Reply {
    NativeBus& api; Message* message{};
    BusError error{};
    ~Reply() { if (message) api.message_unref(message);api.error_free(&error); }
    void query(const char* method, const char* contents) {
        check(api.call(api.bus, api.owner.c_str(), "/org/freedesktop/systemd1",
            "org.freedesktop.systemd1.Manager", method, &error, &message, ""), method);
        if (!message) throw std::runtime_error("systemd: respuesta nula");
        const int entered = api.enter(message, 'a', contents);
        check(entered, "systemd array");
        if (!entered) throw std::runtime_error("systemd: falta array esperado");
    }
};
Support property_support(int rc,const BusError& error){
    const std::string_view name=error.name?error.name:"";
    return rc==-EACCES||rc==-EPERM||name=="org.freedesktop.DBus.Error.AccessDenied"||name=="org.freedesktop.DBus.Error.InteractiveAuthorizationRequired"?Support::permission_required:Support::unsupported;
}
std::optional<std::string> manager_owner(NativeBus& api){
    Reply reply{api};
    const auto rc=api.call(api.bus,"org.freedesktop.DBus","/org/freedesktop/DBus","org.freedesktop.DBus","GetNameOwner",&reply.error,&reply.message,"s","org.freedesktop.systemd1");
    const std::string_view error=reply.error.name?reply.error.name:"";
    if(rc<0&&(error=="org.freedesktop.DBus.Error.NameHasNoOwner"||error=="org.freedesktop.DBus.Error.ServiceUnknown"))return {};
    check(rc,"systemd GetNameOwner");const char* name{};
    if(!reply.message||api.read(reply.message,"s",&name)<=0)throw std::runtime_error("systemd: invalid manager owner reply");
    const auto owner=bounded(name);const auto dot=owner.find('.',1);
    if(owner.size()<4||owner.size()>255||owner[0]!=':'||dot==owner.npos||dot==1||dot+1==owner.size()||
        owner.find_first_not_of("0123456789",1)<dot||owner.find_first_not_of("0123456789",dot+1)!=owner.npos)throw std::runtime_error("systemd: invalid unique owner");
    return owner;
}
Metric<std::string> string_property(NativeBus& api,const std::string& path,const char* interface,const char* property){
    Reply reply{api};const char* value{};
    const auto rc=api.call(api.bus,api.owner.c_str(),path.c_str(),"org.freedesktop.DBus.Properties","Get",&reply.error,&reply.message,"ss",interface,property);
    if(rc<0)return absent<std::string>(std::string("Cannot query systemd ")+property,property_support(rc,reply.error));
    if(!reply.message||api.enter(reply.message,'v',"s")<=0||api.read(reply.message,"s",&value)<=0||api.exit(reply.message)<0)return absent<std::string>("Invalid systemd string property reply");
    return {bounded(value),Support::available,std::string("systemd property ")+property};
}
Metric<std::string> verify_user_manager(NativeBus& api){
    Reply reply{api};std::uint32_t uid{};
    const auto rc=api.call(api.bus,"org.freedesktop.DBus","/org/freedesktop/DBus","org.freedesktop.DBus","GetConnectionUnixUser",
        &reply.error,&reply.message,"s",api.owner.c_str());
    if(rc<0)return absent<std::string>("Cannot verify user-manager owner UID",property_support(rc,reply.error));
    if(!reply.message||api.read(reply.message,"u",&uid)<=0||uid!=geteuid())
        return absent<std::string>("User-manager owner UID does not match current effective UID",Support::permission_required);
    return {api.owner,Support::available,"Current effective UID verified for pinned manager owner"};
}
}
#endif
Metric<LinuxServiceInventory> read_native_systemd_inventory(LinuxServiceScope scope) {
    if(scope!=LinuxServiceScope::system&&scope!=LinuxServiceScope::current_user)
        return absent<LinuxServiceInventory>("Invalid Linux service scope");
#ifdef __linux__
    std::error_code marker_error;
    if (scope==LinuxServiceScope::system&&(!std::filesystem::exists("/run/systemd/system", marker_error) || marker_error))
        return absent<LinuxServiceInventory>("No active systemd system manager marker");
    NativeBus api(scope);
    if (!api.bus) return absent<LinuxServiceInventory>("Cannot open selected local service bus (missing runtime/API, unavailable bus or denied access)",
        api.error==-EACCES||api.error==-EPERM?Support::permission_required:Support::unsupported);
    const auto initial_owner=manager_owner(api);
    if(!initial_owner)return absent<LinuxServiceInventory>("Selected bus has no running systemd manager; not activated");
    api.owner=*initial_owner;
    if(scope==LinuxServiceScope::current_user){
        const auto verified=verify_user_manager(api);
        if(!metric_is_known(verified))return absent<LinuxServiceInventory>(verified.note,verified.support);
    }
    std::size_t wire_payload=0;
    SystemdServiceReader reader;
    reader.loaded = [&] {
        Reply reply{api}; reply.query("ListUnits", "(ssssssouso)");
        std::vector<SystemdUnit> units;
        for (;;) {
            const char *name{}, *description{}, *load{}, *active{}, *sub{}, *following{}, *path{}, *job_type{}, *job_path{};
            unsigned job{};
            const int rc = api.read(reply.message, "(ssssssouso)", &name, &description, &load,
                &active, &sub, &following, &path, &job, &job_type, &job_path);
            check(rc, "systemd ListUnits parse");
            if (!rc) break;
            if (units.size() == max_units) throw std::runtime_error("systemd: demasiadas unidades");
            SystemdUnit unit{bounded(name), bounded(description), bounded(load), bounded(active), bounded(sub), bounded(path)};
            for(const auto* s:{&unit.name,&unit.description,&unit.load,&unit.active,&unit.sub,&unit.object_path})account(wire_payload,*s);
            account(wire_payload,bounded(following));account(wire_payload,bounded(job_type));account(wire_payload,bounded(job_path));
            units.push_back(std::move(unit));
        }
        check(api.exit(reply.message), "systemd ListUnits array end");
        return units;
    };
    reader.installed = [&] {
        Reply reply{api}; reply.query("ListUnitFiles", "(ss)");
        std::vector<SystemdUnitFile> files;
        for (;;) {
            const char *path{}, *state{};
            const int rc = api.read(reply.message, "(ss)", &path, &state);
            check(rc, "systemd ListUnitFiles parse");
            if (!rc) break;
            if (files.size() == max_units) throw std::runtime_error("systemd: demasiados archivos");
            SystemdUnitFile file{bounded(path),bounded(state)};account(wire_payload,file.path);account(wire_payload,file.state);files.push_back(std::move(file));
        }
        check(api.exit(reply.message), "systemd ListUnitFiles array end");
        return files;
    };
    reader.main_pid = [&](const std::string& path) -> Metric<std::uint32_t> {
        std::uint32_t pid{};
        Reply reply{api};
        const int rc = api.property(api.bus, api.owner.c_str(), path.c_str(),
            "org.freedesktop.systemd1.Service", "MainPID", &reply.error, 'u', &pid);
        if (rc < 0) return absent<std::uint32_t>("MainPID unavailable (unit gone, permission or unsupported property)",property_support(rc,reply.error));
        return {pid,Support::available,"Service.MainPID; native0 means no main PID reported, not a valid process identity"};
    };
    reader.unit_id=[&](const std::string& path){return string_property(api,path,"org.freedesktop.systemd1.Unit","Id");};
    reader.slice=[&](const std::string& path){return string_property(api,path,"org.freedesktop.systemd1.Service","Slice");};
    auto result=read_systemd_services(reader);
    if(manager_owner(api)!=initial_owner)throw std::runtime_error("systemd: manager owner changed during inventory");
    LinuxServiceInventory inventory;inventory.scope=scope;inventory.manager_owner=api.owner;
    if(scope==LinuxServiceScope::current_user){
        inventory.user_uid=static_cast<std::uint32_t>(geteuid());
        for(auto& row:result)if(row.linux_details.value)row.linux_details.value->provider="systemd-user-manager";
    }
    inventory.services=std::move(result);
    return {std::move(inventory),Support::available,"Selected systemd manager enumerated; non-atomic observation, no activation or control"};
#else
    return absent<LinuxServiceInventory>("Linux systemd inventory is unavailable on this platform");
#endif
}
std::optional<std::vector<ServiceInfo>> read_native_systemd_services(){
    auto inventory=read_native_systemd_inventory(LinuxServiceScope::system);
    if(!metric_is_known(inventory))return {};
    return std::move(inventory.value->services);
}
Metric<LinuxServiceView> select_linux_service(const LinuxServiceTarget& target,const Metric<LinuxServiceInventory>& inventory){
    if((target.scope!=LinuxServiceScope::system&&target.scope!=LinuxServiceScope::current_user)||!valid_systemd_service_name(target.name))
        return absent<LinuxServiceView>("Invalid service target: explicit scope and canonical .service name required");
    if(!metric_is_known(inventory))return absent<LinuxServiceView>(inventory.note,inventory.support);
    const auto& data=*inventory.value;
    if(data.scope!=target.scope||data.manager_owner.empty()||
        ((target.scope==LinuxServiceScope::current_user)!=data.user_uid.has_value()))
        return absent<LinuxServiceView>("Inventory manager identity does not match target");
    const ServiceInfo* selected{};
    for(const auto& row:data.services)if(row.id==target.name){
        if(selected)return absent<LinuxServiceView>("Ambiguous service identity in inventory");
        selected=&row;
    }
    if(!selected)return absent<LinuxServiceView>("Service name not observed in selected manager; not loaded or started to resolve it");
    return {LinuxServiceView{target,data.user_uid,data.manager_owner,*selected},inventory.support,
        "Read-only service details from selected manager; refresh before acting; owner/PID are not persistent authority tokens"};
}
ActionResult control_current_user_service(const std::string& name,ServiceAction action){
#ifdef __linux__
    bool request_attempted=false;
    try{
        NativeBus bus(LinuxServiceScope::current_user);
        if(!bus.bus)return {bus.error==-EACCES||bus.error==-EPERM?ActionError::permission_denied:ActionError::not_supported,
            "Cannot open current effective UID's local service bus; no fallback or activation"};
        const auto owner=manager_owner(bus);
        if(!owner)return {ActionError::not_supported,"No running systemd user manager; not activated"};
        bus.owner=*owner;
        const auto verified=verify_user_manager(bus);
        if(!metric_is_known(verified))return {verified.support==Support::permission_required?ActionError::permission_denied:ActionError::not_supported,verified.note};
        const auto api=load_systemd_action_api();
        if(!api)return {ActionError::not_supported,"libsystemd action API unavailable"};
        request_attempted=true;
        auto result=run_systemd_service_job(api,reinterpret_cast<ActionBus*>(bus.bus),*owner,name,action);
        if(manager_owner(bus)!=owner)return {ActionError::system_error,"User-manager owner changed during action; outcome unknown, no retry or cancellation"};
        return result;
    }catch(const std::exception& error){return {ActionError::system_error,std::string(error.what())+
        (request_attempted?"; action may have been accepted, outcome unknown":"; action not sent")};}
#else
    (void)name;(void)action;
    return {ActionError::not_supported,"systemd user-service control requires Linux"};
#endif
}
}

namespace pcinsight {
Metric<LinuxServiceInventory> query_linux_service_inventory(LinuxServiceScope scope){
    try{return detail::read_native_systemd_inventory(scope);}
    catch(const std::system_error& error){return {{},error.code().value()==EACCES||error.code().value()==EPERM?Support::permission_required:Support::unsupported,error.what()};}
    catch(const std::exception& error){return {{},Support::unsupported,error.what()};}
}
Metric<LinuxServiceView> open_linux_service(const LinuxServiceTarget& target){
    // Reject inputs before any bus operation, including on non-Linux platforms.
    if((target.scope!=LinuxServiceScope::system&&target.scope!=LinuxServiceScope::current_user)||!detail::valid_systemd_service_name(target.name))
        return {{},Support::unsupported,"Invalid service target"};
    return detail::select_linux_service(target,query_linux_service_inventory(target.scope));
}
ActionResult control_linux_service(const LinuxServiceTarget& target,ServiceAction action){
    if((target.scope!=LinuxServiceScope::system&&target.scope!=LinuxServiceScope::current_user)||!detail::valid_systemd_service_name(target.name)||
        (action!=ServiceAction::start&&action!=ServiceAction::stop&&action!=ServiceAction::restart))
        return {ActionError::invalid_argument,"Invalid Linux service scope, name or action"};
    if(target.scope==LinuxServiceScope::system)return detail::control_systemd_service(target.name,action);
    return detail::control_current_user_service(target.name,action);
}
}
