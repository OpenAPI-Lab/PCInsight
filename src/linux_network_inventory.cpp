#include "linux_network_inventory.hpp"
#include "linux_network_sampler.hpp"
#include "linux_process_details.hpp"
#include <algorithm>
#include <charconv>
#include <exception>
#include <cerrno>
#ifdef __linux__
#include <dirent.h>
#include <fcntl.h>
#include <linux/magic.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support s=Support::unsupported){return {{},s==Support::permission_required?s:Support::unsupported,std::move(note)};}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
LinuxNetworkProcIdentity local(const LinuxNetworkSelfIdentity& s){return {s.local_pid,s.local_real_uid,s.start_ticks,s.procfs_device,s.procfs_inode,s.process_inode,s.pid_namespace_device,s.pid_namespace_inode,s.user_namespace_device,s.user_namespace_inode};}
bool valid_context(const NetworkSelectionContext& c){return (c.boot[0]||c.boot[1])&&valid_network_self(c.observer);}
}
Metric<LinuxNetworkProcessInventory> collect_linux_network_inventory(bool authorized,const NetworkInventoryApi& api){
    using Out=LinuxNetworkProcessInventory;
    if(!authorized)return absent<Out>("Explicit permission required to inspect visible process identities",Support::permission_required);
    if(!api)return absent<Out>("Process inventory source unavailable");
    try{
        const auto before=api.observe(api.context);if(!exact(before))return absent<Out>(before.note,before.support);
        if(!valid_context(*before.value))return absent<Out>("Invalid inventory observer context");
        const auto begin=api.now(api.context);if(!exact(begin)||!*begin.value)return absent<Out>("Inventory clock unavailable",begin.support);
        const auto pids=api.enumerate(api.context,*before.value);if(!exact(pids))return absent<Out>(pids.note,pids.support);
        if(pids.value->size()>linux_network_inventory_max_processes)return absent<Out>("Visible process inventory exceeds100000; no truncation");
        auto ordered=*pids.value;std::sort(ordered.begin(),ordered.end());
        for(std::size_t i=0;i<ordered.size();++i)if(!ordered[i]||ordered[i]>2147483647U||(i&&ordered[i]==ordered[i-1]))return absent<Out>("Invalid/duplicate numeric entry in procfs enumeration");
        std::uint64_t last=*begin.value;
        auto clock=[&]()->bool{const auto now=api.now(api.context);if(!exact(now)||*now.value<last||*now.value-*begin.value>linux_network_inventory_max_span_ns)return false;last=*now.value;return true;};
        Out out;out.boot=before.value->boot;out.observer=local(before.value->observer);out.started_ns=*begin.value;out.processes.reserve(ordered.size());
        for(const auto pid:ordered){
            if(!clock())return absent<Out>("Inventory clock regressed or5s cooperative budget exceeded; no partial result");
            const auto read=api.inspect(api.context,pid);LinuxNetworkInventoryEntry row;row.pid=pid;row.state=read.state;
            if(read.state==LinuxNetworkInventoryState::verified){
                if(!exact(read.identity))return absent<Out>("Verified inventory row lacks exact identity");
                const auto& p=*read.identity.value;
                if(!valid_network_self(p)||p.local_pid!=pid||p.procfs_device!=out.observer.procfs_device||p.procfs_inode!=out.observer.procfs_inode)return absent<Out>("Inventory process identity/domain inconsistent");
                row.candidate={LinuxNetworkProcessCandidate{pid,p.start_ticks},Support::available,"Verified pidfd/procfs identity in observer view; no future liveness guarantee"};
                row.observer_real_uid={p.local_real_uid,Support::available,"Procfs observer-domain real UID, not initial kernel UID"};
            }else{
                if(read.identity.value||read.identity.note.empty()||
                    (read.state!=LinuxNetworkInventoryState::permission_denied&&read.state!=LinuxNetworkInventoryState::disappeared&&read.state!=LinuxNetworkInventoryState::unavailable))return absent<Out>("Malformed inventory failure row");
                const auto support=read.state==LinuxNetworkInventoryState::permission_denied?Support::permission_required:Support::unsupported;
                if(read.identity.support!=support)return absent<Out>("Inventory failure state/support inconsistent");
                row.candidate=absent<LinuxNetworkProcessCandidate>(read.identity.note,support);row.observer_real_uid=absent<std::uint32_t>(read.identity.note,support);
            }
            out.processes.push_back(std::move(row));
        }
        const auto after=api.observe(api.context);if(!exact(after))return absent<Out>(after.note,after.support);
        if(*after.value!=*before.value)return absent<Out>("Observer boot/procfs/namespace changed during inventory");
        if(!clock())return absent<Out>("Inventory final clock/budget invalid; no partial result");
        out.finished_ns=last;out.enumeration_completed=true;
        return {std::move(out),Support::available,"Completed visible /proc directory enumeration; per-row identity quality, dynamic/restricted view, NOT all host processes or network totals"};
    }catch(const std::exception& e){return absent<Out>(std::string("Inventory failed; nothing published: ")+e.what());}
}
NetworkInventoryApi native_network_inventory_api(){
#ifdef __linux__
    return {nullptr,
        [](void*){const auto api=native_network_selection_source_api();return api.observe(api.context);},
        [](void*){const auto api=native_network_sampler_api();return api.now(api.context);},
        [](void*,const NetworkSelectionContext& expected)->Metric<std::vector<std::uint32_t>>{
            const int fd=::open("/proc",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
            if(fd<0)return absent<std::vector<std::uint32_t>>("Cannot open procfs inventory",errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported);
            struct Fd {int n;~Fd(){if(n>=0)::close(n);}} held{fd};struct stat st{};struct statfs fs{};
            if(::fstat(fd,&st)||::fstatfs(fd,&fs)||fs.f_type!=PROC_SUPER_MAGIC||static_cast<std::uint64_t>(st.st_dev)!=expected.observer.procfs_device||static_cast<std::uint64_t>(st.st_ino)!=expected.observer.procfs_inode)
                return absent<std::vector<std::uint32_t>>("Inventory root is not observer's pinned procfs");
            DIR* raw=::fdopendir(fd);if(!raw)return absent<std::vector<std::uint32_t>>("Cannot enumerate procfs directory");held.n=-1;
            struct Dir {DIR* p;~Dir(){::closedir(p);}} dir{raw};std::vector<std::uint32_t> pids;
            for(;;){errno=0;const auto entry=::readdir(dir.p);if(!entry){if(errno)return absent<std::vector<std::uint32_t>>("procfs readdir failed; no partial enumeration",errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported);break;}
                const std::string_view name(entry->d_name);if(name.empty()||!std::all_of(name.begin(),name.end(),[](char c){return c>='0'&&c<='9';}))continue;
                std::uint32_t pid=0;const auto [end,error]=std::from_chars(name.data(),name.data()+name.size(),pid);
                if(error!=std::errc{}||end!=name.data()+name.size()||!pid||pid>2147483647U||name!=std::to_string(pid))return absent<std::vector<std::uint32_t>>("Invalid numeric procfs entry");
                if(pids.size()==linux_network_inventory_max_processes)return absent<std::vector<std::uint32_t>>("Visible process inventory exceeds100000; no truncation");
                pids.push_back(pid);
            }
            return {std::move(pids),Support::available,"Pinned procfs directory reached EOF; membership not atomic and visibility may be restricted"};},
        [](void*,std::uint32_t pid)->NetworkInventoryObservation{
            auto failure=[](std::string note,Support support=Support::unsupported,LinuxNetworkInventoryState state=LinuxNetworkInventoryState::unavailable){
                if(support==Support::permission_required)state=LinuxNetworkInventoryState::permission_denied;
                return NetworkInventoryObservation{state,absent<LinuxNetworkSelfIdentity>(std::move(note),support)};};
            const auto api=native_network_selection_source_api();const auto opened=api.open(api.context,pid);
            if(opened.fd<0)return failure("Cannot acquire inventory pidfd; errno="+std::to_string(opened.error),opened.error==EACCES||opened.error==EPERM?Support::permission_required:Support::unsupported,
                opened.error==ESRCH||opened.error==ENOENT?LinuxNetworkInventoryState::disappeared:LinuxNetworkInventoryState::unavailable);
            struct Fd {NetworkSelectionSourceApi api;int n;~Fd(){api.close(api.context,n);}} held{api,opened.fd};
            auto exited=[&]{pollfd p{held.n,POLLIN,0};return ::poll(&p,1,0)>0&&(p.revents&(POLLIN|POLLHUP));};
            if(exited())return failure("Process exited before inventory inspection",Support::unsupported,LinuxNetworkInventoryState::disappeared);
            const auto stat=read_linux_process_identity("/proc/"+std::to_string(pid));
            if(!exact(stat)){
                if(exited())return failure("Process disappeared during inventory stat read",Support::unsupported,LinuxNetworkInventoryState::disappeared);
                return failure(stat.note.empty()?"Inventory stat identity unavailable":stat.note,stat.support);}
            if(stat.value->pid!=pid||!stat.value->start_ticks)return failure("Inventory stat PID/start identity invalid or changed");
            auto process=api.inspect(api.context,held.n,pid,stat.value->start_ticks);
            if(exited())return failure("Process exited during inventory identity read",Support::unsupported,LinuxNetworkInventoryState::disappeared);
            if(!exact(process))return failure(process.note.empty()?"Inventory pidfd/procfs identity unavailable":process.note,process.support);
            return {LinuxNetworkInventoryState::verified,std::move(process)};
        }};
#else
    return {};
#endif
}
}
namespace pcinsight {
Metric<LinuxNetworkProcessInventory> read_linux_network_process_inventory(bool authorized){return detail::collect_linux_network_inventory(authorized,detail::native_network_inventory_api());}
Metric<std::vector<LinuxNetworkProcessCandidate>> linux_network_inventory_candidates(const Metric<LinuxNetworkProcessInventory>& source){
    auto fail=[](std::string note,Support s=Support::unsupported)->Metric<std::vector<LinuxNetworkProcessCandidate>>{return {{},s==Support::permission_required?s:Support::unsupported,std::move(note)};};
    if(!source.value||source.support!=Support::available)return fail(source.note.empty()?"Inventory is absent or not verified":source.note,source.support);
    const auto& s=*source.value;
    const auto& o=s.observer;
    if(!s.enumeration_completed||!(s.boot[0]||s.boot[1])||!o.pid||o.pid>2147483647U||!o.start_ticks||!o.procfs_device||!o.procfs_inode||!o.process_inode||!o.pid_namespace_device||!o.pid_namespace_inode||!o.user_namespace_device||!o.user_namespace_inode||
        !s.started_ns||s.finished_ns<s.started_ns||s.finished_ns-s.started_ns>linux_network_inventory_max_span_ns||s.processes.size()>linux_network_inventory_max_processes)return fail("Malformed/incomplete inventory; no selection prepared");
    if(s.processes.size()>LinuxProcessNetworkSelectionOwner::max_processes)return fail("Visible inventory exceeds capture capacity; no truncation/rotation");
    std::vector<LinuxNetworkProcessCandidate> rows;rows.reserve(s.processes.size());std::uint32_t previous=0;
    for(const auto& row:s.processes){
        if(!row.pid||row.pid>2147483647U||row.pid<=previous)return fail("Inventory PID order/identity invalid");
        previous=row.pid;
        if(row.state!=LinuxNetworkInventoryState::verified)return fail("Inventory PID="+std::to_string(row.pid)+" is not verified; no filtering to hide an unavailable process",row.state==LinuxNetworkInventoryState::permission_denied?Support::permission_required:Support::unsupported);
        if(!row.candidate.value||row.candidate.support!=Support::available||row.candidate.value->pid!=row.pid||!row.candidate.value->process_start_ticks||!row.observer_real_uid.value||row.observer_real_uid.support!=Support::available)return fail("Verified inventory row has no exact identity/UID");
        rows.push_back(*row.candidate.value);
    }
    return {std::move(rows),Support::available,"ALL encountered identities within capacity; still restricted/dynamic procfs view, not all-host coverage or capture authorization"};
}
}
