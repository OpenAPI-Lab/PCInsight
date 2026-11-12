#include "../src/linux_virtualization.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cerrno>
#include <chrono>
#include <fstream>
#include <iostream>
#ifdef __linux__
#include <sys/stat.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs=std::filesystem;
void write(const fs::path& p,const std::string& text){std::ofstream out(p,std::ios::binary);assert(out);out<<text;out.close();assert(out);}
int main(){
    const LinuxKvmDevice registered{10,232};
    assert(parse_linux_kvm_device("10:232\n")==registered);
    assert(parse_linux_kvm_device(" \t4294967295:4294967295\r\n"));
    assert((parse_linux_kvm_device("0:0")==LinuxKvmDevice{0,0}));
    for(const auto& text:std::vector<std::string>{""," ","10","10:",":232","10:232 extra","10:232:1","-1:232","+10:232","10:-1","10:+232","10 :232","10: 232","4294967296:1","1:4294967296",std::string("10:232\0",7),std::string(65,'1')})assert(!parse_linux_kvm_device(text));
    assert(linux_kvm_device_matches(true,registered,registered));
    assert(!linux_kvm_device_matches(false,registered,registered));
    assert(!linux_kvm_device_matches(true,{10,3},registered));
    assert(!linux_kvm_device_matches(true,{11,232},registered));
    const auto positive=linux_kvm_query_result(12);assert(positive.value==12u&&positive.support==Support::available&&!positive.note.empty());
    // An unexpected version equal to an errno number is still an ABI mismatch,
    // never permission_required or an accepted zero-valued API.
    for(int version:{-1,0,1,11,13,14,999}){const auto result=linux_kvm_query_result(version);assert(!result.value&&result.support==Support::unsupported);}
    for(auto stage:{LinuxKvmFailure::registration,LinuxKvmFailure::open,LinuxKvmFailure::descriptor,LinuxKvmFailure::query}){
        for(auto error:{EACCES,EPERM}){const auto result=linux_kvm_failure(stage,error);assert(!result.value&&result.support==Support::permission_required);}
        for(auto error:{0,ENOENT,ENODEV,ENOTTY,EINTR,EIO}){const auto result=linux_kvm_failure(stage,error);assert(!result.value&&result.support==Support::unsupported&&!result.note.empty());}
    }
    for(auto stage:{LinuxKvmFailure::identity,LinuxKvmFailure::changed,LinuxKvmFailure::version})assert(linux_kvm_failure(stage,EACCES).support==Support::unsupported);
    CpuInfo cpu;cpu.virtualization_enabled={true,Support::available,"old guessed state"};
    apply_linux_virtualization(cpu,positive);assert(!cpu.virtualization_enabled.value&&cpu.virtualization_enabled.support==Support::unsupported&&cpu.linux_kvm_api_version.value==12u);
    apply_linux_virtualization(cpu,linux_kvm_failure(LinuxKvmFailure::open,EACCES));
    assert(!cpu.virtualization_enabled.value&&!cpu.linux_kvm_api_version.value&&cpu.linux_kvm_api_version.support==Support::permission_required);
    apply_linux_virtualization(cpu,positive);apply_linux_virtualization(cpu,linux_kvm_failure(LinuxKvmFailure::registration));assert(!cpu.linux_kvm_api_version.value);
#ifdef __linux__
    const auto root=fs::temp_directory_path()/("pcinsight-kvm-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(fs::create_directory(root));struct Cleanup{fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}} cleanup{root};
    const auto device=root/"kvm",reg=root/"dev";
    auto result=read_linux_kvm_api(device,reg);assert(!result.value&&result.support==Support::unsupported);
    write(reg,"10:232\n");result=read_linux_kvm_api(device,reg);assert(!result.value&&result.note.find("metadata")!=std::string::npos);
    write(device,"not KVM");result=read_linux_kvm_api(device,reg);assert(!result.value&&result.note.find("no ioctl")!=std::string::npos);
    result=read_linux_kvm_api(root,reg);assert(!result.value&&result.note.find("no ioctl")!=std::string::npos);
    // Existing but wrong character device is rejected before the query. No
    // ioctl is sent to /dev/null, no character devices are created for tests.
    result=read_linux_kvm_api("/dev/null",reg);assert(!result.value&&result.note.find("no ioctl")!=std::string::npos);
    const auto link=root/"link";fs::create_symlink(device,link);result=read_linux_kvm_api(link,reg);assert(!result.value&&result.note.find("no ioctl")!=std::string::npos);
    const auto fifo=root/"fifo";assert(::mkfifo(fifo.c_str(),0600)==0);result=read_linux_kvm_api(fifo,reg);assert(!result.value&&result.note.find("no ioctl")!=std::string::npos); // bounded/nonblocking, not a fake device probe
    for(const auto& invalid:std::vector<std::string>{"","10:232 extra",std::string(65,'1')}){
        write(reg,invalid);result=read_linux_kvm_api(device,reg);assert(!result.value&&result.note.find("registration")!=std::string::npos);
    }
    write(reg,"10:232");fs::remove(device);apply_linux_virtualization(cpu,read_linux_kvm_api(device,reg));assert(!cpu.virtualization_enabled.value&&!cpu.linux_kvm_api_version.value);
    fill_linux_virtualization(cpu);assert(!cpu.virtualization_enabled.value);
    if(cpu.linux_kvm_api_version.value)assert(cpu.linux_kvm_api_version.value==12u&&cpu.linux_kvm_api_version.support==Support::available);
    std::cout<<"Native KVM API: ";if(cpu.linux_kvm_api_version.value)std::cout<<*cpu.linux_kvm_api_version.value;else std::cout<<"unavailable";
    std::cout<<"; "<<cpu.linux_kvm_api_version.note<<'\n';
    SampleOptions options;options.include_processes=false;options.include_app_history=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto sample=monitor.sample(options);
    assert(!sample.cpu.virtualization_enabled.value&&sample.cpu.virtualization_enabled.support==Support::unsupported);
    if(sample.cpu.linux_kvm_api_version.value)assert(sample.cpu.linux_kvm_api_version.value==12u&&sample.cpu.linux_kvm_api_version.support==Support::available);
    std::cout<<"Linux filesystem guards: missing/regular/directory/wrong char/symlink/FIFO and malformed registration rejected; no VM created\n";
#else
    assert(!read_linux_kvm_api().value);
#endif
    std::cout<<"Virtualization evidence: API12 result, unexpected versions, permissions vs unknown, identity checks and stale clearing passed; no BIOS enablement inferred\n";
}
