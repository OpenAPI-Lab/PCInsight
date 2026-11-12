#include "linux_startup_birth.hpp"
#include "linux_network_selection.hpp"
#include <bit>
#include <atomic>
#include <limits>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <linux/bpf.h>
#include <poll.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string n,Support s=Support::unsupported){return {{},s,std::move(n)};}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
bool valid(PrivateNetworkIdentity k){return k.kernel_tgid&&k.kernel_tgid<=2147483647U&&k.leader_start_boottime_ns;}
}
Metric<std::optional<StartupChildBirth>> decode_startup_birth(const StartupBirthRaw& r,bool exec_enabled){
    if(r[0]>1)return absent<std::optional<StartupChildBirth>>("Invalid birth publication state");
    if(!r[0])return {std::optional<StartupChildBirth>{},Support::available,"No published birth in this slot"};
    if(!r[1]||r[1]>2147483647U||r[2]>UINT32_MAX||!r[3]||!r[4])return absent<std::optional<StartupChildBirth>>("Invalid birth identity/time");
    if(r[8]>1)return absent<std::optional<StartupChildBirth>>("Invalid child final publication state");
    StartupChildBirth row;row.kernel={static_cast<std::uint32_t>(r[1]),static_cast<std::uint32_t>(r[2]),r[3]};row.monotonic_ns=r[4];row.scheduler_runtime_at_birth_ns=r[5];row.read_bytes_at_birth=r[6];row.write_bytes_at_birth=r[7];
    row.cpu_time={{},Support::unsupported,"Final child CPU pending/not requested/outside creator-reaper coverage"};
    row.read_bytes=row.write_bytes={{},Support::unsupported,"Final child IO pending/not requested/outside creator-reaper coverage"};
    row.first_executable={{},Support::unsupported,exec_enabled?"No first successful exec metadata retained yet":"First exec capture not requested"};
    if(r[15]>3||r[15]==2)return absent<std::optional<StartupChildBirth>>("Invalid/in-flight first exec publication");
    if(!r[15])for(unsigned n=16;n<20;++n)if(r[n])return absent<std::optional<StartupChildBirth>>("Unclaimed first exec metadata");
    if(!exec_enabled){for(unsigned n=15;n<20;++n)if(r[n])return absent<std::optional<StartupChildBirth>>("Unexpected exec data while disabled");}
    row.exec_state=static_cast<std::uint32_t>(r[15]);
    if(r[15]==1){
        if(!r[16]||r[16]<r[4]||(r[8]&&r[16]>r[9])||r[18]>UINT32_MAX||r[19]>UINT32_MAX)return absent<std::optional<StartupChildBirth>>("Invalid first successful exec time/device/UID");
        row.first_executable={StartupBirthObservation::Executable{r[16],r[17],static_cast<std::uint32_t>(r[18]>>20),static_cast<std::uint32_t>(r[18]&0xfffff),static_cast<std::uint32_t>(r[19])},Support::available,"Filesystem identifiers at FIRST successful exec; not current executable/path/app/autostart proof"};
    }else if(r[15]==3)row.first_executable={{},Support::unsupported,"First exec metadata/enrollment failed; never substituted a later exec"};
    row.local_pid={{},Support::unsupported,"Birth PID namespace mapping unavailable"};
    if(r[13]||r[14]){
        if(!r[13]||r[13]>UINT32_MAX||!r[14]||r[14]>2147483647U)return absent<std::optional<StartupChildBirth>>("Invalid namespace-qualified birth PID");
        row.local_pid={StartupBirthLocalPid{r[13],static_cast<std::uint32_t>(r[14])},Support::available,"TGID in captured observer PID namespace at BIRTH, not a live process selection"};
    }
    if(r[8]){
        if(!r[9]||r[9]<r[4]||r[10]<r[5]||r[11]<r[6]||r[12]<r[7])return absent<std::optional<StartupChildBirth>>("Child final time/counter decreased; no fabricated delta");
        row.final_monotonic_ns=r[9];
        const auto cpu=r[10]-r[5];
        if(cpu<=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))row.cpu_time={std::chrono::nanoseconds(static_cast<std::int64_t>(cpu)),Support::available,"Birth-to-direct-release observed group scheduler CPU; no post-switch finality guarantee or waited-child CPU"};
        else row.cpu_time={{},Support::unsupported,"Child CPU duration overflow"};
        row.read_bytes={r[11]-r[6],Support::available,"Birth-to-release group-accounted read bytes, includes waited-child IO"};
        row.write_bytes={r[12]-r[7],Support::available,"Birth-to-release group-accounted write bytes, includes waited-child IO"};
    }
    return {std::move(row),Support::available,"Selected creator birth/final record; optional namespace-qualified PID, no app/automatic-startup claim"};
}
Metric<std::vector<NetworkProbeInstruction>> make_startup_birth_probe(int control,int records,const LinuxStartupResourceBtfLayout& l,PrivateNetworkIdentity root,std::uint32_t capacity,const StartupBirthPidProfile* profile,int allowlist){
    if(std::endian::native!=std::endian::little||control<0||records<0||control==records||!valid(root)||!capacity||capacity>64)return absent<std::vector<NetworkProbeInstruction>>("Invalid birth identity/maps/capacity");
    for(auto n:{l.tgid,l.leader,l.start,l.cred,l.uid,l.signal,l.live,l.nr_threads,l.task_runtime,l.signal_runtime,l.task_read,l.signal_read,l.task_write,l.signal_write})if(n>1048576)return absent<std::vector<NetworkProbeInstruction>>("Birth offset exceeds bound");
    if(allowlist>=0&&(allowlist==control||allowlist==records))return absent<std::vector<NetworkProbeInstruction>>("Exec allowlist must be a distinct map");
    if(profile){const auto& p=profile->layout;
        if(!profile->namespace_inode||profile->namespace_inode>UINT32_MAX||p.upid_size<8||p.upid_size>256||p.nr>p.upid_size-4||p.ns>p.upid_size-8)return absent<std::vector<NetworkProbeInstruction>>("Invalid birth PID profile");
        for(auto n:{p.thread_pid,p.level,p.numbers,p.ns_inum})if(n>1048576)return absent<std::vector<NetworkProbeInstruction>>("Birth PID offset exceeds bound");
    }
    std::vector<NetworkProbeInstruction> c;std::vector<std::size_t> exits,errors,full,duplicate,pid_errors,mapped,exec_errors;
    auto op=[&](unsigned code,unsigned d,unsigned s,int off,int imm){c.push_back({static_cast<std::uint8_t>(code),static_cast<std::uint8_t>(d|(s<<4)),static_cast<std::int16_t>(off),imm});};
    auto jump=[&](auto& list,unsigned code,unsigned d,int imm){list.push_back(c.size());op(code,d,0,0,imm);};
    auto compare=[&](auto& list,unsigned code,unsigned d,unsigned s){list.push_back(c.size());op(code,d,s,0,0);};
    auto constant=[&](unsigned d,std::uint64_t v){op(0x18,d,0,0,static_cast<std::int32_t>(v));op(0,0,0,0,static_cast<std::int32_t>(v>>32));};
    auto read=[&](unsigned base,unsigned offset,unsigned bytes,auto& failure){
        op(0x7a,10,0,-16,0);op(0xbf,3,base,0,0);op(0x07,3,0,0,static_cast<int>(offset));op(0xbf,1,10,0,0);op(0x07,1,0,0,-16);
        op(0xb7,2,0,0,static_cast<int>(bytes));op(0x85,0,0,0,113);jump(failure,0x55,0,0);op(bytes==8?0x79:0x61,0,10,-16,0);
    };
    auto lookup=[&](int fd){op(0xbf,2,10,0,0);op(0x07,2,0,0,-4);op(0x18,1,1,0,fd);op(0,0,0,0,0);op(0x85,0,0,0,1);};
    op(0xbf,6,1,0,0);op(0x85,0,0,0,14);op(0x77,0,0,0,32);jump(exits,0x55,0,static_cast<int>(root.kernel_tgid));
    // Authenticate the selected creator before looking at the new task.
    op(0x85,0,0,0,35);op(0xbf,7,0,0,0);read(7,l.leader,8,errors);op(0xbf,7,0,0,0);
    read(7,l.start,8,errors);constant(9,root.leader_start_boottime_ns);compare(exits,0x5d,0,9);
    op(0x85,0,0,0,15);op(0xbc,0,0,0,0);constant(9,root.kernel_real_uid);compare(exits,0x5d,0,9);
    op(0x79,6,6,0,0);read(6,l.leader,8,errors);compare(exits,0x5d,0,6); // New group only; skip CLONE_THREAD.
    read(6,l.signal,8,errors);jump(errors,0x15,0,0);op(0xbf,7,0,0,0);
    read(7,l.live,4,errors);jump(errors,0x55,0,1);read(7,l.nr_threads,4,errors);jump(errors,0x55,0,1);
    // Reserve one unique slot. Saturating early check bounds concurrent overshoot
    // to at most the creator's concurrent threads, far below uint64 wrap.
    op(0x62,10,0,-4,0);lookup(control);jump(errors,0x15,0,0);op(0x79,9,0,0,0);jump(full,0x35,9,static_cast<int>(capacity));
    op(0xb7,9,0,0,1);op(0xdb,0,9,0,1);jump(full,0x35,9,static_cast<int>(capacity));
    op(0x7b,10,9,-24,0);op(0x63,10,9,-4,0);lookup(records);jump(errors,0x15,0,0);op(0xbf,8,0,0,0);op(0x79,9,8,0,0);jump(duplicate,0x55,9,0);
    read(6,l.tgid,4,errors);op(0x7b,8,0,8,0);read(6,l.start,8,errors);op(0x7b,8,0,24,0);
    read(6,l.cred,8,errors);jump(errors,0x15,0,0);op(0xbf,9,0,0,0);read(9,l.uid,4,errors);op(0x7b,8,0,16,0);
    const std::array<std::array<unsigned,3>,3> pairs{{{l.task_runtime,l.signal_runtime,40},{l.task_read,l.signal_read,48},{l.task_write,l.signal_write,56}}};
    for(const auto& p:pairs){read(6,p[0],8,errors);op(0xbf,9,0,0,0);read(7,p[1],8,errors);op(0x0f,9,0,0,0);compare(errors,0xad,9,0);op(0x7b,8,9,static_cast<int>(p[2]),0);}
    if(profile){const auto& p=profile->layout;
        read(6,p.thread_pid,8,pid_errors);jump(pid_errors,0x15,0,0);op(0xbf,7,0,0,0);
        read(7,p.level,4,pid_errors);jump(pid_errors,0x25,0,32);op(0xbf,6,0,0,0);
        // Only inspect indices <= pid.level. No heuristic equating local/global PID.
        for(unsigned n=0;n<=32;++n){std::vector<std::size_t> next;
            jump(pid_errors,0xa5,6,static_cast<int>(n));
            const auto at=p.numbers+n*p.upid_size;
            read(7,at+p.ns,8,pid_errors);jump(pid_errors,0x15,0,0);op(0xbf,9,0,0,0);
            read(9,p.ns_inum,4,pid_errors);constant(9,profile->namespace_inode);compare(next,0x5d,0,9);
            read(7,at+p.nr,4,pid_errors);jump(pid_errors,0x15,0,0);jump(pid_errors,0x25,0,2147483647);
            op(0x7b,8,0,112,0);constant(9,profile->namespace_inode);op(0x7b,8,9,104,0);
            mapped.push_back(c.size());op(0x05,0,0,0,0);
            for(auto v:next)c[v].offset=static_cast<std::int16_t>(c.size()-v-1);
        }
        pid_errors.push_back(c.size());op(0x05,0,0,0,0);
    }
    const auto publish=c.size();for(auto v:mapped)c[v].offset=static_cast<std::int16_t>(publish-v-1);
    if(allowlist>=0){
        op(0x79,9,8,8,0);op(0x63,10,9,-4,0);op(0x18,1,1,0,allowlist);op(0,0,0,0,0);
        op(0xbf,2,10,0,0);op(0x07,2,0,0,-4);op(0xbf,3,10,0,0);op(0x07,3,0,0,-24);op(0xb7,4,0,0,0);op(0x85,0,0,0,2);jump(exec_errors,0x55,0,0);
    }
    const auto publish_state=c.size();
    op(0x85,0,0,0,5);op(0x7b,8,0,32,0);op(0xb7,9,0,0,1);op(0xdb,8,9,0,0xe1);
    const auto done=c.size();op(0xb7,0,0,0,0);op(0x95,0,0,0,0);
    auto failure=[&](auto& patches,int flag){const auto at=c.size();for(auto n:patches)c[n].offset=static_cast<std::int16_t>(at-n-1);
        op(0x62,10,0,-4,1);lookup(control);const auto missing=c.size();op(0x15,0,0,0,0);op(0xb7,9,0,0,flag);op(0xdb,0,9,0,0x40);
        const auto end=c.size();op(0xb7,0,0,0,0);op(0x95,0,0,0,0);c[missing].offset=static_cast<std::int16_t>(end-missing-1);
    };
    failure(errors,1);failure(full,2);failure(duplicate,4);for(auto n:exits)c[n].offset=static_cast<std::int16_t>(done-n-1);
    if(profile){const auto at=c.size();for(auto v:pid_errors)c[v].offset=static_cast<std::int16_t>(at-v-1);
        op(0x62,10,0,-4,1);lookup(control);const auto missing=c.size();op(0x15,0,0,0,0);op(0xb7,9,0,0,32);op(0xdb,0,9,0,0x40);
        const auto resume=c.size();op(0x05,0,0,static_cast<int>(publish)-static_cast<int>(resume)-1,0);c[missing].offset=static_cast<std::int16_t>(resume-missing-1);
    }
    if(allowlist>=0){const auto at=c.size();for(auto v:exec_errors)c[v].offset=static_cast<std::int16_t>(at-v-1);
        op(0x7a,8,0,120,3);op(0x62,10,0,-4,1);lookup(control);const auto missing=c.size();op(0x15,0,0,0,0);op(0xb7,9,0,0,64);op(0xdb,0,9,0,0x40);
        const auto resume=c.size();op(0x05,0,0,static_cast<int>(publish_state)-static_cast<int>(resume)-1,0);c[missing].offset=static_cast<std::int16_t>(resume-missing-1);
    }
    return {std::move(c),Support::available,"Selected creator birth-before-wake; bounded records with explicit loss flags"};
}
void LinuxStartupBirthCapture::stop() noexcept {
#ifdef __linux__
    for(auto p:{&link_,&final_link_,&exec_link_,&program_,&final_program_,&exec_program_,&exec_allowlist_,&records_,&control_,&pidfd_})if(*p>=0){::close(*p);*p=-1;}
#endif
    creator_.reset();owner_=euid_=capacity_=0;totals_=false;pid_mapping_=false;exec_=false;pid_mapping_note_.clear();
}
Metric<PrivatePidfdNetworkJoin> LinuxStartupBirthCapture::start(bool authorized,int borrowed,std::uint32_t pid,std::uint64_t ticks,std::uint32_t capacity,bool totals,bool exec){
    if(!authorized)return absent<PrivatePidfdNetworkJoin>("Explicit selected creator birth authorization required",Support::permission_required);
    if(creator_||!capacity||capacity>64)return absent<PrivatePidfdNetworkJoin>("Birth capture active or capacity outside1..64");
#ifdef __linux__
    auto fail=[&](std::string n,Support s=Support::unsupported){stop();return absent<PrivatePidfdNetworkJoin>(std::move(n),s);};
    try{
        owner_=static_cast<std::uint32_t>(::getpid());euid_=static_cast<std::uint32_t>(::geteuid());capacity_=capacity;totals_=totals;exec_=exec;
        pidfd_=::fcntl(borrowed,F_DUPFD_CLOEXEC,3);if(pidfd_<0)return fail("Cannot own creator pidfd");
        const auto blob=read_linux_kernel_btf_blob();if(!exact(blob))return fail(blob.note,blob.support);
        const auto network=resolve_linux_network_btf(*blob.value,sizeof(void*));const auto layout=resolve_linux_startup_resource_btf(*blob.value,sizeof(void*));const auto hook=resolve_linux_startup_birth_hook(*blob.value);
        if(!exact(network))return fail(network.note,network.support);
        if(!exact(layout))return fail(layout.note,layout.support);
        if(!exact(hook))return fail(hook.note,hook.support);
        const auto selected=read_private_pidfd_network_join(true,pidfd_,pid,ticks,&*network.value);if(!exact(selected))return fail(selected.note,selected.support);
        auto pointer=[](const void* p){return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(p));};
        auto map=[&](unsigned count,unsigned bytes){union bpf_attr a{};a.map_type=BPF_MAP_TYPE_ARRAY;a.key_size=4;a.value_size=bytes;a.max_entries=count;return static_cast<int>(::syscall(SYS_bpf,BPF_MAP_CREATE,&a,sizeof(a)));};
        control_=map(2,8);records_=map(capacity,sizeof(StartupBirthRaw));if(control_<0||records_<0)return fail("Birth maps unavailable",errno==EPERM||errno==EACCES?Support::permission_required:Support::unsupported);
        if(exec){
            const auto el=resolve_linux_birth_exec_btf(*blob.value,sizeof(void*));if(!exact(el))return fail(el.note);
            union bpf_attr a{};a.map_type=BPF_MAP_TYPE_HASH;a.key_size=4;a.value_size=8;a.max_entries=capacity;exec_allowlist_=static_cast<int>(::syscall(SYS_bpf,BPF_MAP_CREATE,&a,sizeof(a)));if(exec_allowlist_<0)return fail("Exec allowlist unavailable");
            const auto code=make_startup_birth_exec_probe(control_,records_,exec_allowlist_,*layout.value,*el.value,capacity);if(!exact(code))return fail(code.note);
            const char license[]="GPL";union bpf_attr load{};load.prog_type=BPF_PROG_TYPE_RAW_TRACEPOINT;load.insn_cnt=static_cast<__u32>(code.value->size());load.insns=pointer(code.value->data());load.license=pointer(license);
            exec_program_=static_cast<int>(::syscall(SYS_bpf,BPF_PROG_LOAD,&load,sizeof(load)));if(exec_program_<0)return fail("Exec program load errno="+std::to_string(errno),errno==EPERM||errno==EACCES?Support::permission_required:Support::unsupported);
            const char name[]="sched_process_exec";union bpf_attr attach{};attach.raw_tracepoint.name=pointer(name);attach.raw_tracepoint.prog_fd=exec_program_;
            exec_link_=static_cast<int>(::syscall(SYS_bpf,BPF_RAW_TRACEPOINT_OPEN,&attach,sizeof(attach)));if(exec_link_<0)return fail("Exec tracepoint attach errno="+std::to_string(errno));
        } // Exec must precede birth enrollment; nothing is allowlisted yet.
        if(totals){
            const auto final_code=make_startup_birth_final_probe(control_,records_,*layout.value,selected.value->kernel,capacity);if(!exact(final_code))return fail(final_code.note);
            const char license[]="GPL";union bpf_attr load{};load.prog_type=BPF_PROG_TYPE_TRACING;load.expected_attach_type=BPF_TRACE_FENTRY;load.attach_btf_id=layout.value->function_id;load.insn_cnt=static_cast<__u32>(final_code.value->size());load.insns=pointer(final_code.value->data());load.license=pointer(license);
            final_program_=static_cast<int>(::syscall(SYS_bpf,BPF_PROG_LOAD,&load,sizeof(load)));if(final_program_<0)return fail("Child final program load errno="+std::to_string(errno),errno==EPERM||errno==EACCES?Support::permission_required:Support::unsupported);
            union bpf_attr attach{};attach.link_create.prog_fd=final_program_;attach.link_create.attach_type=BPF_TRACE_FENTRY;final_link_=static_cast<int>(::syscall(SYS_bpf,BPF_LINK_CREATE,&attach,sizeof(attach)));if(final_link_<0)return fail("Child final attach errno="+std::to_string(errno));
        } // Final hook MUST precede birth hook: no birth-to-final setup hole.
        const auto pid_layout=resolve_linux_birth_pid_btf(*blob.value,sizeof(void*));
        std::optional<StartupBirthPidProfile> profile;
        const auto inode=selected.value->local.observer.pid_namespace_inode;
        if(exact(pid_layout)&&inode&&inode<=UINT32_MAX)profile=StartupBirthPidProfile{*pid_layout.value,inode};
        pid_mapping_=profile.has_value();pid_mapping_note_=profile?"Capture observer namespace-qualified TGID at birth; no live-PID authority":pid_layout.value?"Observer namespace inode outside supported uint32 domain":pid_layout.note;
        const auto code=make_startup_birth_probe(control_,records_,*layout.value,selected.value->kernel,capacity,profile?&*profile:nullptr,exec_allowlist_);if(!exact(code))return fail(code.note);
        const char license[]="GPL";union bpf_attr a{};a.prog_type=BPF_PROG_TYPE_TRACING;a.expected_attach_type=BPF_TRACE_FENTRY;a.attach_btf_id=*hook.value;a.insn_cnt=static_cast<__u32>(code.value->size());a.insns=pointer(code.value->data());a.license=pointer(license);
        program_=static_cast<int>(::syscall(SYS_bpf,BPF_PROG_LOAD,&a,sizeof(a)));if(program_<0)return fail("Birth program load errno="+std::to_string(errno),errno==EPERM||errno==EACCES?Support::permission_required:Support::unsupported);
        union bpf_attr b{};b.link_create.prog_fd=program_;b.link_create.attach_type=BPF_TRACE_FENTRY;link_=static_cast<int>(::syscall(SYS_bpf,BPF_LINK_CREATE,&b,sizeof(b)));if(link_<0)return fail("Birth attach errno="+std::to_string(errno));
        const auto after=read_private_pidfd_network_join(true,pidfd_,pid,ticks,&*network.value);
        if(!exact(after)||after.value->local!=selected.value->local||after.value->kernel.kernel_tgid!=selected.value->kernel.kernel_tgid||after.value->kernel.kernel_real_uid!=selected.value->kernel.kernel_real_uid||after.value->kernel.leader_start_boottime_ns!=selected.value->kernel.leader_start_boottime_ns||owner_!=static_cast<std::uint32_t>(::getpid())||euid_!=static_cast<std::uint32_t>(::geteuid()))return fail("Creator changed during birth attachment");
        creator_=*selected.value;return selected;
    }catch(...){stop();throw;}
#else
    (void)borrowed;(void)pid;(void)ticks;(void)totals;(void)exec;return absent<PrivatePidfdNetworkJoin>("Birth capture requires Linux");
#endif
}
Metric<StartupBirthRead> LinuxStartupBirthCapture::sample(){
    if(!creator_)return absent<StartupBirthRead>("Birth capture inactive");
#ifdef __linux__
    auto fail=[&](std::string n){stop();return absent<StartupBirthRead>(std::move(n));};
    if(owner_!=static_cast<std::uint32_t>(::getpid())||euid_!=static_cast<std::uint32_t>(::geteuid()))return fail("Birth caller/fork/euid changed; inherited descriptors closed, no shared map writes");
    try{
        const auto source=native_network_selection_source_api();const auto before=source.observe(source.context);
        if(!exact(before)||before.value->boot!=creator_->local.boot||before.value->observer!=creator_->local.observer)return fail("Birth observer context changed");
        StartupBirthRead out;out.creator=*creator_;out.creator_reaped_totals_enabled=totals_;out.pid_mapping_enabled=pid_mapping_;out.pid_mapping_note=pid_mapping_note_;
        out.first_exec_enabled=exec_;
        pollfd p{pidfd_,POLLIN,0};const auto rc=::poll(&p,1,0);if(rc<0||p.revents&~(POLLIN|POLLHUP))return fail("Creator pidfd guard unavailable");out.creator_exited=rc>0;
        auto lookup=[&](int fd,std::uint32_t key,void* value){union bpf_attr a{};a.map_fd=fd;a.key=static_cast<__u64>(reinterpret_cast<std::uintptr_t>(&key));a.value=static_cast<__u64>(reinterpret_cast<std::uintptr_t>(value));return ::syscall(SYS_bpf,BPF_MAP_LOOKUP_ELEM,&a,sizeof(a))==0;};
        std::uint64_t flags_before{};if(!lookup(control_,1,&flags_before))return fail("Birth quality read unavailable");
        for(std::uint32_t n=0;n<capacity_;++n){StartupBirthRaw a{},b{};if(!lookup(records_,n,&a))return fail("Birth record read unavailable");
            std::atomic_thread_fence(std::memory_order_seq_cst);
            if(!lookup(records_,n,&b))return fail("Birth record recheck unavailable");
            if(a[0]!=b[0]||a[8]!=b[8]||a[15]!=b[15]||b[15]==2){++out.publishing;continue;}
            if(b[0]&&b[13]&&(!pid_mapping_||b[13]!=creator_->local.observer.pid_namespace_inode))return fail("Birth PID namespace differs from capture observer");
            const auto row=decode_startup_birth(b,exec_);if(!exact(row))return fail(row.note);if(row.value->has_value())out.births.push_back(**row.value);
        }
        if(!lookup(control_,1,&out.quality_flags))return fail("Birth quality recheck unavailable");
        out.quality_flags|=flags_before;
        if(out.quality_flags&~127ULL)return fail("Unknown birth quality flags");
        if(!exec_&&(out.quality_flags&64))return fail("Unexpected exec diagnostics while disabled");
        const auto after=source.observe(source.context);if(!exact(after)||*after.value!=*before.value||owner_!=static_cast<std::uint32_t>(::getpid())||euid_!=static_cast<std::uint32_t>(::geteuid()))return fail("Birth observer changed during read");
        const auto support=out.quality_flags||out.publishing?Support::estimated:Support::available;
        return {std::move(out),support,"Births/optional finals/namespace-qualified birth PIDs; partial flags explicit; kernel IDs NOT interchangeable with local PIDs, no app-origin claim"};
    }catch(...){stop();throw;}
#else
    return absent<StartupBirthRead>("Birth capture requires Linux");
#endif
}
}
