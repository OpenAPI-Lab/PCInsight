#include "linux_network_selection.hpp"
#include "linux_network_sampler.hpp"
#include <algorithm>
#include <cerrno>
#include <exception>
#ifdef __linux__
#include <poll.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support s=Support::unsupported){return {{},s,std::move(note)};}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
bool valid(const NetworkSelectionContext& c,std::uint64_t caller){return (c.boot[0]||c.boot[1])&&valid_network_self(c.observer)&&c.observer.local_pid==caller;}
}
Metric<LinuxNetworkSelectionChange> LinuxNetworkSelectionOwner::refresh(bool authorized,std::span<const LinuxNetworkProcessCandidate> candidates){
    using Change=LinuxNetworkSelectionChange;
    if(!authorized)return absent<Change>("Explicit permission required to prepare network selection",Support::permission_required);
    if(candidates.size()>LinuxProcessNetworkSelectionOwner::max_processes)return absent<Change>("Selection exceeds process capacity; no truncation or automatic rotation");
    std::vector<LinuxNetworkProcessCandidate> ordered(candidates.begin(),candidates.end());std::sort(ordered.begin(),ordered.end());
    for(std::size_t i=0;i<ordered.size();++i)if(!ordered[i].pid||ordered[i].pid>2147483647U||!ordered[i].process_start_ticks||(i&&ordered[i-1].pid==ordered[i].pid))
        return absent<Change>("Complete selection requires distinct valid PIDs and known start ticks; no PID-only fallback");
    auto reject=[&](std::string note,Support s=Support::unsupported){clear();return absent<Change>(std::move(note),s==Support::permission_required?s:Support::unsupported);};
    if(!api_)return reject("Selection source unavailable");
    if(creator_&&api_.caller(api_.context)!=creator_)return reject("Forked selection owner rejected; inherited handles closed");
    try{
        const auto caller=api_.caller(api_.context);const auto before=api_.observe(api_.context);
        if(!caller||!exact(before)||!valid(*before.value,caller))return reject(before.note.empty()?"Observer context unavailable":before.note,before.support);
        if(context_&&*context_!=*before.value)return reject("Observer boot/procfs/namespace context changed; refresh must not reuse handles");
        std::vector<Entry> next;next.reserve(ordered.size());std::size_t retained=0;
        for(const auto& candidate:ordered){const auto old=std::find_if(entries_.begin(),entries_.end(),[&](const auto& e){return e.candidate==candidate;});
            std::shared_ptr<OwnedFd> handle;
            if(old!=entries_.end()){handle=old->handle;++retained;}
            else{handle=std::make_shared<OwnedFd>();handle->api=api_;const auto opened=api_.open(api_.context,candidate.pid);handle->fd=opened.fd;
                if(handle->fd<0)return reject("Cannot acquire selected pidfd; errno="+std::to_string(opened.error),opened.error==EPERM||opened.error==EACCES?Support::permission_required:Support::unsupported);}
            const auto process=api_.inspect(api_.context,handle->fd,candidate.pid,candidate.process_start_ticks);
            if(!exact(process))return reject(process.note,process.support);
            const auto& p=*process.value;
            if(!valid_network_self(p)||p.local_pid!=candidate.pid||p.start_ticks!=candidate.process_start_ticks||
                p.procfs_device!=before.value->observer.procfs_device||p.procfs_inode!=before.value->observer.procfs_inode||
                (old!=entries_.end()&&old->process!=p))return reject("Selected process identity/context changed");
            next.push_back({candidate,std::move(handle),p});
        }
        const auto after=api_.observe(api_.context);
        if(!exact(after))return reject(after.note,after.support);
        if(*after.value!=*before.value||api_.caller(api_.context)!=caller)return reject("Observer context changed during selection refresh");
        // Revalidate ALL actors after acquisition, including early actors that
        // could exit/change while a later pidfd was being opened.
        for(const auto& e:next){const auto checked=api_.inspect(api_.context,e.handle->fd,e.candidate.pid,e.candidate.process_start_ticks);
            if(!exact(checked))return reject(checked.note,checked.support);
            if(*checked.value!=e.process)return reject("Selected actor changed during selection refresh");}
        const auto final=api_.observe(api_.context);
        if(!exact(final))return reject(final.note,final.support);
        if(*final.value!=*before.value||api_.caller(api_.context)!=caller)return reject("Observer changed after actor revalidation");
        const bool changed=!generation_||retained!=entries_.size()||retained!=next.size();
        const auto serial=changed?next_private_network_epoch():generation_;if(!serial)return reject("Selection generation exhausted");
        const Change result{serial,changed,next.size()-retained,entries_.size()-retained,retained};
        entries_=std::move(next);context_=*before.value;creator_=caller;generation_=serial;
        return {result,Support::available,"Complete declared candidate set pinned; no enumeration/capture or future liveness guarantee"};
    }catch(...){clear();throw;}
}
Metric<std::vector<LinuxProcessNetworkSelection>> LinuxNetworkSelectionOwner::selections(){
    if(!generation_)return absent<std::vector<LinuxProcessNetworkSelection>>("Selection owner uninitialized/cleared");
    if(api_.caller(api_.context)!=creator_){clear();return absent<std::vector<LinuxProcessNetworkSelection>>("Forked selection rows rejected; inherited handles closed");}
    std::vector<LinuxProcessNetworkSelection> rows;rows.reserve(entries_.size());
    for(const auto& e:entries_)rows.push_back({e.handle->fd,e.candidate.pid,e.candidate.process_start_ticks,0});
    return {std::move(rows),Support::available,"Borrowed pidfds until next refresh/clear; downstream must verify current liveness/kernel identity"};
}
NetworkSelectionSourceApi native_network_selection_source_api(){
#ifdef __linux__
    return {nullptr,[](void*) noexcept->std::uint64_t{return static_cast<std::uint64_t>(::getpid());},
        [](void*)->Metric<NetworkSelectionContext>{const auto source=native_network_sampler_api();const auto boot=source.boot(source.context);
            if(!exact(boot))return absent<NetworkSelectionContext>(boot.note,boot.support);
            const auto observer=read_linux_network_self();
            if(!exact(observer))return absent<NetworkSelectionContext>(observer.note,observer.support);
            return {NetworkSelectionContext{*boot.value,*observer.value},Support::available,{}};},
        [](void*,std::uint32_t pid)->NetworkSessionFd{const int fd=static_cast<int>(::syscall(SYS_pidfd_open,pid,0));return {fd,fd<0?errno:0};},
        [](void*,int fd,std::uint32_t pid,std::uint64_t start)->Metric<LinuxNetworkSelfIdentity>{
            auto alive=[&]{pollfd p{fd,POLLIN,0};return ::poll(&p,1,0)==0&&!p.revents;};
            if(!alive())return absent<LinuxNetworkSelfIdentity>("Selected pidfd exited or cannot be polled");
            auto identity=read_linux_network_pidfd_process(fd,pid,start);if(!alive())return absent<LinuxNetworkSelfIdentity>("Selected pidfd exited during inspection");return identity;},
        [](void*,int fd) noexcept {::close(fd);}};
#else
    return {};
#endif
}
}
namespace pcinsight {
struct LinuxProcessNetworkSelectionOwner::Impl {detail::LinuxNetworkSelectionOwner owner;};
LinuxProcessNetworkSelectionOwner::LinuxProcessNetworkSelectionOwner():impl_(std::make_unique<Impl>()){}
LinuxProcessNetworkSelectionOwner::~LinuxProcessNetworkSelectionOwner()=default;
LinuxProcessNetworkSelectionOwner::LinuxProcessNetworkSelectionOwner(LinuxProcessNetworkSelectionOwner&&) noexcept=default;
LinuxProcessNetworkSelectionOwner& LinuxProcessNetworkSelectionOwner::operator=(LinuxProcessNetworkSelectionOwner&&) noexcept=default;
void LinuxProcessNetworkSelectionOwner::clear() noexcept {if(impl_)impl_->owner.clear();}
Metric<LinuxNetworkSelectionChange> LinuxProcessNetworkSelectionOwner::refresh(bool authorized,std::span<const LinuxNetworkProcessCandidate> rows){
    if(!authorized)return {{},Support::permission_required,"Explicit selection preparation permission required"};
    try{if(!impl_)impl_=std::make_unique<Impl>();return impl_->owner.refresh(true,rows);}
    catch(const std::exception& e){clear();return {{},Support::unsupported,std::string("Selection preparation failed; all handles cleared: ")+e.what()};}
}
Metric<std::vector<LinuxProcessNetworkSelection>> LinuxProcessNetworkSelectionOwner::selections(){
    if(!impl_)return {{},Support::unsupported,"Moved-from selection owner"};
    return impl_->owner.selections();
}
}
