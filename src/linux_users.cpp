#include "linux_users.hpp"
#include "linux_session_actions.hpp"
#include <cerrno>
#include <cmath>
#include <limits>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <system_error>
#ifdef __linux__
#include <cstring>
#include <utmpx.h>
#endif
namespace pcinsight::detail {
namespace {
constexpr std::size_t max_records = 10000, max_processes = 100000;
constexpr std::size_t max_payload = 16 * 1024 * 1024;
void text(const std::string& value, std::size_t& bytes) {
    if (value.size() > 4096 || value.find('\0') != std::string::npos ||
        value.size() > max_payload - bytes) throw std::invalid_argument("users: identidad/payload excesivo o inválido");
    bytes += value.size();
}
template<class T> struct Sum {
    T value{};
    std::size_t known{}, missing{}, denied{};
    bool permission{}, overflow{};
    void add(const Metric<T>& metric) {
        if (!metric.value || metric.support == Support::unsupported || metric.support == Support::permission_required) {
            ++missing; permission |= metric.support == Support::permission_required;
            if(metric.support==Support::permission_required)++denied;
            return;
        }
        const auto next = *metric.value;
        if constexpr (std::is_floating_point_v<T>) {
            if (!std::isfinite(next) || next < 0 || next > 100) { ++missing; return; }
            if (next > 100 - value) overflow = true;
        } else if (next > std::numeric_limits<T>::max() - value) overflow = true;
        ++known;
        if (!overflow) value += next;
    }
    Metric<T> result(const std::string& identity="username") const {
        if (overflow) return {std::nullopt, Support::unsupported,
            "suma visible inválida/fuera de rango; no se truncó ni se inventó un valor"};
        if (!known) return {std::nullopt, permission ? Support::permission_required : Support::unsupported,
            "sin valores accesibles para este usuario; ausencia no equivale a cero"};
        return {value, Support::estimated, "suma por "+identity+" de procesos visibles, no por sesión; " +
            std::to_string(known) + " valores conocidos, " + std::to_string(missing) +
            " ausentes/invalidos; cobertura global y snapshot atómico no garantizados"};
    }
};
struct Resources {
    Sum<double> cpu;
    Sum<std::uint64_t> memory;
};
struct IoResources {
    std::uint64_t matching{}, identity_unverified{};
    Sum<std::uint64_t> disk;
};
LinuxUserIoDetails io_details(const LinuxLoginRecord& login,const std::map<std::uint32_t,IoResources>& users,
    std::size_t inputs,std::size_t without_uid){
    LinuxUserIoDetails d;d.scope="linux-visible-processes-by-reported-real-uid";
    d.input_processes=inputs;d.processes_without_uid=without_uid;
    d.real_uid={{},Support::unsupported,"Session has no reported numeric UID; username is not an I/O attribution key"};
    d.network_bytes_per_second={{},Support::unsupported,"See linux_user_network unique observer-domain UID report; reported session UID/username alone does not verify a matching user namespace"};
    if(metric_is_known(login.details))d.real_uid=login.details.value->unix_uid;
    else if(login.details.support==Support::permission_required)d.real_uid.support=Support::permission_required;
    if(!metric_is_known(d.real_uid)){
        const auto support=d.real_uid.support==Support::permission_required?Support::permission_required:Support::unsupported;
        d.real_uid.value.reset();d.real_uid.support=support;
        d.matching_processes={{},support,"Cannot match processes without a numeric session UID"};
        d.disk_coverage={{},support,"No UID-keyed disk aggregation attempted"};
        d.disk_bytes_per_second={{},support,"No numeric session UID; disk rates not associated by username"};
        return d;
    }
    const auto it=users.find(*d.real_uid.value);const IoResources empty;
    const auto& r=it==users.end()?empty:it->second;
    d.matching_processes={r.matching,Support::available,"Count of input processes with the same reported real UID, not proof of complete host coverage"};
    d.disk_coverage={LinuxUserIoCoverage{r.disk.known,r.disk.missing,r.disk.denied,r.identity_unverified,r.disk.overflow},Support::available,
        "Coverage of matched visible rows only; unknown UID rows and invisible/exited processes are not included"};
    d.disk_bytes_per_second=r.disk.result("reported real UID; endpoint-attributed disk intervals");
    return d;
}
template<class T> bool same_metric(const Metric<T>& a,const Metric<T>& b){return a.value==b.value&&a.support==b.support&&a.note==b.note;}
bool same_details(const Metric<LinuxSessionDetails>& a,const Metric<LinuxSessionDetails>& b){
    if(a.support!=b.support||a.note!=b.note||a.value.has_value()!=b.value.has_value())return false;
    if(!a.value)return true;
    return a.value->provider==b.value->provider&&same_metric(a.value->state,b.value->state)&&same_metric(a.value->active,b.value->active)&&same_metric(a.value->remote,b.value->remote)&&same_metric(a.value->unix_uid,b.value->unix_uid);
}
template<class T> void metric_text(const Metric<T>& m,std::size_t& bytes){text(m.note,bytes);if constexpr(std::is_same_v<T,std::string>)if(m.value)text(*m.value,bytes);}
}
std::vector<UserSession> aggregate_linux_users(std::span<const LinuxLoginRecord> logins,
                                              std::span<const ProcessInfo> processes) {
    if (logins.size() > max_records || processes.size() > max_processes)
        throw std::length_error("users: inventario excesivo");
    std::size_t bytes = 0;
    std::map<std::string, LinuxLoginRecord> sessions;
    std::map<std::string, Resources> users;
    std::map<std::uint32_t,IoResources> io_users;
    std::size_t without_uid=0;
    for (const auto& row : logins) {
        text(row.id, bytes); text(row.username, bytes); text(row.status, bytes); text(row.logout_token, bytes);
        text(row.details.note,bytes);
        if(row.details.value){const auto& d=*row.details.value;text(d.provider,bytes);metric_text(d.state,bytes);metric_text(d.active,bytes);metric_text(d.remote,bytes);metric_text(d.unix_uid,bytes);}
        if (row.id.empty() || row.username.empty()) throw std::invalid_argument("users: sesión sin identidad");
        const auto [it, added] = sessions.emplace(row.id, row);
        if (!added && (it->second.username != row.username || it->second.status != row.status || it->second.logout_token != row.logout_token||!same_details(it->second.details,row.details)))
            throw std::invalid_argument("users: sesión duplicada contradictoria");
        users.try_emplace(row.username);
    }
    std::set<std::uint32_t> pids;
    for (const auto& process : processes) {
        text(process.username, bytes);
        if (!process.pid || !pids.insert(process.pid).second)
            throw std::invalid_argument("users: PID inválido/duplicado; no se suma dos veces");
        if(metric_is_known(process.linux_details.real_uid)){
            auto& io=io_users[*process.linux_details.real_uid.value];++io.matching;
            if(metric_is_known(process.linux_start_ticks)&&metric_is_known(process.linux_details.disk_rate_real_uid)&&
                process.linux_details.disk_rate_real_uid.value==process.linux_details.real_uid.value)
                io.disk.add(process.disk_bytes_per_second);
            else{
                ++io.identity_unverified;
                const bool denied=process.disk_bytes_per_second.support==Support::permission_required||
                    process.linux_details.disk_rate_real_uid.support==Support::permission_required||process.linux_start_ticks.support==Support::permission_required;
                io.disk.add({{},denied?Support::permission_required:Support::unsupported,"No matching endpoint-attributed disk UID/process identity"});
            }
        }else ++without_uid;
        if(const auto it=users.find(process.username);it!=users.end()){
            it->second.cpu.add(process.cpu_percent);it->second.memory.add(process.memory_bytes);
        }
    }
    std::vector<UserSession> result;
    for (const auto& [id, login] : sessions) {
        const auto& resources = users.at(login.username);
        UserSession row;
        row.id = id; row.username = row.display_name = login.username;
        row.status = login.status;
        row.linux_details=login.details;
        row.resource_scope = "linux-visible-processes-by-username";
        row.disconnect_available = linux_session_disconnect_capability(login.details);
        row.logout_token = login.logout_token;
        row.logout_available = { !row.logout_token.empty(), Support::estimated,
            "cierre destructivo separado; token presente no garantiza autorización ni permanencia de sesión"};
        row.cpu_percent = resources.cpu.result(); row.memory_bytes = resources.memory.result();
        auto io=io_details(login,io_users,processes.size(),without_uid);
        row.disk_bytes_per_second=io.disk_bytes_per_second;row.network_bytes_per_second=io.network_bytes_per_second;
        row.linux_io_details={std::move(io),Support::available,"UID-keyed visible I/O aggregation report; CPU/RAM retain their separate username scope, no global completeness guarantee"};
        result.push_back(std::move(row));
    }
    return result;
}
Metric<std::vector<LinuxLoginRecord>> read_linux_login_records() {
#ifdef __linux__
    std::vector<LinuxLoginRecord> result;
    // Serializes this provider, not external libc utmp readers in the host.
    static std::mutex mutex;
    std::lock_guard lock(mutex);
    struct End { ~End() { endutxent(); } } end;
    setutxent();
    std::size_t scanned = 0;
    while (true) {
        errno = 0;
        const auto* row = getutxent();
        if (!row) {
            if (errno && errno != ESRCH && errno != ENOENT)
                throw std::system_error(errno, std::generic_category(), "utmp: lectura incompleta");
            if(errno==ENOENT)return {{},Support::unsupported,"utmp source not found; empty legacy list is not evidence of no sessions"};
            break;
        }
        if (++scanned > 100000) throw std::length_error("utmp: demasiados registros");
        if (result.size() >= max_records) throw std::length_error("utmp: inventario excesivo");
        if (row->ut_type != USER_PROCESS) continue;
        std::string user(row->ut_user, strnlen(row->ut_user, sizeof row->ut_user));
        std::string line(row->ut_line, strnlen(row->ut_line, sizeof row->ut_line));
        if (user.empty() || line.empty()) continue;
        LinuxSessionDetails details;details.provider="utmp";
        details.state={"reported_login",Support::available,"libc USER_PROCESS record; not evidence of a live, connected or foreground session"};
        details.active={{},Support::unsupported,"utmp does not report foreground activity"};
        details.remote={{},Support::unsupported,"utmp hostname is not verified remote connection state"};
        details.unix_uid={{},Support::unsupported,"utmp username is not a verified numeric UID"};
        result.push_back({std::move(line), std::move(user), "reported_login",{},
            {std::move(details),Support::available,"utmp report only; no session-control identity"}});
    }
    return {std::move(result),Support::available,"utmp libc enumeration completed; records may be stale/incomplete, not all host sessions"};
#else
    return {{},Support::unsupported,"Linux utmp source unavailable on this backend"};
#endif
}
std::vector<LinuxLoginRecord> read_linux_logins() {
    const auto report=read_linux_login_records();
    return report.value.value_or(std::vector<LinuxLoginRecord>{});
}
}
