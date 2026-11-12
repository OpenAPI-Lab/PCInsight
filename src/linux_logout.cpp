#include "linux_logout.hpp"
#include "linux_service_actions.hpp"
#include <algorithm>
#include <charconv>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <set>
#ifdef __linux__
#include <dlfcn.h>
#endif
namespace pcinsight::detail {
namespace {
bool owner_ok(const std::string& s) {
    if (s.size() < 4 || s.size() > 255 || s[0] != ':') return false;
    const auto dot = s.find('.', 1);
    return dot != std::string::npos && dot > 1 && dot + 1 < s.size() &&
        std::all_of(s.begin() + 1, s.begin() + dot, [](char c) { return c >= '0' && c <= '9'; }) &&
        std::all_of(s.begin() + dot + 1, s.end(), [](char c) { return c >= '0' && c <= '9'; });
}
bool path_ok(const std::string& s) {
    constexpr std::string_view prefix = "/org/freedesktop/login1/session/";
    return s.starts_with(prefix) && s.size() > prefix.size() && s.size() <= 1024 &&
        s.substr(prefix.size()) != "auto" && s.substr(prefix.size()) != "self" &&
        std::all_of(s.begin() + prefix.size(), s.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_';
        });
}
}
std::string logout_token(const LogoutToken& token) {
    if (!owner_ok(token.owner) || !path_ok(token.path) || !token.timestamp)
        throw std::invalid_argument("identidad logind inválida");
    return "login1@" + token.owner + token.path + "#" + std::to_string(token.timestamp);
}
std::optional<LogoutToken> parse_logout_token(const std::string& id) {
    if (id.size() > 1340 || !id.starts_with("login1@")) return {};
    const auto slash = id.find('/', 7), hash = id.find('#', slash);
    if (slash == std::string::npos || hash == std::string::npos) return {};
    LogoutToken result{id.substr(7, slash - 7), id.substr(slash, hash - slash), 0};
    const auto* begin = id.data() + hash + 1;
    const auto parsed = std::from_chars(begin, id.data() + id.size(), result.timestamp);
    if (parsed.ec != std::errc{} || parsed.ptr != id.data() + id.size() || !result.timestamp ||
        !owner_ok(result.owner) || !path_ok(result.path)) return {};
    return result;
}
LinuxLoginRecord make_logind_login(const LogindSessionObservation& o){
    const auto bounded_text=[](const std::string& s){return !s.empty()&&s.size()<=4096&&s.find('\0')==s.npos;};
    if(!bounded_text(o.id)||!bounded_text(o.username)||!owner_ok(o.identity.owner)||!path_ok(o.identity.path))throw std::invalid_argument("login1: invalid session identity");
    LinuxLoginRecord row{"login1:"+o.id,o.username,"unknown"};
    LinuxSessionDetails d;d.provider="login1";
    d.unix_uid={o.uid,Support::available,"UID reported by ListSessions, not a global identity across user namespaces; resources still grouped by username"};
    const bool stable=o.before.error==ActionError::none&&o.after.error==ActionError::none&&o.before.timestamp&&o.before.timestamp==o.after.timestamp&&
        metric_is_known(o.object_id)&&metric_is_known(o.object_username)&&metric_is_known(o.object_uid)&&
        o.object_id.value==o.id&&o.object_username.value==o.username&&o.object_uid.value==o.uid;
    const auto support=o.before.error==ActionError::permission_denied||o.after.error==ActionError::permission_denied||o.object_id.support==Support::permission_required||o.object_username.support==Support::permission_required||o.object_uid.support==Support::permission_required?Support::permission_required:Support::unsupported;
    if(stable){
        auto identity=o.identity;identity.timestamp=o.before.timestamp;row.logout_token=logout_token(identity);
        d.state=o.state;d.active=o.active;d.remote=o.remote;
        if(!metric_is_known(d.state)||!bounded_text(*d.state.value)||d.state.value->size()>128||d.state.value->find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")!=std::string::npos)
            d.state={{},o.state.support==Support::permission_required?Support::permission_required:Support::unsupported,"login1 State missing, unsupported or invalid"};
        else d.state.note="Native login1 State; not Windows connected/disconnected, foreground activity or lock state";
        const auto normalize=[](Metric<bool>& value,const char* note){
            if(!metric_is_known(value)){value.value.reset();value.support=value.support==Support::permission_required?Support::permission_required:Support::unsupported;}
            value.note=note;
        };
        normalize(d.active,"Native login1 Active foreground flag; false does not prove disconnected, idle or locked");
        normalize(d.remote,"Native login1 Remote origin flag; not current network connection health");
        row.status=d.state.value.value_or("unknown");
    }else{
        constexpr auto note="Session ID/name/UID or timestamp unavailable/changed during metadata read; no verified state or logout token";
        d.state={{},support,note};d.active={{},support,note};d.remote={{},support,note};
    }
    row.details={std::move(d),Support::available,"ListSessions report with per-field availability; property reads are not an atomic session snapshot"};
    return row;
}
ActionResult logout_linux_session(const std::string& id, const LogoutApi& api) {
    const auto token = parse_logout_token(id);
    if (!token) return {ActionError::invalid_argument, "se requiere logout_token logind del inventario, no PID/TTY/session ID"};
    if (!api.owner || !api.query || !api.terminate || !api.next)
        return {ActionError::not_supported, "proveedor logind de cierre no disponible"};
    const auto [owner, error] = api.owner();
    if (error != ActionError::none) return {error, "no se pudo identificar el proveedor logind; sin cierre"};
    if (owner != token->owner) return {ActionError::stale_target, "instancia logind cambió; sin cierre"};
    const auto before = api.query(*token);
    if (before.error != ActionError::none) return {before.error, "no se verificó identidad de sesión; sin cierre"};
    if (before.timestamp != token->timestamp) return {ActionError::stale_target, "identidad de sesión cambió; sin cierre"};
    const auto accepted = api.terminate(*token);
    if (!accepted) return accepted;
    do {
        const auto after = api.query(*token);
        if (after.error == ActionError::not_found ||
            (after.error == ActionError::none && after.timestamp != token->timestamp))
            return {ActionError::none, "identidad de sesión retirada por logind; no se conserva la sesión para reconectar"};
        if (after.error != ActionError::none) return {ActionError::system_error,
            "cierre aceptado; confirmación no disponible, sin rollback"};
    } while (api.next());
    return {ActionError::system_error, "cierre aceptado; sesión no retirada dentro del plazo, sin rollback"};
}
#ifdef __linux__
namespace {
constexpr const char* manager = "/org/freedesktop/login1";
constexpr const char* session_interface = "org.freedesktop.login1.Session";
ActionError bus_error(int rc, const ActionBusError& error) {
    if (error.name) {
        const std::string_view name(error.name);
        if (name == "org.freedesktop.DBus.Error.UnknownObject" || name == "org.freedesktop.login1.NoSuchSession") return ActionError::not_found;
        if (name == "org.freedesktop.DBus.Error.NameHasNoOwner" || name == "org.freedesktop.DBus.Error.ServiceUnknown") return ActionError::not_supported;
        if (name == "org.freedesktop.DBus.Error.AccessDenied" || name == "org.freedesktop.DBus.Error.InteractiveAuthorizationRequired") return ActionError::permission_denied;
    }
    return rc == -EACCES || rc == -EPERM ? ActionError::permission_denied : ActionError::system_error;
}
std::string bounded(const char* s) {
    if (!s || strnlen(s, 4097) > 4096) throw std::runtime_error("login1: cadena inválida/excesiva");
    return s;
}
struct LoginBus {
    SystemdActionApi api{load_systemd_action_api()};
    ActionBus* bus{};
    int (*enter)(ActionMessage*, char, const char*){};
    int (*exit)(ActionMessage*){};
    int (*interactive)(ActionBus*, int){};
    LoginBus() {
        if (!api) return;
        enter = reinterpret_cast<decltype(enter)>(dlsym(api.library.get(), "sd_bus_message_enter_container"));
        exit = reinterpret_cast<decltype(exit)>(dlsym(api.library.get(), "sd_bus_message_exit_container"));
        interactive = reinterpret_cast<decltype(interactive)>(dlsym(api.library.get(), "sd_bus_set_allow_interactive_authorization"));
        if (!enter || !exit || !interactive) return;
        if (api.open(&bus) < 0 || !bus) return;
        if (api.timeout(bus, 5000000) < 0 || interactive(bus, 0) < 0) { api.unref(bus); bus = nullptr; }
    }
    ~LoginBus() { if (bus) api.unref(bus); }
};
struct Reply {
    LoginBus& connection;
    ActionMessage* message{};
    ActionBusError error{};
    ~Reply() { if (message) connection.api.message_unref(message); connection.api.error_free(&error); }
};
std::pair<std::string, ActionError> owner(LoginBus& bus) {
    Reply reply{bus};
    const auto rc = bus.api.call(bus.bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "GetNameOwner", &reply.error, &reply.message, "s", "org.freedesktop.login1");
    if (rc < 0) return {"", bus_error(rc, reply.error)};
    const char* result{};
    if (!reply.message || bus.api.read(reply.message, "s", &result) <= 0) return {"", ActionError::system_error};
    auto name = bounded(result);
    return owner_ok(name) ? std::pair{name, ActionError::none} : std::pair{std::string{}, ActionError::system_error};
}
LogoutQuery query(LoginBus& bus, const LogoutToken& token) {
    Reply reply{bus};
    auto rc = bus.api.call(bus.bus, token.owner.c_str(), token.path.c_str(), "org.freedesktop.DBus.Properties",
        "Get", &reply.error, &reply.message, "ss", session_interface, "TimestampMonotonic");
    if (rc < 0) return {0, bus_error(rc, reply.error)};
    std::uint64_t time{};
    if (!reply.message || bus.enter(reply.message, 'v', "t") <= 0 ||
        bus.api.read(reply.message, "t", &time) <= 0 || bus.exit(reply.message) < 0 || !time)
        return {0, ActionError::system_error};
    return {time, ActionError::none};
}
Metric<std::string> text_property(LoginBus& bus, const LogoutToken& token,const char* property) {
    Reply reply{bus};
    const auto rc = bus.api.call(bus.bus, token.owner.c_str(), token.path.c_str(), "org.freedesktop.DBus.Properties",
        "Get", &reply.error, &reply.message, "ss", session_interface, property);
    const char* value{};
    if(rc<0)return {{},bus_error(rc,reply.error)==ActionError::permission_denied?Support::permission_required:Support::unsupported,std::string("login1 query failed: ")+property};
    if (!reply.message || bus.enter(reply.message, 'v', "s") <= 0 ||
        bus.api.read(reply.message, "s", &value) <= 0||bus.exit(reply.message)<0) return {{},Support::unsupported,"Invalid login1 string reply"};
    return {bounded(value),Support::available,std::string("login1 property: ")+property};
}
Metric<std::uint32_t> session_uid(LoginBus& bus,const LogoutToken& token){
    Reply reply{bus};unsigned value{};const char* path{};
    const auto rc=bus.api.call(bus.bus,token.owner.c_str(),token.path.c_str(),"org.freedesktop.DBus.Properties","Get",&reply.error,&reply.message,"ss",session_interface,"User");
    if(rc<0)return {{},bus_error(rc,reply.error)==ActionError::permission_denied?Support::permission_required:Support::unsupported,"login1 User query failed"};
    if(!reply.message||bus.enter(reply.message,'v',"(uo)")<=0||bus.api.read(reply.message,"(uo)",&value,&path)<=0||bus.exit(reply.message)<0)return {{},Support::unsupported,"Invalid login1 User reply"};
    (void)bounded(path);return {static_cast<std::uint32_t>(value),Support::available,"login1 Session.User UID"};
}
Metric<bool> flag(LoginBus& bus,const LogoutToken& token,const char* property){
    Reply reply{bus};int value{};
    const auto rc=bus.api.call(bus.bus,token.owner.c_str(),token.path.c_str(),"org.freedesktop.DBus.Properties","Get",&reply.error,&reply.message,"ss",session_interface,property);
    if(rc<0)return {{},bus_error(rc,reply.error)==ActionError::permission_denied?Support::permission_required:Support::unsupported,std::string("login1 query failed: ")+property};
    if(!reply.message||bus.enter(reply.message,'v',"b")<=0||bus.api.read(reply.message,"b",&value)<=0||bus.exit(reply.message)<0||(value!=0&&value!=1))return {{},Support::unsupported,"Invalid login1 boolean reply"};
    return {value!=0,Support::available,std::string("login1 property: ")+property};
}
}
ActionResult logout_linux_session(const std::string& id) {
    if (!parse_logout_token(id)) return {ActionError::invalid_argument, "logout_token logind inválido"};
    LoginBus bus;
    if (!bus.bus) return {ActionError::not_supported, "bus/libsystemd de login1 no disponible; no se solicitó cierre"};
    LogoutApi api;
    api.owner = [&] { return owner(bus); };
    api.query = [&](const LogoutToken& token) { return query(bus, token); };
    api.terminate = [&](const LogoutToken& token) {
        Reply reply{bus};
        const auto rc = bus.api.call(bus.bus, token.owner.c_str(), token.path.c_str(), session_interface,
            "Terminate", &reply.error, &reply.message, "");
        return rc < 0 ? ActionResult{bus_error(rc, reply.error), "logind rechazó el cierre; no hubo elevación ni autorización interactiva"} : ActionResult{};
    };
    std::optional<std::chrono::steady_clock::time_point> deadline;
    api.next = [&] {
        if (!deadline) deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        if (std::chrono::steady_clock::now() >= *deadline) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); return true;
    };
    return logout_linux_session(id, api);
}
std::optional<std::vector<LinuxLoginRecord>> read_logind_logins() {
    LoginBus bus; if (!bus.bus) return {};
    const auto [name, error] = owner(bus);
    if (error == ActionError::not_supported) return {};
    if (error != ActionError::none) throw std::runtime_error("login1: no se pudo identificar instancia");
    Reply reply{bus};
    const auto rc = bus.api.call(bus.bus, name.c_str(), manager, "org.freedesktop.login1.Manager", "ListSessions",
        &reply.error, &reply.message, "");
    if (rc < 0 || !reply.message || bus.enter(reply.message, 'a', "(susso)") <= 0)
        throw std::runtime_error("login1: inventario no disponible/incompleto");
    std::vector<LinuxLoginRecord> rows;
    std::set<std::string> object_paths,session_ids;
    std::size_t payload = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    for (;;) {
        const char *id{}, *user{}, *seat{}, *path{}; unsigned uid{};
        const auto parsed = bus.api.read(reply.message, "(susso)", &id, &uid, &user, &seat, &path);
        if (parsed < 0) throw std::runtime_error("login1: inventario inválido");
        if (!parsed) break;
        if (rows.size() >= 10000 || std::chrono::steady_clock::now() >= deadline)
            throw std::runtime_error("login1: límite/plazo de inventario excedido");
        const auto session_id = bounded(id), username = bounded(user), object = bounded(path);
        (void)bounded(seat);
        if (session_id.empty() || username.empty() || !path_ok(object)) throw std::runtime_error("login1: identidad inválida");
        LogoutToken token{name, object, 0};
        const auto time = query(bus, token);
        const auto object_id=text_property(bus,token,"Id"),object_username=text_property(bus,token,"Name");
        const auto object_uid=session_uid(bus,token);
        const auto status=text_property(bus,token,"State");const auto active=flag(bus,token,"Active"),remote=flag(bus,token,"Remote");
        const auto after=query(bus,token);
        auto row=make_logind_login({session_id,username,uid,token,time,after,status,active,remote,object_id,object_username,object_uid});
        if(!object_paths.insert(object).second||!session_ids.insert(session_id).second)throw std::runtime_error("login1: duplicate session ID/object path");
        const auto bytes = row.id.size() + username.size() + row.status.size() + row.logout_token.size()+object.size();
        if (bytes > 16 * 1024 * 1024 - payload) throw std::runtime_error("login1: payload excesivo");
        payload += bytes;
        rows.push_back(std::move(row));
    }
    if (bus.exit(reply.message) < 0) throw std::runtime_error("login1: final de inventario inválido");
    const auto [after_owner,after_error]=owner(bus);
    if(after_error!=ActionError::none||after_owner!=name)throw std::runtime_error("login1: provider changed/disappeared during inventory");
    return rows;
}
#else
ActionResult logout_linux_session(const std::string&) { return {ActionError::not_supported, "Linux requerido"}; }
std::optional<std::vector<LinuxLoginRecord>> read_logind_logins() { return {}; }
#endif
}
