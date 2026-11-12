#include "linux_startup_candidates.hpp"
#include "pcinsight/startup_candidates.hpp"
#include "linux_startup_inventory.hpp"
#include "linux_process_stat.hpp"
#include <charconv>
#include <algorithm>
#include <filesystem>
#include <limits>
#include <map>
#include <set>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
GioLaunchEvidence parse_gio_launch_evidence(std::string_view text,std::uint32_t pid) {
    if(!pid || text.size()>65536 || (!text.empty() && text.back()!='\0'))return {false,{}};
    std::optional<std::string_view> file,process;
    while(!text.empty()) {
        const auto end=text.find('\0');const auto item=text.substr(0,end);
        const auto equal=item.find('=');const auto key=item.substr(0,equal);
        if(key=="GIO_LAUNCHED_DESKTOP_FILE" || key=="GIO_LAUNCHED_DESKTOP_FILE_PID") {
            if(equal==item.npos)return {false,{}};
            auto& field=key=="GIO_LAUNCHED_DESKTOP_FILE"?file:process;
            if(field)return {false,{}};
            field=item.substr(equal+1);
        }
        text.remove_prefix(end+1);
    }
    if(!file || !process)return {}; // absent/insufficient evidence, not an error
    if(file->empty() || file->size()>32768 || file->front()!='/' || !file->ends_with(".desktop"))return {false,{}};
    std::uint32_t target{};const auto parsed=std::from_chars(process->data(),process->data()+process->size(),target);
    if(parsed.ec!=std::errc{} || parsed.ptr!=process->data()+process->size() || !target || target>static_cast<std::uint32_t>(std::numeric_limits<int>::max()))return {false,{}};
    if(target!=pid)return {}; // inherited by descendants: explicitly ignore
    return {true,std::string(*file)};
}
}
namespace pcinsight {
#ifdef __linux__
namespace {
struct Fd {int fd{-1};~Fd(){if(fd>=0)close(fd);}};
struct Read {std::optional<std::string> text;int error{};std::size_t bytes{};};
Read read_at(int dir,const char* name,std::size_t limit=65536) {
    Fd file{openat(dir,name,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK)};
    if(file.fd<0)return {{},errno};
    std::string text;char buffer[4096];
    for(;;){const auto n=read(file.fd,buffer,std::min(sizeof buffer,limit-text.size()+1));
        if(n<0){if(errno==EINTR)continue;return {{},errno,text.size()};}
        if(!n)break;
        if(static_cast<std::size_t>(n)>limit-text.size())return {{},EOVERFLOW,text.size()+static_cast<std::size_t>(n)};
        text.append(buffer,static_cast<std::size_t>(n));
    }
    const auto bytes=text.size();return {std::move(text),0,bytes};
}
bool supported(const Metric<std::uint64_t>& m){return m.value && (m.support==Support::available || m.support==Support::estimated);}
}
#endif
StartupCandidateReport discover_startup_process_candidates(const SystemSnapshot& snapshot) {
    StartupCandidateReport report;
#ifdef __linux__
    namespace fs=std::filesystem;
    auto fail=[&](ActionError e,const char* message){report.complete=false;report.error=e;report.message=message;return report;};
    if(snapshot.processes.size()>100000 || snapshot.startup_entries.size()>100000)
        return fail(ActionError::invalid_argument,"startup candidate input exceeds count limit");
    std::set<std::uint32_t> seen;
    for(const auto& p:snapshot.processes)if(!p.pid || p.pid>static_cast<std::uint32_t>(std::numeric_limits<int>::max()) || !seen.insert(p.pid).second)
        return fail(ActionError::invalid_argument,"invalid/duplicate candidate process PID");
    std::set<std::string> selected;
    std::size_t input_bytes=0;
    std::vector<detail::XdgAutostartDirectory> dirs;
    try {dirs=detail::native_xdg_autostart_directories();}catch(const std::invalid_argument&){return fail(ActionError::invalid_argument,"invalid candidate XDG context");}
    for(const auto& entry:snapshot.startup_entries) {
        if(entry.id.empty() || entry.id.size()>32768 || entry.id.find('\0')!=entry.id.npos)
            return fail(ActionError::invalid_argument,"invalid candidate startup identifier");
        if(entry.id.size()>8*1024*1024-input_bytes)return fail(ActionError::invalid_argument,"candidate startup metadata exceeds limit");
        input_bytes+=entry.id.size();
        const auto path=fs::path(entry.id);
        bool recognized=false;for(const auto& dir:dirs)if(dir.path==path.parent_path())recognized=true;
        if(!path.is_absolute() || path.extension()!=".desktop" || !recognized || !selected.insert(entry.id).second)
            return fail(ActionError::invalid_argument,"startup candidate ID outside context/duplicate");
    }
    if(selected.empty()){report.message="no startup entries provided; no association inferred";return report;}
    const auto effective=detail::read_xdg_startup_inventory(dirs);
    if(!effective.complete)return fail(ActionError::system_error,"effective startup precedence unresolved");
    std::set<std::string> current;
    for(const auto& entry:effective.entries)current.insert(entry.id);
    for(const auto& id:selected)if(!current.contains(id))return fail(ActionError::stale_target,"startup snapshot is shadowed/stale; reload before correlation");
    Fd root{open("/proc",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
    struct statfs type{};
    if(root.fd<0 || fstatfs(root.fd,&type)!=0)return fail(ActionError::system_error,"candidate procfs unavailable");
    if(type.f_type!=0x9fa0)return fail(ActionError::not_supported,"candidate discovery requires genuine procfs");
    const auto self=read_at(root.fd,"self/stat");const auto own=self.text?detail::parse_linux_process_stat(*self.text):std::nullopt;
    if(!own || own->pid!=static_cast<std::uint32_t>(getpid()))return fail(ActionError::not_supported,"candidate procfs PID namespace mismatch");
    std::size_t payload=0,visited=0;
    for(const auto& process:snapshot.processes) {
        if(++visited>4096){report.complete=false;report.message="candidate process scan limit reached";break;}
        Fd directory{openat(root.fd,std::to_string(process.pid).c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
        struct stat owner{};
        if(directory.fd<0 || fstat(directory.fd,&owner)!=0){++report.unavailable;report.complete=false;continue;}
        if(owner.st_uid!=geteuid()){++report.ignored;continue;} // never read another user's environment, even as root
        ++report.examined;
        if(!supported(process.linux_start_ticks) || !*process.linux_start_ticks.value){++report.unavailable;report.complete=false;continue;}
        const auto before=read_at(directory.fd,"stat");const auto first=before.text?detail::parse_linux_process_stat(*before.text):std::nullopt;
        if(!first || first->pid!=process.pid || first->start_ticks!=*process.linux_start_ticks.value){++report.unavailable;report.complete=false;continue;}
        if(payload==8*1024*1024){report.complete=false;report.message="candidate environment read budget reached";break;}
        const auto environment=read_at(directory.fd,"environ",std::min<std::size_t>(65536,8*1024*1024-payload));
        if(environment.bytes>8*1024*1024-payload){report.complete=false;report.message="candidate environment read budget reached";break;}
        payload+=environment.bytes; // oversized/failed reads consume the budget too
        if(!environment.text){++report.unavailable;report.complete=false;continue;}
        const auto evidence=detail::parse_gio_launch_evidence(*environment.text,process.pid);
        // Unrelated environment strings are never retained or exposed.
        if(!evidence.well_formed){++report.unavailable;report.complete=false;continue;}
        if(!evidence.desktop_file || !selected.contains(*evidence.desktop_file)){++report.ignored;continue;}
        const auto after=read_at(directory.fd,"stat");const auto last=after.text?detail::parse_linux_process_stat(*after.text):std::nullopt;
        struct stat final_owner{};
        if(!last || last->pid!=process.pid || last->start_ticks!=first->start_ticks || fstat(directory.fd,&final_owner)!=0 || final_owner.st_uid!=geteuid()) {
            ++report.unavailable;report.complete=false;continue;
        }
        const auto sample=read_startup_process_sample(process.pid,first->start_ticks);
        if(!sample){++report.unavailable;report.complete=false;continue;}
        report.candidates.push_back({*evidence.desktop_file,*sample.value,Support::estimated,
            {std::nullopt,Support::unsupported,"launcher markers do not prove automatic startup or trusted app identity"}});
    }
    // Detect precedence changes without relabeling the launched path by filename.
    const auto final=detail::read_xdg_startup_inventory(dirs);
    std::set<std::string> final_ids;for(const auto& entry:final.entries)final_ids.insert(entry.id);
    if(!final.complete || final_ids!=current){report.candidates.clear();return fail(ActionError::stale_target,"startup inventory changed during correlation");}
    if(report.message.empty())report.message="caller-UID GIO launch candidates only; not proof of autostart, no observer registration";
#else
    (void)snapshot;report.complete=false;report.error=ActionError::not_supported;report.message="native startup candidate discovery requires Linux";
#endif
    return report;
}
}
