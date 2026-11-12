#include "../src/linux_manual_store.hpp"
#include "../src/startup_store_bytes.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef __linux__
#include <sys/stat.h>
#endif
using namespace pcinsight;using namespace pcinsight::detail;using namespace std::chrono_literals;namespace fs=std::filesystem;
std::string read(const fs::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void repair(std::string& s){std::uint64_t h=14695981039346656037ULL;for(unsigned char c:std::string_view(s).substr(0,s.size()-8)){h^=c;h*=1099511628211ULL;}for(unsigned n=0;n<8;++n)s[s.size()-8+n]=static_cast<char>(h>>(n*8));}
LinuxManualLaunchArchive frame(){
    LinuxManualLaunchArchive a;auto& b=a.births;b.window=1000ns;b.policy.medium_cpu=1ns;b.policy.high_cpu=10ns;b.policy.medium_disk_bytes=1;b.policy.high_disk_bytes=50;
    auto& c=b.capture;c.capture_id="private-capture";c.boot={1,2};c.state=StartupBirthObserverState::closed;c.pid_namespace_device=4;c.pid_namespace_inode=5;c.pid_mapping_enabled=c.final_resources_enabled=c.first_exec_enabled=true;c.message="closed original";
    StartupBirthObservation row;row.key={c.capture_id,c.boot,100,0,10};row.birth_monotonic_ns=1000;row.final_monotonic_ns=2000;row.local_pid={StartupBirthPid{4,5,8},Support::available,"namespace"};row.cpu_time={11ns,Support::available,"CPU original"};row.read_bytes={0,Support::available,"zero"};row.write_bytes={50,Support::available,"write"};row.first_executable={StartupBirthObservation::Executable{1500,55,8,1,0},Support::available,"birth event"};c.observations.push_back(row);
    LinuxDesktopRoute route{"/private/a.desktop",{"/bin/fixture",{"","two words","; touch NO","$HOME","%"},"/private",false}};
    PrivatePidfdNetworkJoin selected{{100,0,10},{{1,2},{7,42,100,1,2,3,4,5,6,7},{8,42,200,1,2,8,4,5,6,7}}};
    SelectedExecEvidence evidence;evidence.first_exec={StartupBirthObservation::Executable{1600,55,8,1,0},Support::available,"receipt event note"};evidence.pinned_file_matches={true,Support::available,"pin note"};
    a.receipt=project_linux_manual_receipt(route,selected,{55,8,1},8,ActionError::none,{SelectedExecRead{selected,evidence,true},Support::available,"source"});assert(a.receipt.value);return a;
}
int main(){
    auto a=frame();const auto encoded=encode_linux_manual_launch(a);if(!encoded.value)std::cerr<<encoded.note<<'\n';assert(encoded.value);const auto& bytes=*encoded.value;
    auto loaded=decode_linux_manual_launch(bytes);assert(loaded&&loaded.archive->receipt.value);const auto& r=*loaded.archive->receipt.value;
    assert(r.origin==LinuxDesktopLaunchOrigin::manual_request&&r.entry_first_exec_matches.value==true&&r.route.request.arguments==a.receipt.value->route.request.arguments&&r.selected.local==a.receipt.value->selected.local&&r.evidence.first_exec.note=="receipt event note"&&r.evidence.pinned_file_matches.note=="pin note");
    assert(loaded.archive->births.capture.observations[0].read_bytes.value==0&&loaded.archive->births.window==1000ns&&!loaded.archive->births.capture.observations[0].startup_entry_id.value);
    const auto m=measure_linux_manual_launch_resources(loaded.archive->receipt,loaded.archive->births.capture,loaded.archive->births.window,loaded.archive->births.policy);assert(m.value&&m.value->measurement.cpu_time.value==11ns&&m.value->measurement.disk_bytes.value==50&&m.value->measurement.impact.value==StartupImpact::high&&!m.value->measurement.automatic_startup_origin.value);
    assert(encode_linux_manual_launch(*loaded.archive).value==encoded.value);
    // Derived caller caches cannot be persisted as proof or override raw evidence.
    a.receipt.value->entry_first_exec_matches={false,Support::available,"fabricated cache"};assert(decode_linux_manual_launch(*encode_linux_manual_launch(a).value).archive->receipt.value->entry_first_exec_matches.value==true);a=frame();
    for(std::size_t n=0;n<bytes.size();++n){assert(!decode_linux_manual_launch(std::string_view(bytes).substr(0,n)));auto bad=bytes;bad[n]^=1;assert(!decode_linux_manual_launch(bad));}
    std::uint32_t base_size=0;for(unsigned n=0;n<4;++n)base_size|=std::uint32_t{static_cast<unsigned char>(bytes[12+n])}<<(8*n);const std::size_t receipt_offset=16+base_size;
    for(const auto offset:{std::size_t{8},receipt_offset,receipt_offset+1,receipt_offset+2}){auto bad=bytes;bad[offset]=static_cast<char>(99);repair(bad);assert(!decode_linux_manual_launch(bad));}
    {auto bad=bytes;const auto count=receipt_offset+3+4+a.receipt.value->route.entry_id.size()+4+a.receipt.value->route.request.executable.size();for(unsigned i=0;i<4;++i)bad[count+i]=static_cast<char>(255);repair(bad);assert(!decode_linux_manual_launch(bad));}
    assert(!decode_linux_manual_launch(std::string(16*1024*1024+1,'x')));
    {auto bad=bytes.substr(0,bytes.size()-8)+"trailing"+std::string(8,'\0');repair(bad);assert(!decode_linux_manual_launch(bad));}
    {auto absent=frame();absent.receipt={{},Support::permission_required,"capture denied"};const auto back=decode_linux_manual_launch(*encode_linux_manual_launch(absent).value);assert(back&&!back.archive->receipt.value&&back.archive->receipt.support==Support::permission_required&&back.archive->receipt.note=="capture denied");}
    {auto pending=frame();auto& r=*pending.receipt.value;r.evidence.first_exec={};r.evidence.pinned_file_matches={};r.entry_first_exec_matches={};r.caller_owned_pid=0;r.launch_error=ActionError::system_error;const auto back=decode_linux_manual_launch(*encode_linux_manual_launch(pending).value);assert(back&&!back.archive->receipt.value->entry_first_exec_matches.value);}
    {auto quality=frame();quality.receipt.value->evidence.quality_flags=64;quality.receipt.support=Support::estimated;const auto back=decode_linux_manual_launch(*encode_linux_manual_launch(quality).value);assert(back&&back.archive->receipt.value->evidence.first_exec.value&&!back.archive->receipt.value->entry_first_exec_matches.value);}
    {auto zero=frame();zero.receipt.value->pinned_file={};auto& e=*zero.receipt.value->evidence.first_exec.value;e.inode=e.device_major=e.device_minor=0;const auto back=decode_linux_manual_launch(*encode_linux_manual_launch(zero).value);assert(back&&back.archive->receipt.value->entry_first_exec_matches.value==true);}
    for(unsigned n=0;n<8;++n){auto invalid=frame();auto& r=*invalid.receipt.value;if(n==0)r.origin=static_cast<LinuxDesktopLaunchOrigin>(1);if(n==1)++r.selected.local.boot[0];if(n==2)r.evidence.first_exec.support=Support::unsupported;if(n==3)r.evidence.first_exec.note.resize(4097,'x');if(n==4)r.route.request.arguments.push_back(std::string("bad\0arg",7));if(n==5)r.caller_owned_pid=9;if(n==6)r.evidence.publishing=true;if(n==7)invalid.births.window=0ns;assert(!encode_linux_manual_launch(invalid).value);}
    const auto suffix="pcinsight-manual-store-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
#ifdef __linux__
    const auto root=fs::path("/tmp")/suffix;
#else
    const auto root=fs::current_path()/suffix;
#endif
    assert(fs::create_directory(root));struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};const auto path=root/"capture";
    assert(save_linux_manual_launch(path,frame()));const auto original=read(path);assert(original==bytes&&load_linux_manual_launch(path));
    auto invalid=frame();invalid.receipt.value->origin=static_cast<LinuxDesktopLaunchOrigin>(1);const auto rejected=save_linux_manual_launch(path,invalid);assert(!rejected&&!rejected.replaced&&read(path)==original);
    assert(!save_linux_manual_launch(root,frame())&&fs::is_directory(root));assert(!load_linux_manual_launch(root/"missing"));
    {std::ofstream f(root/"unrelated.backup");f<<"keep";}assert(save_linux_manual_launch(path,frame())&&read(root/"unrelated.backup")=="keep");
#ifdef __linux__
    struct stat info{};assert(::stat(path.c_str(),&info)==0&&(info.st_mode&0777)==0600);fs::create_symlink(path,root/"link");assert(!load_linux_manual_launch(root/"link")&&!save_linux_manual_launch(root/"link",frame())&&read(path)==original);fs::create_directory(root/"blocked");assert(!save_linux_manual_launch(root/"blocked",frame()));
    // Probe a private fixture on the build filesystem as well. If POSIX mode is
    // ignored (e.g. this WSL /mnt/c), save must refuse before payload/replacement.
    const auto other=fs::current_path()/suffix;assert(fs::create_directory(other));Cleanup other_cleanup{other};const auto probe=other/"capture";{std::ofstream f(probe);f<<"preserve";}
    const auto result=save_linux_manual_launch(probe,frame());if(!result){assert(result.error==ActionError::not_supported&&!result.replaced&&read(probe)=="preserve");std::cout<<"Non-POSIX-private filesystem refusal: old file preserved, no payload written\n";}else assert(::stat(probe.c_str(),&info)==0&&(info.st_mode&0777)==0600);
#endif
    StartupImpactArchive legacy;legacy.capture_id="legacy";legacy.native_context="old";const auto old=encode_startup_archive_bytes(legacy);assert(old.value&&decode_startup_archive_bytes(*old.value)&&!decode_linux_manual_launch(*old.value));assert(!decode_startup_archive_bytes(bytes));
    std::cout<<"Manual envelope roundtrip/presence/notes/known-zero/quality/reprojection/byte corruption/atomic preservation/legacy separation passed; historical ONLY\n";
}
