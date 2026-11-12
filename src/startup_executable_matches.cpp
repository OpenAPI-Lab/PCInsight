#include "startup_executable_matches.hpp"
#include "linux_startup_properties.hpp"
#include <algorithm>
#include <set>
#include <tuple>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#if __has_include(<linux/openat2.h>)
#include <linux/openat2.h>
#endif
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string n,Support s=Support::unsupported){return {{},s,std::move(n)};}
bool bounded(const std::string& s,std::size_t n,bool empty=false){return (empty||!s.empty())&&s.size()<=n&&s.find('\0')==s.npos;}
bool same_properties(const StartupProperties& a,const StartupProperties& b){
    if(a.entry.id!=b.entry.id||a.entry.command!=b.entry.command||a.entry.status!=b.entry.status||a.fields.size()!=b.fields.size())return false;
    for(std::size_t n=0;n<a.fields.size();++n)if(a.fields[n].group!=b.fields[n].group||a.fields[n].key!=b.fields[n].key||a.fields[n].raw_value!=b.fields[n].raw_value)return false;
    return true;
}
auto signature(const XdgStartupInventory& i){std::vector<std::tuple<std::string,std::string,std::string,std::string,StartupStatus>> s;for(const auto& e:i.entries)s.emplace_back(e.id,e.name,e.command,e.source,e.status);std::sort(s.begin(),s.end());return s;}
StartupPropertiesResult native_properties(const std::string& path){
    // Only IDs from the outer effective inventory reach this reader. The outer
    // census is repeated at the end, instead of O(N) full scans for EACH entry.
#if defined(__linux__) && defined(SYS_openat2) && defined(RESOLVE_NO_MAGICLINKS)
    auto fail=[](const char* n,ActionError e=ActionError::not_supported){return StartupPropertiesResult{{},e,n};};
    struct Fd{int n;~Fd(){if(n>=0)::close(n);}};
    auto pin=[&]{open_how h{};h.flags=O_PATH|O_CLOEXEC;h.resolve=RESOLVE_NO_MAGICLINKS;return static_cast<int>(::syscall(SYS_openat2,AT_FDCWD,path.c_str(),&h,sizeof(h)));};
    const Fd first{pin()};if(first.n<0)return fail("Desktop file cannot be pinned",errno==EACCES||errno==EPERM?ActionError::permission_denied:ActionError::not_supported);
    struct stat a{},b{},c{},d{};if(::fstat(first.n,&a)<0||!S_ISREG(a.st_mode)||a.st_size<0||a.st_size>1048576)return fail("Desktop source must be a regular file <=1MiB");
    const auto owned="/proc/self/fd/"+std::to_string(first.n);
    const Fd reader{::open(owned.c_str(),O_RDONLY|O_CLOEXEC|O_NONBLOCK)};if(reader.n<0)return fail("Cannot read pinned regular desktop file",errno==EACCES||errno==EPERM?ActionError::permission_denied:ActionError::not_supported);
    auto stamp=[](const struct stat& s){return std::tuple{s.st_dev,s.st_ino,s.st_mode,s.st_size,s.st_mtim.tv_sec,s.st_mtim.tv_nsec,s.st_ctim.tv_sec,s.st_ctim.tv_nsec};};
    if(::fstat(reader.n,&b)<0||stamp(a)!=stamp(b))return fail("Pinned desktop reader changed",ActionError::stale_target);
    std::string text;char buffer[4096];unsigned interrupts=0;
    for(;;){const auto n=::read(reader.n,buffer,sizeof buffer);if(n<0){if(errno==EINTR&&++interrupts<=4)continue;return fail("Desktop read unavailable");}if(!n)break;
        if(static_cast<std::size_t>(n)>1048576-text.size())return fail("Desktop source exceeds1MiB");
        text.append(buffer,static_cast<std::size_t>(n));}
    const Fd last{pin()};if(last.n<0||::fstat(reader.n,&b)<0||::fstat(first.n,&c)<0||::fstat(last.n,&d)<0||stamp(a)!=stamp(b)||stamp(a)!=stamp(c)||stamp(a)!=stamp(d)||text.size()!=static_cast<std::size_t>(a.st_size))return fail("Desktop source/path changed",ActionError::stale_target);
    auto p=parse_startup_properties(text);if(!p)return fail("Desktop source malformed",ActionError::invalid_argument);p->entry.id=path;return {std::move(p),ActionError::none,"Pinned desktop properties, effective precedence checked by outer census"};
#else
    (void)path;return {{},ActionError::not_supported,"Desktop matching requires Linux openat2"};
#endif
}
Metric<StartupExecutableFileId> native_file(const std::string& path){
#if defined(__linux__) && defined(SYS_openat2) && defined(RESOLVE_NO_MAGICLINKS)
    struct Fd{int n;~Fd(){if(n>=0)::close(n);}};
    auto open=[&]{open_how h{};h.flags=O_PATH|O_CLOEXEC;h.resolve=RESOLVE_NO_MAGICLINKS;return static_cast<int>(::syscall(SYS_openat2,AT_FDCWD,path.c_str(),&h,sizeof(h)));};
    auto error=[]{return absent<StartupExecutableFileId>("Executable metadata unavailable (openat2 NO_MAGICLINKS); no path fallback",errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported);};
    const Fd first{open()};if(first.n<0)return error();struct stat a{},b{},c{};
    if(::fstat(first.n,&a)<0)return error();
    if(!S_ISREG(a.st_mode)||a.st_size<0)return absent<StartupExecutableFileId>("Exec target is not a regular file; no device/FIFO I/O");
    const Fd last{open()};if(last.n<0)return error();if(::fstat(first.n,&b)<0||::fstat(last.n,&c)<0)return error();
    auto stamp=[](const struct stat& s){return std::tuple{s.st_dev,s.st_ino,s.st_mode,s.st_uid,s.st_gid,s.st_size,s.st_mtim.tv_sec,s.st_mtim.tv_nsec,s.st_ctim.tv_sec,s.st_ctim.tv_nsec};};
    if(stamp(a)!=stamp(b)||stamp(a)!=stamp(c))return absent<StartupExecutableFileId>("Exec target/path changed during metadata query");
    return {StartupExecutableFileId{a.st_ino,major(a.st_dev),minor(a.st_dev)},Support::estimated,"Pinned current regular file, non-atomic revalidation; no historical path/content/launch guarantee"};
#else
    (void)path;return absent<StartupExecutableFileId>("Executable matching requires Linux openat2 NO_MAGICLINKS");
#endif
}
}
Metric<std::string> direct_xdg_exec_path(const StartupProperties& p){
    auto unknown=[](const char* n){return absent<std::string>(n);};
    if(p.fields.size()>65536)return unknown("Excessive desktop fields");
    std::optional<std::string> type,exec,dbus,terminal;
    for(const auto& f:p.fields)if(f.group=="Desktop Entry"){
        auto* target=f.key=="Type"?&type:f.key=="Exec"?&exec:f.key=="DBusActivatable"?&dbus:f.key=="Terminal"?&terminal:nullptr;
        if(target){if(*target)return unknown("Duplicate desktop routing field");*target=f.raw_value;}
    }
    if(type!="Application"||!exec||!bounded(*exec,32768))return unknown("Application with one bounded Exec required");
    if((dbus&&*dbus!="false")||(terminal&&*terminal!="false"))return unknown("D-Bus/terminal routing or invalid boolean not resolved as direct Exec");
    // Two distinct layers: Desktop Entry string escapes, THEN Exec quoting.
    std::string decoded;
    for(std::size_t n=0;n<exec->size();++n){char c=(*exec)[n];if(c=='\\'){
        if(++n==exec->size())return unknown("Truncated desktop string escape");
        switch((*exec)[n]){case 's':c=' ';break;case 'n':c='\n';break;case 't':c='\t';break;case 'r':c='\r';break;case '\\':c='\\';break;default:return unknown("Unsupported desktop string escape");}
    }decoded+=c;}
    std::vector<std::string> words;unsigned file_codes=0;
    for(std::size_t n=0;n<decoded.size();){while(n<decoded.size()&&decoded[n]==' ')++n;if(n==decoded.size())break;
        if(words.size()>=1024)return unknown("Exec token limit");
        const bool quoted=decoded[n]=='"';if(quoted)++n;bool closed=!quoted;std::string word;
        while(n<decoded.size()){
            char c=decoded[n++];if(quoted&&c=='"'){closed=true;break;}if(!quoted&&c==' ')break;
            if(static_cast<unsigned char>(c)<32||static_cast<unsigned char>(c)>=127)return unknown("Only printable ASCII Exec subset resolved");
            if(quoted){if(c=='\\'){if(n==decoded.size()||std::string_view("\"`$\\").find(decoded[n])==std::string_view::npos)return unknown("Invalid quoted Exec escape");c=decoded[n++];}
                else if(c=='$'||c=='`')return unknown("Unescaped reserved quoted character");
            }else if(std::string_view("\"'\\><~|&;$*?#()`").find(c)!=std::string_view::npos)return unknown("Reserved unquoted Exec character");
            word+=c;
        }
        if(!closed||(quoted&&n<decoded.size()&&decoded[n]!=' '))return unknown("Incomplete/concatenated Exec quoting");
        if(words.empty()&&word.empty())return unknown("Empty executable");
        std::string literal;
        for(std::size_t k=0;k<word.size();++k){if(word[k]!='%'){literal+=word[k];continue;}
            if(quoted||++k==word.size())return unknown("Quoted/truncated Exec field code");
            const char code=word[k];
            if(code=='%'){literal+='%';continue;}
            if(words.empty()||std::string_view("fFuUickdDnNvm").find(code)==std::string_view::npos)return unknown("Variable executable/unknown field code");
            if(std::string_view("fFuU").find(code)!=std::string_view::npos&&++file_codes>1)return unknown("Multiple file/URL placeholders");
            if((code=='F'||code=='U'||code=='i')&&word.size()!=2)return unknown("Multi-argument field code not standalone");
        }
        words.push_back(std::move(literal));
    }
    if(words.empty()||words[0].empty()||words[0][0]!='/'||words[0].size()>4095||words[0].find('=')!=words[0].npos)return unknown("Only literal absolute Exec paths resolved; launcher PATH unknown");
    return {std::move(words[0]),Support::estimated,"Literal absolute Exec program only; no arguments expanded or command run"};
}
StartupExecutableMatchReport discover_startup_executable_matches(const StartupBirthObserverReport& capture,const StartupBirthProcessKey& key,const StartupExecutableMatchApi& api){
    StartupExecutableMatchReport out;out.automatic_startup_origin=absent<bool>("File identifiers/current XDG catalog cannot establish automatic-startup cause");out.unique_candidate=absent<std::string>("No complete unique current-file candidate");
    auto fail=[&](ActionError e,const char* n){out.complete=false;out.error=e;out.message=n;out.unique_candidate=absent<std::string>(n);return out;};
    if(!api.context||!api.inventory||!api.properties||!api.file||!bounded(capture.capture_id,128)||capture.capture_id!=key.capture_id||capture.boot!=key.boot||!(key.boot[0]||key.boot[1])||capture.observations.size()>64||!key.kernel_tgid||key.kernel_tgid>2147483647U||!key.kernel_start_boottime_ns)
        return fail(ActionError::invalid_argument,"Invalid capture/key/matcher source");
    const StartupBirthObservation* observed=nullptr;
    for(const auto& row:capture.observations)if(row.key==key){if(observed)return fail(ActionError::invalid_argument,"Duplicate captured key");observed=&row;}
    if(!observed)return fail(ActionError::stale_target,"Exact retained observation key required");
    if(!capture.first_exec_enabled||!metric_is_known(observed->first_executable))return fail(observed->first_executable.support==Support::permission_required?ActionError::permission_denied:ActionError::not_supported,"First successful exec evidence unavailable");
    const auto& executable=*observed->first_executable.value;
    if(!observed->birth_monotonic_ns||executable.observed_monotonic_ns<observed->birth_monotonic_ns||(observed->final_monotonic_ns&&executable.observed_monotonic_ns>*observed->final_monotonic_ns)||executable.device_major>4095||executable.device_minor>1048575)
        return fail(ActionError::invalid_argument,"Malformed first-exec evidence");
    try{
        const auto before=api.context();if(!metric_is_known(before)||before.support!=Support::available)return fail(before.support==Support::permission_required?ActionError::permission_denied:ActionError::not_supported,"Caller context unavailable");
        const auto& context=*before.value;
        if(!valid_network_self(context.observer)||context.boot!=capture.boot||context.observer.pid_namespace_device!=capture.pid_namespace_device||context.observer.pid_namespace_inode!=capture.pid_namespace_inode)return fail(ActionError::stale_target,"Capture boot/PID namespace differs from current caller");
        const auto inventory=api.inventory();if(inventory.entries.size()>4096)return fail(ActionError::invalid_argument,"XDG candidate limit4096");
        out.complete=inventory.complete;std::set<std::string> seen;std::size_t properties_bytes=0;
        for(const auto& e:inventory.entries){
            if(!bounded(e.id,32768)||!bounded(e.name,32768,true)||!bounded(e.command,32768,true)||!seen.insert(e.id).second)return fail(ActionError::invalid_argument,"Invalid/duplicate/excessive inventory entry");
            StartupExecutableCandidate c;c.entry_id=e.id;c.name=e.name;c.current_status=e.status;c.same_file_identifiers=absent<bool>("Entry unresolved");
            if(properties_bytes>=16*1024*1024){c.direct_executable=absent<std::string>("Desktop properties query budget16MiB reached");++out.unresolved;out.complete=false;out.candidates.push_back(std::move(c));continue;}
            const auto props=api.properties(e.id);
            if(!props||props.value->entry.id!=e.id||props.value->entry.command!=e.command||props.value->entry.status!=e.status)c.direct_executable=absent<std::string>("Entry unavailable/changed in effective caller inventory",props.error==ActionError::permission_denied?Support::permission_required:Support::unsupported);
            else{
                if(props.value->file_size_bytes>1048576||props.value->file_size_bytes>16*1024*1024-properties_bytes){c.direct_executable=absent<std::string>("Desktop properties budget exceeded");properties_bytes=16*1024*1024;}
                else{properties_bytes+=props.value->file_size_bytes;c.direct_executable=direct_xdg_exec_path(*props.value);}
                if(metric_is_known(c.direct_executable)){
                    const auto file=api.file(*c.direct_executable.value);const auto rechecked=api.properties(e.id);
                    bool budget_ok=true;
                    if(rechecked.value){if(rechecked.value->file_size_bytes>1048576||rechecked.value->file_size_bytes>16*1024*1024-properties_bytes){budget_ok=false;properties_bytes=16*1024*1024;}else properties_bytes+=rechecked.value->file_size_bytes;}
                    if(!budget_ok)c.same_file_identifiers=absent<bool>("Desktop properties budget exceeded");
                    else if(!rechecked||!same_properties(*props.value,*rechecked.value))c.same_file_identifiers=absent<bool>("Effective desktop properties changed during file query");
                    else if(!metric_is_known(file))c.same_file_identifiers=absent<bool>(file.note,file.support);
                    else{const auto& id=*file.value;c.same_file_identifiers={id.inode==executable.inode&&id.major==executable.device_major&&id.minor==executable.device_minor,Support::estimated,"Current file identifier tuple vs historical first exec only; no launch/content/desktop attribution proof"};}
                }
            }
            if(!metric_is_known(c.direct_executable))c.same_file_identifiers=absent<bool>(c.direct_executable.note,c.direct_executable.support);
            if(!metric_is_known(c.same_file_identifiers)){++out.unresolved;out.complete=false;}else if(*c.same_file_identifiers.value)++out.matched;
            out.candidates.push_back(std::move(c));
        }
        const auto last=api.inventory();const auto after=api.context();
        if(last.entries.size()>4096||!metric_is_known(after)||after.support!=Support::available||*after.value!=context||last.complete!=inventory.complete||signature(last)!=signature(inventory)){
            for(auto& c:out.candidates)c.same_file_identifiers=absent<bool>("Caller/effective inventory changed; requery");
            out.matched=0;out.unresolved=out.candidates.size();return fail(ActionError::stale_target,"Caller/effective XDG inventory changed during query");
        }
        if(out.complete&&out.matched==1)for(const auto& c:out.candidates)if(metric_is_known(c.same_file_identifiers)&&*c.same_file_identifiers.value)out.unique_candidate={c.entry_id,Support::estimated,"One CURRENT CALLER inventory file match, not target-UID/launch-time identity or automatic attribution"};
        out.message="Read-only current caller XDG filesystem candidates; no association/launch/impact update";return out;
    }catch(...){for(auto& c:out.candidates)c.same_file_identifiers=absent<bool>("Query exception; no stable candidate result");out.matched=0;out.unresolved=out.candidates.size();return fail(ActionError::system_error,"Executable candidate query failed");}
}
}
namespace pcinsight {
StartupExecutableMatchReport discover_startup_executable_matches(const StartupBirthObserverReport& report,const StartupBirthProcessKey& key){
    detail::StartupExecutableMatchApi api;
    api.context=[]{const auto source=detail::native_network_selection_source_api();if(!source.observe)return Metric<detail::NetworkSelectionContext>{{},Support::unsupported,"Requires Linux"};return source.observe(source.context);};
    api.inventory=detail::read_native_xdg_startup_inventory;api.properties=detail::native_properties;api.file=detail::native_file;
    return detail::discover_startup_executable_matches(report,key,api);
}
}
