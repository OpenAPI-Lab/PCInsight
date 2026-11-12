#include "linux_s6_inventory.hpp"
#include "linux_network_selection.hpp"
#include <algorithm>
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
template<class T>Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
#ifdef __linux__
struct Fd{int n;explicit Fd(int value):n(value){}~Fd(){if(n>=0)::close(n);}Fd(const Fd&)=delete;};
struct Dir{DIR* p;~Dir(){if(p)::closedir(p);}};
int open_directory(int base,const std::string& path,bool enumerate=false){open_how h{};h.flags=static_cast<std::uint64_t>((enumerate?(O_RDONLY|O_NONBLOCK):O_PATH)|O_DIRECTORY|O_CLOEXEC);h.resolve=RESOLVE_NO_MAGICLINKS;return static_cast<int>(::syscall(SYS_openat2,base,path.c_str(),&h,sizeof h));}
bool same(int a,int b){struct stat x{},y{};return ::fstat(a,&x)==0&&::fstat(b,&y)==0&&x.st_dev==y.st_dev&&x.st_ino==y.st_ino;}
bool stable(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;}
Metric<bool> error_presence(){const auto e=errno;if(e==ENOENT||e==ENOTDIR)return {false,Support::available,"No real FIFO at observed control path; NOT a stopped-service inference"};return absent<bool>("Cannot inspect control FIFO type",e==EACCES||e==EPERM?Support::permission_required:Support::unsupported);}
Metric<bool> control_fifo(int service){Fd dir(open_directory(service,"supervise"));if(dir.n<0)return error_presence();Fd pin(::openat(dir.n,"control",O_PATH|O_NOFOLLOW|O_CLOEXEC));if(pin.n<0)return error_presence();struct stat st{};if(::fstat(pin.n,&st)!=0)return error_presence();return {bool(S_ISFIFO(st.st_mode)),Support::available,"Observed real control FIFO type only; no writer/reader/peer/permission/liveness probe"};}
#endif
}
Metric<LinuxS6Inventory> query_linux_s6_inventory(const std::string& root,std::uint32_t maximum){
    if(root.empty()||root.front()!='/'||root.size()>32768||root.find('\0')!=root.npos||!maximum||maximum>4096)return absent<LinuxS6Inventory>("Invalid explicit s6 scan root/bound1..4096");
#ifdef __linux__
    const auto api=detail::native_network_selection_source_api();const auto before=api.observe(api.context);const auto uid=::geteuid();if(!before.value||before.support!=Support::available)return absent<LinuxS6Inventory>(before.note,before.support);
    Fd fd(open_directory(AT_FDCWD,root,true));if(fd.n<0)return absent<LinuxS6Inventory>("Cannot open explicit s6 scan root",errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported);
    struct stat initial{},last{};if(::fstat(fd.n,&initial)!=0)return absent<LinuxS6Inventory>("s6 inventory root stat failed");
    const int copy=open_directory(fd.n,".",true);if(copy<0)return absent<LinuxS6Inventory>("Cannot enumerate pinned s6 root");Dir dir{::fdopendir(copy)};if(!dir.p){::close(copy);return absent<LinuxS6Inventory>("Cannot initialize s6 enumeration");}
    LinuxS6Inventory out;out.root=root;std::size_t bytes=0,visits=0;bool capped=false;
    auto bad=[&](std::string note){out.complete=false;if(out.diagnostics.size()<64)out.diagnostics.push_back(std::move(note));};
    auto visit=[&]{if(visits>=16384){bad("s6 visit bound16384 reached");capped=true;return false;}++visits;return true;};
    auto append=[&](int parent,const std::string& leaf,const std::string& relative,bool logger,const std::string& parent_name,int pinned,int code){
        if(pinned<0&&code==ENOTDIR)return;
        if(pinned<0&&logger&&code==ENOENT){struct stat raw{};if(::fstatat(parent,leaf.c_str(),&raw,AT_SYMLINK_NOFOLLOW)!=0&&errno==ENOENT)return;} // Missing log is not a broken log symlink.
        const auto cost=2*relative.size()+root.size()+1+parent_name.size();
        if(out.entries.size()>=maximum||cost>8*1024*1024-bytes){bad("s6 entry/payload bound reached; no silent truncation");capped=true;return;}
        bytes+=cost;LinuxS6InventoryEntry row;row.name=relative;row.directory=root+(root.back()=='/'?"":"/")+relative;row.logger=logger;row.parent_name=parent_name;
        if(pinned<0){bad("Unresolved/unreadable s6 directory candidate: "+relative);row.status_record=absent<LinuxS6StatusRecord>("Cannot pin directory candidate",code==EACCES||code==EPERM?Support::permission_required:Support::unsupported);row.real_control_fifo=absent<bool>("Unresolved directory; not known absent");}
        else {row.status_record=detail::read_linux_s6_record_at(pinned,".");row.real_control_fifo=control_fifo(pinned);Fd selected(open_directory(parent,leaf));if(selected.n<0||!same(selected.n,pinned)){bad("s6 candidate binding changed: "+relative);row.status_record=absent<LinuxS6StatusRecord>("Candidate directory changed during observation");row.real_control_fifo=absent<bool>("Candidate directory changed during observation");}}
        out.entries.push_back(std::move(row));
    };
    for(;;){errno=0;const auto e=::readdir(dir.p);if(!e){if(errno)bad("s6 enumeration ended with an error");break;}if(!visit())break;const std::string name=e->d_name;if(name.empty()||name.front()=='.')continue;
        // Only direct directories/aliases; no logger recursion or manager control.
        Fd candidate(open_directory(fd.n,name));const int code=errno;if(candidate.n<0&&code==ENOTDIR)continue;
        const auto row_begin=out.entries.size();append(fd.n,name,name,false,{},candidate.n,code);if(capped)break;
        if(candidate.n>=0){if(!visit())break;Fd log(open_directory(candidate.n,"log"));const int log_error=errno;append(candidate.n,"log",name+"/log",true,name,log.n,log_error);if(capped)break;Fd current(open_directory(fd.n,name));if(current.n<0||!same(current.n,candidate.n)){bad("Parent service changed while observing its logger: "+name);for(auto i=row_begin;i<out.entries.size();++i){out.entries[i].status_record=absent<LinuxS6StatusRecord>("Parent service binding changed");out.entries[i].real_control_fifo=absent<bool>("Parent service binding changed");}}}
    }
    Fd current(open_directory(AT_FDCWD,root));if(current.n<0||!same(current.n,fd.n)||::fstat(fd.n,&last)!=0||!stable(initial,last))bad("s6 scan root binding/membership changed");
    const auto after=api.observe(api.context);if(!after.value||after.support!=Support::available||*after.value!=*before.value||uid!=::geteuid())return absent<LinuxS6Inventory>("s6 inventory caller context changed");
    std::sort(out.entries.begin(),out.entries.end(),[](const auto& a,const auto& b){return a.name<b.name;});const auto support=out.complete?Support::available:Support::estimated;
    return {std::move(out),support,"Explicit s6 candidates/logger observations only; records and FIFO types are not live/active/installed-manager proof or action tokens"};
#else
    return absent<LinuxS6Inventory>("s6 inventory requires Linux");
#endif
}
}
