#include "pcinsight/linux_s6.hpp"
#include "linux_s6_inventory.hpp"
#include "linux_network_selection.hpp"
#include <algorithm>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <linux/openat2.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight {
namespace {
template<class T>Metric<T> unknown(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
#ifdef __linux__
struct Fd {int n=-1;~Fd(){if(n>=0)::close(n);}Fd(const Fd&)=delete;explicit Fd(int fd):n(fd){}};
int open_dir(int base,const std::string& path){open_how h{};h.flags=O_PATH|O_DIRECTORY|O_CLOEXEC;h.resolve=RESOLVE_NO_MAGICLINKS;return static_cast<int>(::syscall(SYS_openat2,base,path.c_str(),&h,sizeof h));}
bool same(int a,int b){struct stat x{},y{};return ::fstat(a,&x)==0&&::fstat(b,&y)==0&&x.st_dev==y.st_dev&&x.st_ino==y.st_ino;}
using Bytes=std::array<std::uint8_t,35>;
Metric<Bytes> io_error(){const auto code=errno;return unknown<Bytes>("s6 status I/O unavailable",code==EACCES||code==EPERM?Support::permission_required:Support::unsupported);}
Metric<Bytes> read_record(int directory){
    Fd pin(::openat(directory,"status",O_PATH|O_CLOEXEC|O_NOFOLLOW));struct stat before{},after{};
    if(pin.n<0||::fstat(pin.n,&before)!=0)return io_error();
    if(!S_ISREG(before.st_mode)||before.st_size!=35)return unknown<Bytes>("s6 status is not a real regular35 file");
    // Only our held, type-checked descriptor is reopened. Never open an unchecked path for I/O.
    Fd fd(::open(("/proc/self/fd/"+std::to_string(pin.n)).c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC));
    if(fd.n<0)return io_error();
    if(!same(pin.n,fd.n))return unknown<Bytes>("s6 pinned status descriptor changed");
    std::array<std::uint8_t,36> bytes{};ssize_t n;do{n=::read(fd.n,bytes.data(),bytes.size());}while(n<0&&errno==EINTR);
    if(n<0)return io_error();
    if(n!=35)return unknown<Bytes>("s6 status read length changed");
    if(::fstat(fd.n,&after)!=0)return io_error();
    if(before.st_size!=after.st_size||before.st_mtim.tv_sec!=after.st_mtim.tv_sec||before.st_mtim.tv_nsec!=after.st_mtim.tv_nsec||before.st_ctim.tv_sec!=after.st_ctim.tv_sec||before.st_ctim.tv_nsec!=after.st_ctim.tv_nsec)return unknown<Bytes>("s6 status metadata changed during read");
    Bytes out{};std::copy_n(bytes.begin(),35,out.begin());return {out,Support::available,"Validated bytes only"};
}
#endif
}
Metric<LinuxS6StatusRecord> decode_linux_s6_status_record(std::span<const std::uint8_t> b){
    if(b.size()!=35)return unknown<LinuxS6StatusRecord>("Expected current s6 35-byte record, not runit20/daemontools18/other formats");
    for(const unsigned offset:{8U,20U}){std::uint32_t ns=0;for(unsigned i=0;i<4;++i)ns=(ns<<8)|b[offset+i];if(ns>=1000000000)return unknown<LinuxS6StatusRecord>("Malformed TAI64N nanoseconds");}
    if(b[34]&0xf0U)return unknown<LinuxS6StatusRecord>("Unknown/reserved s6 flags; no forward-format assumption");
    LinuxS6StatusRecord out;std::copy_n(b.begin(),12,out.changed_at_tai64n.begin());std::copy_n(b.begin()+12,12,out.ready_changed_at_tai64n.begin());
    for(unsigned i=24;i<32;++i)out.reported_pid=(out.reported_pid<<8)|b[i];
    out.reported_wait_status=static_cast<std::uint16_t>((std::uint16_t{b[32]}<<8)|b[33]);
    out.paused=bool(b[34]&1);out.finishing=bool(b[34]&2);out.want_up=bool(b[34]&4);out.ready_reported=bool(b[34]&8);
    return {out,Support::available,"Raw s6 record only: timestamps not Unix/uptime, PID not local identity, ready flag NOT liveness/readiness proof"};
}
Metric<LinuxS6StatusRecord> read_linux_s6_status_record(const std::string& root){
    if(root.empty()||root.front()!='/'||root.size()>32768||root.find('\0')!=root.npos)return unknown<LinuxS6StatusRecord>("Absolute s6 service directory required");
    return detail::read_linux_s6_record_at(-100,root);
}
Metric<LinuxS6StatusRecord> detail::read_linux_s6_record_at(int base,const std::string& root){
#ifdef __linux__
    const auto api=detail::native_network_selection_source_api();const auto before=api.observe(api.context);const auto uid=::geteuid();
    if(!before.value||before.support!=Support::available)return unknown<LinuxS6StatusRecord>(before.note,before.support);
    Fd service(open_dir(base,root));if(service.n<0)return unknown<LinuxS6StatusRecord>("Cannot pin explicit s6 service directory (openat2 required)",errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported);
    Fd supervise(open_dir(service.n,"supervise"));if(supervise.n<0)return unknown<LinuxS6StatusRecord>("Cannot pin s6 supervise directory",errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported);
    const auto a=read_record(supervise.n),b=read_record(supervise.n);
    if(!a.value||!b.value||a.value!=b.value)return unknown<LinuxS6StatusRecord>("s6 status missing/non-regular/invalid-size/changed/unreadable; not zero or down",a.support==Support::permission_required||b.support==Support::permission_required?Support::permission_required:Support::unsupported);
    Fd selected(open_dir(service.n,"supervise")),current(open_dir(base,root));const auto after=api.observe(api.context);
    if(selected.n<0||current.n<0||!same(selected.n,supervise.n)||!same(current.n,service.n)||!after.value||after.support!=Support::available||*after.value!=*before.value||uid!=::geteuid())return unknown<LinuxS6StatusRecord>("s6 selection/caller context changed during record read");
    return decode_linux_s6_status_record(*b.value);
#else
    (void)base;(void)root;
    return unknown<LinuxS6StatusRecord>("s6 record reader requires Linux");
#endif
}
}
