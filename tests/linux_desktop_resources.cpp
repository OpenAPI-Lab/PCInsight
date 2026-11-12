#include "../src/linux_desktop_resources.hpp"
#include "../src/linux_manual_store.hpp"
#include "../src/startup_recorder.hpp"
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#endif
using namespace pcinsight;using namespace pcinsight::detail;using namespace std::chrono_literals;
#ifdef __linux__
namespace fs=std::filesystem;
struct Env{std::map<std::string,std::optional<std::string>> saved;void set(const char* k,const std::string& v){const auto old=::getenv(k);saved.emplace(k,old?std::optional<std::string>(old):std::nullopt);assert(::setenv(k,v.c_str(),1)==0);}~Env(){for(const auto& [k,v]:saved){if(v)::setenv(k.c_str(),v->c_str(),1);else ::unsetenv(k.c_str());}}};
void write(const fs::path& p,const std::string& text){std::ofstream f(p);assert(f);f<<text;f.close();assert(f);}
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
std::int64_t cpu(){timespec t{};assert(::clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&t)==0);return t.tv_sec*1000000000LL+t.tv_nsec;}
#endif
int main(int argc,char** argv){
#ifdef __linux__
    if(argc==3&&std::string_view(argv[1])=="--child"){const int fd=::open(argv[2],O_CREAT|O_EXCL|O_WRONLY,0600);if(fd<0)return 120;char data[4096]{};for(unsigned n=0;n<256;++n)assert(::write(fd,data,sizeof data)==sizeof data);assert(::fsync(fd)==0);::close(fd);const auto begin=cpu();while(cpu()-begin<20000000){}return 7;}
    if(argc==5&&std::string_view(argv[1])=="--wait-child"){const int input=std::stoi(argv[3]),output=std::stoi(argv[4]);::close(output);write(argv[2],"ready\n");char c{};const auto n=::read(input,&c,1);::close(input);return n==1&&c=='G'?7:121;}
#endif
    LinuxDesktopRoute route{"/private/a.desktop",{"/bin/fixture",{},"",false}};
    PrivatePidfdNetworkJoin selected{{100,0,10},{{1,2},{7,42,100,1,2,3,4,5,6,7},{8,42,200,1,2,8,4,5,6,7}}};
    SelectedExecFile pin{55,8,1};SelectedExecEvidence evidence;
    evidence.first_exec={StartupBirthObservation::Executable{1600,55,8,1,0},Support::available,"receipt event"};evidence.pinned_file_matches={true,Support::available,"pin"};
    const Metric<SelectedExecRead> read{SelectedExecRead{selected,evidence,true},Support::available,"native source fixture"};
    auto receipt=project_linux_manual_receipt(route,selected,pin,8,ActionError::none,read);assert(receipt.value);
    StartupBirthObserverReport report;report.capture_id="resources";report.boot={1,2};report.state=StartupBirthObserverState::closed;report.pid_mapping_enabled=report.final_resources_enabled=report.first_exec_enabled=true;report.pid_namespace_device=4;report.pid_namespace_inode=5;
    StartupBirthObservation row;row.key={report.capture_id,report.boot,100,0,10};row.birth_monotonic_ns=1000;row.final_monotonic_ns=2000;row.local_pid={StartupBirthPid{4,5,8},Support::available,"native PID"};row.cpu_time={11ns,Support::available,"CPU"};row.read_bytes={20,Support::available,"read"};row.write_bytes={30,Support::available,"write"};row.first_executable={StartupBirthObservation::Executable{1500,55,8,1,0},Support::available,"different sensor timestamp"};report.observations.push_back(row);
    StartupImpactPolicy policy;policy.medium_cpu=1ns;policy.high_cpu=10ns;policy.medium_disk_bytes=1;policy.high_disk_bytes=50;
    auto join=[&](const auto& r){return measure_linux_manual_launch_resources(receipt,r,1000ns,policy);};
    const auto measured=join(report);if(!measured.value)std::cerr<<measured.note<<'\n';assert(measured.value&&measured.support==Support::estimated&&measured.value->origin==LinuxDesktopLaunchOrigin::manual_request&&measured.value->controlled_entry_link.value==true);
    const auto& m=measured.value->measurement;assert(m.cpu_time.value==11ns&&m.disk_bytes.value==50&&m.impact.value==StartupImpact::high&&!m.automatic_startup_origin.value&&m.startup_entry_id.value==route.entry_id);
    assert(!report.observations[0].startup_entry_id.value&&report.observations[0].first_executable.value->observed_monotonic_ns==1500); // No mutation/no bitwise cross-sensor time comparison.
    auto alive=report;auto& pending=alive.observations[0];pending.final_monotonic_ns.reset();pending.cpu_time={};pending.read_bytes={};pending.write_bytes={};const auto partial=join(alive);assert(partial.value&&!partial.value->measurement.cpu_time.value&&!partial.value->measurement.disk_bytes.value&&!partial.value->measurement.released_within_window);
    auto late=report;late.observations[0].final_monotonic_ns=2001;assert(join(late).value&&!join(late).value->measurement.cpu_time.value);
    for(unsigned n=0;n<14;++n){auto bad=report;auto& r=bad.observations[0];switch(n){case 0:++bad.boot[0];break;case 1:++r.key.kernel_tgid;break;case 2:++r.key.kernel_real_uid;break;case 3:++r.key.kernel_start_boottime_ns;break;case 4:++r.local_pid.value->tgid;break;case 5:++bad.pid_namespace_inode;break;case 6:++r.first_executable.value->inode;break;case 7:r.first_executable.value->observed_monotonic_ns=999;break;case 8:bad.quality_flags=1;break;case 9:bad.observations.push_back(r);break;case 10:r.startup_entry_id={"/other.desktop",Support::estimated,"conflict"};break;case 11:r.first_executable={};break;case 12:bad.state=StartupBirthObserverState::recording;break;case 13:r.read_bytes.support=Support::unsupported;break;}assert(!join(bad).value);}
    auto saved=receipt;receipt.value->origin=static_cast<LinuxDesktopLaunchOrigin>(9);assert(!join(report).value);receipt=saved;receipt.value->entry_first_exec_matches.value=false;assert(!join(report).value);receipt=saved;
    LinuxDesktopResourceSession denied;assert(denied.start("/never/read.desktop",false,true,"x").error==ActionError::permission_denied&&denied.start("/never/read.desktop",true,false,"x").error==ActionError::permission_denied&&!denied.finish().value);
#ifdef __linux__
    const bool recorder_mode=argc==2&&std::string_view(argv[1])=="--authorized-private-recorder-store";
    const bool store=recorder_mode||(argc==2&&std::string_view(argv[1])=="--authorized-private-resources-store");
    const bool native=store||(argc==2&&std::string_view(argv[1])=="--authorized-private-resources");assert(argc==1||native);if(!native){std::cout<<"Pure manual-resource join/identity/time/quality/unknown/no-mutation checks passed; NO probes loaded\n";return 0;}::alarm(45);const auto initial=fds();
    char pattern[]="/tmp/pcinsight-desktop-resources-XXXXXX";const auto d=::mkdtemp(pattern);assert(d);const fs::path root=d;struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};Env env;
    fs::create_directories(root/"config/autostart");fs::create_directories(root/"system/autostart");env.set("XDG_CONFIG_HOME",(root/"config").string());env.set("XDG_CONFIG_DIRS",(root/"system").string());env.set("XDG_CURRENT_DESKTOP","GNOME");
    const auto image=fs::read_symlink("/proc/self/exe").string();const auto entry=root/"config/autostart/a.desktop";const auto output=root/"output";
    for(unsigned mode=0;mode<4;++mode){auto executable=image;const auto cwd=mode==1?root/"missing":root;int gate[2]{-1,-1};
        if(mode==2){const auto bad=root/"bad-elf";write(bad,"\177ELFbroken");assert(::chmod(bad.c_str(),0700)==0);executable=bad.string();}
        std::string args=" --child \""+output.string()+"\"";if(mode==3){assert(::pipe(gate)==0);args=" --wait-child \""+output.string()+"\" "+std::to_string(gate[0])+" "+std::to_string(gate[1]);}
        write(entry,"[Desktop Entry]\nType=Application\nName=Private resource\nExec=\""+executable+"\""+args+"\nPath="+cwd.string()+"\n");
        LinuxDesktopResourceSession session;LinuxStartupRecorder recorder;StartupRecorderManualOptions recorder_options;recorder_options.capture_id="private-resource-"+std::to_string(mode);
        const auto launched=recorder_mode?StartupRecorderAccess::start_manual(recorder,entry.string(),true,true,recorder_options):session.start(entry.string(),true,true,recorder_options.capture_id);int status{};
        if(mode==0){if(!launched)std::cerr<<launched.message<<'\n';assert(launched&&launched.process_id);assert(::waitpid(static_cast<pid_t>(launched.process_id),&status,0)==static_cast<pid_t>(launched.process_id)&&WIFEXITED(status)&&WEXITSTATUS(status)==7&&fs::file_size(output)==1048576);}
        else if(mode==3){assert(launched&&launched.process_id);::close(gate[0]);for(unsigned i=0;i<200&&!fs::exists(output);++i)::usleep(10000);assert(fs::exists(output));}
        else assert(!launched&&!launched.process_id&&!fs::exists(output));
        Metric<LinuxDesktopResourceReport> finished;
        if(recorder_mode){const auto done=StartupRecorderAccess::finish_manual(recorder);if(!done)std::cerr<<done.message<<'\n';assert(done);const auto archive=StartupRecorderAccess::manual_archive(recorder);assert(archive.value);const auto& a=*archive.value;
            finished={LinuxDesktopResourceReport{a.receipt,a.births.capture,measure_linux_manual_launch_resources(a.receipt,a.births.capture,a.births.window,a.births.policy)},Support::available,"recorder manual archive"};
            const auto public_view=recorder.report();assert(public_view.state==StartupRecorderState::closed&&public_view.scans==0&&public_view.observations.empty()&&public_view.birth_impacts.empty()&&!public_view.automatic_startup_origin.value);assert(recorder.save(root/"must-not-write").error==ActionError::not_supported);
            const auto saved=StartupRecorderAccess::save_manual(recorder,root/"recorder.evidence");if(!saved)std::cerr<<saved.message<<'\n';assert(saved&&load_linux_manual_launch(root/"recorder.evidence"));std::cout<<"Owned recorder manual finish/save mode"<<mode<<": no GIO scan/legacy impact mixing\n";
        }else finished=session.finish();
        if(!finished.value)std::cerr<<finished.note<<'\n';assert(finished.value);const auto& f=*finished.value;assert(f.births.state==StartupBirthObserverState::closed&&f.births.observations.size()==1&&!f.births.observations[0].startup_entry_id.value);
        if(store){const auto path=root/"manual.evidence";LinuxManualLaunchArchive archive{{f.births,60s,{}},f.receipt};const auto saved=save_linux_manual_launch(path,archive);if(!saved)std::cerr<<saved.message<<'\n';assert(saved&&saved.replaced);const auto loaded=load_linux_manual_launch(path);if(!loaded)std::cerr<<loaded.message<<'\n';assert(loaded&&loaded.archive->receipt.value&&loaded.archive->receipt.value->origin==LinuxDesktopLaunchOrigin::manual_request&&!loaded.archive->births.capture.observations[0].startup_entry_id.value);
            const auto replay=measure_linux_manual_launch_resources(loaded.archive->receipt,loaded.archive->births.capture,loaded.archive->births.window,loaded.archive->births.policy);assert(replay.value.has_value()==f.resources.value.has_value());if(replay.value)assert(replay.value->measurement.cpu_time.value==f.resources.value->measurement.cpu_time.value&&replay.value->measurement.disk_bytes.value==f.resources.value->measurement.disk_bytes.value&&!replay.value->measurement.automatic_startup_origin.value);
            struct stat info{};assert(::stat(path.c_str(),&info)==0&&(info.st_mode&0777)==0600);std::cout<<"Historical manual envelope roundtrip mode"<<mode<<"/0600/reprojection/rawunbound passed; NO live authority restored\n";
        }
        if(mode==0||mode==3){if(!f.resources.value)std::cerr<<f.resources.note<<'\n';assert(f.receipt.value&&f.resources.value&&f.resources.value->controlled_entry_link.value==true&&!f.resources.value->measurement.automatic_startup_origin.value);
            const auto& measured=f.resources.value->measurement;const auto& original=f.births.observations[0];
            if(mode==0){assert(measured.released_within_window&&measured.cpu_time.value==original.cpu_time.value&&measured.cpu_time.value->count()>0&&measured.disk_bytes.value&&*measured.disk_bytes.value>=1048576&&*measured.disk_bytes.value==*original.read_bytes.value+*original.write_bytes.value);std::cout<<"Manual resource positive AFTER reap: CPU="<<measured.cpu_time.value->count()<<"ns disk="<<*measured.disk_bytes.value<<"B\n";}
            else {assert(!measured.released_within_window&&!measured.cpu_time.value&&!measured.disk_bytes.value);assert(::waitpid(static_cast<pid_t>(launched.process_id),&status,WNOHANG)==0);assert(::write(gate[1],"G",1)==1);::close(gate[1]);assert(::waitpid(static_cast<pid_t>(launched.process_id),&status,0)==static_cast<pid_t>(launched.process_id)&&WIFEXITED(status)&&WEXITSTATUS(status)==7);std::cout<<"Finish while alive: resource UNKNOWN, NO hidden kill/reap, caller released/waited\n";}
        }else assert(!f.resources.value&&f.receipt.value&&!f.receipt.value->entry_first_exec_matches.value);
        if(recorder_mode)assert(!StartupRecorderAccess::finish_manual(recorder)&&!StartupRecorderAccess::start_manual(recorder,entry.string(),true,true,recorder_options));
        else assert(!session.finish().value&&session.start(entry.string(),true,true,"again").error==ActionError::stale_target);
        assert(fds()==initial);if(fs::exists(output))fs::remove(output);std::cout<<"Private resource mode"<<mode<<" passed; raw rows unbound/FD and probe cleanup exact\n";
    }::alarm(0);
#else
    (void)argc;(void)argv;assert(denied.start("/tmp/a.desktop",true,true,"x").error==ActionError::not_supported);
#endif
}
