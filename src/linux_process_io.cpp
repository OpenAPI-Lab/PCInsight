#include "linux_process_io.hpp"
#include <array>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <stdexcept>
#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> missing(std::string note){return {{},Support::unsupported,std::move(note)};}
std::string_view trim_io(std::string_view s){const auto first=s.find_first_not_of(" \t\r"),last=s.find_last_not_of(" \t\r");return first==s.npos?std::string_view{}:s.substr(first,last-first+1);}
#ifdef __linux__
struct Fd {int value;~Fd(){if(value>=0)::close(value);}};
Metric<std::string> io_error(int code){return {{},code==EACCES||code==EPERM?Support::permission_required:Support::unsupported,"Cannot read process I/O source (errno="+std::to_string(code)+")"};}
Metric<std::string> read_at(int dir,const char* name){
    const Fd fd{::openat(dir,name,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK)};
    if(fd.value<0){const int error=errno;return io_error(error);}
    std::array<char,8192> buffer{};std::string text;unsigned interrupts=0;
    for(;;){
        const auto bytes=::read(fd.value,buffer.data(),buffer.size());
        if(bytes<0){const int error=errno;if(error==EINTR&&++interrupts<=3)continue;return io_error(error);}
        if(!bytes)break;
        if(static_cast<std::size_t>(bytes)>65536-text.size())return missing<std::string>("Process I/O/stat source exceeds 64KiB limit");
        text.append(buffer.data(),static_cast<std::size_t>(bytes));
    }
    return {std::move(text),Support::available,"Bounded read through pinned process directory"};
}
#endif
}
Metric<LinuxProcessIoCounters> parse_linux_process_io_counters(std::string_view text){
    auto fail=[](){return missing<LinuxProcessIoCounters>("Process I/O counters absent, duplicate, invalid or over bounds");};
    if(text.empty()||text.size()>65536)return fail();
    std::optional<std::uint64_t> read,written;unsigned lines=0;
    while(!text.empty()){
        const auto newline=text.find('\n');const auto line=text.substr(0,newline);
        if(++lines>10000||line.find('\0')!=line.npos)return fail();
        const auto colon=line.find(':');const auto key=colon==line.npos?std::string_view{}:trim_io(line.substr(0,colon));
        if(key=="read_bytes"||key=="write_bytes"){
            const auto field=trim_io(line.substr(colon+1));if(field.empty())return fail();
            std::uint64_t value{};const auto [end,ec]=std::from_chars(field.data(),field.data()+field.size(),value);
            if(ec!=std::errc{}||end!=field.data()+field.size())return fail();
            auto& target=key=="read_bytes"?read:written;if(target)return fail();target=value;
        }
        if(newline==text.npos)break;
        text.remove_prefix(newline+1);
    }
    if(!read||!written)return fail();
    return {LinuxProcessIoCounters{*read,*written},Support::available,"Kernel read_bytes/write_bytes accounting; separate cumulative byte counters, no cancelled-write subtraction"};
}
Metric<LinuxProcessIoCounters> linux_process_io_for_identity(const LinuxProcessStat& expected,const std::optional<LinuxProcessStat>& before,const std::optional<LinuxProcessStat>& after,Metric<LinuxProcessIoCounters> counters){
    auto same=[&](const std::optional<LinuxProcessStat>& row){return row&&row->pid==expected.pid&&row->start_ticks==expected.start_ticks;};
    if(!expected.pid||!same(before)||!same(after))return missing<LinuxProcessIoCounters>("Process PID/start_ticks changed or absent around I/O read");
    return counters;
}
Metric<LinuxProcessIoCounters> read_linux_process_io_counters(const std::filesystem::path& process_dir,const LinuxProcessStat& expected){
    if(!expected.pid)return missing<LinuxProcessIoCounters>("Invalid expected process PID");
#ifdef __linux__
    const Fd dir{::open(process_dir.c_str(),O_PATH|O_DIRECTORY|O_CLOEXEC)};
    if(dir.value<0){const int error=errno;const auto error_result=io_error(error);return {{},error_result.support,error_result.note};}
    const auto first=read_at(dir.value,"stat");if(!first.value)return {{},first.support,first.note};
    const auto before=parse_linux_process_stat(*first.value);
    if(!before||before->pid!=expected.pid||before->start_ticks!=expected.start_ticks)return missing<LinuxProcessIoCounters>("Process identity differs before I/O read");
    const auto source=read_at(dir.value,"io");auto counters=source.value?parse_linux_process_io_counters(*source.value):Metric<LinuxProcessIoCounters>{{},source.support,source.note};
    const auto last=read_at(dir.value,"stat");if(!last.value)return {{},last.support,last.note};
    return linux_process_io_for_identity(expected,before,parse_linux_process_stat(*last.value),std::move(counters));
#else
    (void)process_dir;return missing<LinuxProcessIoCounters>("Native process I/O reader is Linux-only");
#endif
}
Metric<std::uint32_t> linux_process_io_uid(const Metric<LinuxProcessStatus>& before,const Metric<LinuxProcessStatus>& after){
    if(!metric_is_known(before)||!metric_is_known(after)||!before.value->real_uid||!after.value->real_uid)
        return {{},before.support==Support::permission_required||after.support==Support::permission_required?Support::permission_required:Support::unsupported,
            "Real UID unavailable around I/O counter read; no user attribution"};
    if(before.value->pid!=after.value->pid||!before.value->pid||before.value->real_uid!=after.value->real_uid)
        return missing<std::uint32_t>("PID/real UID changed around I/O counter read; no user attribution");
    return {*before.value->real_uid,Support::estimated,"Same real UID observed before/after I/O read; intermediate credential changes not excluded"};
}
Metric<std::uint32_t> LinuxProcessIoRateTracker::attributed_uid(std::uint32_t pid) const{
    const auto it=attributed_uids_.find(pid);
    return it==attributed_uids_.end()?missing<std::uint32_t>("No current user-attributed I/O rate"):it->second;
}
std::map<std::uint32_t,Metric<std::uint64_t>> LinuxProcessIoRateTracker::sample(std::span<const LinuxProcessIoObservation> rows){
    try{
        if(rows.size()>100000)throw std::invalid_argument("Too many process I/O observations");
        std::map<std::uint32_t,LinuxProcessIoObservation> next;
        std::map<std::uint32_t,Metric<std::uint64_t>> out;
        std::map<std::uint32_t,Metric<std::uint32_t>> owners;
        for(const auto& row:rows){
            if(!row.pid||out.contains(row.pid))throw std::invalid_argument("Invalid/duplicate process I/O PID");
            auto result=missing<std::uint64_t>("Requires two consecutive readable samples of the same process identity and increasing monotonic time");
            if(!row.counters.value||row.counters.support==Support::unsupported||row.counters.support==Support::permission_required){
                result={{},row.counters.support==Support::permission_required?Support::permission_required:Support::unsupported,row.counters.note};
            }else{
                const auto old=previous_.find(row.pid);
                if(old!=previous_.end()&&old->second.start_ticks==row.start_ticks){
                    const auto& previous=old->second;
                    const auto& a=*previous.counters.value;const auto& b=*row.counters.value;
                    const auto seconds=std::chrono::duration<long double>(row.at.time_since_epoch()).count()-std::chrono::duration<long double>(previous.at.time_since_epoch()).count();
                    if(row.at>previous.at&&std::isfinite(seconds)&&seconds>0&&b.read_bytes>=a.read_bytes&&b.write_bytes>=a.write_bytes){
                        const auto delta=static_cast<long double>(b.read_bytes-a.read_bytes)+static_cast<long double>(b.write_bytes-a.write_bytes);
                        const auto rate=delta/seconds;
                        if(std::isfinite(rate)&&rate>=0&&rate<std::ldexp(1.0L,64))result={static_cast<std::uint64_t>(rate),Support::estimated,"Process (read_bytes delta + write_bytes delta) / own monotonic sample interval, rounded down; storage-layer accounting, not physical device throughput; writes charged on dirtying, shared/waited-child attribution and cancelled writes have limits"};
                        else result=missing<std::uint64_t>("Process I/O byte rate outside uint64 range");
                    }else result=missing<std::uint64_t>("Process I/O counter decreased or sample time did not advance; interval discarded");
                }
                next.emplace(row.pid,row);
            }
            auto owner=missing<std::uint32_t>("No consecutive valid I/O rate with matching observed real UIDs");
            const auto old=previous_.find(row.pid);
            if(metric_is_known(result)&&old!=previous_.end()&&metric_is_known(row.real_uid)&&metric_is_known(old->second.real_uid)&&row.real_uid.value==old->second.real_uid.value)
                owner={row.real_uid.value,Support::estimated,"Same real UID around both endpoint I/O reads; sampled attribution, not continuous credential/causal ownership proof"};
            else if(row.real_uid.support==Support::permission_required||(old!=previous_.end()&&old->second.real_uid.support==Support::permission_required))
                owner={{},Support::permission_required,"Real UID denied at an I/O interval endpoint"};
            owners.emplace(row.pid,std::move(owner));
            out.emplace(row.pid,std::move(result));
        }
        previous_=std::move(next);attributed_uids_=std::move(owners);return out;
    }catch(...){reset();throw;}
}
}
