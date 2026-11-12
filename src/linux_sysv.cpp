#include "pcinsight/linux_sysv.hpp"
#include "linux_network_selection.hpp"
#include "linux_sysv_lsb.hpp"
#include <algorithm>
#include <map>
#ifdef __linux__
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <linux/openat2.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight {
namespace {
template<class T>Metric<T> absent(std::string note){return {{},Support::unsupported,std::move(note)};}
bool absolute(const std::string& s){return !s.empty()&&s.front()=='/'&&s.size()<=32768&&s.find('\0')==s.npos;}
#ifdef __linux__
struct Fd{int n;explicit Fd(int value):n(value){}~Fd(){if(n>=0)::close(n);}Fd(const Fd&)=delete;};
struct Dir{DIR* p;~Dir(){if(p)::closedir(p);}};
int open_safe(int base,const std::string& p,int flags){open_how h{};h.flags=static_cast<std::uint64_t>(flags|O_CLOEXEC);h.resolve=RESOLVE_NO_MAGICLINKS;return static_cast<int>(::syscall(SYS_openat2,base,p.c_str(),&h,sizeof h));}
using Node=detail::SysVLsbFileIdentity;
Node node(const struct stat& s){return {static_cast<std::uint64_t>(s.st_dev),s.st_ino,s.st_mode,static_cast<std::uint64_t>(s.st_size),s.st_mtim.tv_sec,s.st_mtim.tv_nsec,s.st_ctim.tv_sec,s.st_ctim.tv_nsec};}
std::optional<Node> inspect(int fd,const std::string& name){Fd pin(open_safe(fd,name,O_PATH));struct stat s{};if(pin.n<0||::fstat(pin.n,&s)!=0)return {};return node(s);}
struct Script{std::string name;std::optional<Node> target;bool operator==(const Script&)const=default;};
struct Ref{std::string name;std::optional<Node> raw,target;bool operator==(const Ref&)const=default;};
struct Level{std::string name;bool known=false;std::optional<Node> dir;std::vector<Ref> refs;bool operator==(const Level&)const=default;};
struct Snapshot{
    std::vector<Script> scripts;std::vector<Level> levels;bool complete=true;std::size_t visits{},bytes{},entries{};std::vector<std::string> notes;
    void bad(std::string text){complete=false;if(notes.size()<64)notes.push_back(std::move(text));}
    bool entry(std::uint32_t max){if(entries>=max){bad("SysV combined script/reference bound reached");return false;}++entries;return true;}
};
template<class Callback>void enumerate(int fd,Snapshot& s,Callback call){
    struct stat before{},after{};if(::fstat(fd,&before)!=0){s.bad("SysV directory initial stat failed");return;}
    const int copy=open_safe(fd,".",O_RDONLY|O_DIRECTORY|O_NONBLOCK);if(copy<0){s.bad("SysV directory cannot be enumerated");return;}Dir dir{::fdopendir(copy)};if(!dir.p){::close(copy);s.bad("SysV fdopendir failed");return;}
    for(;;){errno=0;const auto e=::readdir(dir.p);if(!e){if(errno)s.bad("SysV partial readdir error");break;}if(s.visits>=16384){s.bad("SysV visit bound16384 reached");break;}++s.visits;
        const std::string name=e->d_name;if(name.empty()||name.front()=='.')continue;if(name.size()>8*1024*1024-s.bytes){s.bad("SysV name payload bound8MiB reached");break;}s.bytes+=name.size();if(!call(name))break;
    }
    if(::fstat(fd,&after)!=0||node(before)!=node(after))s.bad("SysV directory changed during census");
}
Snapshot scan(int init,int rc,std::uint32_t maximum){
    Snapshot s;
    if(init>=0)enumerate(init,s,[&](const std::string& name){struct stat raw{};if(::fstatat(init,name.c_str(),&raw,AT_SYMLINK_NOFOLLOW)!=0){s.bad("SysV init.d entry disappeared");return true;}if(!S_ISREG(raw.st_mode)&&!S_ISLNK(raw.st_mode))return true;if(!s.entry(maximum))return false;auto target=inspect(init,name);if(!target||!S_ISREG(target->mode)){s.bad("SysV init.d target unresolved/non-regular");target.reset();}s.scripts.push_back({name,target});return true;});
    std::sort(s.scripts.begin(),s.scripts.end(),[](const auto& a,const auto& b){return a.name<b.name;});
    for(const char label:std::string("0123456789S")){Level row;row.name=std::string(1,label);const auto path="rc"+row.name+".d";
        if(rc<0){s.levels.push_back(std::move(row));continue;}
        row.dir=inspect(rc,path);const int code=errno;
        if(!row.dir){struct stat raw{};if(code==ENOENT&&::fstatat(rc,path.c_str(),&raw,AT_SYMLINK_NOFOLLOW)!=0&&errno==ENOENT)row.known=true;else s.bad("SysV runlevel target unavailable/broken: "+path);s.levels.push_back(std::move(row));continue;}
        if(!S_ISDIR(row.dir->mode)){s.bad("SysV runlevel path is not a directory: "+path);s.levels.push_back(std::move(row));continue;}
        Fd fd(open_safe(rc,path,O_RDONLY|O_DIRECTORY|O_NONBLOCK));struct stat first{},last{};
        if(fd.n<0||::fstat(fd.n,&first)!=0||node(first)!=*row.dir){s.bad("SysV runlevel selection unavailable/changed: "+path);s.levels.push_back(std::move(row));continue;}row.known=true;
        enumerate(fd.n,s,[&](const std::string& name){if(!s.entry(maximum))return false;Ref ref;ref.name=name;struct stat raw{};if(::fstatat(fd.n,name.c_str(),&raw,AT_SYMLINK_NOFOLLOW)==0)ref.raw=node(raw);else s.bad("SysV runlevel entry disappeared");ref.target=inspect(fd.n,name);if(!ref.target)s.bad("SysV reference target unavailable/broken");row.refs.push_back(std::move(ref));return true;});
        if(::fstat(fd.n,&last)!=0||node(first)!=node(last)||inspect(rc,path)!=row.dir)s.bad("SysV runlevel binding/metadata changed: "+path);
        std::sort(row.refs.begin(),row.refs.end(),[](const auto& a,const auto& b){return a.name<b.name;});s.levels.push_back(std::move(row));
    }
    return s;
}
using Key=std::pair<std::uint64_t,std::uint64_t>;
Key key(const Node& s){return {s.dev,s.ino};}
#endif
}
Metric<LinuxSysVLinkConvention> decode_linux_sysv_link_name(const std::string& name){
    if(name.size()<4||name.size()>255||name.find('\0')!=name.npos||name.find('/')!=name.npos||(name[0]!='S'&&name[0]!='K')||name[1]<'0'||name[1]>'9'||name[2]<'0'||name[2]>'9')return absent<LinuxSysVLinkConvention>("Not an S/K+two-digit+nonempty-suffix filename; no enabled/state inference");
    return {LinuxSysVLinkConvention{name[0]=='S'?LinuxSysVLinkAction::start:LinuxSysVLinkAction::stop,static_cast<std::uint8_t>((name[1]-'0')*10+name[2]-'0'),name.substr(3)},Support::available,"Filename convention only; not execution policy/order/identity or effective enablement"};
}
namespace {
struct MetadataCollection {
    std::vector<Metric<LinuxSysVLsbMetadata>> rows;
    bool attempted=true;
    std::uint64_t reserved{},budget{};
};
Metric<LinuxSysVCatalog> query_catalog(const std::string& init_root,const std::string& rc_root,std::uint32_t maximum,MetadataCollection* metadata,const detail::SysVDescribedSource& source){
    if(!absolute(init_root)||!absolute(rc_root)||!maximum||maximum>4096)return absent<LinuxSysVCatalog>("Invalid absolute SysV roots/bound1..4096");
#ifdef __linux__
    const auto api=detail::native_network_selection_source_api();const auto before=api.observe(api.context);const auto uid=::geteuid();if(!before.value||before.support!=Support::available)return {{},before.support,before.note};
    Fd init(open_safe(AT_FDCWD,init_root,O_RDONLY|O_DIRECTORY|O_NONBLOCK)),rc(open_safe(AT_FDCWD,rc_root,O_RDONLY|O_DIRECTORY|O_NONBLOCK));
    const auto init_before=init.n>=0?inspect(init.n,"."):std::optional<Node>{};const auto rc_before=rc.n>=0?inspect(rc.n,"."):std::optional<Node>{};
    auto a=scan(init.n,rc.n,maximum);
    if(metadata){
        if(source.after_first_scan)source.after_first_scan();
        for(const auto& row:a.scripts){
            auto value=absent<LinuxSysVLsbMetadata>("Incomplete catalog/candidate; metadata read not attempted");
            bool attempted=false;
            if(a.complete&&init_before&&rc_before&&row.target){
                Fd pin(open_safe(init.n,row.name,O_PATH));struct stat current{};
                const auto cost=2*std::min<std::uint64_t>(65536,row.target->size);
                if(pin.n<0||::fstat(pin.n,&current)!=0||node(current)!=*row.target)value=absent<LinuxSysVLsbMetadata>("Candidate binding changed before metadata read");
                else if(!metadata->budget||cost>metadata->budget-metadata->reserved)value=absent<LinuxSysVLsbMetadata>("Metadata prefix budget exhausted/disabled; no body read");
                else {
                    metadata->reserved+=cost;attempted=true;
                    value=source.read(pin.n,*row.target);
                    if(inspect(init.n,row.name)!=row.target)value=absent<LinuxSysVLsbMetadata>("Candidate binding changed after pinned metadata read");
                }
            }
            metadata->attempted=metadata->attempted&&attempted;metadata->rows.push_back(std::move(value));
        }
    }
    auto b=scan(init.n,rc.n,maximum);LinuxSysVCatalog out;out.init_directory=init_root;out.runlevel_root=rc_root;out.complete=a.complete&&b.complete&&a.scripts==b.scripts&&a.levels==b.levels;out.diagnostics=std::move(a.notes);out.diagnostics.insert(out.diagnostics.end(),b.notes.begin(),b.notes.end());
    if(a.scripts!=b.scripts||a.levels!=b.levels)out.diagnostics.push_back("SysV catalog changed between passes");
    if(!init_before||!rc_before||inspect(init.n,".")!=init_before||inspect(rc.n,".")!=rc_before||inspect(AT_FDCWD,init_root)!=init_before||inspect(AT_FDCWD,rc_root)!=rc_before){out.complete=false;out.diagnostics.push_back("SysV roots missing/unreadable/replaced/changed; no creation attempted");}
    const auto after=api.observe(api.context);if(!after.value||after.support!=Support::available||*after.value!=*before.value||uid!=::geteuid())return absent<LinuxSysVCatalog>("SysV caller context changed");
    if(metadata&&!out.complete){metadata->attempted=false;metadata->rows.assign(b.scripts.size(),absent<LinuxSysVLsbMetadata>("Partial/unstable catalog; no bound metadata published"));}
    std::map<Key,std::vector<std::uint32_t>> indices;
    for(std::uint32_t i=0;i<b.scripts.size();++i){const auto& row=b.scripts[i];auto mode=absent<bool>("Partial/unstable/unresolved script metadata");if(out.complete&&row.target){mode={bool(row.target->mode&0111),Support::available,"Any execute mode bit, NOT caller access/interpreter validity or launch success"};indices[key(*row.target)].push_back(i);}out.scripts.push_back({row.name,std::move(mode)});}
    for(const auto& row:b.levels){LinuxSysVRunlevel level;level.name=row.name;level.directory_present=absent<bool>("Partial/unstable/unavailable runlevel directory");if(out.complete&&row.known)level.directory_present={row.dir.has_value(),Support::available,"Directory observed only; NOT current/effective runlevel"};
        for(const auto& ref:row.refs){LinuxSysVRunlevelReference r;r.name=ref.name;r.convention=decode_linux_sysv_link_name(ref.name);r.symlink=absent<bool>("Partial/unstable/raw metadata unavailable");r.regular_target=absent<bool>("Partial/unstable/target metadata unavailable");r.script_index=absent<std::uint32_t>("No unique target identity in a complete supplied init.d catalog");if(out.complete){if(ref.raw)r.symlink={bool(S_ISLNK(ref.raw->mode)),Support::available,"Node type only"};if(ref.target){r.regular_target={bool(S_ISREG(ref.target->mode)),Support::available,"Resolved target type only, not runnable/live"};if(S_ISREG(ref.target->mode)){const auto found=indices.find(key(*ref.target));if(found!=indices.end()&&found->second.size()==1)r.script_index={found->second.front(),Support::available,"Unique dev/inode match in this snapshot only, NOT action authority"};}}}level.entries.push_back(std::move(r));}
        out.runlevels.push_back(std::move(level));
    }
    const auto support=out.complete?Support::available:Support::estimated;return {std::move(out),support,metadata?"Explicit SysV candidates with opt-in pinned LSB comments; not live state/enablement or action authority":"Explicit SysV filesystem candidates/references only; no effective enablement, running/PID, dependency evaluation, body reads or execution"};
#else
    (void)metadata;(void)source;
    return absent<LinuxSysVCatalog>("SysV catalog requires Linux");
#endif
}
}
Metric<LinuxSysVCatalog> query_linux_sysv_catalog(const std::string& init_root,const std::string& rc_root,std::uint32_t maximum){return query_catalog(init_root,rc_root,maximum,nullptr,{});}
Metric<LinuxSysVDescribedCatalog> detail::query_sysv_described_with_source(const std::string& init_root,const std::string& rc_root,std::uint32_t maximum,std::uint64_t budget,SysVDescribedSource source){
    if(budget>16*1024*1024||!source.read)return absent<LinuxSysVDescribedCatalog>("Invalid metadata budget0..16MiB/reader");
    MetadataCollection metadata;metadata.budget=budget;
    auto catalog=query_catalog(init_root,rc_root,maximum,&metadata,source);
    if(!catalog.value)return {{},catalog.support,catalog.note};
    LinuxSysVDescribedCatalog out{std::move(*catalog.value),std::move(metadata.rows),metadata.attempted,metadata.reserved};
    return {std::move(out),catalog.support,"Row-aligned metadata from pinned candidates in this query only; per-row availability and read budget are separate from catalog completeness"};
}
Metric<LinuxSysVDescribedCatalog> query_linux_sysv_described_catalog(const std::string& init_root,const std::string& rc_root,std::uint32_t maximum,std::uint64_t budget){return detail::query_sysv_described_with_source(init_root,rc_root,maximum,budget,{{},detail::read_sysv_lsb_pinned});}
}
