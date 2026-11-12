#include "../src/startup_birth_observer.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;using namespace pcinsight::detail;
struct Fake {
    unsigned starts{},reads{},stops{};std::uint64_t owner=77;bool throw_read{},throw_start{},change_caller_on_read{};
    PrivatePidfdNetworkJoin selected;Metric<StartupBirthRead> frame;
    Fake(){selected.kernel={100,0,555};selected.local.boot={123,456};
        auto& o=selected.local.observer;o.local_pid=77;o.start_ticks=5;o.procfs_inode=1;o.process_inode=2;o.pid_namespace_inode=99;o.user_namespace_inode=98;o.pid_namespace_device=4;
        selected.local.process=o;selected.local.process.local_pid=10;selected.local.process.start_ticks=20;
        StartupBirthRead s;s.creator=selected;s.pid_mapping_enabled=true;s.creator_reaped_totals_enabled=true;
        frame={s,Support::available,"private fake frame"};}
    StartupBirthObserverApi api(){return {[this]{auto observer=selected.local.observer;observer.local_pid=static_cast<std::uint32_t>(owner);return Metric<NetworkSelectionContext>{NetworkSelectionContext{selected.local.boot,observer},Support::available,"caller context"};},[this](bool a,int f,std::uint32_t p,std::uint64_t t,std::uint32_t n,bool final,bool exec){assert(a&&f==3&&p==10&&t==20&&n==4&&final&&exec==frame.value->first_exec_enabled);++starts;if(throw_start)throw 1;return Metric<PrivatePidfdNetworkJoin>{selected,Support::available,"selected"};},
        [this]{++reads;if(throw_read)throw 2;if(change_caller_on_read)++owner;return frame;},[this]{++stops;}};}
    void birth(std::uint64_t start=111,std::uint32_t pid=21){StartupBirthRaw r{1,200,0,start,1000,0,0,0,0,0,0,0,0,99,pid};frame.value->births.push_back(**decode_startup_birth(r,frame.value->first_exec_enabled).value);}
    void finish(unsigned row=0){auto& r=frame.value->births.at(row);r.final_monotonic_ns=2000;r.cpu_time={std::chrono::nanoseconds(0),Support::available,"CPU0"};r.read_bytes={0,Support::available,"read0"};r.write_bytes={500,Support::available,"write500 includes waited child"};}
};
StartupBirthObserverOptions options(){return {"capture-A",4,true};}
int main(){
    {Fake f;auto o=StartupBirthObserverAccess::make(f.api());assert(o.report().state==StartupBirthObserverState::idle&&f.starts==0&&f.reads==0);
        assert(!o.start(false,3,10,20,options())&&f.starts==0);auto bad=options();bad.capacity=65;assert(!o.start(true,3,10,20,bad)&&!f.starts);
        assert(o.start(true,3,10,20,options()));assert(!o.start(true,3,10,20,options())&&f.starts==1);assert(o.poll()&&o.report().observations.empty());
        f.birth();assert(o.poll());auto report=o.report();assert(report.observations.size()==1&&!report.observations[0].startup_entry_id.value&&!report.automatic_startup_origin.value);
        const auto key=report.observations[0].key;assert(report.observations[0].local_pid.value==StartupBirthPid({4,99,21}));
        assert(o.associate(key,"example.desktop"));assert(o.associate(key,"example.desktop"));assert(!o.associate(key,"other.desktop"));assert(!o.associate(key,std::string("a\0b",3)));
        for(unsigned n=0;n<4;++n){auto foreign=key;if(n==0)foreign.capture_id="capture-B";if(n==1)++foreign.boot[0];if(n==2)++foreign.kernel_start_boottime_ns;if(n==3)++foreign.kernel_real_uid;assert(!o.associate(foreign,"example.desktop"));}
        f.finish();assert(o.poll());assert(o.poll());report=o.report();assert(report.observations.size()==1&&report.observations[0].startup_entry_id.value=="example.desktop"&&report.observations[0].startup_entry_id.support==Support::estimated);
        assert(report.observations[0].cpu_time.value==std::chrono::nanoseconds(0)&&report.observations[0].write_bytes.value==500);
        // Same PID/global TGID, different kernel start -> distinct historical key.
        f.birth(222,21);f.finish(1);assert(o.poll());report=o.report();assert(report.observations.size()==2&&!report.observations[1].startup_entry_id.value);
        assert(o.associate(report.observations[1].key,"example.desktop")); // NO summing of overlapping IO.
        const auto reads=f.reads;o.close();o.close();assert(f.stops==1&&f.reads==reads&&!o.poll());
        assert(o.report().state==StartupBirthObserverState::closed&&o.report().observations.size()==2&&o.associate(key,"example.desktop"));assert(!o.start(true,3,10,20,options()));
    }
    // Every malformed update is transactional and preserves the accepted binding.
    for(unsigned mode=0;mode<16;++mode){Fake f;auto o=StartupBirthObserverAccess::make(f.api());assert(o.start(true,3,10,20,options()));f.birth();f.finish();assert(o.poll());const auto key=o.report().observations[0].key;assert(o.associate(key,"kept.desktop"));
        if(mode==0)++f.frame.value->creator.local.boot[0];if(mode==1)++f.frame.value->creator.kernel.leader_start_boottime_ns;
        if(mode==2)f.frame.value->births.push_back(f.frame.value->births[0]);
        if(mode==3)++f.frame.value->births[0].monotonic_ns;if(mode==4)++f.frame.value->births[0].kernel.kernel_real_uid;
        if(mode==5)f.frame.value->births[0].local_pid.value->tgid=22;
        if(mode==6)f.frame.value->births[0].final_monotonic_ns.reset();
        if(mode==7)f.frame.value->births[0].write_bytes.value=1;
        if(mode==8)f.frame.value->births[0].cpu_time.value=std::chrono::nanoseconds(-1);
        if(mode==9)f.frame.value->quality_flags=64;
        if(mode==10)f.frame.value->creator_reaped_totals_enabled=false;
        if(mode==11)f.frame.value->pid_mapping_enabled=false;
        if(mode==12)f.frame.value->births[0].local_pid.value->pid_namespace_inode=98;
        if(mode==13)f.frame.value->births[0].write_bytes.note.resize(4097,'x');
        if(mode==14){f.frame.value->births[0].read_bytes.support=Support::unsupported;}
        if(mode==15)f.frame.value->births[0].cpu_time.value=std::chrono::nanoseconds(1); // Even 1ns mutation of the SAME final is refused.
        assert(!o.poll());const auto after=o.report();assert(after.state==StartupBirthObserverState::failed&&after.observations.size()==1&&after.observations[0].cpu_time.value==std::chrono::nanoseconds(0)&&after.observations[0].write_bytes.value==500&&after.observations[0].startup_entry_id.value=="kept.desktop"&&f.stops==1);
    }
    {Fake f;auto o=StartupBirthObserverAccess::make(f.api());assert(o.start(true,3,10,20,options()));f.birth();f.frame.value->quality_flags=32;f.frame.support=Support::estimated;f.frame.value->births[0].local_pid={{},Support::unsupported,"mapping fault"};assert(o.poll());f.finish();assert(o.poll());assert(!o.report().observations[0].local_pid.value&&o.report().observations[0].write_bytes.value==500);f.frame.value->quality_flags=0;assert(!o.poll());}
    {Fake f;auto o=StartupBirthObserverAccess::make(f.api());assert(o.start(true,3,10,20,options()));f.birth();assert(o.poll());f.frame.value->births.clear();f.frame.value->publishing=1;assert(o.poll()&&o.report().observations.size()==1);f.owner=78;assert(!o.poll()&&f.stops==1);}
    {Fake f;f.throw_start=true;auto o=StartupBirthObserverAccess::make(f.api());assert(!o.start(true,3,10,20,options())&&f.stops==1);}
    {Fake f;auto o=StartupBirthObserverAccess::make(f.api());assert(o.start(true,3,10,20,options()));f.birth();assert(o.poll());const auto original=f.frame.value->births[0];f.frame.value->births.clear();f.birth(222);auto bad=original;++bad.monotonic_ns;f.frame.value->births.push_back(bad);assert(!o.poll()&&o.report().observations.size()==1);}
    {Fake f;auto o=StartupBirthObserverAccess::make(f.api());assert(o.start(true,3,10,20,options()));f.birth();assert(o.poll());const auto key=o.report().observations[0].key;f.owner=78;assert(!o.associate(key,"no.desktop")&&f.stops==1&&!o.report().observations[0].startup_entry_id.value);}
    {Fake f;auto o=StartupBirthObserverAccess::make(f.api());assert(o.start(true,3,10,20,options()));f.birth();f.change_caller_on_read=true;assert(!o.poll()&&o.report().observations.empty()&&f.stops==1);}
    {Fake f;f.selected.local.process.start_ticks=21;auto o=StartupBirthObserverAccess::make(f.api());assert(!o.start(true,3,10,20,options())&&f.stops==1);}
    {Fake f;auto o=StartupBirthObserverAccess::make(f.api());assert(o.start(true,3,10,20,options()));f.birth();f.frame.value->creator_exited=true;assert(o.poll());f.frame.value->creator_exited=false;assert(!o.poll()&&o.report().creator_exited);}
    for(unsigned n=0;n<4;++n){Fake f;auto o=StartupBirthObserverAccess::make(f.api());assert(o.start(true,3,10,20,options()));f.birth();assert(o.poll());const auto key=o.report().observations[0].key;
        // Same numeric caller PID in another namespace/boot/incarnation is NOT owner.
        if(n==0)++f.selected.local.observer.pid_namespace_inode;if(n==1)++f.selected.local.observer.start_ticks;if(n==2)++f.selected.local.boot[1];if(n==3)++f.selected.local.observer.process_inode;
        assert(!o.associate(key,"wrong-owner.desktop")&&f.stops==1&&!o.report().observations[0].startup_entry_id.value);}
    {Fake f;auto o=StartupBirthObserverAccess::make(f.api());assert(o.start(true,3,10,20,options()));f.throw_read=true;assert(!o.poll()&&f.stops==1);}
    for(unsigned n=0;n<6;++n){Fake f;auto opt=options();opt.observe_first_exec=true;f.frame.value->first_exec_enabled=true;auto o=StartupBirthObserverAccess::make(f.api());assert(o.start(true,3,10,20,opt));f.birth();assert(o.poll());const auto key=o.report().observations[0].key;assert(o.associate(key,"declared.desktop"));
        auto& r=f.frame.value->births[0];r.exec_state=1;r.first_executable={StartupBirthObservation::Executable{1500,12345,8,1,0},Support::available,"first exec"};
        if(n==1){r.exec_state=3;r.first_executable={{},Support::unsupported,"first failed"};f.frame.value->quality_flags=64;}
        if(n==2)r.first_executable.value->observed_monotonic_ns=999;
        if(n==3){f.finish();r.first_executable.value->observed_monotonic_ns=2100;}
        if(n==4)r.first_executable.value->device_major=4096;
        if(n==5)r.exec_state=0;
        if(n>=2){assert(!o.poll()&&!o.report().observations[0].first_executable.value);continue;}
        assert(o.poll());const auto accepted=o.report().observations[0];assert(accepted.startup_entry_id.support==Support::estimated&&!o.report().automatic_startup_origin.value);
        r.exec_state=1;r.first_executable={StartupBirthObservation::Executable{1500,999,8,1,0},Support::available,"rewritten or later exec"};assert(!o.poll());assert(o.report().observations[0].first_executable.value==accepted.first_executable.value);
    }
    {LinuxStartupBirthObserver native;assert(!native.start(false,-1,0,0,{}));assert(native.report().state==StartupBirthObserverState::idle);native.close();
#ifndef __linux__
        assert(native.start(true,3,10,20,options()).error==ActionError::not_supported);
#endif
    }
    std::cout<<"public birth observer: explicit attribution/idempotence/PID reuse/namespace/transaction/zero/partial/cleanup passed\n";
}
