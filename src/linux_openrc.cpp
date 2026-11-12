#include "pcinsight/linux_openrc.hpp"
#include "linux_network_selection.hpp"
#include <array>
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
template<class T>Metric<T> unknown(std::string note,Support s=Support::unsupported){return {{},s,std::move(note)};}
#ifdef __linux__
struct Fd{int n=-1;~Fd(){if(n>=0)::close(n);}};
struct Dir{DIR* p;~Dir(){if(p)::closedir(p);}};
int open_dir(int base,const std::string& path,bool beneath){open_how h{};h.flags=O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NONBLOCK;h.resolve=RESOLVE_NO_MAGICLINKS|(beneath?(RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS):0);return static_cast<int>(::syscall(SYS_openat2,base,path.c_str(),&h,sizeof h));}
bool stable(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;}
using Rows=std::map<std::string,std::uint32_t>;
struct Scan{Rows rows;bool complete=true;std::vector<std::string> notes;void bad(std::string note){complete=false;notes.push_back(std::move(note));}};
Scan scan(int root,std::uint32_t maximum){
    constexpr std::array<const char*,7> categories{"started","stopping","starting","inactive","failed","hotplugged","wasinactive"};
    Scan out;std::size_t visits=0,bytes=0;
    for(unsigned i=0;i<categories.size();++i){const std::string category=categories[i];Fd fd{open_dir(root,category,true)};
        if(fd.n<0){if(errno!=ENOENT)out.bad(category+": unreadable/non-directory/symlink category");continue;}
        struct stat before{},after{},selected{};if(::fstat(fd.n,&before)!=0){out.bad(category+": initial stat failed");continue;}
        const int copy=::fcntl(fd.n,F_DUPFD_CLOEXEC,3);if(copy<0){out.bad(category+": duplicate failed");continue;}
        Dir dir{::fdopendir(copy)};if(!dir.p){::close(copy);out.bad(category+": enumeration unavailable");continue;}
        for(;;){errno=0;const auto e=::readdir(dir.p);if(!e){if(errno)out.bad(category+": partial enumeration error");break;}
            if(++visits>16384){out.bad("Visit bound16384 reached; not an exhaustive census");return out;}
            const std::string name=e->d_name;if(name.empty()||name.front()=='.')continue;
            struct stat marker{};if(::fstatat(fd.n,name.c_str(),&marker,AT_SYMLINK_NOFOLLOW)!=0){out.bad(category+": marker disappeared/unreadable");continue;}
            if(!S_ISLNK(marker.st_mode)){out.bad(category+": non-symlink marker rejected");continue;}
            const auto found=out.rows.find(name);if(found==out.rows.end()){
                if(out.rows.size()>=maximum||name.size()>8*1024*1024-bytes){out.bad("Entry/payload bound reached; no silent truncation");return out;}
                bytes+=name.size();out.rows.emplace(name,1U<<i);
            }else found->second|=1U<<i;
        }
        if(::fstat(fd.n,&after)!=0||::fstatat(root,category.c_str(),&selected,AT_SYMLINK_NOFOLLOW)!=0||!S_ISDIR(selected.st_mode)||!stable(before,after)||!stable(after,selected))out.bad(category+": membership/selection changed during scan");
    }
    return out;
}
#endif
}
Metric<LinuxOpenrcMarkedState> decode_linux_openrc_markers(std::uint32_t mask){
    const auto primary=mask&15U;
    if(mask&~127U)return unknown<LinuxOpenrcMarkedState>("Unknown marker bits; not the RC_SERVICE ABI");
    if(!primary||(primary&(primary-1)))return unknown<LinuxOpenrcMarkedState>("No unique primary marker; NOT inferred stopped/running");
    const auto state=primary==1?LinuxOpenrcMarkedState::started:primary==2?LinuxOpenrcMarkedState::stopping:primary==4?LinuxOpenrcMarkedState::starting:LinuxOpenrcMarkedState::inactive;
    return {state,Support::available,"Filesystem primary mark only; target validity/liveness/crashed/readiness unknown"};
}
Metric<LinuxOpenrcMarkerInventory> query_linux_openrc_markers(const std::string& root,std::uint32_t max_entries){
    if(root.empty()||root.front()!='/'||root.size()>32768||root.find('\0')!=root.npos||!max_entries||max_entries>4096)return unknown<LinuxOpenrcMarkerInventory>("Invalid absolute OpenRC runtime root/bound1..4096");
#ifdef __linux__
    const auto api=detail::native_network_selection_source_api();const auto before=api.observe(api.context);const auto uid=::geteuid();
    if(!before.value||before.support!=Support::available)return unknown<LinuxOpenrcMarkerInventory>(before.note,before.support);
    Fd fd{open_dir(AT_FDCWD,root,false)};if(fd.n<0)return unknown<LinuxOpenrcMarkerInventory>("Explicit OpenRC root unavailable; no directories created",errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported);
    struct stat initial{},last{};if(::fstat(fd.n,&initial)!=0)return unknown<LinuxOpenrcMarkerInventory>("OpenRC root stat unavailable");
    auto first=scan(fd.n,max_entries);auto second=scan(fd.n,max_entries);
    LinuxOpenrcMarkerInventory out;out.root=root;out.complete=first.complete&&second.complete&&first.rows==second.rows;
    out.diagnostics=std::move(first.notes);out.diagnostics.insert(out.diagnostics.end(),second.notes.begin(),second.notes.end());
    if(first.rows!=second.rows)out.diagnostics.push_back("Marker sets changed between passes");
    Fd current{open_dir(AT_FDCWD,root,false)};struct stat selected{};
    if(::fstat(fd.n,&last)!=0||current.n<0||::fstat(current.n,&selected)!=0||!stable(initial,last)||!stable(last,selected)){out.complete=false;out.diagnostics.push_back("Runtime root changed/unavailable during census");}
    const auto after=api.observe(api.context);if(!after.value||after.support!=Support::available||*after.value!=*before.value||uid!=::geteuid())return unknown<LinuxOpenrcMarkerInventory>("Caller context changed during OpenRC census");
    for(auto& [name,mask]:second.rows)out.entries.push_back({name,mask,out.complete?decode_linux_openrc_markers(mask):unknown<LinuxOpenrcMarkedState>("Partial/unstable inventory; inspect raw observed marks only")});
    const auto support=out.complete?Support::available:Support::estimated;
    return {std::move(out),support,"Explicit OpenRC filesystem marks only; not installed-service inventory or librc/liveness state"};
#else
    return unknown<LinuxOpenrcMarkerInventory>("OpenRC marker reader requires Linux");
#endif
}
}
