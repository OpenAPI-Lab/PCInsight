// MANUAL private creator/children only; no CTest/global observation.
#include "../src/linux_startup_birth.hpp"
#include "../src/linux_process_details.hpp"
#include "../src/startup_birth_observer.hpp"
#include "../src/startup_recorder.hpp"
#include "pcinsight/startup_birth_observer.hpp"
#include "pcinsight/startup_birth_impact.hpp"
#include "pcinsight/startup_birth_overlay.hpp"
#include "pcinsight/startup_executable_matches.hpp"
#include "startup_match_files.hpp"
#include <cerrno>
#include <algorithm>
#include <csignal>
#include <dirent.h>
#include <fcntl.h>
#include <sched.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <fstream>
#include <atomic>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <unistd.h>
using namespace pcinsight;using namespace pcinsight::detail;
using namespace std::chrono_literals;
namespace {
void check(bool v,const char* n){if(!v)throw std::runtime_error(n);}
void transfer(int fd,void* p,std::size_t size,bool write){auto c=static_cast<char*>(p);while(size){const auto n=write?::write(fd,c,size) : ::read(fd,c,size);if(n<0&&errno==EINTR)continue;check(n>0,"private IPC");c+=n;size-=static_cast<std::size_t>(n);}}
std::size_t fds(){auto d=::opendir("/proc/self/fd");check(d,"fd census");std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
struct Guard {pid_t root=-1;int go[2]{-1,-1},out[2]{-1,-1},pidfd=-1;~Guard(){if(root>0){
    // If a failing status check already reaped the creator, its bare PID could
    // be reused. Prefer the still-owned pidfd for cleanup, never that new task.
    if(pidfd>=0)::syscall(SYS_pidfd_send_signal,pidfd,SIGKILL,nullptr,0);
    else ::kill(root,SIGKILL); // Only pre-pidfd setup: our child is not reaped yet.
    while(::waitpid(root,nullptr,0)<0&&errno==EINTR){}
}for(auto f:{go[0],go[1],out[0],out[1],pidfd})if(f>=0)::close(f);}};
struct Oracle {std::uint64_t cpu{},read{},write{},exe_inode{};std::uint32_t self_pid{},exe_major{},exe_minor{};};
struct Completed {std::array<std::uint32_t,2> children{};std::array<Oracle,2> oracles{};};
std::uint64_t cpu(){timespec t{};check(::clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&t)==0,"CPU oracle");return static_cast<std::uint64_t>(t.tv_sec)*1000000000ULL+static_cast<std::uint64_t>(t.tv_nsec);}
void work(unsigned n,bool disk){const auto until=cpu()+10000000ULL*(n+1);while(cpu()<until)std::atomic_signal_fence(std::memory_order_seq_cst);
    if(disk){char name[]="/tmp/pcinsight-child-final-XXXXXX";const auto fd=::mkstemp(name);check(fd>=0,"private IO file");check(::unlink(name)==0,"private unlink");std::array<char,65536> data{};for(unsigned k=0;k<=n;++k)transfer(fd,data.data(),data.size(),true);check(::fdatasync(fd)==0,"private sync");::close(fd);}}
Oracle oracle(bool executable=false){Oracle o;if(executable){const auto fd=::open("/proc/self/exe",O_PATH|O_CLOEXEC);struct stat st{};check(fd>=0&&::fstat(fd,&st)==0,"private exe oracle");::close(fd);o.exe_inode=st.st_ino;o.exe_major=major(st.st_dev);o.exe_minor=minor(st.st_dev);}
    std::ifstream in("/proc/self/io");std::string key;std::uint64_t count{};bool r=false,w=false;while(in>>key>>count){if(key=="read_bytes:"){o.read=count;r=true;}if(key=="write_bytes:"){o.write=count;w=true;}}check(r&&w,"IO oracle");o.self_pid=static_cast<std::uint32_t>(::getpid());o.cpu=cpu();return o;}
[[noreturn]] void exec_self(unsigned n,unsigned stage,int output,int image_fd=-1){check(::fcntl(output,F_SETFD,0)==0,"private exec oracle fd inheritance");const auto f=std::to_string(output),index=std::to_string(n),step=std::to_string(stage);
    if(image_fd>=0){char name[]="pcinsight-private-exec",flag[]="--private-exec";char* args[]{name,flag,const_cast<char*>(f.c_str()),const_cast<char*>(index.c_str()),const_cast<char*>(step.c_str()),nullptr};::syscall(SYS_execveat,image_fd,"",args,::environ,AT_EMPTY_PATH);}
    else ::execl("/proc/self/exe","pcinsight-private-exec","--private-exec",f.c_str(),index.c_str(),step.c_str(),nullptr);std::cerr<<"private exec errno="<<errno<<'\n';::_exit(124);}
void child_work(unsigned n,unsigned mode,int output){
    if(mode==7||mode==9)exec_self(n,mode==9?1U:0U,output);
    if(mode==10){const auto image=::open("/proc/self/exe",O_PATH|O_CLOEXEC);check(image>=0,"pin private image before leader exit");std::thread([=]{exec_self(n,0,output,image);}).detach();::syscall(SYS_exit,0);::_exit(123);}
    if(mode==8){::execl("/proc/self/pcinsight-no-such-executable","pcinsight-missing",nullptr);check(errno==ENOENT,"private failed exec fixture");}
    if(mode==3){const auto child=::fork();check(child>=0,"waited child");if(!child){work(n,true);::_exit(0);}int status{};check(::waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"waited child reaping");}
    work(n,mode!=5);auto o=oracle(mode==11);transfer(output,&o,sizeof(o),true);::_exit(7);
}
void run(unsigned capacity,unsigned mode=0){::alarm(25);::signal(SIGCHLD,SIG_DFL);::signal(SIGPIPE,SIG_IGN);const auto before=fds();{
    Guard g;check(::pipe2(g.go,O_CLOEXEC)==0&&::pipe2(g.out,O_CLOEXEC)==0,"pipes");g.root=::fork();check(g.root>=0,"creator fork");
    if(!g.root){::alarm(20);::close(g.go[1]);::close(g.out[0]);char go{};transfer(g.go[0],&go,1,false);
        // This private thread must NOT consume a process-group birth slot.
        std::thread t([]{});t.join();Completed completed;if(mode==4)::signal(SIGCHLD,SIG_IGN);
        for(unsigned n=0;n<(mode==6?1U:2U);++n){int result[2];check(::pipe2(result,O_CLOEXEC)==0,"child oracle pipe");
            // Private creator only: change namespace-for-children, never mount
            // procfs or alter any existing process's active PID namespace.
            if(mode==6&&::unshare(CLONE_NEWPID)<0){std::cerr<<"private child namespace errno="<<errno<<'\n';::_exit(125);}
            const auto child=::fork();if(child<0)::_exit(120);
            if(!child){::close(result[0]);if(mode==2){std::thread([=]{const auto end=std::chrono::steady_clock::now()+3s;bool dead=false;while(std::chrono::steady_clock::now()<end){std::ifstream f("/proc/self/stat");std::string line;std::getline(f,line);const auto at=line.rfind(')');if(at!=line.npos&&at+2<line.size()&&line[at+2]=='Z'){dead=true;break;}std::this_thread::sleep_for(1ms);}if(!dead)::_exit(122);child_work(n,mode,result[1]);}).detach();::syscall(SYS_exit,0);::_exit(123);}child_work(n,mode,result[1]);}
            ::close(result[1]);completed.children[n]=static_cast<std::uint32_t>(child);
            try{transfer(result[0],&completed.oracles[n],sizeof(Oracle),false);}catch(...){int state{};const auto waited=::waitpid(child,&state,0);std::cerr<<"private oracle EOF mode="<<mode<<" child="<<child<<" waited="<<waited<<" status="<<state<<" errno="<<errno<<'\n';::_exit(126);}::close(result[0]);
            if(mode==4){const auto end=std::chrono::steady_clock::now()+3s;bool gone=false;while(std::chrono::steady_clock::now()<end){if(::kill(child,0)<0&&errno==ESRCH){gone=true;break;}std::this_thread::sleep_for(1ms);}if(!gone)::_exit(124);}
            else {int status{};const auto waited=::waitpid(child,&status,0);if(waited!=child||!WIFEXITED(status)||WEXITSTATUS(status)!=7){std::cerr<<"private child wait mode="<<mode<<" child="<<child<<" returned="<<waited<<" status="<<status<<" errno="<<errno<<'\n';::_exit(121);}}
        }
        // Only NOW, after BOTH children are reaped, are their local PIDs exposed
        // to the observer. It never had a chance to sample/pidfd-select them.
        transfer(g.out[1],&completed,sizeof(completed),true);transfer(g.go[0],&go,1,false);::_exit(0);
    }
    ::close(g.go[0]);g.go[0]=-1;::close(g.out[1]);g.out[1]=-1;
    g.pidfd=static_cast<int>(::syscall(SYS_pidfd_open,g.root,0));check(g.pidfd>=0,"creator pidfd");const auto id=read_linux_process_identity(std::filesystem::path("/proc")/std::to_string(g.root));check(id.value.has_value(),"creator start");
    LinuxStartupBirthCapture capture;const auto started=capture.start(true,g.pidfd,static_cast<std::uint32_t>(g.root),id.value->start_ticks,capacity,mode!=0,mode>=7);check(started.value.has_value(),started.note.c_str());
    // Exact facade/source comparison MUST use the same immutable capture.
    // Two independently attached probes observe different instants at release;
    // their CPU values are not a bit-for-bit passthrough oracle for each other.
    const auto context=native_network_selection_source_api();check(context.observe,"native observer context");
    unsigned shared_starts=0,shared_stops=0;
    auto public_observer=StartupBirthObserverAccess::make({
        [&]{return context.observe(context.context);},
        [&](bool a,int fd,std::uint32_t pid,std::uint64_t ticks,std::uint32_t cap,bool final,bool exec){
            check(a&&fd==g.pidfd&&pid==static_cast<std::uint32_t>(g.root)&&ticks==id.value->start_ticks&&cap==capacity&&final==(mode!=0)&&exec==(mode>=7),"shared native start contract");
            ++shared_starts;return started;},
        [&]{return capture.sample();},[&]{++shared_stops;capture.stop();}});
    const auto public_started=public_observer.start(true,g.pidfd,static_cast<std::uint32_t>(g.root),id.value->start_ticks,{"manual-private-birth",capacity,mode!=0,mode>=7});check(static_cast<bool>(public_started),public_started.message.c_str());
    // Keep genuine default-constructor/native wiring covered too. Its results
    // are checked against the child's independent clock/IO/file oracles below,
    // never against another sensor's CPU snapshot as if it were the same read.
    LinuxStartupBirthObserver independent_observer;
    const auto independent_started=independent_observer.start(true,g.pidfd,static_cast<std::uint32_t>(g.root),id.value->start_ticks,{"manual-independent-birth",capacity,mode!=0,mode>=7});
    check(static_cast<bool>(independent_started),independent_started.message.c_str());
    // Native birth factory under the REAL recorder lifecycle. Legacy scans are
    // deliberately empty/private here, not a root-wide live process census.
    const auto own_pid=static_cast<std::uint32_t>(::getpid());const auto own=read_linux_process_identity("/proc/self");check(own.value.has_value(),"private recorder caller");
    StartupRecorderApi recorder_api;
    recorder_api.context=[=]{if(static_cast<std::uint32_t>(::getpid())!=own_pid)return StartupProcessSampleResult{{},ActionError::stale_target,"forked private recorder"};return read_startup_process_sample(own_pid,own.value->start_ticks);};
    recorder_api.snapshot=[]{return SystemSnapshot{};};recorder_api.candidates=[](const auto&){return StartupCandidateReport{};};recorder_api.sample=read_startup_process_sample;
    recorder_api.now=[]{return std::chrono::steady_clock::now();};recorder_api.wall=[]{return std::chrono::system_clock::now();};
    recorder_api.birth=make_native_startup_recorder_birth_source;
    auto recorder=StartupRecorderAccess::make(std::move(recorder_api));StartupRecorderOptions recorder_options;
    recorder_options.capture_id="manual-private-recorder";recorder_options.accept_estimated_gio_associations=true;recorder_options.scan_interval=60s;
    check(static_cast<bool>(recorder.start(recorder_options)),"private recorder start");
    check(!recorder.watch_births(false,g.pidfd,static_cast<std::uint32_t>(g.root),id.value->start_ticks,{"manual-recorder-birth",capacity,mode!=0,mode>=7}),"recorder birth consent bypass");
    check(static_cast<bool>(recorder.watch_births(true,g.pidfd,static_cast<std::uint32_t>(g.root),id.value->start_ticks,{"manual-recorder-birth",capacity,mode!=0,mode>=7})),"recorder native birth attach");
    auto sample=capture.sample();check(sample.value&&sample.value->births.empty()&&!sample.value->quality_flags,"invented initial birth");
    const auto sentinel=::fork();check(sentinel>=0,"sentinel");if(!sentinel)::_exit(0);int status{};check(::waitpid(sentinel,&status,0)==sentinel,"sentinel wait");sample=capture.sample();check(sample.value&&sample.value->births.empty()&&!sample.value->quality_flags,"unselected creator captured");
    const auto copy=::fork();check(copy>=0,"capture copy fork");if(!copy)::_exit(capture.sample().value?1:0);check(::waitpid(copy,&status,0)==copy&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"inherited capture accepted");
    char go='G';transfer(g.go[1],&go,1,true);Completed completed;transfer(g.out[0],&completed,sizeof(completed),false);
    for(auto pid:completed.children)if(pid)check(!read_linux_process_identity(std::filesystem::path("/proc")/std::to_string(pid)).value,"private child still in procfs");
    sample=capture.sample();check(sample.value.has_value(),sample.note.c_str());check(sample.value->births.size()==(capacity==1||mode==6?1U:2U)&&!sample.value->publishing&&!sample.value->creator_exited,"wrong birth count/status");
    check(static_cast<bool>(public_observer.poll()),"public birth observer poll after reap");
    check(static_cast<bool>(independent_observer.poll()),"independent native observer poll after reap");
    const auto independent_rows=independent_observer.report();
    check(independent_rows.observations.size()==sample.value->births.size()&&!independent_rows.automatic_startup_origin.value,"independent native count/origin");
    check(static_cast<bool>(recorder.poll()),"recorder native birth poll BEFORE legacy scan due");const auto recorder_rows=recorder.report();
    check(recorder_rows.birth_capture&&recorder_rows.birth_capture->observations.size()==sample.value->births.size()&&recorder_rows.birth_impacts.empty()&&recorder_rows.observations.empty(),"recorder native separate/unbound rows");
    for(const auto& row:recorder_rows.birth_capture->observations)check(static_cast<bool>(recorder.associate_birth(row.key,"private-fixture.desktop")),"recorder explicit birth binding");
    const auto recorder_bound=recorder.report();check(recorder_bound.birth_impacts.size()==sample.value->births.size()&&recorder_bound.observations.empty(),"recorder native bound projections/no legacy coercion");
    const auto public_rows=public_observer.report();check(public_rows.observations.size()==sample.value->births.size()&&!public_rows.automatic_startup_origin.value,"public birth count/origin");
    for(const auto& r:public_rows.observations){check(!r.startup_entry_id.value&&r.local_pid.value,"invented association/missing public namespace PID");
        const auto match=std::find_if(sample.value->births.begin(),sample.value->births.end(),[&](const auto& b){return b.kernel.kernel_tgid==r.key.kernel_tgid&&b.kernel.leader_start_boottime_ns==r.key.kernel_start_boottime_ns;});
        check(match!=sample.value->births.end(),"public child identity mismatch");
        if(r.cpu_time.value!=match->cpu_time.value||r.read_bytes.value!=match->read_bytes.value||r.write_bytes.value!=match->write_bytes.value){
            auto number=[](const auto& v){return v?std::to_string(*v):"unknown";};
            std::cerr<<"resource differential mode="<<mode<<" key="<<r.key.kernel_tgid<<" CPU-public="<<(r.cpu_time.value?std::to_string(r.cpu_time.value->count()):"unknown")<<" CPU-private="<<(match->cpu_time.value?std::to_string(match->cpu_time.value->count()):"unknown")<<" read-public="<<number(r.read_bytes.value)<<" read-private="<<number(match->read_bytes.value)<<" write-public="<<number(r.write_bytes.value)<<" write-private="<<number(match->write_bytes.value)<<'\n';
            check(false,"public resource mismatch");
        }
        check(r.first_executable.value.has_value()==match->first_executable.value.has_value(),"public exec availability mismatch");if(r.first_executable.value)check(r.first_executable.value->inode==match->first_executable.value->inode&&r.first_executable.value->device_major==match->first_executable.value->device_major&&r.first_executable.value->device_minor==match->first_executable.value->device_minor,"public first exec file differs");
        if(r.first_executable.value){StartupMatchFiles catalog;const auto image=std::filesystem::read_symlink("/proc/self/exe").string();const auto entry=catalog.desktop("a.desktop","\""+image+"\" --not-executed");
            const auto unique=discover_startup_executable_matches(public_rows,r.key);check(unique.complete&&unique.unique_candidate.value==entry&&unique.unique_candidate.support==Support::estimated&&!unique.automatic_startup_origin.value,"private current-file candidate missing");
            catalog.desktop("b.desktop","\""+image+"\" --other-profile",true);const auto ambiguous=discover_startup_executable_matches(public_rows,r.key);check(ambiguous.complete&&ambiguous.matched==2&&!ambiguous.unique_candidate.value,"shared executable ambiguity hidden");
            std::cout<<"current caller XDG candidate: unique then shared/disabled ambiguous, no binding or origin promotion\n";
        }
        check(static_cast<bool>(public_observer.associate(r.key,"private-fixture.desktop")),"public explicit association");
        check(!public_observer.associate(r.key,"wrong-fixture.desktop"),"public reassignment accepted");
    }
    // Project the retained, explicitly bound rows without any procfs identity,
    // steady-clock invention or process-tree aggregation after child reaping.
    const auto bound_rows=public_observer.report();
    for(const auto& row:bound_rows.observations){
        const auto projected=measure_startup_birth_impact(bound_rows,row.key);
        check(projected.value.has_value()&&projected.value->startup_entry_id.value=="private-fixture.desktop"&&!projected.value->automatic_startup_origin.value,"birth impact projection/binding/origin");
        const auto& impact=*projected.value;
        if(row.final_monotonic_ns){
            check(impact.released_within_window&&impact.cpu_time.value==row.cpu_time.value&&row.read_bytes.value&&row.write_bytes.value&&impact.disk_bytes.value==*row.read_bytes.value+*row.write_bytes.value,"birth impact native counters/window");
            check(!impact.impact.value,"small private workload falsely certified low/medium/high under default thresholds");
            StartupImpactPolicy p;p.medium_cpu=500us;p.high_cpu=1ms;p.high_disk_bytes=std::numeric_limits<std::uint64_t>::max();
            const auto high=measure_startup_birth_impact(bound_rows,row.key,60s,p);
            check(high.value&&high.value->impact.value==StartupImpact::high&&high.value->impact.support==Support::estimated,"custom-threshold native CPU lower-bound impact missing");
        }else check(!impact.released_within_window&&!impact.cpu_time.value&&!impact.disk_bytes.value&&!impact.impact.value,"uncovered native final converted to zero/impact");
        std::cout<<"birth impact projection: retained bound key, window/counters/high-only or unknown verified AFTER reap; no legacy clocks or origin promotion\n";
    }
    check(sample.value->quality_flags==(capacity==1?2U:0U),"capacity loss not reported or thread consumed slot");
    check(sample.support==(capacity==1?Support::estimated:Support::available),"partial quality not surfaced");
    for(const auto& birth:sample.value->births){check(birth.kernel.kernel_tgid!=started.value->kernel.kernel_tgid&&birth.kernel.leader_start_boottime_ns&&birth.monotonic_ns,"birth identity missing");check(birth.scheduler_runtime_at_birth_ns==0&&birth.read_bytes_at_birth==0&&birth.write_bytes_at_birth==0,"birth counters not sampled before first run");}
    check(sample.value->creator_reaped_totals_enabled==(mode!=0),"final mode not reported");
    check(sample.value->first_exec_enabled==(mode>=7)&&public_rows.first_exec_enabled==(mode>=7),"exec mode not reported");
    check(sample.value->pid_mapping_enabled,sample.value->pid_mapping_note.c_str());
    for(std::size_t n=0;n<sample.value->births.size();++n){const auto& record=sample.value->births[n];const auto& o=completed.oracles[n];
        const auto rec=std::find_if(recorder_bound.birth_impacts.begin(),recorder_bound.birth_impacts.end(),[&](const auto& m){return m.key.kernel_tgid==record.kernel.kernel_tgid&&m.key.kernel_start_boottime_ns==record.kernel.leader_start_boottime_ns;});
        check(rec!=recorder_bound.birth_impacts.end()&&rec->key.capture_id=="manual-recorder-birth"&&!rec->automatic_startup_origin.value,"recorder native exact key/origin");
        if(mode==0||mode==4)check(!rec->cpu_time.value&&!rec->disk_bytes.value&&!rec->impact.value,"recorder invented uncovered final");
        else {check(rec->released_within_window&&rec->cpu_time.value&&rec->disk_bytes.value==o.read+o.write,"recorder native window/IO oracle");const auto measured=static_cast<std::uint64_t>(rec->cpu_time.value->count());check(measured+1000000>=o.cpu&&measured<o.cpu+20000000,"recorder native CPU SAME private clock bounds");}
        const auto independent=std::find_if(independent_rows.observations.begin(),independent_rows.observations.end(),[&](const auto& r){return r.key.kernel_tgid==record.kernel.kernel_tgid&&r.key.kernel_start_boottime_ns==record.kernel.leader_start_boottime_ns&&r.key.kernel_real_uid==record.kernel.kernel_real_uid;});
        check(independent!=independent_rows.observations.end()&&independent->local_pid.value&&independent->local_pid.value->tgid==completed.children[n],"independent native identity/PID");
        check(independent->key.boot==public_rows.boot&&independent->key.capture_id=="manual-independent-birth"&&!independent->startup_entry_id.value,"independent native boot/capture/binding");
        check(record.local_pid.value&&record.local_pid.value->pid_namespace_inode==started.value->local.observer.pid_namespace_inode&&record.local_pid.value->tgid==completed.children[n],"birth local PID does not match private post-reap oracle");
        if(mode==7||mode==9||mode==10){check(record.exec_state==1&&record.first_executable.value&&record.first_executable.value->inode==o.exe_inode&&record.first_executable.value->device_major==o.exe_major&&record.first_executable.value->device_minor==o.exe_minor,"first successful exe differs from self fstat oracle");std::cout<<"first exec mode="<<mode<<" inode="<<o.exe_inode<<" dev="<<o.exe_major<<':'<<o.exe_minor<<" verified after reap\n";}
        else check(!record.first_executable.value&&!record.exec_state,"invented successful exec from fork/failed exec");
        check(independent->first_executable.value.has_value()==record.first_executable.value.has_value(),"independent native first exec availability");
        if(independent->first_executable.value)check(independent->first_executable.value->inode==o.exe_inode&&independent->first_executable.value->device_major==o.exe_major&&independent->first_executable.value->device_minor==o.exe_minor,"independent first exec vs private file oracle");
        if(mode==6){check(o.self_pid==1&&record.local_pid.value->tgid!=o.self_pid,"namespace fixture failed to distinguish child/observer PID domains");std::cout<<"PID namespace proof: child self="<<o.self_pid<<" observer="<<record.local_pid.value->tgid<<" kernel="<<record.kernel.kernel_tgid<<" inode="<<record.local_pid.value->pid_namespace_inode<<" AFTER reap\n";}
        if(mode==0||mode==4){check(!record.final_monotonic_ns&&!record.cpu_time.value&&!record.read_bytes.value&&!record.write_bytes.value,"invented final/zero on disabled or auto-reap");
            check(!independent->final_monotonic_ns&&!independent->cpu_time.value&&!independent->read_bytes.value&&!independent->write_bytes.value,"independent native invented final/zero");}
        else {check(record.final_monotonic_ns&&*record.final_monotonic_ns>=record.monotonic_ns&&record.cpu_time.value&&record.read_bytes.value&&record.write_bytes.value,"final counters not retained after reaping");
            const auto measured=static_cast<std::uint64_t>(record.cpu_time.value->count());check(measured+1000000>=o.cpu&&measured<o.cpu+20000000,"final CPU vs independent clock bound");check(*record.read_bytes.value==o.read&&*record.write_bytes.value==o.write,"final IO differs from procfs oracle");
            check(independent->final_monotonic_ns&&*independent->final_monotonic_ns>=independent->birth_monotonic_ns&&independent->cpu_time.value&&independent->read_bytes.value&&independent->write_bytes.value,"independent native missing final counters");
            const auto independent_cpu=static_cast<std::uint64_t>(independent->cpu_time.value->count());
            check(independent_cpu+1000000>=o.cpu&&independent_cpu<o.cpu+20000000,"independent native CPU vs SAME clock bound");
            check(*independent->read_bytes.value==o.read&&*independent->write_bytes.value==o.write,"independent native IO vs exact private procfs oracle");
            if(mode==3)check(o.write>=131072ULL*(n+1),"waited-child IO not included");if(mode==5)check(*record.write_bytes.value==0,"known zero IO not preserved");
            std::cout<<"child final mode="<<mode<<" slot="<<n<<" CPU_ns="<<measured<<" oracle_ns="<<o.cpu<<" read="<<o.read<<" write="<<o.write<<" AFTER reap, never child-selected in userland\n";
        }
    }
    if(sample.value->births.size()>1)check(sample.value->births[0].kernel.kernel_tgid!=sample.value->births[1].kernel.kernel_tgid,"duplicate child identity");
    go='E';transfer(g.go[1],&go,1,true);check(::waitpid(g.root,&status,0)==g.root&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"creator wait");g.root=-1;
    const auto replay=capture.sample();check(replay.value&&replay.value->creator_exited&&replay.value->births.size()==sample.value->births.size()&&replay.value->quality_flags==sample.value->quality_flags,"records lost after creator reap");
    check(static_cast<bool>(recorder.poll()),"recorder native creator-reap replay");recorder.close();const auto recorder_closed=recorder.report();
    check(recorder_closed.state==StartupRecorderState::closed&&recorder_closed.birth_capture&&recorder_closed.birth_capture->state==StartupBirthObserverState::closed&&recorder_closed.birth_capture->creator_exited&&recorder_closed.birth_impacts.size()==recorder_bound.birth_impacts.size(),"recorder native close retention");
    for(std::size_t n=0;n<recorder_bound.birth_impacts.size();++n)check(recorder_closed.birth_impacts[n].key==recorder_bound.birth_impacts[n].key&&recorder_closed.birth_impacts[n].cpu_time.value==recorder_bound.birth_impacts[n].cpu_time.value&&recorder_closed.birth_impacts[n].disk_bytes.value==recorder_bound.birth_impacts[n].disk_bytes.value,"recorder native OWN immutable replay");
    {StartupMatchFiles files;const auto path=files.path/"births.bin";const auto saved=recorder.save(path);check(saved&&saved.replaced,"recorder birth-aware save");
        const auto loaded=StartupImpactStore::load(path);check(loaded&&loaded.archive->births&&loaded.archive->diagnostics&&loaded.archive->observations.empty(),"recorder v3 birth load/no legacy coercion");
        const auto& a=*loaded.archive->births;check(a.window==60s&&a.capture.observations.size()==recorder_closed.birth_impacts.size()&&a.capture.state==StartupBirthObserverState::closed,"recorder v3 metadata/retention");
        for(const auto& old:recorder_closed.birth_impacts){const auto projected=measure_startup_birth_impact(a.capture,old.key,a.window,a.policy);check(projected.value&&projected.value->cpu_time.value==old.cpu_time.value&&projected.value->disk_bytes.value==old.disk_bytes.value&&projected.value->impact.value==old.impact.value&&!projected.value->automatic_startup_origin.value,"recorder v3 reprojection differs");}
        SystemSnapshot view;StartupEntry entry;entry.id="private-fixture.desktop";view.startup_entries.push_back(entry);
        StartupBirthOverlayContext accepted{a.capture.capture_id,a.capture.boot,a.capture.pid_namespace_device,a.capture.pid_namespace_inode,true};
        check(apply_startup_birth_impact(view,a,accepted).applied_entries==1&&!view.startup_entries[0].observed_startup_interval.value&&view.startup_entries[0].impact_scope.find("HISTORICAL")!=std::string::npos,"native loaded birth overlay scope/clock");
        if(a.capture.observations.size()>1)check(!view.startup_entries[0].observed_startup_cpu_time.value&&!view.startup_entries[0].observed_startup_disk_bytes.value,"native overlay aggregated overlapping groups");
        else check(view.startup_entries[0].observed_startup_cpu_time.value==recorder_closed.birth_impacts[0].cpu_time.value&&view.startup_entries[0].observed_startup_disk_bytes.value==recorder_closed.birth_impacts[0].disk_bytes.value,"native single overlay changed counters");
        auto custom=a;custom.policy.medium_cpu=500us;custom.policy.high_cpu=1ms;
        check(static_cast<bool>(apply_startup_birth_impact(view,custom,accepted)),"native custom birth overlay");
        check((view.startup_entries[0].observed_startup_impact.value==StartupImpact::high)==(mode!=0&&mode!=4),"native high-only/unknown birth overlay");
        std::cout<<"native birth overlay: loaded v3/context/explicit binding/historical scope/no summed resources/no legacy interval/high-only verified\n";
        struct stat info{};check(::stat(path.c_str(),&info)==0&&(info.st_mode&0777)==0600,"recorder v3 private permissions");
    }
    std::cout<<"recorder native births: rows="<<recorder_closed.birth_impacts.size()<<" captured before child launch; post-reap exact bindings/private CPU-IO oracles/separate windows/close/replay/v3 save-load-reprojection verified\n";
    check(static_cast<bool>(public_observer.poll()),"public replay after creator reap");public_observer.close();const auto closed_public=public_observer.report();
    check(static_cast<bool>(independent_observer.poll()),"independent native replay after creator reap");independent_observer.close();
    const auto independent_closed=independent_observer.report();check(independent_closed.state==StartupBirthObserverState::closed&&independent_closed.creator_exited&&independent_closed.observations.size()==independent_rows.observations.size(),"independent native close/reap retention");
    for(std::size_t n=0;n<independent_rows.observations.size();++n){const auto& a=independent_rows.observations[n];const auto& b=independent_closed.observations[n];
        check(a.key==b.key&&a.cpu_time.value==b.cpu_time.value&&a.read_bytes.value==b.read_bytes.value&&a.write_bytes.value==b.write_bytes.value&&a.first_executable.value==b.first_executable.value,"independent native OWN immutable replay changed");}
    check(shared_starts==1&&shared_stops==1,"shared native lifecycle was not exactly once");
    check(closed_public.state==StartupBirthObserverState::closed&&closed_public.creator_exited&&closed_public.observations.size()==public_rows.observations.size(),"public close lost report");
    for(const auto& r:closed_public.observations)check(r.startup_entry_id.value=="private-fixture.desktop","public replay lost binding");
    std::cout<<"public birth observer: rows="<<closed_public.observations.size()<<" exact bindings/resources/replay/close verified; autostart origin unknown\n";
    std::cout<<"independent native observer: rows="<<independent_rows.observations.size()<<" private CPU bounds/exact IO/first exec/OWN replay verified; not cross-sensor CPU equality\n";
    for(std::size_t n=0;n<sample.value->births.size();++n)check(replay.value->births[n].monotonic_ns==sample.value->births[n].monotonic_ns&&replay.value->births[n].final_monotonic_ns==sample.value->births[n].final_monotonic_ns&&replay.value->births[n].cpu_time.value==sample.value->births[n].cpu_time.value&&replay.value->births[n].local_pid.value==sample.value->births[n].local_pid.value,"immutable birth/final/PID changed");
    capture.stop();check(!capture.sample().value,"stopped birth capture active");
    std::cout<<"private selected creator: capacity="<<capacity<<" mode="<<mode<<" children reaped BEFORE any child PID read; births="<<sample.value->births.size()<<" quality="<<sample.value->quality_flags<<" thread ignored/sentinel ignored/fork denied/creator reaped/replay retained; namespace-qualified local birth PIDs verified; no automatic-startup claim\n";
}check(fds()==before,"birth descriptors leaked");::alarm(0);}
}
int main(int argc,char** argv){
    if(argc==5&&std::string_view(argv[1])=="--private-exec"){try{const auto fd=std::stoi(argv[2]),n=std::stoi(argv[3]),stage=std::stoi(argv[4]);check(fd>=3&&fd<1024&&n>=0&&n<2&&stage>=0&&stage<=1,"private helper args");if(stage)exec_self(static_cast<unsigned>(n),0,fd);child_work(static_cast<unsigned>(n),11,fd);}catch(...){return 127;}}
    if(argc!=2||(std::string_view(argv[1])!="--authorized-private-birth"&&std::string_view(argv[1])!="--authorized-private-birth-pidns"&&std::string_view(argv[1])!="--authorized-private-birth-exec")){std::cerr<<"Explicit manual private-process authorization required\n";return 77;}
    try{if(std::string_view(argv[1])=="--authorized-private-birth-pidns"){run(4,6);return 0;}if(std::string_view(argv[1])=="--authorized-private-birth-exec"){run(4,7);run(4,8);run(4,9);run(4,10);return 0;}run(4);run(1);run(4,1);run(4,2);run(4,3);run(4,4);run(4,5);run(1,1);run(4,6);run(4,7);run(4,8);run(4,9);run(4,10);return 0;}catch(const std::exception& e){std::cerr<<"FAIL birth: "<<e.what()<<'\n';return 1;}}
