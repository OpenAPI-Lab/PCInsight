#include "linux_process_memory.hpp"
#include <array>
#include <cerrno>
#include <charconv>
#include <limits>
#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
Metric<std::uint64_t> missing(std::string note){return {{},Support::unsupported,std::move(note)};}
std::string_view trim_key(std::string_view s){const auto first=s.find_first_not_of(" \t\r"),last=s.find_last_not_of(" \t\r");return first==s.npos?std::string_view{}:s.substr(first,last-first+1);}
std::array<std::string_view,4> tokens(std::string_view line,std::size_t& count){
    std::array<std::string_view,4> out{};count=0;
    while(!line.empty()){
        const auto first=line.find_first_not_of(" \t\r");if(first==line.npos)break;
        line.remove_prefix(first);const auto end=line.find_first_of(" \t\r");
        if(count==out.size())break;
        out[count++]=line.substr(0,end);if(end==line.npos)break;line.remove_prefix(end);
    }
    return out;
}
template<class T> bool number(std::string_view s,T& value){
    if(s.empty())return false;
    const auto [end,ec]=std::from_chars(s.data(),s.data()+s.size(),value);
    return ec==std::errc{}&&end==s.data()+s.size();
}
#ifdef __linux__
struct Fd {int value;~Fd(){if(value>=0)::close(value);}};
Metric<std::string> io_error(const char* operation,int code){return {{},code==EACCES||code==EPERM?Support::permission_required:Support::unsupported,std::string(operation)+" (errno="+std::to_string(code)+")"};}
Metric<std::string> read_at(int dir,const char* name,std::size_t limit){
    const Fd fd{::openat(dir,name,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK)};
    if(fd.value<0){const int error=errno;return io_error("Cannot open process memory source",error);}
    std::string text;std::array<char,8192> buffer{};unsigned interrupts=0;
    for(;;){
        const auto bytes=::read(fd.value,buffer.data(),buffer.size());
        if(bytes<0){const int error=errno;if(error==EINTR&&++interrupts<=3)continue;return io_error("Cannot read process memory source",error);}
        if(!bytes)break;
        if(static_cast<std::size_t>(bytes)>limit-text.size())return {{},Support::unsupported,"Process memory source exceeds byte limit"};
        text.append(buffer.data(),static_cast<std::size_t>(bytes));
    }
    return {std::move(text),Support::available,"Bounded read through pinned process directory"};
}
#endif
}
Metric<std::uint64_t> parse_linux_process_rss(std::string_view text,std::optional<std::uint32_t> expected_pid){
    if(text.empty()||text.size()>1024*1024||(expected_pid&&!*expected_pid))return missing("Empty/oversized process status or invalid expected PID");
    std::optional<std::uint64_t> rss;std::optional<std::uint32_t> pid;unsigned lines=0;
    while(!text.empty()){
        const auto newline=text.find('\n');const auto line=text.substr(0,newline);
        if(++lines>10000||line.size()>65536||line.find('\0')!=line.npos)return missing("Invalid process status text/bounds");
        const auto colon=line.find(':');const auto key=colon==line.npos?std::string_view{}:trim_key(line.substr(0,colon));
        std::size_t count{};const auto fields=tokens(colon==line.npos?std::string_view{}:line.substr(colon+1),count);
        if(key=="VmRSS"){
            std::uint64_t kib{};
            if(rss||count!=2||fields[1]!="kB"||!number(fields[0],kib)||kib>std::numeric_limits<std::uint64_t>::max()/1024)return missing("Duplicate/invalid/overflowing VmRSS; expected uint64 kB");
            rss=kib*1024;
        }else if(key=="Pid"){
            std::uint32_t value{};
            if(pid||count!=1||!number(fields[0],value)||!value)return missing("Duplicate/invalid process status PID");
            pid=value;
        }
        if(newline==text.npos)break;
        text.remove_prefix(newline+1);
    }
    if(expected_pid&&pid!=expected_pid)return missing("Process status PID absent or inconsistent with expected identity");
    if(!rss)return missing("VmRSS absent; kernel task/exited/inaccessible process is not a measured zero");
    return {*rss,Support::estimated,"Linux status VmRSS in bytes (kB=1024); asynchronous resident-set accounting including shared pages, not private/PSS/unique memory, swap or total allocation"};
}
Metric<std::uint64_t> linux_process_rss_for_identity(const LinuxProcessStat& expected,const std::optional<LinuxProcessStat>& before,const std::optional<LinuxProcessStat>& after,Metric<std::uint64_t> rss){
    auto same=[&](const std::optional<LinuxProcessStat>& value){return value&&value->pid==expected.pid&&value->start_ticks==expected.start_ticks;};
    if(!expected.pid||!same(before)||!same(after))return missing("Process PID/start_ticks absent or changed around RSS read; no memory attribution");
    return rss;
}
Metric<std::uint64_t> read_linux_process_memory(const std::filesystem::path& process_dir,const LinuxProcessStat& expected){
    if(!expected.pid)return missing("Invalid expected process PID");
#ifdef __linux__
    const Fd dir{::open(process_dir.c_str(),O_PATH|O_DIRECTORY|O_CLOEXEC)};
    if(dir.value<0){const int error=errno;const auto result=io_error("Cannot pin process directory",error);return {{},result.support,result.note};}
    const auto first=read_at(dir.value,"stat",65536);if(!first.value)return {{},first.support,first.note};
    const auto before=parse_linux_process_stat(*first.value);
    if(!before||before->pid!=expected.pid||before->start_ticks!=expected.start_ticks)return missing("Process identity differs before RSS read");
    const auto status=read_at(dir.value,"status",1024*1024);
    auto rss=status.value?parse_linux_process_rss(*status.value,expected.pid):Metric<std::uint64_t>{{},status.support,status.note};
    const auto last=read_at(dir.value,"stat",65536);if(!last.value)return {{},last.support,last.note};
    return linux_process_rss_for_identity(expected,before,parse_linux_process_stat(*last.value),std::move(rss));
#else
    (void)process_dir;return missing("Native process RSS reader is Linux-only");
#endif
}
}
