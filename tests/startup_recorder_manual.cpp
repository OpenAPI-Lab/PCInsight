#include "../src/startup_recorder.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace pcinsight;using namespace pcinsight::detail;using namespace std::chrono_literals;namespace fs=std::filesystem;
struct Source {
    unsigned starts{},finishes{},closes{},contexts{},scans{};bool active{},context_bad{},bad_id{},throw_finish{},estimated{};
    std::string context="private-context",id,entry;
    auto now()const{return time;}
    std::chrono::steady_clock::time_point time=std::chrono::steady_clock::time_point{}+1s;
    LinuxDesktopResourceReport frame(){
        LinuxDesktopResourceReport f;auto& c=f.births;c.capture_id=bad_id?"wrong":id;c.boot={1,2};c.state=StartupBirthObserverState::closed;c.pid_namespace_device=4;c.pid_namespace_inode=5;c.pid_mapping_enabled=c.final_resources_enabled=c.first_exec_enabled=true;
        StartupBirthObservation row;row.key={c.capture_id,c.boot,100,0,10};row.birth_monotonic_ns=1000;row.final_monotonic_ns=2000;row.local_pid={StartupBirthPid{4,5,8},Support::available,"PID"};row.cpu_time={11ns,Support::available,"CPU"};row.read_bytes={0,Support::available,"zero"};row.write_bytes={50,Support::available,"disk"};row.first_executable={StartupBirthObservation::Executable{1500,55,8,1,0},Support::available,"first"};c.observations.push_back(row);
        LinuxDesktopRoute route{entry,{"/bin/fixture",{},"",false}};PrivatePidfdNetworkJoin selected{{100,0,10},{{1,2},{7,42,100,1,2,3,4,5,6,7},{8,42,200,1,2,8,4,5,6,7}}};SelectedExecEvidence e;e.first_exec={StartupBirthObservation::Executable{1600,55,8,1,0},Support::available,"event"};e.pinned_file_matches={true,Support::available,"pin"};f.receipt=project_linux_manual_receipt(route,selected,{55,8,1},8,ActionError::none,{SelectedExecRead{selected,e,true},Support::available,"fixture"});return f;
    }
    StartupRecorderApi api(){StartupRecorderApi a;
        a.context=[this]{++contexts;StartupProcessSample s;s.pid=7;s.linux_start_ticks=100;s.native_context=context;return context_bad?StartupProcessSampleResult{{},ActionError::stale_target,"changed"}:StartupProcessSampleResult{s,ActionError::none,{}};};
        a.now=[this]{return time;};a.wall=[]{return std::chrono::system_clock::time_point{}+1h;};a.snapshot=[this]{++scans;return SystemSnapshot{};};
        a.manual=[this]{return StartupRecorderManualSource{
            [this](const std::string& e,bool launch,bool capture,const std::string& name){assert(launch&&capture);++starts;active=true;id=name;entry=e;return ActionResult{ActionError::none,{},8};},
            [this](auto,const auto&)->Metric<LinuxDesktopResourceReport>{++finishes;if(throw_finish)throw 1;return {frame(),estimated?Support::estimated:Support::available,"owned source"};},
            [this]{++closes;active=false;}};};return a;
    }
};
StartupRecorderManualOptions options(){StartupRecorderManualOptions o;o.capture_id="manual";o.window=1000ns;o.policy.medium_cpu=1ns;o.policy.high_cpu=10ns;o.policy.medium_disk_bytes=1;o.policy.high_disk_bytes=50;return o;}
int main(){
    Source s;auto recorder=StartupRecorderAccess::make(s.api());const std::string entry="/private/a.desktop";
    assert(StartupRecorderAccess::start_manual(recorder,entry,false,true,options()).error==ActionError::permission_denied&&s.contexts==0&&s.starts==0);
    auto invalid=options();invalid.window=0ns;assert(!StartupRecorderAccess::start_manual(recorder,entry,true,true,invalid)&&s.contexts==0);
    assert(StartupRecorderAccess::start_manual(recorder,entry,true,true,options()).process_id==8&&s.starts==1&&s.active&&s.scans==0);
    assert(!StartupRecorderAccess::start_manual(recorder,entry,true,true,options())&&!StartupRecorderAccess::save_manual(recorder,{})&&!StartupRecorderAccess::manual_archive(recorder).value);
    assert(recorder.poll()&&s.finishes==0&&s.scans==0);assert(recorder.watch_births(true,1,1,1,{}).error==ActionError::not_supported);
    assert(StartupRecorderAccess::finish_manual(recorder)&&s.finishes==1&&s.closes==1&&!s.active);
    const auto report=recorder.report();assert(report.state==StartupRecorderState::closed&&report.scans==0&&report.observations.empty()&&report.birth_impacts.empty()&&report.birth_capture&&!report.birth_capture->observations[0].startup_entry_id.value&&!report.automatic_startup_origin.value);
    const auto archive=StartupRecorderAccess::manual_archive(recorder);assert(archive.value&&archive.value->receipt.value->origin==LinuxDesktopLaunchOrigin::manual_request&&archive.value->births.window==1000ns);
    const auto m=measure_linux_manual_launch_resources(archive.value->receipt,archive.value->births.capture,archive.value->births.window,archive.value->births.policy);assert(m.value&&m.value->measurement.cpu_time.value==11ns&&m.value->measurement.disk_bytes.value==50&&m.value->measurement.impact.value==StartupImpact::high);
    recorder.close();assert(recorder.poll()&&s.finishes==1&&s.closes==1&&!StartupRecorderAccess::finish_manual(recorder));assert(recorder.save({}).error==ActionError::not_supported);
#ifdef __linux__
    const auto parent=fs::path("/tmp");
#else
    const auto parent=fs::current_path();
#endif
    const auto root=parent/("pcinsight-recorder-manual-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));assert(fs::create_directory(root));struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};const auto path=root/"manual";
    assert(StartupRecorderAccess::save_manual(recorder,path)&&load_linux_manual_launch(path));s.context="changed";assert(StartupRecorderAccess::save_manual(recorder,path).error==ActionError::stale_target);s.context="private-context";
    {std::ofstream file(root/"corrupt");file<<"keep";}assert(!StartupRecorderAccess::save_manual(recorder,root/"corrupt"));{std::ifstream file(root/"corrupt");std::string text;file>>text;assert(text=="keep");}
    {auto moved=std::move(recorder);assert(StartupRecorderAccess::manual_archive(moved).value&&!StartupRecorderAccess::manual_archive(recorder).value&&StartupRecorderAccess::save_manual(moved,path));}
    for(unsigned mode=0;mode<7;++mode){Source f;auto c=StartupRecorderAccess::make(f.api());assert(StartupRecorderAccess::start_manual(c,entry,true,true,options()));
        if(mode==0){c.close();assert(!f.active&&f.finishes==0&&!StartupRecorderAccess::manual_archive(c).value);continue;}
        if(mode==1){f.time=c.report().deadline;assert(c.poll()&&f.finishes==1&&StartupRecorderAccess::manual_archive(c).value);continue;}
        if(mode==2)f.context_bad=true;if(mode==3)f.bad_id=true;if(mode==4)f.throw_finish=true;if(mode==5)f.time-=1s;if(mode==6)f.estimated=true;
        assert(!StartupRecorderAccess::finish_manual(c)&&!f.active&&c.report().state==StartupRecorderState::failed&&!StartupRecorderAccess::manual_archive(c).value&&!StartupRecorderAccess::save_manual(c,path));
    }
    {Source f;{auto c=StartupRecorderAccess::make(f.api());assert(StartupRecorderAccess::start_manual(c,entry,true,true,options()));auto moved=std::move(c);assert(moved.poll()&&f.active);}assert(!f.active&&f.closes==1&&f.finishes==0);}
    std::cout<<"Manual recorder start/finish/deadline/abandon/move/context/owned source/canonical archive/safe explicit save passed; no scans/probes/automatic origin\n";
}
