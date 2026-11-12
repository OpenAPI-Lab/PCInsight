#include "pcinsight/startup_store.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
using namespace pcinsight;using namespace std::chrono_literals;namespace fs=std::filesystem;
std::string read(const fs::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void write(const fs::path& p,const std::string& s){std::ofstream f(p,std::ios::binary|std::ios::trunc);f.write(s.data(),static_cast<std::streamsize>(s.size()));assert(f.good());}
void repair(std::string& s){std::uint64_t h=14695981039346656037ULL;for(unsigned char c:std::string_view(s).substr(0,s.size()-8)){h^=c;h*=1099511628211ULL;}for(unsigned n=0;n<8;++n)s[s.size()-8+n]=static_cast<char>(h>>(n*8));}
StartupImpactArchive capture(){
    StartupImpactArchive a;a.capture_id="outer";a.native_context="legacy-context";a.recorded_at=std::chrono::system_clock::time_point{}+1h;
    StartupBirthArchive b;auto& c=b.capture;c.capture_id="birth";c.boot={1,2};c.pid_namespace_device=4;c.pid_namespace_inode=99;c.pid_mapping_enabled=true;c.final_resources_enabled=true;c.first_exec_enabled=true;c.creator_exited=true;c.state=StartupBirthObserverState::closed;c.message="historical";c.pid_mapping_note="mapped";
    c.automatic_startup_origin={{},Support::unsupported,"not proven"};b.window=1000ns;b.policy.medium_cpu=1ns;b.policy.high_cpu=10ns;b.policy.medium_disk_bytes=1;b.policy.high_disk_bytes=50;
    StartupBirthObservation r;r.key={"birth",{1,2},100,0,10};r.birth_monotonic_ns=1000;r.final_monotonic_ns=2000;r.local_pid={StartupBirthPid{4,99,21},Support::available,"local"};
    r.startup_entry_id={"app.desktop",Support::estimated,"declaration"};r.cpu_time={11ns,Support::available,"CPU"};r.read_bytes={0,Support::available,"read zero"};r.write_bytes={50,Support::available,"write"};
    r.first_executable={StartupBirthObservation::Executable{1500,0,0,0,0},Support::available,"measured zero identifiers"};c.observations.push_back(r);
    ++r.key.kernel_start_boottime_ns;r.key.kernel_tgid=101;r.local_pid.value->tgid=22;r.final_monotonic_ns.reset();r.cpu_time={{},Support::permission_required,"unknown CPU"};r.read_bytes={};r.write_bytes={};r.startup_entry_id={};r.first_executable={};c.observations.push_back(r);
    a.births=b;return a;
}
int main(){
    fs::path root;for(unsigned n=0;n<100;++n){const auto p=fs::temp_directory_path()/("pcinsight-birth-store-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(n));if(fs::create_directory(p)){root=p;break;}}assert(!root.empty());
    struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};const auto path=root/"capture",bad=root/"bad";
    auto a=capture();const auto first=StartupImpactStore::save(path,a);if(!first)std::cerr<<"birth initial save error="<<static_cast<unsigned>(first.error)<<" replaced="<<first.replaced<<" note="<<first.message<<'\n';assert(first);const auto original=read(path);assert(original[8]==3);
    auto loaded=StartupImpactStore::load(path);assert(loaded&&loaded.archive->births&&!loaded.archive->diagnostics&&loaded.archive->observations.empty());
    const auto& b=*loaded.archive->births;assert(b.window==1000ns&&b.policy.high_cpu==10ns&&b.policy.high_disk_bytes==50&&b.capture.creator_exited&&b.capture.message=="historical");
    const auto& row=b.capture.observations[0];assert(row.key==a.births->capture.observations[0].key&&row.local_pid.value==StartupBirthPid({4,99,21})&&row.cpu_time.note=="CPU"&&row.read_bytes.value==0);
    assert(row.first_executable.value==StartupBirthObservation::Executable({1500,0,0,0,0})&&row.first_executable.support==Support::available&&!b.capture.automatic_startup_origin.value);
    const auto& unknown=b.capture.observations[1];assert(!unknown.cpu_time.value&&unknown.cpu_time.support==Support::permission_required&&unknown.cpu_time.note=="unknown CPU"&&!unknown.startup_entry_id.value&&!unknown.final_monotonic_ns);
    const auto measured=measure_startup_birth_impact(b.capture,row.key,b.window,b.policy);assert(measured.value->impact.value==StartupImpact::high&&measured.value->disk_bytes.value==50);
    assert(StartupImpactStore::save(path,*loaded.archive)&&read(path)==original); // No sums/hidden migration.
    for(unsigned mode=0;mode<21;++mode){auto invalid=a;auto& c=invalid.births->capture;auto& r=c.observations[0];
        if(mode==0)c.state=StartupBirthObserverState::recording;if(mode==1)c.boot={};if(mode==2)r.key.boot={};if(mode==3)c.capture_id="outer";
        if(mode==4)c.quality_flags=128;if(mode==5)c.automatic_startup_origin={false,Support::estimated,"not unknown"};if(mode==6)r.cpu_time.support=Support::unsupported;
        if(mode==7)c.observations.push_back(r);if(mode==8)r.final_monotonic_ns=999;if(mode==9)r.local_pid.value->namespace_inode=98;
        if(mode==10)r.first_executable.value->observed_monotonic_ns=2001;if(mode==11)c.first_exec_enabled=false;if(mode==12)c.final_resources_enabled=false;
        if(mode==13)r.write_bytes={};if(mode==14)c.message.resize(4097,'x');if(mode==15)r.cpu_time.value=-1ns;if(mode==16)invalid.births->window=0ns;
        if(mode==17)invalid.births->policy.high_cpu=0ns;if(mode==18)c.error=ActionError::system_error;if(mode==19)r.startup_entry_id.value=std::string("a\0b",3);if(mode==20)c.observations.resize(65);
        const auto saved=StartupImpactStore::save(path,invalid);assert(!saved&&!saved.replaced&&read(path)==original);
    }
    for(std::size_t n=0;n<original.size();++n){write(bad,original.substr(0,n));auto result=StartupImpactStore::load(bad);assert(!result&&!result.archive);}
    for(std::size_t n=0;n<original.size();++n){auto corrupt=original;corrupt[n]^=1;write(bad,corrupt);assert(!StartupImpactStore::load(bad));}
    // Repaired checksums cannot hide structural truncation of the extension.
    const std::size_t extension=12+4+a.capture_id.size()+4+a.native_context.size()+8+4;
    for(std::size_t n=extension;n<original.size()-8;++n){auto truncated=original.substr(0,n)+std::string(8,'\0');repair(truncated);write(bad,truncated);assert(!StartupImpactStore::load(bad));}
    auto corrupt=original;corrupt[extension]=2;repair(corrupt);write(bad,corrupt);assert(!StartupImpactStore::load(bad)); // Diagnostic presence.
    const auto metadata=extension+1+4+a.births->capture.capture_id.size()+16+24;
    for(const auto offset:{metadata,metadata+4,metadata+6}){corrupt=original;corrupt[offset]=static_cast<char>(offset==metadata?65:offset==metadata+4?1:2);repair(corrupt);write(bad,corrupt);const auto result=StartupImpactStore::load(bad);assert(!result&&!result.archive);} // Publishing bound/state/boolean despite valid CRC.
    corrupt=original;corrupt[metadata-8]=static_cast<char>(128);repair(corrupt);write(bad,corrupt);assert(!StartupImpactStore::load(bad)); // Unknown quality flag.
    corrupt=original.substr(0,original.size()-8)+"junk"+std::string(8,'\0');repair(corrupt);write(bad,corrupt);assert(!StartupImpactStore::load(bad));
    auto legacy=a;legacy.births.reset();assert(StartupImpactStore::save(bad,legacy)&&read(bad)[8]==1&&!StartupImpactStore::load(bad).archive->births);
    StartupCaptureDiagnostics d;d.requested_duration=100ms;d.scan_interval=20ms;legacy.diagnostics=d;assert(StartupImpactStore::save(bad,legacy)&&read(bad)[8]==2&&!StartupImpactStore::load(bad).archive->births);
    a.diagnostics=d;assert(StartupImpactStore::save(bad,a));loaded=StartupImpactStore::load(bad);assert(loaded&&loaded.archive->births&&loaded.archive->diagnostics);
    a.births->capture.observations.clear();assert(StartupImpactStore::save(bad,a));assert(StartupImpactStore::load(bad).archive->births->capture.observations.empty()); // Present empty != absent legacy.
    a=capture();a.births->capture.pid_mapping_enabled=false;a.births->capture.observations[0].local_pid={};a.births->capture.observations[1].local_pid={};
    a.births->capture.state=StartupBirthObserverState::failed;a.births->capture.error=ActionError::system_error;a.births->capture.quality_flags=32;
    assert(StartupImpactStore::save(bad,a)&&StartupImpactStore::load(bad).archive->births->capture.state==StartupBirthObserverState::failed);
    for(const auto& f:fs::directory_iterator(root))assert(f.path().extension()!=".tmp");
    std::cout<<"birth store v3: exact keys/metrics/notes/policy/exec/unknown/zero/empty/failure/legacy/checksum/truncation/validation/preservation passed\n";
}
