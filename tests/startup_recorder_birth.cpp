#include "../src/startup_recorder.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;using namespace pcinsight::detail;using namespace std::chrono_literals;
struct Fixture {
    std::chrono::steady_clock::time_point now=std::chrono::steady_clock::time_point{}+1s;
    StartupBirthObserverReport frame;unsigned factories{},polls{},reports{},closes{},active{},scans{};
    bool deny{},throw_factory{},throw_report{},fail_poll{},slow{},context_bad{};
    Fixture(){frame.capture_id="birth";frame.boot={1,2};frame.pid_namespace_device=4;frame.pid_namespace_inode=99;frame.pid_mapping_enabled=true;frame.final_resources_enabled=true;frame.state=StartupBirthObserverState::recording;}
    StartupProcessSample sample(){StartupProcessSample s;s.pid=21;s.linux_start_ticks=4;s.native_context=context_bad?"changed":"legacy-context";s.sampled_at=now;s.cumulative_cpu_time={0ns,Support::estimated,"CPU"};s.cumulative_disk_bytes={0,Support::estimated,"IO"};return s;}
    StartupBirthProcessKey birth(bool bound=false){StartupBirthObservation o;o.key={"birth",{1,2},200,0,111};o.birth_monotonic_ns=1000;o.local_pid={StartupBirthPid{4,99,21},Support::available,"local"};if(bound)o.startup_entry_id={"app.desktop",Support::estimated,"declared"};frame.observations.push_back(o);return o.key;}
    void final(){auto& o=frame.observations.at(0);o.final_monotonic_ns=2000;o.cpu_time={11ns,Support::available,"CPU"};o.read_bytes={20,Support::available,"read"};o.write_bytes={30,Support::available,"write"};}
    StartupRecorderApi api(){StartupRecorderApi a;
        a.context=[this]{return StartupProcessSampleResult{sample(),ActionError::none,{}};};
        a.snapshot=[this]{++scans;return SystemSnapshot{};};
        a.candidates=[this](const auto&){StartupCandidateReport r;if(scans==1){StartupProcessCandidate c;c.startup_entry_id="app.desktop";c.baseline=sample();r.candidates.push_back(c);}return r;};
        a.sample=[this](auto,auto){now+=1ns;return StartupProcessSampleResult{sample(),ActionError::none,{}};};
        a.now=[this]{return now;};a.wall=[]{return std::chrono::system_clock::time_point{}+1h;};
        a.birth=[this](bool auth,int fd,std::uint32_t pid,std::uint64_t ticks,const StartupBirthObserverOptions& o)->StartupRecorderBirthResult{
            assert(auth&&fd==3&&pid==10&&ticks==20&&o.capture_id=="birth"&&o.capacity==4&&o.recover_final_resources);++factories;
            if(throw_factory)throw 1;if(deny)return {{},{ActionError::permission_denied,"denied"}};
            ++active;
            return {StartupRecorderBirthSource{
                [this]{++polls;if(slow)now+=1s;if(fail_poll){frame.state=StartupBirthObserverState::failed;frame.error=ActionError::system_error;return ActionResult{ActionError::system_error,"source failed"};}return ActionResult{};},
                [this]{++reports;if(throw_report)throw 2;return frame;},
                [this](const auto& key,const std::string& id){for(auto& o:frame.observations)if(o.key==key){if(o.startup_entry_id.value&&o.startup_entry_id.value!=id)return ActionResult{ActionError::stale_target,"conflict"};o.startup_entry_id={id,Support::estimated,"declared"};return ActionResult{};}return ActionResult{ActionError::stale_target,"missing"};},
                [this]{++closes;assert(active==1);--active;if(frame.state==StartupBirthObserverState::recording)frame.state=StartupBirthObserverState::closed;}}, {}};
        };return a;
    }
};
StartupRecorderOptions options(){StartupRecorderOptions o;o.capture_id="recorder";o.accept_estimated_gio_associations=true;o.duration=100ms;o.scan_interval=20ms;return o;}
StartupBirthObserverOptions birth_options(){return {"birth",4,true,false};}
int main(){
    {Fixture f;auto r=StartupRecorderAccess::make(f.api());assert(!r.watch_births(false,3,10,20,birth_options())&&!f.factories);assert(r.start(options()));
        const auto legacy=r.report().observations;assert(r.watch_births(true,3,10,20,birth_options())&&f.active==1&&f.factories==1);
        assert(!r.watch_births(true,3,10,20,birth_options())&&f.factories==1);
        const auto key=f.birth();assert(r.poll()&&f.polls==1&&f.scans==1);assert(r.report().birth_capture->observations.size()==1&&r.report().birth_impacts.empty());
        assert(r.associate_birth(key,"app.desktop"));auto view=r.report();assert(view.birth_impacts.size()==1&&!view.birth_impacts[0].cpu_time.value);
        assert(!r.associate_birth(key,"wrong.desktop"));f.final();assert(r.poll()&&f.polls==2);
        view=r.report();assert(view.birth_impacts.size()==1&&view.birth_impacts[0].disk_bytes.value==50&&view.birth_impacts[0].cpu_time.value==11ns&&!view.birth_impacts[0].impact.value&&!view.birth_impacts[0].automatic_startup_origin.value);
        assert(view.observations.size()==1&&view.observations[0].measurement->disk_bytes.value==legacy[0].measurement->disk_bytes.value); // SAME app/local PID, still no sum/coercion.
        const auto reads=f.polls;r.close();assert(!f.active&&f.closes==1&&f.polls==reads&&r.report().birth_capture->state==StartupBirthObserverState::closed);
        assert(r.associate_birth(key,"app.desktop")&&f.polls==reads);assert(!r.save({}));assert(r.poll()&&f.polls==reads);
        auto moved=std::move(r);assert(moved.report().birth_impacts.size()==1&&!r.associate_birth(key,"app.desktop"));moved.close();assert(f.closes==1);
    }
    for(unsigned mode=0;mode<9;++mode){Fixture f;{auto r=StartupRecorderAccess::make(f.api());assert(r.start(options()));const auto key=f.birth(true);f.final();assert(r.watch_births(true,3,10,20,birth_options()));
        if(mode==0)f.fail_poll=true;if(mode==1)f.throw_report=true;if(mode==2)++f.frame.boot[0];if(mode==3)f.frame.observations[0].cpu_time.value=12ns;
        if(mode==4)f.frame.observations.clear();if(mode==5)f.context_bad=true;if(mode==6)f.frame.quality_flags=128;
        if(mode==7){f.now=r.report().deadline;assert(r.poll());assert(r.report().state==StartupRecorderState::closed);}
        else if(mode<7){assert(!r.poll()&&r.report().state==StartupRecorderState::failed);assert(r.report().birth_impacts.size()==1&&r.report().birth_impacts[0].cpu_time.value==11ns);}
        else {auto moved=std::move(r);assert(moved.report().birth_capture);}
        (void)key;}assert(!f.active&&f.closes==1);
    }
    for(unsigned mode=0;mode<5;++mode){Fixture f;auto r=StartupRecorderAccess::make(f.api());assert(r.start(options()));
        if(mode==0)f.deny=true;if(mode==1)f.throw_factory=true;if(mode==2)f.throw_report=true;
        if(mode==3)f.frame.capture_id="foreign";if(mode==4)f.frame.observations.resize(5);
        assert(!r.watch_births(true,3,10,20,birth_options())&&!f.active);assert(f.closes==(mode>=2?1U:0U));
    }
    {Fixture f;auto a=f.api();a.birth={};auto r=StartupRecorderAccess::make(a);assert(r.start(options()));assert(r.watch_births(true,3,10,20,birth_options()).error==ActionError::not_supported);}
    {Fixture f;auto r=StartupRecorderAccess::make(f.api());assert(r.start(options()));f.now=r.report().deadline;assert(!r.watch_births(true,3,10,20,birth_options())&&!f.factories&&r.report().state==StartupRecorderState::closed);}
    {Fixture f;auto r=StartupRecorderAccess::make(f.api());assert(r.start(options()));assert(r.watch_births(true,3,10,20,birth_options()));f.slow=true;assert(r.poll());assert(r.report().state==StartupRecorderState::closed&&!f.active&&f.closes==1&&f.scans==1);}
    {Fixture f;auto r=StartupRecorderAccess::make(f.api());assert(r.start(options()));auto o=birth_options();o.capture_id="recorder";assert(!r.watch_births(true,3,10,20,o)&&!f.factories);assert(!r.watch_births(true,-1,10,20,birth_options())&&!f.factories);}
    assert(!make_native_startup_recorder_birth_source(false,-1,0,0,{}).source);
    std::cout<<"recorder births: explicit ownership/poll-before-scan/binding/separate domains/no sum/close/failure/rollback/move/deadline/save refusal passed\n";
}
