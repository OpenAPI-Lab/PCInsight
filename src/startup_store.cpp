#include "startup_birth_archive.hpp"
#include "startup_store_bytes.hpp"
#include "pcinsight/startup_overlay.hpp"
#include <atomic>
#include <array>
#include <limits>
#include <string_view>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace pcinsight {
namespace {
constexpr std::size_t file_limit=16*1024*1024;
constexpr std::string_view magic{"PCISTART",8};
std::atomic<std::uint64_t> serial{};
std::uint64_t hash(std::string_view bytes) {
    std::uint64_t value=14695981039346656037ULL;
    for(unsigned char c:bytes){value^=c;value*=1099511628211ULL;}return value;
}
void put(std::string& bytes,std::uint64_t value,unsigned width) {
    for(unsigned i=0;i<width;++i)bytes.push_back(static_cast<char>((value>>(8*i))&255));
}
void put_string(std::string& bytes,const std::string& text) {put(bytes,text.size(),4);bytes+=text;}
struct Reader {
    std::string_view bytes;std::size_t offset{};bool ok{true};
    std::uint64_t get(unsigned width) {
        if(width>bytes.size()-offset){ok=false;return 0;}
        std::uint64_t value=0;
        for(unsigned i=0;i<width;++i)value|=static_cast<std::uint64_t>(static_cast<unsigned char>(bytes[offset++]))<<(8*i);
        return value;
    }
    std::string text(std::size_t limit) {
        const auto size=get(4);
        if(!ok || size>limit || size>bytes.size()-offset){ok=false;return {};}
        std::string value(bytes.substr(offset,static_cast<std::size_t>(size)));offset+=static_cast<std::size_t>(size);return value;
    }
};
template<class T> void metric(std::string& bytes,const Metric<T>& value,std::uint64_t number) {
    put(bytes,static_cast<unsigned>(value.support),1);put(bytes,value.value.has_value(),1);
    if(value.value)put(bytes,number,8);
    put_string(bytes,value.note);
}
template<class T,class Convert> Metric<T> metric(Reader& in,Convert convert) {
    Metric<T> value;const auto support=in.get(1),present=in.get(1);
    if(support>static_cast<unsigned>(Support::estimated) || present>1){in.ok=false;return value;}
    value.support=static_cast<Support>(support);
    if(present)value.value=convert(in.get(8),in.ok);
    value.note=in.text(4096);return value;
}
// Conservative overflow checks before duration_cast, also on platforms whose
// steady/system clocks have a different integral tick period.
template<class To,class From> std::optional<To> duration(From from) {
    using Ratio=std::ratio_divide<typename From::period,typename To::period>;
    if(from.count()<0 || static_cast<std::uint64_t>(from.count())>
        static_cast<std::uint64_t>(std::numeric_limits<typename To::rep>::max())/Ratio::num)return {};
    return std::chrono::duration_cast<To>(from);
}
bool text_valid(const std::string& s,std::size_t max,bool empty=false){return (empty||!s.empty())&&s.size()<=max&&s.find('\0')==s.npos;}
template<class T> bool birth_metric_valid(const Metric<T>& m){return text_valid(m.note,4096,true)&&static_cast<unsigned>(m.support)<=static_cast<unsigned>(Support::estimated)&&
    (m.value?(m.support==Support::available||m.support==Support::estimated):(m.support==Support::unsupported||m.support==Support::permission_required));}
std::string validate_births(const StartupBirthArchive& a,const std::string& outer){
    const auto& c=a.capture;const auto& p=a.policy;
    if(!text_valid(c.capture_id,128)||c.capture_id==outer||!(c.boot[0]||c.boot[1])||!c.pid_namespace_device||!c.pid_namespace_inode||c.quality_flags&~127ULL||c.publishing>64||c.observations.size()>64||
       (c.state!=StartupBirthObserverState::closed&&c.state!=StartupBirthObserverState::failed)||static_cast<unsigned>(c.error)>static_cast<unsigned>(ActionError::stale_target)||
       (c.state==StartupBirthObserverState::closed&&c.error!=ActionError::none)||(c.state==StartupBirthObserverState::failed&&c.error==ActionError::none)||
       !text_valid(c.message,4096,true)||!text_valid(c.pid_mapping_note,4096,true)||!birth_metric_valid(c.automatic_startup_origin)||c.automatic_startup_origin.value||
       (!c.first_exec_enabled&&(c.quality_flags&64))||a.window.count()<=0||a.window>std::chrono::hours(1)||p.medium_cpu.count()<=0||p.high_cpu<p.medium_cpu||!p.medium_disk_bytes||p.high_disk_bytes<p.medium_disk_bytes)
        return "invalid historical birth capture metadata/window/policy";
    for(std::size_t n=0;n<c.observations.size();++n){const auto& r=c.observations[n];const auto& k=r.key;
        if(k.capture_id!=c.capture_id||k.boot!=c.boot||!k.kernel_tgid||k.kernel_tgid>2147483647U||!k.kernel_start_boottime_ns||!r.birth_monotonic_ns||
           !birth_metric_valid(r.startup_entry_id)||!birth_metric_valid(r.local_pid)||!birth_metric_valid(r.cpu_time)||!birth_metric_valid(r.read_bytes)||!birth_metric_valid(r.write_bytes)||!birth_metric_valid(r.first_executable)||
           (r.startup_entry_id.value&&!text_valid(*r.startup_entry_id.value,32768))||(r.cpu_time.value&&r.cpu_time.value->count()<0)||
           (!r.final_monotonic_ns&&(r.cpu_time.value||r.read_bytes.value||r.write_bytes.value))||
           (r.final_monotonic_ns&&(!c.final_resources_enabled||*r.final_monotonic_ns<r.birth_monotonic_ns||!r.read_bytes.value||!r.write_bytes.value))||
           r.birth_monotonic_ns>UINT64_MAX-static_cast<std::uint64_t>(a.window.count()))return "invalid historical birth identity/binding/resources";
        if(r.local_pid.value){const auto& v=*r.local_pid.value;if(!c.pid_mapping_enabled||v.namespace_device!=c.pid_namespace_device||v.namespace_inode!=c.pid_namespace_inode||!v.tgid||v.tgid>2147483647U)return "invalid historical namespace PID";}
        else if(c.pid_mapping_enabled&&!(c.quality_flags&32))return "missing historical namespace PID without loss flag";
        if(r.first_executable.value){const auto& e=*r.first_executable.value;if(!c.first_exec_enabled||e.device_major>4095||e.device_minor>1048575||e.observed_monotonic_ns<r.birth_monotonic_ns||(r.final_monotonic_ns&&e.observed_monotonic_ns>*r.final_monotonic_ns))return "invalid historical first exec";}
        for(std::size_t j=0;j<n;++j)if(c.observations[j].key.kernel_tgid==k.kernel_tgid&&c.observations[j].key.kernel_start_boottime_ns==k.kernel_start_boottime_ns)return "duplicate historical birth group";
        const auto projected=measure_startup_birth_impact(c,k,a.window,p);
        if(!projected&&projected.error!=ActionError::not_found)return "unprojectable historical birth row";
    }
    return {};
}
template<class T,class Write> void compound(std::string& bytes,const Metric<T>& m,Write write){
    put(bytes,static_cast<unsigned>(m.support),1);put(bytes,m.value.has_value(),1);if(m.value)write(*m.value);put_string(bytes,m.note);
}
template<class T,class Read> Metric<T> compound(Reader& in,Read read){
    Metric<T> m;const auto support=in.get(1),present=in.get(1);if(support>3||present>1){in.ok=false;return m;}
    m.support=static_cast<Support>(support);if(present)m.value=read();m.note=in.text(4096);return m;
}
void encode_births(std::string& b,const StartupBirthArchive& a){
    const auto& c=a.capture;put_string(b,c.capture_id);for(auto v:c.boot)put(b,v,8);
    for(auto v:{c.pid_namespace_device,c.pid_namespace_inode,c.quality_flags})put(b,v,8);
    put(b,c.publishing,4);put(b,static_cast<unsigned>(c.state),1);put(b,static_cast<unsigned>(c.error),1);
    for(bool v:{c.creator_exited,c.pid_mapping_enabled,c.final_resources_enabled,c.first_exec_enabled})put(b,v,1);
    put_string(b,c.pid_mapping_note);put_string(b,c.message);metric(b,c.automatic_startup_origin,0);
    put(b,static_cast<std::uint64_t>(a.window.count()),8);put(b,static_cast<std::uint64_t>(a.policy.medium_cpu.count()),8);put(b,static_cast<std::uint64_t>(a.policy.high_cpu.count()),8);
    put(b,a.policy.medium_disk_bytes,8);put(b,a.policy.high_disk_bytes,8);put(b,c.observations.size(),4);
    for(const auto& r:c.observations){
        put(b,r.key.kernel_tgid,4);put(b,r.key.kernel_real_uid,4);put(b,r.key.kernel_start_boottime_ns,8);put(b,r.birth_monotonic_ns,8);
        put(b,r.final_monotonic_ns.has_value(),1);if(r.final_monotonic_ns)put(b,*r.final_monotonic_ns,8);
        compound(b,r.startup_entry_id,[&](const auto& s){put_string(b,s);});
        compound(b,r.local_pid,[&](const auto& p){put(b,p.namespace_device,8);put(b,p.namespace_inode,8);put(b,p.tgid,4);});
        metric(b,r.cpu_time,r.cpu_time.value?static_cast<std::uint64_t>(r.cpu_time.value->count()):0);metric(b,r.read_bytes,r.read_bytes.value.value_or(0));metric(b,r.write_bytes,r.write_bytes.value.value_or(0));
        compound(b,r.first_executable,[&](const auto& e){put(b,e.observed_monotonic_ns,8);put(b,e.inode,8);put(b,e.device_major,4);put(b,e.device_minor,4);put(b,e.kernel_real_uid,4);});
    }
}
StartupBirthArchive decode_births(Reader& in){
    StartupBirthArchive a;auto& c=a.capture;c.capture_id=in.text(128);for(auto& v:c.boot)v=in.get(8);
    c.pid_namespace_device=in.get(8);c.pid_namespace_inode=in.get(8);c.quality_flags=in.get(8);c.publishing=static_cast<std::uint32_t>(in.get(4));
    c.state=static_cast<StartupBirthObserverState>(in.get(1));c.error=static_cast<ActionError>(in.get(1));
    auto boolean=[&]{const auto v=in.get(1);if(v>1)in.ok=false;return v!=0;};
    c.creator_exited=boolean();c.pid_mapping_enabled=boolean();c.final_resources_enabled=boolean();c.first_exec_enabled=boolean();
    c.pid_mapping_note=in.text(4096);c.message=in.text(4096);c.automatic_startup_origin=metric<bool>(in,[](auto v,bool& ok){if(v>1)ok=false;return v!=0;});
    auto nanos=[&]{const auto v=in.get(8);if(v>static_cast<std::uint64_t>(INT64_MAX)){in.ok=false;return std::chrono::nanoseconds{};}return std::chrono::nanoseconds(static_cast<std::int64_t>(v));};
    a.window=nanos();a.policy.medium_cpu=nanos();a.policy.high_cpu=nanos();a.policy.medium_disk_bytes=in.get(8);a.policy.high_disk_bytes=in.get(8);
    const auto count=in.get(4);if(!in.ok||count>64){in.ok=false;return a;}
    for(std::uint64_t n=0;n<count&&in.ok;++n){StartupBirthObservation r;r.key.capture_id=c.capture_id;r.key.boot=c.boot;
        r.key.kernel_tgid=static_cast<std::uint32_t>(in.get(4));r.key.kernel_real_uid=static_cast<std::uint32_t>(in.get(4));r.key.kernel_start_boottime_ns=in.get(8);r.birth_monotonic_ns=in.get(8);
        if(boolean())r.final_monotonic_ns=in.get(8);
        r.startup_entry_id=compound<std::string>(in,[&]{return in.text(32768);});
        r.local_pid=compound<StartupBirthPid>(in,[&]{StartupBirthPid p;p.namespace_device=in.get(8);p.namespace_inode=in.get(8);p.tgid=static_cast<std::uint32_t>(in.get(4));return p;});
        r.cpu_time=metric<std::chrono::nanoseconds>(in,[](auto v,bool& ok){if(v>static_cast<std::uint64_t>(INT64_MAX)){ok=false;return std::chrono::nanoseconds{};}return std::chrono::nanoseconds(static_cast<std::int64_t>(v));});
        r.read_bytes=metric<std::uint64_t>(in,[](auto v,bool&){return v;});r.write_bytes=metric<std::uint64_t>(in,[](auto v,bool&){return v;});
        r.first_executable=compound<StartupBirthObservation::Executable>(in,[&]{StartupBirthObservation::Executable e;e.observed_monotonic_ns=in.get(8);e.inode=in.get(8);e.device_major=static_cast<std::uint32_t>(in.get(4));e.device_minor=static_cast<std::uint32_t>(in.get(4));e.kernel_real_uid=static_cast<std::uint32_t>(in.get(4));return e;});
        c.observations.push_back(std::move(r));
    }return a;
}
auto counters(StartupCaptureDiagnostics& d){
    return std::array<std::uint64_t*,16>{&d.scans,&d.partial_scans,&d.examined,&d.ignored,&d.unavailable,&d.registered,&d.rejected,
        &d.poll_attempted,&d.poll_updated,&d.poll_failed,&d.exit_disk_attempted,&d.exit_disk_attached,&d.exit_disk_skipped,&d.exit_disk_unavailable,&d.exit_disk_recovered,&d.exit_disk_released_without_final};
}
std::string validate_diagnostics(const StartupCaptureDiagnostics& d,std::size_t rows){
    using namespace std::chrono_literals;
    if(d.requested_duration<10ms||d.requested_duration>1h||d.scan_interval<10ms||d.scan_interval>60s||d.elapsed.count()<0)
        return "invalid recorder diagnostic durations";
    if(d.partial_scans>d.scans||d.registered!=rows||d.poll_updated>d.poll_attempted||d.poll_failed!=d.poll_attempted-d.poll_updated)
        return "inconsistent recorder scan/observation/poll counts";
    if(d.exit_disk_attempted>d.registered||d.exit_disk_attached>d.exit_disk_attempted||d.exit_disk_unavailable>d.exit_disk_attempted||
       d.exit_disk_recovered>d.exit_disk_attached||d.exit_disk_recovered>d.exit_disk_attempted-d.exit_disk_unavailable||
       d.exit_disk_released_without_final!=d.exit_disk_attempted-d.exit_disk_unavailable-d.exit_disk_recovered||
       d.exit_disk_released_without_final>d.exit_disk_attached-d.exit_disk_recovered)
        return "inconsistent recorder exit-disk counts";
    if(d.exit_disk_enabled){
        if(!d.max_exit_disk_captures||d.max_exit_disk_captures>32||d.exit_disk_skipped!=d.registered-d.exit_disk_attempted)
            return "invalid exit-disk diagnostic bound/admission counts";
    }else if(d.exit_disk_attempted||d.exit_disk_attached||d.exit_disk_skipped||d.exit_disk_unavailable||d.exit_disk_recovered||d.exit_disk_released_without_final)
        return "disabled exit-disk diagnostics cannot claim activity";
    if(static_cast<unsigned>(d.automatic_startup_origin.support)>static_cast<unsigned>(Support::estimated)||d.automatic_startup_origin.note.size()>4096)
        return "invalid startup-origin availability metadata";
    return {};
}
std::string validate(const StartupImpactArchive& archive) {
    if(archive.capture_id.empty() || archive.capture_id.size()>256 || archive.capture_id.find('\0')!=archive.capture_id.npos ||
        archive.recorded_at.time_since_epoch().count()<0 ||
        !duration<std::chrono::milliseconds>(archive.recorded_at.time_since_epoch()))return "invalid capture identity or recorded time";
    for(const auto& row:archive.observations) {
        if(!row.closed || static_cast<unsigned>(row.last_error)>static_cast<unsigned>(ActionError::stale_target))
            return "archive requires closed observations and valid errors";
        if(row.measurement) {
            const auto& m=*row.measurement;
            if(static_cast<unsigned>(m.cpu_time.support)>static_cast<unsigned>(Support::estimated) ||
                static_cast<unsigned>(m.disk_bytes.support)>static_cast<unsigned>(Support::estimated) ||
                static_cast<unsigned>(m.impact.support)>static_cast<unsigned>(Support::estimated) ||
                !duration<std::chrono::nanoseconds>(m.observation_interval))return "invalid support or interval";
        }
    }
    if(archive.diagnostics){const auto error=validate_diagnostics(*archive.diagnostics,archive.observations.size());if(!error.empty())return error;}
    if(archive.births){const auto error=validate_births(*archive.births,archive.capture_id);if(!error.empty())return error;}
    SystemSnapshot empty;const auto checked=apply_startup_impact_observations(empty,archive.observations,archive.native_context);
    return checked?std::string{}:checked.message;
}
std::string encode(const StartupImpactArchive& archive) {
    std::string bytes(magic);put(bytes,archive.births?3:archive.diagnostics?2:1,4);put_string(bytes,archive.capture_id);put_string(bytes,archive.native_context);
    put(bytes,duration<std::chrono::milliseconds>(archive.recorded_at.time_since_epoch())->count(),8);
    put(bytes,archive.observations.size(),4);
    for(const auto& row:archive.observations) {
        put_string(bytes,row.startup_entry_id);put(bytes,row.pid,4);put(bytes,row.linux_start_ticks,8);put(bytes,row.sample_count,8);
        put(bytes,1u|(row.window_complete?2u:0u)|(row.sampling_gap?4u:0u),1);
        put(bytes,static_cast<unsigned>(row.last_error),1);put(bytes,row.measurement.has_value(),1);
        if(row.measurement) {
            const auto& m=*row.measurement;put(bytes,duration<std::chrono::nanoseconds>(m.observation_interval)->count(),8);
            metric(bytes,m.cpu_time,m.cpu_time.value?static_cast<std::uint64_t>(m.cpu_time.value->count()):0);
            metric(bytes,m.disk_bytes,m.disk_bytes.value.value_or(0));
            metric(bytes,m.impact,m.impact.value?static_cast<unsigned>(*m.impact.value):0);
        }
    }
    if(archive.births)put(bytes,archive.diagnostics.has_value(),1);
    if(archive.diagnostics){
        auto d=*archive.diagnostics;
        put(bytes,d.requested_duration.count(),8);put(bytes,d.scan_interval.count(),8);put(bytes,d.elapsed.count(),8);
        put(bytes,d.exit_disk_enabled,1);put(bytes,d.max_exit_disk_captures,4);
        for(const auto p:counters(d))put(bytes,*p,8);
        metric(bytes,d.automatic_startup_origin,d.automatic_startup_origin.value.value_or(false));
    }
    if(archive.births)encode_births(bytes,*archive.births);
    put(bytes,hash(bytes),8);return bytes;
}
StartupArchiveLoadResult decode(std::string_view bytes) {
    auto fail=[](const char* message){return StartupArchiveLoadResult{std::nullopt,ActionError::invalid_argument,message};};
    if(bytes.size()<40 || bytes.substr(0,8)!=magic)return fail("invalid startup archive header");
    Reader checksum{bytes.substr(bytes.size()-8)};
    if(checksum.get(8)!=hash(bytes.substr(0,bytes.size()-8)))return fail("startup archive checksum mismatch");
    Reader in{bytes.substr(0,bytes.size()-8),8};
    const auto version=in.get(4);if(version!=1&&version!=2&&version!=3)return fail("unsupported startup archive version");
    StartupImpactArchive archive;archive.capture_id=in.text(256);archive.native_context=in.text(1024);
    const auto recorded=in.get(8),count=in.get(4);
    if(!in.ok || recorded>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) || count>4096 || count>(in.bytes.size()-in.offset)/27)
        return fail("invalid startup archive time/count");
    const auto time=duration<std::chrono::system_clock::duration>(std::chrono::milliseconds(static_cast<std::int64_t>(recorded)));
    if(!time)return fail("startup archive time overflow");
    archive.recorded_at=std::chrono::system_clock::time_point(*time);archive.observations.reserve(static_cast<std::size_t>(count));
    for(std::uint64_t i=0;i<count;++i) {
        StartupImpactObservation row;row.startup_entry_id=in.text(32768);row.pid=static_cast<std::uint32_t>(in.get(4));
        row.linux_start_ticks=in.get(8);const auto samples=in.get(8),flags=in.get(1),error=in.get(1),present=in.get(1);
        if(!in.ok || samples>std::numeric_limits<std::size_t>::max() || flags>7 || !(flags&1) ||
            error>static_cast<unsigned>(ActionError::stale_target) || present>1)return fail("invalid startup archive observation");
        row.sample_count=static_cast<std::size_t>(samples);row.closed=true;row.window_complete=flags&2;row.sampling_gap=flags&4;
        row.last_error=static_cast<ActionError>(error);row.native_context=archive.native_context;
        if(present) {
            StartupImpactMeasurement m;m.startup_entry_id=row.startup_entry_id;m.pid=row.pid;m.linux_start_ticks=row.linux_start_ticks;
            const auto ns=in.get(8);
            if(ns>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))return fail("startup interval overflow");
            const auto interval=duration<std::chrono::steady_clock::duration>(std::chrono::nanoseconds(static_cast<std::int64_t>(ns)));
            if(!interval || duration<std::chrono::nanoseconds>(*interval)!=std::chrono::nanoseconds(static_cast<std::int64_t>(ns)))
                return fail("startup interval unrepresentable on this platform");
            m.observation_interval=*interval;
            m.cpu_time=metric<std::chrono::nanoseconds>(in,[](auto n,bool& ok){if(n>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())){ok=false;return std::chrono::nanoseconds{};}return std::chrono::nanoseconds(static_cast<std::int64_t>(n));});
            m.disk_bytes=metric<std::uint64_t>(in,[](auto n,bool&){return n;});
            m.impact=metric<StartupImpact>(in,[](auto n,bool& ok){if(n<static_cast<unsigned>(StartupImpact::low) || n>static_cast<unsigned>(StartupImpact::high)){ok=false;return StartupImpact::not_measured;}return static_cast<StartupImpact>(n);});
            row.measurement=std::move(m);
        }
        if(!in.ok)return fail("truncated/invalid startup archive metric");
        archive.observations.push_back(std::move(row));
    }
    const auto has_diagnostics=version==3?in.get(1):version==2?1U:0U;
    if(has_diagnostics>1)return fail("invalid startup diagnostic presence");
    if(has_diagnostics){
        StartupCaptureDiagnostics d;const auto requested=in.get(8),scan=in.get(8),elapsed=in.get(8),enabled=in.get(1),bound=in.get(4);
        constexpr auto max=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
        if(!in.ok||requested>max||scan>max||elapsed>max||enabled>1)return fail("invalid startup diagnostic durations/flags");
        d.requested_duration=std::chrono::milliseconds(static_cast<std::int64_t>(requested));d.scan_interval=std::chrono::milliseconds(static_cast<std::int64_t>(scan));
        d.elapsed=std::chrono::nanoseconds(static_cast<std::int64_t>(elapsed));d.exit_disk_enabled=enabled;d.max_exit_disk_captures=static_cast<std::uint32_t>(bound);
        for(auto p:counters(d))*p=in.get(8);
        d.automatic_startup_origin=metric<bool>(in,[](auto n,bool& ok){if(n>1)ok=false;return n!=0;});
        if(!in.ok)return fail("truncated/invalid startup diagnostics");
        archive.diagnostics=std::move(d);
    }
    if(version==3){archive.births=decode_births(in);if(!in.ok)return fail("truncated/invalid historical birth data");}
    if(in.offset!=in.bytes.size())return fail("trailing startup archive data");
    const auto error=validate(archive);
    if(!error.empty())return {std::nullopt,ActionError::invalid_argument,error};
    return {std::move(archive),ActionError::none,"historical capture loaded; not a live observer checkpoint"};
}
bool path_valid(const std::filesystem::path& path) {
    const auto& text=path.native();const auto filename=path.filename();
    return !text.empty() && text.size()<=32768 && text.find(typename std::filesystem::path::value_type{})==text.npos &&
        !filename.empty() && filename!="." && filename!="..";
}
#ifdef _WIN32
ActionError io_error(DWORD error) {
    return error==ERROR_ACCESS_DENIED?ActionError::permission_denied:
        (error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND)?ActionError::not_found:ActionError::system_error;
}
#else
struct Fd {int fd{-1};~Fd(){if(fd>=0)close(fd);}};
ActionError io_error(int error) {
    return (error==EACCES || error==EPERM)?ActionError::permission_denied:
        (error==ENOENT || error==ENOTDIR)?ActionError::not_found:ActionError::system_error;
}
#endif
}
detail::StartupBytesLoad detail::read_startup_archive_bytes(const std::filesystem::path& path) {
    if(!path_valid(path))return {std::nullopt,ActionError::invalid_argument,"invalid startup archive path"};
    std::string bytes;
#ifdef _WIN32
    HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(file==INVALID_HANDLE_VALUE)return {std::nullopt,io_error(GetLastError()),"cannot open startup archive"};
    struct Handle {HANDLE h;~Handle(){CloseHandle(h);}} handle{file};
    BY_HANDLE_FILE_INFORMATION info{};
    if(!GetFileInformationByHandle(file,&info) || GetFileType(file)!=FILE_TYPE_DISK ||
        (info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)) || info.nFileSizeHigh || info.nFileSizeLow>file_limit)
        return {std::nullopt,ActionError::invalid_argument,"archive must be a bounded regular non-reparse file"};
    char chunk[4096];DWORD n{};
    for(;;){if(!ReadFile(file,chunk,sizeof chunk,&n,nullptr))return {std::nullopt,ActionError::system_error,"startup archive read failed"};
        if(!n)break;
        if(n>file_limit-bytes.size())return {std::nullopt,ActionError::invalid_argument,"startup archive size limit"};
        bytes.append(chunk,n);}
#else
    Fd file{open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NONBLOCK|O_NOFOLLOW)};
    if(file.fd<0)return {std::nullopt,io_error(errno),"cannot open startup archive"};
    struct stat info{};
    if(fstat(file.fd,&info)!=0 || !S_ISREG(info.st_mode) || info.st_size<0 || static_cast<std::uint64_t>(info.st_size)>file_limit)
        return {std::nullopt,ActionError::invalid_argument,"archive must be a bounded regular non-symlink file"};
    char chunk[4096];
    for(;;){const auto n=read(file.fd,chunk,sizeof chunk);if(n<0){if(errno==EINTR)continue;return {std::nullopt,io_error(errno),"startup archive read failed"};}
        if(!n)break;
        if(static_cast<std::size_t>(n)>file_limit-bytes.size())return {std::nullopt,ActionError::invalid_argument,"startup archive size limit"};
        bytes.append(chunk,static_cast<std::size_t>(n));}
#endif
    return {std::move(bytes),ActionError::none,"historical bytes read"};
}
StartupArchiveIoResult detail::write_startup_archive_bytes(const std::filesystem::path& path,std::string_view bytes,bool require_private) {
    if(!path_valid(path))return {ActionError::invalid_argument,false,"invalid startup archive path"};
    if(bytes.size()>file_limit)return {ActionError::invalid_argument,false,"startup archive size limit"};
#ifdef _WIN32
    (void)require_private; // Windows inherits caller-selected directory ACL; no POSIX mode claim.
    const auto attr=GetFileAttributesW(path.c_str());
    if(attr!=INVALID_FILE_ATTRIBUTES && (attr&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))
        return {ActionError::invalid_argument,false,"refusing directory/reparse archive destination"};
    HANDLE file=INVALID_HANDLE_VALUE;std::filesystem::path temp;
    for(unsigned attempt=0;attempt<128;++attempt){temp=path;temp+=L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(serial.fetch_add(1));
        file=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file!=INVALID_HANDLE_VALUE)break;
        if(GetLastError()!=ERROR_FILE_EXISTS)return {io_error(GetLastError()),false,"cannot create temporary startup archive"};}
    if(file==INVALID_HANDLE_VALUE)return {ActionError::system_error,false,"startup archive temporary collisions"};
    BY_HANDLE_FILE_INFORMATION temporary_info{};
    if(GetFileType(file)!=FILE_TYPE_DISK || !GetFileInformationByHandle(file,&temporary_info) ||
        (temporary_info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))) {
        CloseHandle(file);DeleteFileW(temp.c_str());
        return {ActionError::invalid_argument,false,"temporary startup archive is not a regular file"};
    }
    DWORD written{},native_error{};const char* stage="write";
    bool ok=WriteFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)!=0;
    if(!ok)native_error=GetLastError();else if(written!=bytes.size()){ok=false;native_error=ERROR_WRITE_FAULT;}
    if(ok){stage="flush";ok=FlushFileBuffers(file)!=0;if(!ok)native_error=GetLastError();}
    if(!CloseHandle(file)){if(ok){stage="close";native_error=GetLastError();}ok=false;}
    if(ok){stage="replace";ok=MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;if(!ok)native_error=GetLastError();}
    if(!ok){DeleteFileW(temp.c_str());return {ActionError::system_error,false,std::string("startup archive ")+stage+" failed, Windows error "+std::to_string(native_error)};}
#else
    const auto parent=path.parent_path().empty()?std::filesystem::path("."):path.parent_path();
    Fd dir{open(parent.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
    if(dir.fd<0)return {io_error(errno),false,"cannot open startup archive directory"};
    const auto name=path.filename().string();struct stat old{};
    if(fstatat(dir.fd,name.c_str(),&old,AT_SYMLINK_NOFOLLOW)==0){if(!S_ISREG(old.st_mode))return {ActionError::invalid_argument,false,"refusing non-regular archive destination"};}
    else if(errno!=ENOENT)return {io_error(errno),false,"cannot inspect archive destination"};
    std::string temp;int descriptor=-1;
    for(unsigned attempt=0;attempt<128;++attempt){temp=".pcinsight-startup-"+std::to_string(getpid())+"-"+std::to_string(serial.fetch_add(1))+".tmp";
        descriptor=openat(dir.fd,temp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
        if(descriptor>=0)break;
        if(errno!=EEXIST)return {io_error(errno),false,"cannot create temporary startup archive"};}
    if(descriptor<0)return {ActionError::system_error,false,"startup archive temporary collisions"};
    if(require_private){struct stat created{};if(fstat(descriptor,&created)!=0||!S_ISREG(created.st_mode)||(created.st_mode&0777)!=0600||created.st_uid!=geteuid()){
        close(descriptor);unlinkat(dir.fd,temp.c_str(),0);return {ActionError::not_supported,false,"Filesystem did not confirm private 0600/current-owner file; no payload written"};
    }}
    std::size_t offset=0;int code=0;
    while(offset<bytes.size()){const auto n=write(descriptor,bytes.data()+offset,bytes.size()-offset);
        if(n<0 && errno==EINTR)continue;
        if(n<=0){code=n<0?errno:EIO;break;}
        offset+=static_cast<std::size_t>(n);}
    if(!code && fsync(descriptor)!=0)code=errno;
    if(close(descriptor)!=0 && !code)code=errno;
    if(!code && renameat(dir.fd,temp.c_str(),dir.fd,name.c_str())!=0)code=errno;
    if(code){unlinkat(dir.fd,temp.c_str(),0);return {io_error(code),false,"startup archive replacement failed"};}
    if(fsync(dir.fd)!=0)return {io_error(errno),true,"archive replaced, directory durability confirmation failed; no rollback"};
#endif
    return {ActionError::none,true,"historical capture saved"};
}
StartupArchiveLoadResult StartupImpactStore::load(const std::filesystem::path& path) {
    const auto read=detail::read_startup_archive_bytes(path);if(!read.bytes)return {{},read.error,read.message};return decode(*read.bytes);
}
StartupArchiveIoResult StartupImpactStore::save(const std::filesystem::path& path,const StartupImpactArchive& archive) {
    if(!path_valid(path))return {ActionError::invalid_argument,false,"invalid startup archive path"};
    const auto error=validate(archive);if(!error.empty())return {ActionError::invalid_argument,false,error};
    const auto bytes=encode(archive);return detail::write_startup_archive_bytes(path,bytes);
}
}
namespace pcinsight::detail {
std::string validate_startup_birth_archive(const StartupBirthArchive& a){return validate_births(a,{});}
Metric<std::string> encode_startup_archive_bytes(const StartupImpactArchive& a){
    const auto error=validate(a);if(!error.empty())return {{},Support::unsupported,error};
    auto bytes=encode(a);if(bytes.size()>file_limit)return {{},Support::unsupported,"startup archive size limit"};return {std::move(bytes),Support::available,"PCISTART encoded in memory"};
}
StartupArchiveLoadResult decode_startup_archive_bytes(std::string_view bytes){
    if(bytes.size()>file_limit)return {{},ActionError::invalid_argument,"startup archive size limit"};
    return decode(bytes);
}
}
