#include "linux_network_sampler.hpp"
#include <atomic>
#include <cerrno>
#include <limits>
#ifdef __linux__
#include <fcntl.h>
#include <sys/statfs.h>
#include <time.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
}
std::uint64_t next_private_network_epoch(){
    static std::atomic<std::uint64_t> serial{0};auto value=serial.load();
    while(value!=UINT64_MAX)if(serial.compare_exchange_weak(value,value+1))return value+1;
    return 0; // Never wrap/reuse an epoch for the same creator.
}
Metric<std::array<std::uint64_t,2>> parse_network_boot_token(std::string_view text){
    if(text.size()==37&&text.back()=='\n')text.remove_suffix(1);
    if(text.size()!=36)return absent<std::array<std::uint64_t,2>>("Invalid boot UUID length");
    std::array<std::uint64_t,2> out{};unsigned digits=0;
    for(unsigned i=0;i<text.size();++i){
        if(i==8||i==13||i==18||i==23){if(text[i]!='-')return absent<std::array<std::uint64_t,2>>("Invalid boot UUID separator");continue;}
        const char c=text[i];const int n=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;
        if(n<0)return absent<std::array<std::uint64_t,2>>("Invalid boot UUID digit");
        out[digits/16]=(out[digits/16]<<4)|static_cast<unsigned>(n);++digits;
    }
    if(!(out[0]||out[1]))return absent<std::array<std::uint64_t,2>>("Zero boot UUID");
    return {out,Support::available,"Boot token as exposed by observer procfs; not host authenticity proof"};
}
void LinuxPrivateNetworkSampler::stop() noexcept {capture_.stop();reducer_.reset();scope_.reset();last_join_.reset();identity_={};cookie_=local_creator_=last_end_=0;}
Metric<PrivateNetworkIdentity> LinuxPrivateNetworkSampler::start(bool authorized,std::uint64_t cookie){
    if(!authorized)return absent<PrivateNetworkIdentity>("Explicit SELF capture authorization required",Support::permission_required);
    if(scope_)return absent<PrivateNetworkIdentity>("Sampler already started; stop before restart");
    if(!cookie||!source_.boot||!source_.now||!source_.local_process)return absent<PrivateNetworkIdentity>("Socket namespace cookie and source API required");
    try{
        const auto creator=source_.local_process(source_.context);if(!creator)return absent<PrivateNetworkIdentity>("Local process guard unavailable");
        const auto boot=source_.boot(source_.context);if(!exact(boot))return absent<PrivateNetworkIdentity>("Exact boot token required: "+boot.note,boot.support);
        if(!((*boot.value)[0]||(*boot.value)[1]))return absent<PrivateNetworkIdentity>("Zero boot token");
        const auto identity=capture_.start(true,cookie);if(!exact(identity))return identity;
        const auto serial=next_private_network_epoch();if(!serial){stop();return absent<PrivateNetworkIdentity>("Producer epoch exhausted");}
        identity_=*identity.value;cookie_=cookie;local_creator_=creator;last_end_=0;reducer_.reset();
        NetworkAccountingScope s;s.boot=*boot.value;s.producer_epoch=serial;
        s.pid_domain=s.user_domain=NetworkIdentityDomain::kernel_initial;
        s.coverage=NetworkAccountingCoverage::self_single_socket_namespace;
        s.producer_tgid=identity_.kernel_tgid;s.producer_start_boottime_ns=identity_.leader_start_boottime_ns;
        scope_=s;return identity;
    }catch(...){stop();throw;}
}
Metric<PrivateNetworkWindow> LinuxPrivateNetworkSampler::sample(){
    last_join_.reset(); // Raw reads cannot seed a later locally attributed interval.
    auto reject=[&](std::string note,Support support=Support::unsupported){reducer_.reset();last_end_=0;return absent<PrivateNetworkWindow>(std::move(note),support);};
    if(!scope_)return reject("Sampler stopped");
    if(source_.local_process(source_.context)!=local_creator_){stop();return reject("Forked copy is not creator SELF; inherited capture closed");}
    try{
        const auto before=source_.now(source_.context);if(!exact(before))return reject(before.note,before.support);
        const auto read=capture_.read();if(!exact(read))return reject(read.note,read.support);
        const auto after=source_.now(source_.context);if(!exact(after))return reject(after.note,after.support);
        if(!*before.value||*after.value<*before.value||*before.value<last_end_||*after.value-*before.value>max_read_span_ns)
            return reject("Invalid/overlapping clock bracket or read span exceeds1s");
        const auto& r=*read.value;
        if(r.state!=PrivateNetworkSessionState::active||r.target_kind!=PrivateNetworkTargetKind::self||r.identity.kernel_tgid!=identity_.kernel_tgid||
            r.identity.kernel_real_uid!=identity_.kernel_real_uid||r.identity.leader_start_boottime_ns!=identity_.leader_start_boottime_ns||
            r.expected_socket_network_namespace_cookie!=cookie_)return reject("Capture identity/scope changed");
        NetworkAccountingInput input{*scope_,*after.value,0,true,{
            {{identity_.kernel_tgid,identity_.kernel_real_uid,identity_.leader_start_boottime_ns,cookie_},r.counters.bytes,true,false}}};
        // Complete ONLY for the explicitly declared one-bucket scope.
        auto accounting=reducer_.sample({std::move(input),Support::available,"SELF single-cookie scope only"});
        if(!exact(accounting))return reject(accounting.note,accounting.support);
        last_end_=*after.value;
        return {PrivateNetworkWindow{std::move(*accounting.value),r.counters,*before.value,*after.value},Support::available,
            "SELF instrumented single-cookie window; rates estimated at read-end times, UID sum partial; no procfs join/global totals"};
    }catch(...){reducer_.reset();last_end_=0;throw;}
}
Metric<PrivateJoinedNetworkWindow> LinuxPrivateNetworkSampler::sample_joined_self(){
    auto reject=[&](std::string note,Support support=Support::unsupported){reducer_.reset();last_join_.reset();last_end_=0;return absent<PrivateJoinedNetworkWindow>(std::move(note),support);};
    if(!scope_||!source_.self)return reject("Active sampler and SELF procfs source required");
    if(source_.local_process(source_.context)!=local_creator_){stop();return reject("Forked copy is not SELF creator");}
    auto matches=[&](const Metric<PrivateNetworkIdentity>& m){return exact(m)&&m.value->kernel_tgid==identity_.kernel_tgid&&
        m.value->kernel_real_uid==identity_.kernel_real_uid&&m.value->leader_start_boottime_ns==identity_.leader_start_boottime_ns;};
    try{
        const auto local_before=source_.self(source_.context);if(!exact(local_before))return reject(local_before.note,local_before.support);
        if(!valid_network_self(*local_before.value)||local_before.value->local_pid!=local_creator_)return reject("Invalid local SELF token");
        if(!last_join_||*last_join_!=*local_before.value){reducer_.reset();last_end_=0;}
        const auto kernel_before=capture_.read_caller_identity();if(!matches(kernel_before))return reject("Current kernel SELF identity absent/changed: "+kernel_before.note,kernel_before.support);
        const auto boot_before=source_.boot(source_.context);if(!exact(boot_before)||*boot_before.value!=scope_->boot)return reject("SELF boot context absent/changed",boot_before.support);
        auto result=sample();if(!exact(result))return reject(result.note,result.support);
        const auto kernel_after=capture_.read_caller_identity();if(!matches(kernel_after))return reject("Kernel SELF identity changed after sampling",kernel_after.support);
        const auto local_after=source_.self(source_.context);if(!exact(local_after))return reject(local_after.note,local_after.support);
        const auto boot_after=source_.boot(source_.context);if(!exact(boot_after)||*boot_after.value!=scope_->boot)return reject("SELF boot context changed after sampling",boot_after.support);
        if(*local_before.value!=*local_after.value||source_.local_process(source_.context)!=local_creator_)return reject("Local SELF identity/namespace/UID changed after sampling");
        last_join_=*local_after.value;
        return {PrivateJoinedNetworkWindow{std::move(*result.value),*local_after.value},Support::available,
            "Bracketed direct SELF association only; no arbitrary PID/UID mapping, app/executable or global totals"};
    }catch(...){reducer_.reset();last_join_.reset();last_end_=0;throw;}
}
#ifdef __linux__
namespace {
template<class T> Metric<T> syscall_failure(const char* operation){const int error=errno;return absent<T>(std::string(operation)+" errno="+std::to_string(error),error==EPERM||error==EACCES?Support::permission_required:Support::unsupported);}
Metric<std::array<std::uint64_t,2>> boot(void*){
    struct Fd{int n;~Fd(){if(n>=0)::close(n);}} file{::open("/proc/sys/kernel/random/boot_id",O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK)};
    if(file.n<0)return syscall_failure<std::array<std::uint64_t,2>>("boot open");
    struct statfs fs{};if(::fstatfs(file.n,&fs)<0)return syscall_failure<std::array<std::uint64_t,2>>("boot fstatfs");
    if(fs.f_type!=0x9fa0)return absent<std::array<std::uint64_t,2>>("Boot token source is not procfs");
    std::array<char,38> buffer{};std::size_t size=0;unsigned interrupts=0;
    while(size<buffer.size()){
        const auto n=::read(file.n,buffer.data()+size,buffer.size()-size);
        if(n<0){if(errno==EINTR&&++interrupts<=4)continue;return syscall_failure<std::array<std::uint64_t,2>>("boot read");}
        if(!n)break;
        size+=static_cast<std::size_t>(n);
    }
    return parse_network_boot_token({buffer.data(),size});
}
Metric<std::uint64_t> now(void*){timespec t{};if(::clock_gettime(CLOCK_MONOTONIC,&t)<0)return syscall_failure<std::uint64_t>("monotonic clock");
    if(t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1'000'000'000||static_cast<std::uint64_t>(t.tv_sec)>(UINT64_MAX-999999999)/1'000'000'000)
        return absent<std::uint64_t>("Monotonic clock range invalid");
    return {static_cast<std::uint64_t>(t.tv_sec)*1'000'000'000+static_cast<std::uint64_t>(t.tv_nsec),Support::available,{}};
}
std::uint64_t local(void*) noexcept {return static_cast<std::uint64_t>(::getpid());}
Metric<LinuxNetworkSelfIdentity> self(void*){return read_linux_network_self();}
}
#endif
NetworkSamplerApi native_network_sampler_api(){
#ifdef __linux__
    return {nullptr,boot,now,local,self};
#else
    return {};
#endif
}
}
