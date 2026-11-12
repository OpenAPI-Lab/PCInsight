#include "linux_virtualization.hpp"
#include <array>
#include <cerrno>
#include <charconv>
#ifdef __linux__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
std::optional<LinuxKvmDevice> parse_linux_kvm_device(std::string_view text){
    if(text.empty()||text.size()>64)return {};
    const auto first=text.find_first_not_of(" \t\r\n"),last=text.find_last_not_of(" \t\r\n");if(first==text.npos)return {};
    text=text.substr(first,last-first+1);const auto colon=text.find(':');if(colon==text.npos)return {};
    LinuxKvmDevice result;
    auto number=[](std::string_view s,std::uint32_t& n){
        if(s.empty())return false;
        const auto [end,ec]=std::from_chars(s.data(),s.data()+s.size(),n);
        return ec==std::errc{}&&end==s.data()+s.size();
    };
    if(!number(text.substr(0,colon),result.major)||!number(text.substr(colon+1),result.minor))return {};
    return result;
}
bool linux_kvm_device_matches(bool character_device,LinuxKvmDevice actual,LinuxKvmDevice registered){return character_device&&actual==registered;}
Metric<unsigned> linux_kvm_failure(LinuxKvmFailure stage,int code){
    std::string note;
    switch(stage){
        case LinuxKvmFailure::registration:note="KVM sysfs registration missing, unreadable, malformed or over bounds";break;
        case LinuxKvmFailure::open:note="KVM device could not be opened read-only";break;
        case LinuxKvmFailure::descriptor:note="KVM path/descriptor metadata query failed";break;
        case LinuxKvmFailure::identity:note="KVM path/descriptor is not a character device matching sysfs registration; no ioctl issued";break;
        case LinuxKvmFailure::query:note="KVM_GET_API_VERSION failed";break;
        case LinuxKvmFailure::version:note="Unexpected KVM API version; compatible API12 not established";break;
        case LinuxKvmFailure::changed:note="KVM sysfs registration changed or disappeared during probe";break;
    }
    const bool error_stage=stage==LinuxKvmFailure::registration||stage==LinuxKvmFailure::open||stage==LinuxKvmFailure::descriptor||stage==LinuxKvmFailure::query;
    const auto support=error_stage&&(code==EACCES||code==EPERM)?Support::permission_required:Support::unsupported;
    if(code)note+=" (code="+std::to_string(code)+")";
    return {{},support,std::move(note)};
}
Metric<unsigned> linux_kvm_query_result(int version){
    if(version!=12)return linux_kvm_failure(LinuxKvmFailure::version,version);
    return {12u,Support::available,"KVM_GET_API_VERSION=12 on read-only fd matching sysfs KVM device registration; not firmware enablement or proof that creating/running a VM will succeed"};
}
Metric<unsigned> read_linux_kvm_api(const std::filesystem::path& device,const std::filesystem::path& registration){
#ifdef __linux__
    struct Fd {int value;~Fd(){if(value>=0)::close(value);}};
    auto registered=[&]()->Metric<LinuxKvmDevice>{
        const Fd fd{::open(registration.c_str(),O_RDONLY|O_CLOEXEC|O_NONBLOCK)};
        if(fd.value<0){const int error=errno;auto fail=linux_kvm_failure(LinuxKvmFailure::registration,error);return {{},fail.support,std::move(fail.note)};}
        std::array<char,65> text{};std::size_t used=0;unsigned interrupts=0;
        while(used<text.size()){
            const auto bytes=::read(fd.value,text.data()+used,text.size()-used);
            if(bytes<0){const int error=errno;if(error==EINTR&&++interrupts<=3)continue;auto fail=linux_kvm_failure(LinuxKvmFailure::registration,error);return {{},fail.support,std::move(fail.note)};}
            if(bytes==0)break;
            used+=static_cast<std::size_t>(bytes);
        }
        auto value=parse_linux_kvm_device(std::string_view(text.data(),used));
        if(!value)return {{},Support::unsupported,"KVM registration format/bounds invalid"};
        return {*value,Support::available,"Kernel sysfs KVM device number"};
    };
    const auto before=registered();if(!before.value)return {{},before.support,before.note};
    auto matches=[&](const struct stat& info){
        return linux_kvm_device_matches(S_ISCHR(info.st_mode),{static_cast<std::uint32_t>(::major(info.st_rdev)),static_cast<std::uint32_t>(::minor(info.st_rdev))},*before.value);
    };
    struct stat path_metadata{};
    if(::lstat(device.c_str(),&path_metadata)!=0){const int error=errno;return linux_kvm_failure(LinuxKvmFailure::descriptor,error);}
    if(!matches(path_metadata))return linux_kvm_failure(LinuxKvmFailure::identity);
    const Fd fd{::open(device.c_str(),O_RDONLY|O_CLOEXEC|O_NONBLOCK|O_NOFOLLOW)};
    if(fd.value<0){const int error=errno;return linux_kvm_failure(LinuxKvmFailure::open,error);}
    struct stat metadata{};
    if(::fstat(fd.value,&metadata)!=0){const int error=errno;return linux_kvm_failure(LinuxKvmFailure::descriptor,error);}
    if(!matches(metadata))return linux_kvm_failure(LinuxKvmFailure::identity);
    // Stable Linux UAPI: KVM_GET_API_VERSION = _IO(0xAE, 0x00).
    // No linux/kvm.h dependency, KVM_CREATE_VM, MSR access or state-changing ioctl.
    const int version=::ioctl(fd.value,_IO(0xAE,0x00),0);
    if(version<0){const int error=errno;return linux_kvm_failure(LinuxKvmFailure::query,error);}
    const auto after=registered();if(!after.value||after.value!=before.value)return linux_kvm_failure(LinuxKvmFailure::changed);
    return linux_kvm_query_result(version);
#else
    (void)device;(void)registration;return {{},Support::unsupported,"KVM system API probe is Linux-only"};
#endif
}
void apply_linux_virtualization(CpuInfo& cpu,Metric<unsigned> kvm_api){
    cpu.virtualization_enabled={{},Support::unsupported,"Firmware virtualization state not established by this Linux provider; device presence, CPU flags and KVM API access are separate evidence"};
    cpu.linux_kvm_api_version=std::move(kvm_api);
}
void fill_linux_virtualization(CpuInfo& cpu){apply_linux_virtualization(cpu,read_linux_kvm_api());}
}
