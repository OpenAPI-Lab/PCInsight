#include "linux_network_self.hpp"
#include "linux_process_stat.hpp"
#include "linux_tcp.hpp"
#include <array>
#include <cerrno>
#include <charconv>
#ifdef __linux__
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
bool number(std::string_view text,std::uint32_t& out){auto r=std::from_chars(text.data(),text.data()+text.size(),out);return !text.empty()&&r.ec==std::errc{}&&r.ptr==text.data()+text.size();}
std::string_view token(std::string_view& text){const auto begin=text.find_first_not_of(" \t\r");if(begin==text.npos){text={};return {};}
    text.remove_prefix(begin);const auto end=text.find_first_of(" \t\r");auto value=text.substr(0,end);text.remove_prefix(end==text.npos?text.size():end);return value;}
}
Metric<NetworkSelfStatus> parse_network_self_status(std::string_view text){
    if(text.size()>65536||text.find('\0')!=text.npos)return absent<NetworkSelfStatus>("Invalid SELF status size/NUL");
    NetworkSelfStatus out;unsigned seen=0;
    while(!text.empty()){
        const auto end=text.find('\n');auto line=text.substr(0,end);text.remove_prefix(end==text.npos?text.size():end+1);
        const auto colon=line.find(':');if(colon==line.npos)continue;const auto name=line.substr(0,colon);line.remove_prefix(colon+1);
        const unsigned flag=name=="Pid"?1:name=="Tgid"?2:name=="Uid"?4:0;if(!flag)continue;
        if(seen&flag)return absent<NetworkSelfStatus>("Duplicate SELF status identity field");
        seen|=flag;
        std::uint32_t value=0;if(!number(token(line),value))return absent<NetworkSelfStatus>("Invalid SELF status integer");
        if(flag==1)out.pid=value;
        if(flag==2)out.tgid=value;
        if(flag==4){out.real_uid=value;
            for(unsigned i=0;i<3;++i)if(!number(token(line),value))return absent<NetworkSelfStatus>("Incomplete SELF Uid tuple");}
        if(!token(line).empty())return absent<NetworkSelfStatus>("Excess SELF status identity values");
    }
    if(seen!=7||!out.pid||out.pid>2147483647U||!out.tgid||out.tgid>2147483647U)return absent<NetworkSelfStatus>("Missing/invalid SELF status identity");
    return {out,Support::available,{}};
}
bool valid_network_self(const LinuxNetworkSelfIdentity& s){return s.local_pid&&s.local_pid<=2147483647U&&s.start_ticks&&s.procfs_inode&&s.process_inode&&s.pid_namespace_inode&&s.user_namespace_inode;}
#ifdef __linux__
namespace {
struct Fd {int n;explicit Fd(int value):n(value){}~Fd(){if(n>=0)::close(n);}Fd(const Fd&)=delete;};
struct Failure {Support support;std::string note;};
[[noreturn]] void fail(const char* operation){const int e=errno;throw Failure{e==EPERM||e==EACCES?Support::permission_required:Support::unsupported,std::string(operation)+" errno="+std::to_string(e)};}
void require(bool ok,const char* note){if(!ok)throw Failure{Support::unsupported,note};}
struct stat metadata(int fd){struct stat s{};if(::fstat(fd,&s)<0)fail("SELF fstat");return s;}
void procfs(int fd){struct statfs s{};if(::fstatfs(fd,&s)<0)fail("SELF fstatfs");require(s.f_type==0x9fa0,"SELF source not procfs");}
std::string read_at(int directory,const char* path){Fd file(::openat(directory,path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK));if(file.n<0)fail("SELF openat");procfs(file.n);
    std::string text;std::array<char,4096> buffer{};unsigned interrupts=0;
    for(;;){const auto n=::read(file.n,buffer.data(),buffer.size());if(n<0){if(errno==EINTR&&++interrupts<=4)continue;fail("SELF read");}if(!n)break;
        require(static_cast<std::size_t>(n)<=65536-text.size(),"SELF source exceeds64KiB");text.append(buffer.data(),static_cast<std::size_t>(n));}
    return text;
}
std::string self_link(int root){std::array<char,32> name{};const auto n=::readlinkat(root,"self",name.data(),name.size());if(n<0)fail("SELF readlink");
    require(n>0&&static_cast<std::size_t>(n)<name.size(),"SELF symlink invalid/truncated");return {name.data(),static_cast<std::size_t>(n)};}
struct stat ns(int directory,const char* path){
    // Intentionally follow only the kernel's namespace magic symlink, relative
    // to the pinned proc process directory; ordinary data files use NOFOLLOW.
    Fd file(::openat(directory,path,O_RDONLY|O_CLOEXEC));if(file.n<0)fail("SELF namespace open");
    struct statfs fs{};if(::fstatfs(file.n,&fs)<0)fail("SELF namespace fstatfs");require(fs.f_type==0x6e736673,"SELF namespace source not nsfs");return metadata(file.n);
}
bool same_ns(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino;}
}
#endif
Metric<LinuxNetworkSelfIdentity> read_linux_network_self(){
#ifdef __linux__
    try{
        const auto pid=static_cast<std::uint32_t>(::getpid()),uid=static_cast<std::uint32_t>(::getuid());
        const auto tid=static_cast<std::uint32_t>(::syscall(SYS_gettid));
        Fd root(::open("/proc",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));if(root.n<0)fail("SELF proc open");procfs(root.n);
        const auto root_stat=metadata(root.n);const auto path=std::to_string(pid);require(self_link(root.n)==path,"procfs SELF does not match caller PID namespace");
        Fd process(::openat(root.n,path.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));if(process.n<0)fail("SELF process open");procfs(process.n);const auto p=metadata(process.n);
        const auto thread_path="task/"+std::to_string(tid);Fd thread(::openat(process.n,thread_path.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));if(thread.n<0)fail("SELF thread open");procfs(thread.n);
        const auto pidns=ns(process.n,"ns/pid"),userns=ns(process.n,"ns/user");
        require(same_ns(pidns,ns(thread.n,"ns/pid"))&&same_ns(userns,ns(thread.n,"ns/user")),"Caller and leader namespace differ");
        const auto before=parse_linux_process_stat(read_at(process.n,"stat"));
        const auto status=parse_network_self_status(read_at(process.n,"status"));
        const auto current=parse_network_self_status(read_at(thread.n,"status"));
        const auto after=parse_linux_process_stat(read_at(process.n,"stat"));
        require(before&&after&&before->pid==pid&&after->pid==pid&&before->start_ticks&&before->start_ticks==after->start_ticks,"SELF proc stat identity absent/changed");
        require(status.value&&status.value->pid==pid&&status.value->tgid==pid&&status.value->real_uid==uid&&
            current.value&&current.value->pid==tid&&current.value->tgid==pid&&current.value->real_uid==uid,"SELF leader/caller status mismatch");
        require(same_ns(pidns,ns(process.n,"ns/pid"))&&same_ns(userns,ns(process.n,"ns/user"))&&
            same_ns(pidns,ns(thread.n,"ns/pid"))&&same_ns(userns,ns(thread.n,"ns/user")),"SELF namespace changed during read");
        require(self_link(root.n)==path&&pid==static_cast<std::uint32_t>(::getpid())&&uid==static_cast<std::uint32_t>(::getuid()),"SELF caller changed during read");
        return {LinuxNetworkSelfIdentity{pid,uid,before->start_ticks,static_cast<std::uint64_t>(root_stat.st_dev),static_cast<std::uint64_t>(root_stat.st_ino),static_cast<std::uint64_t>(p.st_ino),
            static_cast<std::uint64_t>(pidns.st_dev),static_cast<std::uint64_t>(pidns.st_ino),static_cast<std::uint64_t>(userns.st_dev),static_cast<std::uint64_t>(userns.st_ino)},Support::available,
            "Pinned procfs SELF/caller/leader checks; local namespace identity, not kernel-ID conversion"};
    }catch(const Failure& f){return absent<LinuxNetworkSelfIdentity>(f.note,f.support);}
#else
    return absent<LinuxNetworkSelfIdentity>("SELF procfs identity requires Linux");
#endif
}
Metric<LinuxNetworkSelfIdentity> read_linux_network_pidfd_process(int pidfd,std::uint32_t pid,std::uint64_t start){
    if(pidfd<0||!pid||pid>2147483647U||!start)return absent<LinuxNetworkSelfIdentity>("Invalid selected procfs identity request");
#ifdef __linux__
    try{
        Fd root(::open("/proc",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));if(root.n<0)fail("Selected proc open");procfs(root.n);
        const auto root_stat=metadata(root.n);const auto caller=std::to_string(::getpid());
        require(self_link(root.n)==caller,"Selected procfs view differs from caller PID view");
        Fd observer(::openat(root.n,caller.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));if(observer.n<0)fail("Selected observer open");procfs(observer.n);
        // Threads may unshare their descriptor table. Resolve the held descriptor
        // in this caller thread's table, never assume the leader shares it.
        const auto thread_path="task/"+std::to_string(::syscall(SYS_gettid));
        Fd thread(::openat(observer.n,thread_path.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));if(thread.n<0)fail("Selected observer thread open");procfs(thread.n);
        Fd info(::openat(thread.n,"fdinfo",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));if(info.n<0)fail("Selected fdinfo open");procfs(info.n);
        const auto handle=std::to_string(pidfd);
        auto projection=[&]{auto result=parse_linux_tcp_pidfd_pid(read_at(info.n,handle.c_str()));require(result.value&&*result.value==pid,"pidfd procfs projection absent/changed");};
        projection();const auto path=std::to_string(pid);
        Fd process(::openat(root.n,path.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));if(process.n<0)fail("Selected process open");procfs(process.n);
        const auto p=metadata(process.n),pidns=ns(process.n,"ns/pid"),userns=ns(process.n,"ns/user");
        const auto before=parse_linux_process_stat(read_at(process.n,"stat"));
        const auto status=parse_network_self_status(read_at(process.n,"status"));
        const auto status_after=parse_network_self_status(read_at(process.n,"status"));
        const auto after=parse_linux_process_stat(read_at(process.n,"stat"));
        require(before&&after&&before->pid==pid&&after->pid==pid&&before->start_ticks==start&&after->start_ticks==start,"Selected process start/PID absent/changed");
        require(status.value&&status_after.value&&status.value->pid==pid&&status.value->tgid==pid&&status_after.value->pid==pid&&status_after.value->tgid==pid&&
            status.value->real_uid==status_after.value->real_uid,"Selected process status absent/changed");
        require(same_ns(pidns,ns(process.n,"ns/pid"))&&same_ns(userns,ns(process.n,"ns/user")),"Selected namespaces changed");
        projection();require(self_link(root.n)==caller&&caller==std::to_string(::getpid()),"Selected observer PID changed");
        return {LinuxNetworkSelfIdentity{pid,status.value->real_uid,start,static_cast<std::uint64_t>(root_stat.st_dev),static_cast<std::uint64_t>(root_stat.st_ino),static_cast<std::uint64_t>(p.st_ino),
            static_cast<std::uint64_t>(pidns.st_dev),static_cast<std::uint64_t>(pidns.st_ino),static_cast<std::uint64_t>(userns.st_dev),static_cast<std::uint64_t>(userns.st_ino)},Support::available,
            "Pinned selected procfs process; pidfd projection and expected start checked; UID is observer-domain"};
    }catch(const Failure& f){return absent<LinuxNetworkSelfIdentity>(f.note,f.support);}
#else
    return absent<LinuxNetworkSelfIdentity>("Selected procfs identity requires Linux");
#endif
}
}
