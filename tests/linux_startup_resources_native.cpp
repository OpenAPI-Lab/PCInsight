// Manual, authorized private-process test ONLY. Observer is NOT target's parent.
#include "../src/linux_startup_resources.hpp"
#include "../src/linux_process_details.hpp"
#include "../src/startup_recorder.hpp"
#include <cerrno>
#include <csignal>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <array>
#include <atomic>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
using namespace pcinsight;using namespace pcinsight::detail;using namespace std::chrono_literals;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void transfer(int fd,void* data,std::size_t count,bool write){auto p=static_cast<char*>(data);while(count){const auto n=write?::write(fd,p,count) : ::read(fd,p,count);if(n<0&&errno==EINTR)continue;check(n>0,"private IPC");p+=n;count-=static_cast<std::size_t>(n);}}
std::size_t descriptors(){auto d=::opendir("/proc/self/fd");check(d,"FD census");std::size_t n=0;while(const auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
struct Guard {pid_t parent=-1;int go[2]{-1,-1},out[2]{-1,-1},pidfd=-1;~Guard(){if(go[1]>=0){char c='x';const auto sent=::write(go[1],&c,1);(void)sent;}if(parent>0){::kill(parent,SIGKILL);while(::waitpid(parent,nullptr,0)<0&&errno==EINTR){}}for(int f:{go[0],go[1],out[0],out[1],pidfd})if(f>=0)::close(f);}};
struct Oracle {std::uint64_t cpu{},read{},write{};};
std::uint64_t cpu(){timespec t{};check(::clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&t)==0,"process CPU oracle");return static_cast<std::uint64_t>(t.tv_sec)*1000000000ULL+static_cast<std::uint64_t>(t.tv_nsec);}
void workload(){
    const auto until=cpu()+40000000;while(cpu()<until){std::atomic_signal_fence(std::memory_order_seq_cst);}
    char path[]="/tmp/pcinsight-resource-XXXXXX";const auto fd=::mkstemp(path);check(fd>=0,"private IO file");check(::unlink(path)==0,"private unlink");
    std::array<char,262144> data{};transfer(fd,data.data(),data.size(),true);check(::fdatasync(fd)==0,"private IO sync");::close(fd);
}
Oracle oracle(){Oracle v;std::ifstream in("/proc/self/io");std::string key;std::uint64_t n{};bool r=false,w=false;while(in>>key>>n){if(key=="read_bytes:"){v.read=n;r=true;}if(key=="write_bytes:"){v.write=n;w=true;}}check(r&&w,"private IO oracle");v.cpu=cpu();return v;}
void run(unsigned mode){const bool threaded=mode==1||mode==4;::alarm(25);::signal(SIGPIPE,SIG_IGN);::signal(SIGCHLD,SIG_DFL);const auto base=descriptors();{
    Guard g;check(::pipe2(g.go,O_CLOEXEC)==0&&::pipe2(g.out,O_CLOEXEC)==0,"private pipes");
    g.parent=::fork();check(g.parent>=0,"owner fork");if(!g.parent){::alarm(20);::close(g.go[1]);::close(g.out[0]);
        if(mode==4)::signal(SIGCHLD,SIG_IGN);
        const auto target=::fork();if(target<0)::_exit(120);if(!target){
            // Grandchild is private, blocked until observer attaches. It exits
            // without any second user-space sample and is reaped by its owner.
            char command=0;transfer(g.go[0],&command,1,false);
            if(threaded){check(command=='L',"leader exit command");const auto input=g.go[0],output=g.out[1];
                std::thread([input,output]{
                    const auto deadline=std::chrono::steady_clock::now()+3s;bool zombie=false;
                    while(std::chrono::steady_clock::now()<deadline){std::ifstream file("/proc/self/stat");std::string stat;std::getline(file,stat);const auto end=stat.rfind(')');
                        if(end!=stat.npos&&end+2<stat.size()&&stat[end+2]=='Z'){zombie=true;break;}std::this_thread::sleep_for(1ms);}
                    if(!zombie)::_exit(116);char ready='T';transfer(output,&ready,1,true);char finish=0;transfer(input,&finish,1,false);workload();auto measured=oracle();transfer(output,&measured,sizeof(measured),true);::_exit(finish=='G'?7:117);
                }).detach();
                // Linux thread-only exit, deliberately no C++ stack unwind or
                // exit_group: worker remains live while the leader is zombie.
                ::syscall(SYS_exit,0);::_exit(118);
            }
            if(mode==2){std::thread worker(workload);worker.join();}
            if(mode==3){const auto child=::fork();check(child>=0,"IO-accounted child fork");if(!child){workload();::_exit(0);}int child_status{};check(::waitpid(child,&child_status,0)==child&&WIFEXITED(child_status)&&WEXITSTATUS(child_status)==0,"IO-accounted child wait");}
            if(mode==5)for(unsigned n=0;n<15;++n)workload();
            if(mode==6){const auto until=cpu()+1200000000;while(cpu()<until)std::atomic_signal_fence(std::memory_order_seq_cst);}
            workload();auto measured=oracle();transfer(g.out[1],&measured,sizeof(measured),true);::_exit(command=='G'?7:119);}
        auto pid=static_cast<std::uint32_t>(target);transfer(g.out[1],&pid,sizeof(pid),true);int status=0;
        if(mode==4){
            const auto deadline=std::chrono::steady_clock::now()+10s;bool gone=false;
            while(std::chrono::steady_clock::now()<deadline){if(::kill(target,0)<0&&errno==ESRCH){gone=true;break;}std::this_thread::sleep_for(1ms);}
            char ack=gone?'R':'F';transfer(g.out[1],&ack,1,true);::_exit(gone?0:123);
        }
        siginfo_t zombie{};while(::waitid(P_PID,target,&zombie,WEXITED|WNOWAIT)<0){if(errno!=EINTR)::_exit(121);}
        char ready='Z';transfer(g.out[1],&ready,1,true);char reap=0;transfer(g.go[0],&reap,1,false);if(reap!='R')::_exit(122);
        while(::waitpid(target,&status,0)<0&&errno==EINTR){};char ack=WIFEXITED(status)&&WEXITSTATUS(status)==7?'R':'F';transfer(g.out[1],&ack,1,true);::_exit(ack=='R'?0:1);}
    ::close(g.go[0]);g.go[0]=-1;::close(g.out[1]);g.out[1]=-1;
    std::uint32_t pid=0;transfer(g.out[0],&pid,sizeof(pid),false);
    g.pidfd=static_cast<int>(::syscall(SYS_pidfd_open,pid,0));check(g.pidfd>=0,"private pidfd");
    const auto stat=read_linux_process_identity(std::filesystem::path("/proc")/std::to_string(pid));check(stat.value.has_value(),"private stat baseline");
    siginfo_t info{};errno=0;check(::waitid(P_PIDFD,g.pidfd,&info,WEXITED|WNOHANG|WNOWAIT)<0&&errno==ECHILD,"observer unexpectedly owns target child");
    LinuxStartupResourceCapture capture;auto started=capture.start(true,g.pidfd,pid,stat.value->start_ticks);check(started.value.has_value(),started.note.c_str());
    auto read=capture.sample();check(read.value&&!read.value->resources&&!read.value->pidfd_exited,"invented initial exit");
    // Explicit private census seam, NOT an automatic-startup association proof.
    // The real native disk provider, recorder, observer and store are exercised.
    const auto baseline=read_startup_process_sample(pid,stat.value->start_ticks);check(static_cast<bool>(baseline),"recorder baseline");
    const auto self=read_linux_process_identity("/proc/self");check(self.value.has_value(),"recorder own identity");
    StartupRecorderApi api;
    api.context=[own=static_cast<std::uint32_t>(::getpid()),ticks=self.value->start_ticks]{return read_startup_process_sample(own,ticks);};
    api.snapshot=[pid,ticks=stat.value->start_ticks]{SystemSnapshot s;ProcessInfo p;p.pid=pid;p.linux_start_ticks={ticks,Support::available,"private"};s.processes.push_back(p);return s;};
    api.candidates=[base=*baseline.value](const SystemSnapshot& s){StartupCandidateReport r;r.complete=true;r.examined=s.processes.size();if(!s.processes.empty()){StartupProcessCandidate c;c.startup_entry_id="private-explicit-association";c.baseline=base;r.candidates.push_back(c);}return r;};
    api.sample=read_startup_process_sample;api.now=[]{return std::chrono::steady_clock::now();};api.wall=[]{return std::chrono::system_clock::now();};api.exit_disk=make_native_startup_exit_disk_reader;
    std::optional<std::chrono::nanoseconds> scheduler_baseline;
    api.scheduler_sample=[&](auto selected,auto ticks){auto s=read_startup_scheduler_process_sample(selected,ticks);if(!scheduler_baseline&&s&&metric_is_known(s.value->cumulative_cpu_time))scheduler_baseline=s.value->cumulative_cpu_time.value;return s;};
    auto recorder=StartupRecorderAccess::make(std::move(api));StartupRecorderOptions options;options.capture_id="private-disk-tail";options.duration=15s;options.scan_interval=10ms;options.accept_estimated_gio_associations=true;options.enable_privileged_exit_disk_recovery=true;
    check(static_cast<bool>(recorder.start(options)),"recorder selected exit start");check(recorder.report().exit_disk_attached==1,"recorder native disk provider did not attach");
    // Unselected private process exits: must leave selected map untouched.
    const auto sentinel=::fork();check(sentinel>=0,"sentinel fork");if(!sentinel)::_exit(0);int status=0;check(::waitpid(sentinel,&status,0)==sentinel,"sentinel wait");
    read=capture.sample();check(read.value&&!read.value->resources&&!read.value->pidfd_exited,"captured unselected exit");
    const auto copy=::fork();check(copy>=0,"capture fork check");if(!copy){const bool rejected=!capture.sample().value;capture.stop();::_exit(rejected?0:1);}check(::waitpid(copy,&status,0)==copy&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"forked capture accepted");
    char command='G';
    if(threaded){command='L';transfer(g.go[1],&command,1,true);char ready=0;transfer(g.out[0],&ready,1,false);check(ready=='T',"worker did not confirm leader zombie");
        read=capture.sample();check(read.value&&!read.value->resources&&!read.value->pidfd_exited,"leader exit incorrectly marked whole group exited");command='G';}
    transfer(g.go[1],&command,1,true);Oracle measured;transfer(g.out[0],&measured,sizeof(measured),false);char ack=0;transfer(g.out[0],&ack,1,false);
    if(mode!=4){check(ack=='Z',"owner did not retain zombie");
        read=capture.sample();check(read.value&&read.value->pidfd_exited&&!read.value->resources,"pidfd exit invented early totals or stopped capture");
        command='R';transfer(g.go[1],&command,1,true);transfer(g.out[0],&ack,1,false);
    }
    check(ack=='R',"owner did not reap target");
    // Parent ACK proves reaping has happened BEFORE observer reads the event.
    check(!read_linux_process_identity(std::filesystem::path("/proc")/std::to_string(pid)).value,"private target still in procfs after reap");
    read=capture.sample();check(read.value&&read.value->pidfd_exited,read.note.c_str());
    check(static_cast<bool>(recorder.poll()),"recorder post-reap poll");auto recovered=recorder.report();
    check(recovered.observations.size()==1&&!recovered.automatic_startup_origin.value,"recorder attribution contract");
    if(mode==4)check(!recovered.exit_disk_recovered&&!recovered.observations.front().closed&&recovered.observations.front().sampling_gap,"recorder invented final disk on uncovered exit");
    else{
        const auto& row=recovered.observations.front();check(recovered.exit_disk_recovered==1&&row.closed&&row.sampling_gap&&!row.window_complete&&row.measurement.has_value(),"recorder did not retain partial recovered disk");
        check(row.measurement->disk_bytes.value==measured.read+measured.write-*baseline.value->cumulative_disk_bytes.value,"recorder disk recovery delta mismatch/double count");
        check(scheduler_baseline&&read.value->resources&&row.measurement->cpu_time.value&&
            row.measurement->cpu_time.value->count()==static_cast<std::int64_t>(read.value->resources->scheduler_runtime_ns)-scheduler_baseline->count()&&
            row.measurement->cpu_time.note.find("Recovered final scheduler-runtime")!=std::string::npos,"recorder scheduler CPU baseline/final delta mismatch");
        check(mode==5||mode==6?row.measurement->impact.value==StartupImpact::high:!row.measurement->impact.value,"recorder partial classification");
        if(mode==6)check(*row.measurement->cpu_time.value>1s&&*row.measurement->disk_bytes.value<300000,"CPU-only high impact not measured");
    }
    recorder.close();
    char directory[]="/tmp/pcinsight-disk-tail-XXXXXX";check(::mkdtemp(directory),"private archive directory");
    struct Archive {std::filesystem::path dir,file;~Archive(){::unlink(file.c_str());::rmdir(dir.c_str());}} archive{directory,std::filesystem::path(directory)/"capture.bin"};
    check(static_cast<bool>(recorder.save(archive.file)),"recovered archive save");const auto loaded=StartupImpactStore::load(archive.file);
    check(loaded&&loaded.archive->observations.size()==1&&loaded.archive->observations.front().sampling_gap&&!loaded.archive->observations.front().window_complete,"partial flags lost in archive");
    check(loaded.archive->diagnostics.has_value(),"disk recovery diagnostics absent");const auto& diagnostics=*loaded.archive->diagnostics;
    check(diagnostics.exit_disk_enabled&&diagnostics.exit_disk_attempted==1&&diagnostics.exit_disk_attached==1&&diagnostics.exit_disk_recovered==recovered.exit_disk_recovered&&diagnostics.exit_disk_released_without_final==(mode==4?1U:0U)&&diagnostics.registered==1&&!diagnostics.automatic_startup_origin.value,"disk recovery diagnostics inconsistent after roundtrip");
    check(loaded.archive->observations.front().measurement->disk_bytes.value==recorder.report().observations.front().measurement->disk_bytes.value,"archive disk counter changed");
    if(mode!=4)check(loaded.archive->observations.front().measurement->cpu_time.note.find("Recovered final scheduler-runtime")!=std::string::npos&&loaded.archive->observations.front().measurement->cpu_time.value==recovered.observations.front().measurement->cpu_time.value,"archive lost recovered CPU counter/source");
    struct stat permissions{};check(::stat(archive.file.c_str(),&permissions)==0&&(permissions.st_mode&0777)==0600,"archive permissions");
    std::cout<<"recorder private mode="<<mode<<" recovered="<<recovered.exit_disk_recovered<<" CPU_ns="<<(recovered.observations.front().measurement->cpu_time.value?recovered.observations.front().measurement->cpu_time.value->count():-1)<<" same-kind scheduler baseline/final; partial/archive0600; no autostart cause certified\n";
    if(mode==4){
        check(!read.value->resources,"unexpected internal-repeat coverage; reassess kernel before changing contract");
        auto replay=capture.sample();check(replay.value&&replay.value->pidfd_exited&&!replay.value->resources,"missed release turned into invented totals");
        capture.stop();check(::waitpid(g.parent,&status,0)==g.parent&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"auto-reap owner exit");g.parent=-1;
        std::cout<<"native private foreign PID="<<pid<<" mode=4 auto-reaped leader-first: pidfd exited/procfs gone, resources UNKNOWN (internal release repeat not captured), no invented zero\n";
    }else{
    check(read.value->resources.has_value(),read.note.c_str());
    const auto event=*read.value->resources;check(event.identity.kernel_tgid==started.value->kernel.kernel_tgid&&event.identity.kernel_real_uid==started.value->kernel.kernel_real_uid&&event.identity.leader_start_boottime_ns==started.value->kernel.leader_start_boottime_ns&&event.monotonic_ns,"captured identity/time mismatch");
    check(event.scheduler_runtime_ns+1000000>=measured.cpu&&event.scheduler_runtime_ns<measured.cpu+10000000,"scheduler runtime vs independent CPU clock bound");
    check(event.group_accounted_read_bytes==measured.read&&event.group_accounted_write_bytes==measured.write&&measured.write>=262144,"final group IO vs procfs oracle");
    if(mode==2||mode==3||mode==5)check(measured.write>=524288,"released worker/waited child IO missing");
    if(mode==2)check(event.scheduler_runtime_ns>=80000000,"released worker CPU missing");
    const auto again=capture.sample();check(again.value&&again.value->resources&&again.value->resources->monotonic_ns==event.monotonic_ns&&again.value->pidfd_exited,"exit replay changed event");
    capture.stop();check(!capture.sample().value,"stopped capture still reads event");
    check(::waitpid(g.parent,&status,0)==g.parent&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"owner child exit");g.parent=-1;
    std::cout<<"native private foreign PID="<<pid<<" initial TGID="<<event.identity.kernel_tgid<<" mode="<<mode<<" direct leader-release monotonic_ns="<<event.monotonic_ns<<" CPU scheduler_ns="<<event.scheduler_runtime_ns<<" pre-exit clock_ns="<<measured.cpu<<" read="<<event.group_accounted_read_bytes<<" write="<<event.group_accounted_write_bytes<<" exact procfs IO; unknown before reap, retained AFTER owner reap\n";
    }
}check(descriptors()==base,"selected exit FD leak");::alarm(0);}
}
int main(int argc,char** argv){if(argc!=2||std::string_view(argv[1])!="--authorized-private-resources"){std::cerr<<"No capture without manual private authorization\n";return 77;}try{run(0);run(1);run(2);run(3);run(4);run(5);run(6);return 0;}catch(const std::exception& e){std::cerr<<"FAIL selected resources: "<<e.what()<<'\n';return 1;}}
