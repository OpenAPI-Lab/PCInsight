// Manual authorized private-child test. The injected inventory enumerator sees
// ONLY our children; native procfs/pidfds/BPF unchanged. Never capture full /proc.
#include "../src/linux_network_admission.hpp"
#include "../src/linux_network_admission_monitor.hpp"
#include "pcinsight/linux_history_store.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void close_fd(int& fd){if(fd>=0){::close(fd);fd=-1;}}
void byte(int fd,char& c,bool write){ssize_t n;do{n=write?::write(fd,&c,1) : ::read(fd,&c,1);}while(n<0&&errno==EINTR);require(n==1,"private IPC");}
std::size_t descriptors(){auto d=::opendir("/proc/self/fd");require(d,"FD census");std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
struct Child {pid_t pid=-1;int in[2]{-1,-1},out[2]{-1,-1};~Child(){if(pid>0){::kill(pid,SIGKILL);int s;while(::waitpid(pid,&s,0)<0&&errno==EINTR){}}for(int f:{in[0],in[1],out[0],out[1]})if(f>=0)::close(f);}};
void workload(Child& c,unsigned i){
    const int fd=::socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);require(fd>=0,"UDP socket");sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);socklen_t n=sizeof(addr);
    require(::bind(fd,reinterpret_cast<sockaddr*>(&addr),n)==0&&::getsockname(fd,reinterpret_cast<sockaddr*>(&addr),&n)==0,"private bind");
    std::uint64_t cookie=0;n=sizeof(cookie);require(::getsockopt(fd,SOL_SOCKET,SO_NETNS_COOKIE,&cookie,&n)==0&&cookie,"SO cookie oracle initialization");
    char command='R';byte(c.out[1],command,true);
    for(;;){byte(c.in[0],command,false);if(command=='X'){::close(fd);return;}require(command=='G',"private command");std::array<char,128> data{};
        const auto size=64+i;require(::sendto(fd,data.data(),size,0,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))==size&&::recv(fd,data.data(),data.size(),0)==size,"private payload");
        command='D';byte(c.out[1],command,true);}
}
void run(){::alarm(45);::signal(SIGPIPE,SIG_IGN);::signal(SIGCHLD,SIG_DFL);const auto baseline=descriptors();{
    std::array<Child,2> children;for(auto& c:children)require(::pipe2(c.in,O_CLOEXEC)==0&&::pipe2(c.out,O_CLOEXEC)==0,"private pipes");
    for(unsigned i=0;i<2;++i){const auto pid=::fork();require(pid>=0,"private fork");if(!pid){::alarm(40);
            for(unsigned j=0;j<2;++j){::close(children[j].in[1]);::close(children[j].out[0]);if(j!=i){::close(children[j].in[0]);::close(children[j].out[1]);}}
            try{workload(children[i],i);::_exit(0);}catch(...){::_exit(1);}}
        children[i].pid=pid;}
    std::vector<std::uint32_t> pids;
    for(auto& c:children){close_fd(c.in[0]);close_fd(c.out[1]);char ready=0;byte(c.out[0],ready,false);require(ready=='R',"private child readiness");pids.push_back(static_cast<std::uint32_t>(c.pid));}
    const auto all=pids;const auto before=descriptors();auto inventory=native_network_inventory_api();inventory.context=&pids;
    inventory.enumerate=[](void* p,const NetworkSelectionContext&)->Metric<std::vector<std::uint32_t>>{return {*static_cast<std::vector<std::uint32_t>*>(p),Support::available,"TEST private children only; not real complete procfs view"};};
    inventory.inspect=[](void*,std::uint32_t pid)->NetworkInventoryObservation{
        // Synthetic missing row: NEVER open/inspect/capture a guessed external
        // PID, even if a future kernel could allocate this numerical marker.
        if(pid==2147483647U)return {LinuxNetworkInventoryState::disappeared,{{},Support::unsupported,"TEST missing-row marker; no OS process queried"}};
        const auto native=native_network_inventory_api();return native.inspect(native.context,pid);
    };
    LinuxProcessNetworkSampler capture(LinuxProcessNetworkSelectionMode::observed_namespace_buckets);
    LinuxNetworkAdmission admission(inventory,native_network_selection_source_api(),network_admission_capture_api(capture));
    auto refresh=[&]{auto r=admission.refresh(true);require(r.value.has_value(),r.note.c_str());return *r.value;};
    auto take=[&]{auto r=admission.sample();require(r.value.has_value()&&r.support==Support::estimated,r.note.c_str());return *r.value;};
    auto go=[&]{for(auto& c:children){char g='G';byte(c.in[1],g,true);char done=0;byte(c.out[0],done,false);require(done=='D',"workload completion");}};
    auto row=[&](const auto& w,unsigned i)->const LinuxProcessNetworkSample&{auto at=std::find_if(w.processes.begin(),w.processes.end(),[&](const auto& r){return r.process.pid==all[i];});require(at!=w.processes.end(),"admitted row missing");return *at;};
    auto first=refresh();require(first.capture_active&&first.selection_changed&&first.visible_processes==2&&descriptors()==before+18,"admission initial resources");
    require(take().processes.empty(),"admission invented empty traffic");go();const auto warm=take();
    auto unchanged=refresh();require(!unchanged.selection_changed&&unchanged.selection_epoch==first.selection_epoch,"unchanged refresh reset epoch");go();auto measured=take();
    for(unsigned i=0;i<2;++i){const auto& r=row(measured,i);const auto ns=measured.counter_sweep_finished_ns-warm.counter_sweep_finished_ns;
        require(r.payload_bytes_since_start.value==std::array<std::uint64_t,4>{0,0,2*(64+i),2*(64+i)}&&r.nominal_interval_ns.value==ns,"unchanged admission reset origin/baseline");
        for(unsigned lane=2;lane<4;++lane)require(r.payload_bytes_per_second[lane].value==(64+i)*1'000'000'000ULL/ns,"unchanged admission rate oracle");}
    pids={all[1]};auto removed=refresh();require(removed.selection_changed&&removed.selection_epoch!=first.selection_epoch&&descriptors()==before+16,"admission removal resource/epoch");go();
    pids=all;auto added=refresh();require(added.selection_changed&&added.visible_processes==2&&descriptors()==before+26,"admission readd resources");
    auto readded=take();require(readded.processes.size()==1&&row(readded,1).payload_bytes_since_start.value->at(2)==3*65,"admission retained/gap bucket");go();auto fresh=take();
    require(row(fresh,0).payload_bytes_since_start.value->at(2)==64&&!row(fresh,0).nominal_interval_ns.value&&row(fresh,1).payload_bytes_since_start.value->at(2)==4*65,"admission readmission gap/origin oracle");
    const auto copy=::fork();require(copy>=0,"admission fork");if(!copy){const bool denied=!admission.sample().value;admission.stop();::_exit(denied?0:1);}int status=0;
    require(::waitpid(copy,&status,0)==copy&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"admission fork guard");require(take().processes.size()==2,"fork modified parent capture");
    pids.push_back(2147483647U);
    const auto partial=admission.refresh(true,LinuxNetworkAdmissionPolicy::verified_only);
    require(partial.value&&partial.value->capture_active&&!partial.value->selection_changed&&!partial.value->complete_encountered_set&&partial.value->visible_processes==3&&partial.value->verified_processes==2&&partial.value->excluded.size()==1&&partial.value->excluded[0].pid==2147483647U&&descriptors()==before+26,"explicit partial admission lost coverage/retained state");
    go();const auto qualified=admission.sample_with_admission();require(qualified.value&&qualified.support==Support::estimated,"qualified partial sample unavailable");
    require(qualified.value->admission.excluded.size()==1&&qualified.value->admission.selection_epoch==qualified.value->network.selection_epoch&&qualified.value->network.observed_processes.size()==2&&qualified.value->network.processes.size()==2&&
        row(qualified.value->network,0).payload_bytes_since_start.value->at(2)==2*64&&row(qualified.value->network,1).payload_bytes_since_start.value->at(2)==5*65&&row(qualified.value->network,0).nominal_interval_ns.value.has_value(),"partial sample invented excluded row/reset baseline/origin");
    require(!admission.refresh(true).value&&descriptors()==before&&!admission.sample().value,"default strict policy inherited partial/failed to stop ALL");
    pids=all;require(refresh().capture_active&&take().processes.empty(),"explicit admission restart not fresh");pids.clear();auto empty=refresh();require(!empty.capture_active&&!empty.selection_epoch&&descriptors()==before,"known empty admission did not stop");
    VisibleNetworkMonitorApi provider{&admission,
        [](void* p,bool authorized,LinuxNetworkAdmissionPolicy policy){return static_cast<LinuxNetworkAdmission*>(p)->refresh(authorized,policy);},
        [](void* p){return static_cast<LinuxNetworkAdmission*>(p)->sample_with_admission();},
        [](void* p) noexcept {static_cast<LinuxNetworkAdmission*>(p)->stop();}};
    SampleOptions opt;opt.include_processes=opt.include_disks=opt.include_networks=opt.include_gpus=false;opt.include_linux_process_network=true;opt.include_app_history=true;opt.include_linux_user_network=true;
    {pids=all;pids.push_back(2147483647U);auto monitor=MonitorAdmissionAccess::make(provider);
        auto r=monitor.refresh_linux_visible_process_network(true,LinuxNetworkAdmissionPolicy::verified_only);require(r.value&&descriptors()==before+18,"native Monitor admission setup/resources");
        auto zero=monitor.sample(opt);require(zero.linux_network_admission.value&&zero.linux_process_network.value&&zero.linux_process_network.value->processes.empty()&&zero.linux_network_admission.value->excluded.size()==1,"Monitor lost empty actor coverage");
        go();auto one=monitor.sample(opt);const auto epoch=r.value->selection_epoch;
        r=monitor.refresh_linux_visible_process_network(true,LinuxNetworkAdmissionPolicy::verified_only);require(r.value&&!r.value->selection_changed&&r.value->selection_epoch==epoch,"Monitor unchanged admission reset epoch");
        go();auto two=monitor.sample(opt);require(two.linux_network_admission.value&&two.linux_process_network.value&&two.linux_network_admission.value->selection_epoch==two.linux_process_network.value->selection_epoch,"Monitor provenance/network not paired");
        for(unsigned i=0;i<2;++i){const auto& n=row(*two.linux_process_network.value,i);const auto ns=two.linux_process_network.value->counter_sweep_finished_ns-one.linux_process_network.value->counter_sweep_finished_ns;
            require(n.payload_bytes_since_start.value->at(2)==2*(64+i)&&n.nominal_interval_ns.value==ns&&n.payload_bytes_per_second[2].value==(64+i)*1'000'000'000ULL/ns,"Monitor admission nominal rate oracle");}
        require(two.user_sessions.empty()&&two.linux_user_network.value&&two.linux_user_network.value->users.size()==1,"native UID report incorrectly requires login sessions");
        const auto& uid=two.linux_user_network.value->users.front();const auto ns=two.linux_process_network.value->counter_sweep_finished_ns-one.linux_process_network.value->counter_sweep_finished_ns;
        const auto expected=64*1'000'000'000ULL/ns+65*1'000'000'000ULL/ns;
        require(uid.real_uid==::getuid()&&uid.selected_processes==2&&uid.observed_buckets==2&&uid.idle_processes==0&&uid.initial_real_uids.size()==1,"native observer UID identity/selection coverage");
        for(unsigned lane=0;lane<4;++lane)require(uid.known[lane]==2&&!uid.missing[lane]&&!uid.overflow[lane]&&uid.partial_payload_bytes_per_second[lane].value==(lane<2?0:expected),"native observer UID lane oracle");
        require(uid.partial_send_bytes_per_second.value==expected&&uid.partial_receive_bytes_per_second.value==expected&&uid.partial_total_bytes_per_second.value==2*expected,"native observer UID send/receive/total oracle");
        require(two.linux_user_network.value->observer==two.linux_process_network.value->observer&&two.linux_user_network.value->selection_epoch==two.linux_process_network.value->selection_epoch,"native UID/F03 metadata mismatch");
        require(zero.linux_user_network.value&&zero.linux_user_network.value->users.front().idle_processes==2&&!zero.linux_user_network.value->users.front().partial_total_bytes_per_second.value,"native idle actors invented UID zero");
        require(one.linux_user_network.value&&!one.linux_user_network.value->users.front().partial_total_bytes_per_second.value,"native first bucket invented UID rate");
        const auto replay=summarize_linux_user_network(two.linux_process_network);require(replay.value&&replay.value->users.front().partial_total_bytes_per_second.value==2*expected,"native UID projection replay doubled rate");
        std::cout<<"native unique observer UID="<<uid.real_uid<<" processes=2 buckets=2 send="<<expected<<" receive="<<expected<<" total="<<2*expected<<" B/s; exact rounded-bucket oracle, no sessions required\n";
        require(zero.linux_network_app_history.value&&zero.linux_network_app_history.value->empty(),"new actors invented app network baseline");
        require(one.linux_network_app_history.value&&one.linux_network_app_history.value->size()==1&&!one.linux_network_app_history.value->front().network_bytes.value,"app network first baseline invented bytes");
        require(two.linux_network_app_history.value&&two.linux_network_app_history.value->size()==1&&two.linux_network_app_history.value->front().network_bytes.value==258,"app network executable delta oracle (two private children)");
        const auto& history=two.linux_network_app_history.value->front();require(!history.observed_cpu_time.value&&!history.notifications.value,"app network report invented other counters");
        char pattern[]="/tmp/pcinsight-network-history-XXXXXX";const auto directory=::mkdtemp(pattern);require(directory,"private history directory");
        const auto file=std::filesystem::path(directory)/"network.bin";
        struct Cleanup {std::filesystem::path file,dir;~Cleanup(){::unlink(file.c_str());::rmdir(dir.c_str());}} cleanup{file,directory};
        require(static_cast<bool>(LinuxHistoryStore::save(file,*two.linux_network_app_history.value)),"real network history save");
        auto loaded=LinuxHistoryStore::load(file);require(loaded&&loaded.entries.size()==1&&loaded.entries[0].network_bytes.value==258,"real network history roundtrip");
        for(unsigned repeat=0;repeat<50;++repeat)loaded.entries=LinuxHistoryStore::merge(loaded.entries,*two.linux_network_app_history.value);
        require(loaded.entries[0].network_bytes.value==258,"real network history repeated snapshots doubled");
        auto moved=std::move(monitor);go();auto third=moved.sample(opt);require(third.linux_network_admission.value&&third.linux_network_app_history.value&&third.linux_network_app_history.value->front().network_bytes.value==516,"Monitor move lost history or double counted");
        auto merged=LinuxHistoryStore::merge(loaded.entries,*third.linux_network_app_history.value);require(merged[0].network_bytes.value==516,"real history cumulative merge");
        require(static_cast<bool>(LinuxHistoryStore::save(file,merged))&&LinuxHistoryStore::load(file).entries.front().network_bytes.value==516,"real updated history roundtrip");
        std::cout<<"native app network history:258 ->516 bytes;50 replays unchanged; explicit file roundtrip passed\n";
        auto off=opt;off.include_linux_process_network=false;auto stopped=moved.sample(off);require(!stopped.linux_network_admission.value&&!stopped.linux_user_network.value&&descriptors()==before,"Monitor optout retained admission FDs/user report");require(!moved.sample(opt).linux_network_admission.value,"Monitor flag rearmed admission");
    }
    {pids=all;pids.push_back(2147483647U);auto worker=SamplerAdmissionAccess::make(MonitorAdmissionAccess::make(provider),std::chrono::milliseconds(100),128,opt);
        auto r=worker.start_linux_visible_process_network(true,LinuxNetworkAdmissionPolicy::verified_only);require(r.value&&descriptors()==before+18,"worker admission setup/resources");std::uint64_t epoch=r.value->selection_epoch;worker.start();
        auto wait=[&](auto accept){const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(6);while(std::chrono::steady_clock::now()<deadline){auto w=worker.latest();if(w&&accept(*w))return *w;require(worker.running(),worker.last_error().c_str());std::this_thread::sleep_for(std::chrono::milliseconds(10));}throw std::runtime_error("private admitted worker timeout");};
        auto matches=[&](const SystemSnapshot& s){return s.linux_network_admission.value&&s.linux_process_network.value&&s.linux_network_admission.value->selection_epoch==epoch&&s.linux_process_network.value->selection_epoch==epoch;};
        wait([&](const auto& s){return matches(s)&&s.linux_process_network.value->processes.empty();});go();
        wait([&](const auto& s){return matches(s)&&s.linux_process_network.value->processes.size()==2&&row(*s.linux_process_network.value,0).payload_bytes_since_start.value->at(2)==64;});
        Metric<LinuxNetworkAdmissionChange> unchanged;std::jthread updater([&]{unchanged=worker.refresh_linux_visible_process_network(true,LinuxNetworkAdmissionPolicy::verified_only);});(void)worker.latest();(void)worker.timed_history();updater.join();require(unchanged.value&&!unchanged.value->selection_changed&&unchanged.value->selection_epoch==epoch,"worker unchanged concurrent refresh reset epoch");
        pids={all[1],2147483647U};r=worker.refresh_linux_visible_process_network(true,LinuxNetworkAdmissionPolicy::verified_only);require(r.value&&r.value->selection_changed,"worker removal admission");epoch=r.value->selection_epoch;
        auto retained=wait([&](const auto& s){return matches(s)&&s.linux_process_network.value->observed_processes.size()==1;});require(row(*retained.linux_process_network.value,1).payload_bytes_since_start.value->at(2)==65,"worker removal reset retained origin");go();
        pids=all;pids.push_back(2147483647U);r=worker.refresh_linux_visible_process_network(true,LinuxNetworkAdmissionPolicy::verified_only);require(r.value.has_value(),"worker readmission failed");epoch=r.value->selection_epoch;
        auto rejoin=wait([&](const auto& s){return matches(s)&&s.linux_process_network.value->observed_processes.size()==2;});require(rejoin.linux_process_network.value->processes.size()==1&&row(*rejoin.linux_process_network.value,1).payload_bytes_since_start.value->at(2)==2*65,"worker readmission included gap/reset origin");go();
        auto measured=wait([&](const auto& s){return matches(s)&&s.linux_process_network.value->processes.size()==2&&row(*s.linux_process_network.value,0).payload_bytes_since_start.value->at(2)==64;});require(measured.linux_network_admission.value->excluded.size()==1,"worker history lost exclusion");
        for(const auto& item:worker.timed_history()){const auto& s=item.snapshot;if(s.linux_process_network.value){require(s.linux_network_admission.value&&s.linux_network_admission.value->selection_epoch==s.linux_process_network.value->selection_epoch,"worker history mixed epochs/coverage");
                require(s.linux_user_network.value&&s.linux_user_network.value->selection_epoch==s.linux_process_network.value->selection_epoch&&s.linux_user_network.value->users.size()==1&&s.linux_user_network.value->users.front().selected_processes==s.linux_process_network.value->observed_processes.size(),"worker UID history mixed epochs or duplicated processes");}
            else require(!s.linux_user_network.value,"worker published user network without F03 source");}
        require(!worker.refresh_linux_visible_process_network(true).value,"worker default inherited partial policy");wait([](const auto& s){return !s.linux_network_admission.value&&!s.linux_process_network.value;});require(worker.running(),"admission failure stopped worker");worker.stop();require(descriptors()==before,"worker admission stop leaked FDs");
        worker.start();wait([](const auto& s){return !s.linux_network_admission.value&&!s.linux_process_network.value;});worker.stop();require(worker.last_error().empty()&&descriptors()==before,"worker restart rearmed admission");
    }
    for(auto& c:children){char x='X';byte(c.in[1],x,true);require(::waitpid(c.pid,&status,0)==c.pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"private child exit");c.pid=-1;}
    admission.stop();
}require(descriptors()==baseline,"admission harness FD leak");::alarm(0);
    std::cout<<"PASS admission policies AND Monitor/Sampler integration over PRIVATE injected inventory: native procfs/pidfds/PUBLIC shared BPF; AB->unchangedAB->B->AB, unchanged epoch/rate baseline, retained origin/readd fresh excluding gap, resources18->16->26; Monitor SystemSnapshot paired epoch/coverage, UDP nominal rate oracle, live move/optout no rearm; Sampler running/concurrent refresh-history, changed epochs paired with exclusions, retained origin/readmission excludes gap, default strict failure closes both reports while worker continues, stop/restart no rearm; fork rejected-parent intact; standalone strict/partial/empty/restart; no FD leak/all children normal exit. Per-object Monitor/Sampler injection restricts enumerator to2private children plus synthetic missing marker(no external PID inspected), same production lifecycle routes; NOT validation/authorization of capture across actual full /proc view\n";
}
}
int main(int argc,char** argv){if(argc!=2||std::string_view(argv[1])!="--authorized-private-admission"){std::cerr<<"NO capture: manual private authorization required\n";return 77;}try{run();return 0;}catch(const std::exception& e){std::cerr<<"FAIL private admission: "<<e.what()<<'\n';return 1;}}
