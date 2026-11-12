#include "../src/linux_process_details.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs=std::filesystem;
std::string stat_text(std::uint32_t pid=123,std::uint64_t start=100){return std::to_string(pid)+" (details fixture) S 12 0 0 0 -1 0 0 0 0 0 0 0 0 0 20 0 3 0 "+std::to_string(start)+" 12345 0 0";}
void write(const fs::path& path,const std::string& text){std::ofstream f(path,std::ios::binary);assert(f);f<<text;f.close();assert(f);}
std::array<unsigned char,64> elf(unsigned bits=2,unsigned endian=1,unsigned machine=62){
    std::array<unsigned char,64> h{};h[0]=0x7f;h[1]='E';h[2]='L';h[3]='F';h[4]=bits;h[5]=endian;h[6]=1;
    auto word=[&](unsigned at,unsigned value){h[at]=endian==1?value&255:value>>8;h[at+1]=endian==1?value>>8:value&255;};
    word(16,3);word(18,machine);h[endian==1?20:23]=1;word(bits==1?40:52,bits==1?52:64);return h;
}
int main(){
    const auto x64=elf();assert(linux_executable_architecture(x64).value=="x64");
    assert(linux_executable_architecture(elf(1,1,3)).value=="x86");assert(linux_executable_architecture(elf(2,2,183)).value=="ARM64");
    assert(linux_executable_architecture(elf(1,2,40)).value=="ARM32");assert(linux_executable_architecture(elf(2,1,243)).value=="RISC-V64");
    assert(linux_executable_architecture(elf(2,2,21)).value=="PowerPC64");
    for(std::size_t size=0;size<64;++size)assert(!linux_executable_architecture(std::span(x64.data(),size)).value);
    const auto x86=elf(1,1,3);assert(linux_executable_architecture(std::span(x86.data(),52)).value=="x86");
    for(const unsigned offset:{0u,4u,5u,6u,16u,18u,20u,52u}){auto h=x64;h[offset]=0;assert(!linux_executable_architecture(h).value);}
    assert(!linux_executable_architecture(elf(1,1,62)).value); // class/machine mismatch
    const std::string good="Name:\ttest app\nPid:123\nState:\tS (sleeping)\nUid:\t0 1000 2000 4294967295\n";
    const auto parsed=parse_linux_process_status(good,123);assert(parsed.value&&parsed.value->real_uid==0&&parsed.value->name=="test app"&&parsed.value->state=="S (sleeping)");
    const auto partial=parse_linux_process_status("Pid:123",123);assert(partial.value&&!partial.value->real_uid&&!partial.value->name&&!partial.value->state);
    assert(parse_linux_process_status("Pid:123\nName:a\x01\n",123).value->name=="a\\x01");
    assert(parse_linux_process_status("Pid:123\nState:Q (future kernel state)",123).value);
    for(const auto& bad:std::vector<std::string>{"","Pid:124","Pid:0","Pid:-1","Pid:+123","Pid:4294967296","Pid:123 trailing","Pid:123\nPid:123","Pid:123\nUid:0 0 0","Pid:123\nUid:0 0 0 0 0","Pid:123\nUid:0 0 -1 0","Pid:123\nUid:0 4294967296 0 0","Pid:123\nUid:0 0 0 0\nUid:0 0 0 0","Pid:123\nName:a\nName:b","Pid:123\nState:R\nState:S","Pid:123\nState:Running","Pid:123\nState:","Pid:123\nName:"+std::string(4097,'a'),"Pid:123\nOther:"+std::string(65536,'x'),std::string(1024*1024+1,'x'),std::string("Pid:123\0",8)})assert(!parse_linux_process_status(bad,123).value);
    assert(!parse_linux_process_status(good,0).value);std::string lines="Pid:123\n";for(int n=0;n<10000;++n)lines+="x\n";assert(!parse_linux_process_status(lines,123).value);
    const auto identity=parse_linux_process_stat(stat_text());assert(identity);
    const LinuxExecutableStamp stamp{1,2,64,0100755,100,10,200,20};
    assert(linux_executable_matches(*identity,identity,identity,stamp,stamp,"/a","/a"));
    for(int mode=0;mode<11;++mode){auto other=stamp;auto before=identity,after=identity;std::string first="/a",last="/a";if(mode==0)++other.inode;if(mode==1)++other.device;if(mode==2)++other.size;if(mode==3)++other.mode;if(mode==4)++other.mtime_nanoseconds;if(mode==5)++other.ctime_seconds;if(mode==6)before.reset();if(mode==7)++after->start_ticks;if(mode==8)last="/b";if(mode==9)first.clear();if(mode==10)first=last=std::string("/a\0",3);assert(!linux_executable_matches(*identity,before,after,stamp,other,first,last));}
    assert(linux_process_status_for_identity(*identity,identity,identity,parsed).value);
    for(int mode=0;mode<5;++mode){auto before=identity,after=identity;if(mode==0)before.reset();if(mode==1)after.reset();if(mode==2)before->pid=124;if(mode==3)before->start_ticks=101;if(mode==4)after->start_ticks=101;assert(!linux_process_status_for_identity(*identity,before,after,parsed).value);}
    auto boot=*identity;boot.start_ticks=0;assert(linux_process_status_for_identity(boot,boot,boot,parsed).value);
    Metric<LinuxProcessStatus> denied{{},Support::permission_required,"fixture"};assert(linux_process_status_for_identity(*identity,identity,identity,denied).support==Support::permission_required);
#ifdef __linux__
    const auto root=fs::temp_directory_path()/("pcinsight-details-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));assert(fs::create_directory(root));struct Cleanup{fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}} cleanup{root};
    assert(!read_linux_process_identity(root).value);write(root/"stat",stat_text());assert(read_linux_process_identity(root).value);assert(!read_linux_process_status(root,*identity).value);
    write(root/"status",good);assert(read_linux_process_status(root,*identity).value);write(root/"stat",stat_text(123,101));assert(!read_linux_process_status(root,*identity).value);write(root/"stat",stat_text());
    write(root/"status",std::string(1024*1024+1,'x'));assert(!read_linux_process_status(root,*identity).value);write(root/"stat",std::string(65537,'x'));assert(!read_linux_process_identity(root).value);write(root/"stat",stat_text());
    assert(fs::remove(root/"status"));write(root/"other",good);fs::create_symlink(root/"other",root/"status");assert(!read_linux_process_status(root,*identity).value);
    assert(!read_linux_process_executable(root,*identity).value);
    write(root/"image (deleted)",std::string(reinterpret_cast<const char*>(x64.data()),x64.size()));fs::create_symlink(root/"image (deleted)",root/"exe");
    const auto fake=read_linux_process_executable(root,*identity);assert(fake.value&&fake.value->architecture.value=="x64"&&fake.value->path.value== (root/"image (deleted)").string());
    write(root/"stat",stat_text(123,101));assert(!read_linux_process_executable(root,*identity).value);write(root/"stat",stat_text());
    write(root/"image (deleted)","not ELF");const auto notelf=read_linux_process_executable(root,*identity);assert(notelf.value&&notelf.value->path.value&&!notelf.value->architecture.value);
    assert(fs::remove(root/"exe"));fs::create_symlink(root,root/"exe");assert(!read_linux_process_executable(root,*identity).value);
    assert(fs::remove(root/"exe"));assert(::mkfifo((root/"fifo").c_str(),0600)==0);fs::create_symlink(root/"fifo",root/"exe");assert(!read_linux_process_executable(root,*identity).value);
    assert(fs::remove(root/"exe"));fs::create_symlink("/dev/null",root/"exe");assert(!read_linux_process_executable(root,*identity).value); // pinned but never opened for I/O
    const auto own=read_linux_process_identity("/proc/self");assert(own.value&&own.value->pid==static_cast<std::uint32_t>(::getpid()));const auto native=read_linux_process_status("/proc/self",*own.value);assert(native.value&&native.value->real_uid==static_cast<std::uint32_t>(::getuid())&&native.value->state&&native.value->name);
    const auto account=linux_username_for_uid(::getuid());assert(account.value&&!account.value->empty());
    const auto actual_exe=read_linux_process_executable("/proc/self",*own.value);assert(actual_exe.value&&actual_exe.value->path.value&&actual_exe.value->architecture.value);
    SampleOptions options;options.include_app_history=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;Monitor monitor;const auto sample=monitor.sample(options);const auto it=std::find_if(sample.processes.begin(),sample.processes.end(),[&](const auto& p){return p.pid==own.value->pid;});assert(it!=sample.processes.end()&&it->username==*account.value&&!it->name.empty()&&!it->status.empty()&&it->description.empty());
    assert(it->executable==*actual_exe.value->path.value&&it->architecture==*actual_exe.value->architecture.value);
    std::cout<<"Native PID="<<own.value->pid<<" realUID="<<*native.value->real_uid<<" NSS="<<*account.value<<" status="<<*native.value->state<<" name="<<*native.value->name<<"; pinned architecture="<<it->architecture<<" exe="<<it->executable<<"; Monitor matched; description not fabricated\n";
#else
    assert(!read_linux_process_identity("unused").value);assert(!read_linux_process_status("unused",*identity).value);assert(!linux_username_for_uid(0).value);assert(!read_linux_process_executable("unused",*identity).value);
#endif
    std::cout<<"Process status: bounded strict PID/4 UIDs, duplicate/malformed fields, absence vs UID0, escaped text, identity guard and native files passed\n";
    std::cout<<"Executable: bounded ELF32/64 little/big-endian, missing/invalid architecture, file stamp/path continuity, pinned regular-file checks passed\n";
}
