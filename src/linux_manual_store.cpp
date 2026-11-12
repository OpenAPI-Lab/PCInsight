#include "linux_manual_store.hpp"
#include "startup_birth_archive.hpp"
#include "startup_store_bytes.hpp"
namespace pcinsight::detail {
namespace {
constexpr std::string_view magic{"PCIMAN\0\0",8};constexpr std::size_t limit=16*1024*1024;
std::uint64_t hash(std::string_view s){std::uint64_t h=14695981039346656037ULL;for(unsigned char c:s){h^=c;h*=1099511628211ULL;}return h;}
void put(std::string& s,std::uint64_t n,unsigned bytes=8){for(unsigned i=0;i<bytes;++i)s+=static_cast<char>(n>>(i*8));}
void text(std::string& s,std::string_view v){put(s,v.size(),4);s+=v;}
template<class T,class F>void metric(std::string& s,const Metric<T>& m,F f){put(s,static_cast<unsigned>(m.support),1);put(s,m.value.has_value(),1);if(m.value)f(*m.value);text(s,m.note);}
bool valid_text(const std::string& s,std::size_t max){return s.size()<=max&&s.find('\0')==s.npos;}
template<class T>bool shape(const Metric<T>& m){return valid_text(m.note,4096)&&static_cast<unsigned>(m.support)<=static_cast<unsigned>(Support::estimated)&&(m.value?(m.support==Support::available||m.support==Support::estimated):(m.support==Support::unsupported||m.support==Support::permission_required));}
Metric<LinuxDesktopLaunchReceipt> canonical(const Metric<LinuxDesktopLaunchReceipt>& m){
    if(!shape(m))return {{},Support::unsupported,"Invalid receipt metric shape/note"};
    if(!m.value)return m;
    const auto& r=*m.value;
    if(r.origin!=LinuxDesktopLaunchOrigin::manual_request||!shape(r.evidence.first_exec)||!shape(r.evidence.pinned_file_matches))return {{},Support::unsupported,"Invalid manual origin/exec metric shape"};
    const Metric<SelectedExecRead> source{SelectedExecRead{r.selected,r.evidence,r.pidfd_exited},m.support,"historical recorded evidence, NOT live authentication"};
    auto checked=project_linux_manual_receipt(r.route,r.selected,r.pinned_file,r.caller_owned_pid,r.launch_error,source);
    if(!checked.value||checked.support!=m.support)return {{},Support::unsupported,"Invalid historical receipt: "+checked.note};
    checked.note=m.note;return checked;
}
std::string validate(const LinuxManualLaunchArchive& a){
    auto error=validate_startup_birth_archive(a.births);if(!error.empty())return error;
    if(!shape(a.receipt))return "Invalid outer receipt metric";
    const auto r=canonical(a.receipt);if(a.receipt.value&&!r.value)return r.note;
    if(r.value){const auto& c=r.value->selected.local;const auto& b=a.births.capture;
        if(c.boot!=b.boot||c.observer.pid_namespace_device!=b.pid_namespace_device||c.observer.pid_namespace_inode!=b.pid_namespace_inode)return "Receipt/birth boot or observer namespace mismatch";
    }return {};
}
void self(std::string& s,const LinuxNetworkSelfIdentity& x){for(auto n:{std::uint64_t{x.local_pid},std::uint64_t{x.local_real_uid},x.start_ticks,x.procfs_device,x.procfs_inode,x.process_inode,x.pid_namespace_device,x.pid_namespace_inode,x.user_namespace_device,x.user_namespace_inode})put(s,n);}
void receipt(std::string& s,const LinuxDesktopLaunchReceipt& r){
    put(s,static_cast<unsigned>(r.origin),1);text(s,r.route.entry_id);const auto& q=r.route.request;text(s,q.executable);put(s,q.arguments.size(),4);for(const auto& a:q.arguments)text(s,a);text(s,q.working_directory);put(s,q.elevated,1);
    const auto& k=r.selected.kernel;put(s,k.kernel_tgid);put(s,k.kernel_real_uid);put(s,k.leader_start_boottime_ns);for(auto n:r.selected.local.boot)put(s,n);self(s,r.selected.local.observer);self(s,r.selected.local.process);
    put(s,r.pinned_file.inode);put(s,r.pinned_file.device_major);put(s,r.pinned_file.device_minor);put(s,r.caller_owned_pid);put(s,static_cast<unsigned>(r.launch_error),1);
    metric(s,r.evidence.first_exec,[&](const auto& e){put(s,e.observed_monotonic_ns);put(s,e.inode);put(s,e.device_major);put(s,e.device_minor);put(s,e.kernel_real_uid);});
    metric(s,r.evidence.pinned_file_matches,[&](bool v){put(s,v,1);});put(s,r.evidence.quality_flags);put(s,r.evidence.publishing,1);put(s,r.pidfd_exited,1);
    // entry_first_exec_matches/resources are DERIVED caches, intentionally NOT stored.
}
struct Reader{
    std::string_view bytes;std::size_t offset{};bool ok{true};
    std::uint64_t get(unsigned n=8){if(n>bytes.size()-offset){ok=false;return 0;}std::uint64_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint64_t{static_cast<unsigned char>(bytes[offset++])}<<(i*8);return v;}
    std::uint32_t u32(unsigned n=8){const auto v=get(n);if(v>UINT32_MAX)ok=false;return static_cast<std::uint32_t>(v);}
    bool boolean(){const auto v=get(1);if(v>1)ok=false;return v!=0;}
    std::string_view view(std::size_t max){const auto n=get(4);if(!ok||n>max||n>bytes.size()-offset){ok=false;return {};}const auto v=bytes.substr(offset,static_cast<std::size_t>(n));offset+=n;return v;}
    std::string text(std::size_t max){return std::string(view(max));}
};
template<class T,class F>Metric<T> metric(Reader& in,F f){Metric<T> m;const auto support=in.get(1);if(support>static_cast<unsigned>(Support::estimated))in.ok=false;m.support=static_cast<Support>(support);const auto present=in.boolean();if(present)m.value=f();m.note=in.text(4096);return m;}
LinuxNetworkSelfIdentity self(Reader& in){LinuxNetworkSelfIdentity x;x.local_pid=in.u32();x.local_real_uid=in.u32();x.start_ticks=in.get();x.procfs_device=in.get();x.procfs_inode=in.get();x.process_inode=in.get();x.pid_namespace_device=in.get();x.pid_namespace_inode=in.get();x.user_namespace_device=in.get();x.user_namespace_inode=in.get();return x;}
LinuxDesktopLaunchReceipt receipt(Reader& in){
    LinuxDesktopLaunchReceipt r;const auto origin=in.get(1);if(origin!=0)in.ok=false;r.origin=static_cast<LinuxDesktopLaunchOrigin>(origin);r.route.entry_id=in.text(32768);auto& q=r.route.request;q.executable=in.text(1048576);const auto count=in.get(4);if(count>4096){in.ok=false;return r;}for(std::uint64_t n=0;n<count&&in.ok;++n)q.arguments.push_back(in.text(1048576));q.working_directory=in.text(1048576);q.elevated=in.boolean();
    auto& k=r.selected.kernel;k.kernel_tgid=in.u32();k.kernel_real_uid=in.u32();k.leader_start_boottime_ns=in.get();for(auto& n:r.selected.local.boot)n=in.get();r.selected.local.observer=self(in);r.selected.local.process=self(in);
    r.pinned_file.inode=in.get();r.pinned_file.device_major=in.u32();r.pinned_file.device_minor=in.u32();r.caller_owned_pid=in.u32();const auto error=in.get(1);if(error>static_cast<unsigned>(ActionError::stale_target))in.ok=false;r.launch_error=static_cast<ActionError>(error);
    r.evidence.first_exec=metric<StartupBirthObservation::Executable>(in,[&]{StartupBirthObservation::Executable e;e.observed_monotonic_ns=in.get();e.inode=in.get();e.device_major=in.u32();e.device_minor=in.u32();e.kernel_real_uid=in.u32();return e;});
    r.evidence.pinned_file_matches=metric<bool>(in,[&]{return in.boolean();});r.evidence.quality_flags=in.get();r.evidence.publishing=in.boolean();r.pidfd_exited=in.boolean();return r;
}
}
Metric<std::string> encode_linux_manual_launch(const LinuxManualLaunchArchive& a){
    const auto error=validate(a);if(!error.empty())return {{},Support::unsupported,error};
    StartupImpactArchive inner;inner.capture_id=a.births.capture.capture_id=="manual-envelope"?"manual-envelope-alt":"manual-envelope";inner.native_context="manual evidence envelope";inner.births=a.births;
    const auto base=encode_startup_archive_bytes(inner);if(!base.value)return base;
    std::string out(magic);put(out,1,4);text(out,*base.value);metric(out,a.receipt,[&](const auto& r){receipt(out,r);});
    if(out.size()>limit-8)return {{},Support::unsupported,"Manual envelope exceeds shared 16MiB bound"};
    put(out,hash(out));return {std::move(out),Support::available,"Historical manual envelope encoded; no authentication"};
}
LinuxManualLaunchLoad decode_linux_manual_launch(std::string_view bytes){
    auto fail=[](std::string note){return LinuxManualLaunchLoad{{},ActionError::invalid_argument,std::move(note)};};
    if(bytes.size()<28||bytes.size()>limit||bytes.substr(0,8)!=magic)return fail("Invalid manual envelope header/size");
    Reader checksum{bytes.substr(bytes.size()-8)};if(hash(bytes.substr(0,bytes.size()-8))!=checksum.get())return fail("Manual envelope checksum mismatch");
    Reader in{bytes.substr(0,bytes.size()-8),8};if(in.get(4)!=1)return fail("Unsupported manual envelope version");
    const auto base=decode_startup_archive_bytes(in.view(limit));if(!in.ok||!base||!base.archive->births||base.archive->diagnostics||!base.archive->observations.empty()||base.archive->native_context!="manual evidence envelope")return fail("Invalid embedded birth archive");
    LinuxManualLaunchArchive a;a.births=*base.archive->births;a.receipt=metric<LinuxDesktopLaunchReceipt>(in,[&]{return receipt(in);});
    if(!in.ok||in.offset!=in.bytes.size())return fail("Invalid/truncated/trailing manual receipt");
    const auto error=validate(a);if(!error.empty())return fail(error);a.receipt=canonical(a.receipt);
    return {std::move(a),ActionError::none,"Historical manual evidence loaded; PID is NOT live ownership, no capture/permission/authentication restored"};
}
StartupArchiveIoResult save_linux_manual_launch(const std::filesystem::path& path,const LinuxManualLaunchArchive& a){const auto bytes=encode_linux_manual_launch(a);if(!bytes.value)return {ActionError::invalid_argument,false,bytes.note};return write_startup_archive_bytes(path,*bytes.value,true);}
LinuxManualLaunchLoad load_linux_manual_launch(const std::filesystem::path& path){const auto bytes=read_startup_archive_bytes(path);if(!bytes.bytes)return {{},bytes.error,bytes.message};return decode_linux_manual_launch(*bytes.bytes);}
}
