#include "pcinsight/linux_openrc.hpp"
#include "linux_network_selection.hpp"
#include <algorithm>
#include <map>
#include <memory>
#include <tuple>
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
int open_safe(int base,const std::string& path,int flags,bool beneath=false){open_how h{};h.flags=static_cast<std::uint64_t>(flags|O_CLOEXEC);h.resolve=RESOLVE_NO_MAGICLINKS|(beneath?(RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS):0);return static_cast<int>(::syscall(SYS_openat2,base,path.c_str(),&h,sizeof h));}
struct Node {
    std::uint64_t dev{},ino{},mode{},size{};std::int64_t ms{},mn{},cs{},cn{};
    bool operator==(const Node&)const=default;
};
Node node(const struct stat& s){return {static_cast<std::uint64_t>(s.st_dev),s.st_ino,s.st_mode,static_cast<std::uint64_t>(s.st_size),s.st_mtim.tv_sec,s.st_mtim.tv_nsec,s.st_ctim.tv_sec,s.st_ctim.tv_nsec};}
std::optional<Node> inspect(int base,const std::string& path){Fd f(open_safe(base,path,O_PATH));struct stat st{};if(f.n<0||::fstat(f.n,&st)!=0)return {};return node(st);}
struct Script {std::string name;std::optional<Node> target;bool operator==(const Script&)const=default;};
struct Ref {std::string name;std::optional<Node> target;bool operator==(const Ref&)const=default;};
struct Level {std::string name;std::optional<Node> dir;std::vector<Ref> refs;bool operator==(const Level&)const=default;};
struct Snapshot {
    std::vector<Script> scripts;std::vector<Level> levels;bool complete=true;std::vector<std::string> notes;std::size_t visits{},bytes{},refs{};
    void bad(std::string note){complete=false;if(notes.size()<64)notes.push_back(std::move(note));}
    bool account(std::size_t n){if(n>8*1024*1024-bytes){bad("Catalog payload bound8MiB reached");return false;}bytes+=n;return true;}
};
template<class Callback>void enumerate(int fd,Snapshot& s,Callback call){
    struct stat initial{},last{};if(::fstat(fd,&initial)!=0){s.bad("Directory initial stat failed");return;}
    // A fresh description, not dup: independent directory offset for each pass.
    const int copy=open_safe(fd,".",O_RDONLY|O_DIRECTORY|O_NONBLOCK,true);if(copy<0){s.bad("Directory enumeration open failed");return;}
    Dir dir{::fdopendir(copy)};if(!dir.p){::close(copy);s.bad("Directory enumeration unavailable");return;}
    for(;;){errno=0;const auto e=::readdir(dir.p);if(!e){if(errno)s.bad("Partial directory read error");break;}
        if(s.visits>=16384){s.bad("Catalog total visit bound16384 reached");break;}++s.visits;
        const std::string name=e->d_name;if(name.empty()||name.front()=='.')continue;
        if(!s.account(name.size())||!call(name))break;
    }
    if(::fstat(fd,&last)!=0||node(initial)!=node(last))s.bad("Directory metadata changed during enumeration");
}
Snapshot scan(int init,int levels,std::uint32_t maximum){
    Snapshot s;
    if(init>=0)enumerate(init,s,[&](const std::string& name){
        if(name.ends_with(".sh"))return true;
        struct stat raw{};if(::fstatat(init,name.c_str(),&raw,AT_SYMLINK_NOFOLLOW)!=0){s.bad("init.d entry disappeared");return true;}
        if(!S_ISREG(raw.st_mode)&&!S_ISLNK(raw.st_mode))return true;
        if(s.scripts.size()>=maximum){s.bad("Script bound reached; not exhaustive");return false;}
        auto target=inspect(init,name);if(!target||!S_ISREG(target->mode)){s.bad("Unresolved/non-regular init.d target");target.reset();}
        s.scripts.push_back({name,target});return true;
    });
    std::sort(s.scripts.begin(),s.scripts.end(),[](const auto& a,const auto& b){return a.name<b.name;});
    if(levels>=0)enumerate(levels,s,[&](const std::string& name){
        struct stat raw{};if(::fstatat(levels,name.c_str(),&raw,AT_SYMLINK_NOFOLLOW)!=0){s.bad("Runlevel entry disappeared");return true;}
        if(!S_ISDIR(raw.st_mode)){if(S_ISLNK(raw.st_mode))s.bad("Top-level runlevel alias unsupported");return true;}
        if(s.levels.size()>=256){s.bad("Runlevel bound256 reached");return false;}
        Level row;row.name=name;Fd fd(open_safe(levels,name,O_RDONLY|O_DIRECTORY|O_NONBLOCK,true));
        struct stat first{},last{},selected{};
        if(fd.n<0||::fstat(fd.n,&first)!=0){s.bad("Runlevel directory unavailable");s.levels.push_back(std::move(row));return true;}
        row.dir=node(first);
        enumerate(fd.n,s,[&](const std::string& entry){
            if(entry.ends_with(".sh"))return true;
            if(s.refs>=16384){s.bad("Runlevel reference bound reached");return false;}
            struct stat link{};if(::fstatat(fd.n,entry.c_str(),&link,AT_SYMLINK_NOFOLLOW)!=0){s.bad("Runlevel reference disappeared");return true;}
            // Direct links only. Never read or execute a script/file body.
            std::optional<Node> target;if(S_ISLNK(link.st_mode))target=inspect(fd.n,entry);
            if(!target)s.bad("Non-link/unresolved runlevel reference");
            row.refs.push_back({entry,target});++s.refs;return true;
        });
        if(::fstat(fd.n,&last)!=0||::fstatat(levels,name.c_str(),&selected,AT_SYMLINK_NOFOLLOW)!=0||node(first)!=node(last)||node(last)!=node(selected))s.bad("Runlevel selection/metadata changed");
        std::sort(row.refs.begin(),row.refs.end(),[](const auto& a,const auto& b){return a.name<b.name;});s.levels.push_back(std::move(row));return true;
    });
    std::sort(s.levels.begin(),s.levels.end(),[](const auto& a,const auto& b){return a.name<b.name;});return s;
}
using Key=std::pair<std::uint64_t,std::uint64_t>;
Key key(const Node& n){return {n.dev,n.ino};}
#endif
}
Metric<LinuxOpenrcCatalog> query_linux_openrc_catalog(const std::string& init_root,const std::string& levels_root,std::uint32_t maximum){
    if(!absolute(init_root)||!absolute(levels_root)||!maximum||maximum>4096)return absent<LinuxOpenrcCatalog>("Invalid explicit OpenRC catalog roots/bound1..4096");
#ifdef __linux__
    const auto api=detail::native_network_selection_source_api();const auto before=api.observe(api.context);const auto uid=::geteuid();
    if(!before.value||before.support!=Support::available)return {{},before.support,before.note};
    Fd init(open_safe(AT_FDCWD,init_root,O_RDONLY|O_DIRECTORY|O_NONBLOCK)),levels(open_safe(AT_FDCWD,levels_root,O_RDONLY|O_DIRECTORY|O_NONBLOCK));
    const auto init_before=init.n>=0?inspect(init.n,"."):std::optional<Node>{};const auto levels_before=levels.n>=0?inspect(levels.n,"."):std::optional<Node>{};
    auto first=scan(init.n,levels.n,maximum);auto second=scan(init.n,levels.n,maximum);
    LinuxOpenrcCatalog out;out.init_directory=init_root;out.runlevel_directory=levels_root;out.complete=first.complete&&second.complete&&first.scripts==second.scripts&&first.levels==second.levels;
    out.diagnostics=std::move(first.notes);out.diagnostics.insert(out.diagnostics.end(),second.notes.begin(),second.notes.end());
    if(first.scripts!=second.scripts||first.levels!=second.levels){out.complete=false;out.diagnostics.push_back("Catalog changed between passes");}
    if(!init_before||!levels_before||inspect(init.n,".")!=init_before||inspect(levels.n,".")!=levels_before||inspect(AT_FDCWD,init_root)!=init_before||inspect(AT_FDCWD,levels_root)!=levels_before){out.complete=false;out.diagnostics.push_back("Catalog roots missing/unreadable/replaced/changed; no directories created");}
    const auto after=api.observe(api.context);if(!after.value||after.support!=Support::available||*after.value!=*before.value||uid!=::geteuid())return absent<LinuxOpenrcCatalog>("Caller context changed during catalog census");
    std::map<Key,std::vector<std::uint32_t>> scripts;std::map<Key,std::vector<std::string>> runlevels;
    for(std::uint32_t i=0;i<second.scripts.size();++i){const auto& row=second.scripts[i];
        auto mode=absent<bool>("Partial/unstable/unresolved script metadata");if(out.complete&&row.target){mode={bool(row.target->mode&0111),Support::available,"Any execute mode bit only; not caller access/interpreter validity/launch success"};scripts[key(*row.target)].push_back(i);}out.scripts.push_back({row.name,std::move(mode)});
    }
    if(out.complete)for(const auto& row:second.levels)if(row.dir)runlevels[key(*row.dir)].push_back(row.name);
    for(const auto& row:second.levels){LinuxOpenrcRunlevelEntry level;level.name=row.name;
        for(const auto& ref:row.refs){LinuxOpenrcRunlevelReference r;r.name=ref.name;r.script_index=absent<std::uint32_t>("No unique regular target match in complete supplied catalog");r.stacked_level=absent<std::string>("No unique directory target match in complete supplied catalog");
            if(out.complete&&ref.target){const auto k=key(*ref.target);
                if(S_ISREG(ref.target->mode)){r.kind=LinuxOpenrcReferenceKind::service;const auto found=scripts.find(k);if(found!=scripts.end()&&found->second.size()==1)r.script_index={found->second.front(),Support::available,"Unique target dev/inode match, not runtime state or launch authority"};}
                else if(S_ISDIR(ref.target->mode)){const auto found=runlevels.find(k);if(found!=runlevels.end()&&found->second.size()==1){r.kind=LinuxOpenrcReferenceKind::stacked_runlevel;r.stacked_level={found->second.front(),Support::available,"Direct configured directory link only, no effective/recursive expansion"};}}
            }level.references.push_back(std::move(r));
        }out.runlevels.push_back(std::move(level));
    }
    const auto support=out.complete?Support::available:Support::estimated;return {std::move(out),support,"Explicit init.d candidates/direct runlevel links only; not active/effective OpenRC services, no body reads/exec"};
#else
    return absent<LinuxOpenrcCatalog>("OpenRC catalog requires Linux");
#endif
}
}
