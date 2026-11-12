#include "pcinsight/linux_sysv.hpp"
#include "cpu_identity_text.hpp"
#include "linux_network_selection.hpp"
#include "linux_sysv_lsb.hpp"
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
template<class T>Metric<T> unknown(std::string note,Support s=Support::unsupported){return {{},s,std::move(note)};}
std::string_view trim(std::string_view v){while(!v.empty()&&(v.front()==' '||v.front()=='\t'))v.remove_prefix(1);while(!v.empty()&&(v.back()==' '||v.back()=='\t'))v.remove_suffix(1);return v;}
bool key_name(std::string_view v){if(v.empty()||v.size()>64)return false;for(const unsigned char c:v)if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'))return false;return true;}
std::optional<std::string> field(std::string_view value){value=trim(value);if(value.empty())return std::string{};return detail::cpu_identity_text(value);}
#ifdef __linux__
struct Fd{int n;explicit Fd(int value):n(value){}~Fd(){if(n>=0)::close(n);}Fd(const Fd&)=delete;};
int pin_path(const std::string& p){open_how h{};h.flags=O_PATH|O_CLOEXEC;h.resolve=RESOLVE_NO_MAGICLINKS;return static_cast<int>(::syscall(SYS_openat2,AT_FDCWD,p.c_str(),&h,sizeof h));}
bool stable(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_size==b.st_size&&a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;}
detail::SysVLsbFileIdentity identity(const struct stat& s){return {static_cast<std::uint64_t>(s.st_dev),s.st_ino,s.st_mode,static_cast<std::uint64_t>(s.st_size),s.st_mtim.tv_sec,s.st_mtim.tv_nsec,s.st_ctim.tv_sec,s.st_ctim.tv_nsec};}
Support access_support(){return errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported;}
Metric<std::string> prefix(int fd,std::size_t expected){std::string out(expected,'\0');std::size_t offset=0;while(offset<expected){ssize_t n;do{n=::pread(fd,out.data()+offset,expected-offset,static_cast<off_t>(offset));}while(n<0&&errno==EINTR);if(n<0)return unknown<std::string>("Cannot read LSB prefix",access_support());if(n==0)return unknown<std::string>("LSB file shortened during prefix read");offset+=static_cast<std::size_t>(n);}return {std::move(out),Support::available,"Bounded prefix bytes only"};}
#endif
}
Metric<LinuxSysVLsbMetadata> parse_linux_sysv_lsb_metadata(std::string_view text,bool input_complete){
    if(text.size()>65536)return unknown<LinuxSysVLsbMetadata>("LSB input bound64KiB exceeded; no silent truncation");
    enum class Phase{seeking,block,closed};Phase phase=Phase::seeking;
    bool seen_short=false,seen_long=false,continuation=false,code_seen=false;std::string short_value,long_value;std::size_t lines=0,offset=0;
    while(offset<text.size()){
        if(++lines>1024)return unknown<LinuxSysVLsbMetadata>("LSB preamble line bound1024 exceeded");
        const auto end=text.find('\n',offset);auto line=text.substr(offset,end==text.npos?text.size()-offset:end-offset);offset=end==text.npos?text.size():end+1;if(!line.empty()&&line.back()=='\r')line.remove_suffix(1);
        const auto horizontal=trim(line);
        // Do not discover headers in heredocs/quoted strings after executable text.
        if(!horizontal.empty()&&horizontal.front()!='#'){code_seen=true;break;}
        if(line.size()>4096||line.find('\0')!=line.npos)return unknown<LinuxSysVLsbMetadata>("Invalid/oversized LSB preamble line");
        auto marker=line;while(!marker.empty()&&(marker.back()==' '||marker.back()=='\t'))marker.remove_suffix(1);
        if(marker=="### BEGIN INIT INFO"){if(phase!=Phase::seeking)return unknown<LinuxSysVLsbMetadata>("Duplicate/nested LSB header");phase=Phase::block;continue;}
        if(marker=="### END INIT INFO"){if(phase!=Phase::block)return unknown<LinuxSysVLsbMetadata>("Unmatched LSB end marker");phase=Phase::closed;continuation=false;continue;}
        if(phase!=Phase::block)continue;
        if(line.empty()||line.front()!='#')return unknown<LinuxSysVLsbMetadata>("LSB block requires column-zero comment lines");
        auto content=line.substr(1);
        if(content.empty()||trim(content).empty()){if(content.empty()||content==" ")continuation=false;continue;}
        if(content.front()=='\t'||content.starts_with("  ")){
            if(!continuation)return unknown<LinuxSysVLsbMetadata>("Orphan/non-Description continuation");
            const auto part=field(content);if(!part)return unknown<LinuxSysVLsbMetadata>("Invalid description UTF-8/controls/line bound1024");
            if(!part->empty()){const auto separator=long_value.empty()?0U:1U;if(part->size()+separator>4096-long_value.size())return unknown<LinuxSysVLsbMetadata>("LSB description bound4096 exceeded");if(separator)long_value+=' ';long_value+=*part;}continue;
        }
        if(content.front()!=' ')return unknown<LinuxSysVLsbMetadata>("LSB keyword requires one space after hash");
        content.remove_prefix(1);
        const auto colon=content.find(':');if(colon==content.npos||!key_name(content.substr(0,colon)))return unknown<LinuxSysVLsbMetadata>("Malformed LSB keyword");
        const auto key=content.substr(0,colon);continuation=key=="Description";
        if(key!="Short-Description"&&key!="Description")continue; // No dependency/default policy evaluation.
        auto& seen=key=="Description"?seen_long:seen_short;if(seen)return unknown<LinuxSysVLsbMetadata>("Duplicate descriptive LSB field");seen=true;
        const auto parsed=field(content.substr(colon+1));if(!parsed)return unknown<LinuxSysVLsbMetadata>("Invalid description UTF-8/controls/line bound1024");
        (key=="Description"?long_value:short_value)=*parsed;
    }
    if(phase!=Phase::closed)return unknown<LinuxSysVLsbMetadata>("No complete leading-comment LSB header");
    if(!code_seen&&!input_complete)return unknown<LinuxSysVLsbMetadata>("Truncated comment preamble; cannot establish complete header scope");
    LinuxSysVLsbMetadata out;out.short_description=unknown<std::string>(seen_short?"Empty declared Short-Description":"Short-Description absent");out.description=unknown<std::string>(seen_long?"Empty declared Description":"Description absent");
    if(!short_value.empty())out.short_description={std::move(short_value),Support::available,"Declared LSB comment; normalized spaces/tabs, not evaluated or publisher-verified"};
    if(!long_value.empty())out.description={std::move(long_value),Support::available,"Declared LSB comment/continuations joined, not evaluated or live-service description proof"};
    return {std::move(out),Support::available,"Bounded leading-comment LSB metadata only; unknown keys ignored, no effective dependency/enablement inference"};
}
Metric<LinuxSysVLsbMetadata> read_linux_sysv_lsb_metadata(const std::string& path){
    if(path.empty()||path.front()!='/'||path.size()>32768||path.find('\0')!=path.npos)return unknown<LinuxSysVLsbMetadata>("Absolute explicit SysV script path required");
#ifdef __linux__
    const auto api=detail::native_network_selection_source_api();const auto before=api.observe(api.context);const auto uid=::geteuid();if(!before.value||before.support!=Support::available)return unknown<LinuxSysVLsbMetadata>(before.note,before.support);
    Fd pin(pin_path(path));struct stat initial{};if(pin.n<0||::fstat(pin.n,&initial)!=0)return unknown<LinuxSysVLsbMetadata>("Cannot pin LSB script",access_support());if(!S_ISREG(initial.st_mode)||initial.st_size<0)return unknown<LinuxSysVLsbMetadata>("LSB source must be a regular file, no FIFO/device/directory I/O");
    const auto result=detail::read_sysv_lsb_pinned(pin.n,identity(initial));
    Fd selected(pin_path(path));struct stat current{};const auto context=api.observe(api.context);if(selected.n<0||::fstat(selected.n,&current)!=0||!stable(initial,current)||!context.value||context.support!=Support::available||*context.value!=*before.value||uid!=::geteuid())return unknown<LinuxSysVLsbMetadata>("LSB source path/context changed");
    return result;
#else
    return unknown<LinuxSysVLsbMetadata>("LSB native reader requires Linux");
#endif
}
Metric<LinuxSysVLsbMetadata> detail::read_sysv_lsb_pinned(int pin,const SysVLsbFileIdentity& expected){
#ifdef __linux__
    struct stat initial{},opened{},after{};if(::fstat(pin,&initial)!=0)return unknown<LinuxSysVLsbMetadata>("Cannot inspect pinned LSB candidate",access_support());
    if(!S_ISREG(initial.st_mode)||initial.st_size<0||identity(initial)!=expected)return unknown<LinuxSysVLsbMetadata>("Pinned LSB candidate identity/type changed before I/O");
    Fd fd(::open(("/proc/self/fd/"+std::to_string(pin)).c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC));if(fd.n<0)return unknown<LinuxSysVLsbMetadata>("Cannot open pinned LSB file",access_support());if(::fstat(fd.n,&opened)!=0||!stable(initial,opened))return unknown<LinuxSysVLsbMetadata>("LSB pinned file selection changed");
    const auto length=static_cast<std::size_t>(std::min<std::uint64_t>(65536,expected.size));
    const auto a=prefix(fd.n,length),b=prefix(fd.n,length);if(!a.value||!b.value)return unknown<LinuxSysVLsbMetadata>("LSB prefix unavailable",a.support==Support::permission_required||b.support==Support::permission_required?Support::permission_required:Support::unsupported);if(a.value!=b.value||::fstat(fd.n,&after)!=0||!stable(initial,after))return unknown<LinuxSysVLsbMetadata>("LSB file/prefix changed during observation");
    return parse_linux_sysv_lsb_metadata(*b.value,expected.size<=65536);
#else
    (void)pin;(void)expected;return unknown<LinuxSysVLsbMetadata>("Pinned LSB reader requires Linux");
#endif
}
}
