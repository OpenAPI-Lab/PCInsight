#include "../src/linux_runit.hpp"
#include <cassert>
#include <algorithm>
#include <iostream>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdlib>
#endif
using namespace pcinsight;using namespace pcinsight::detail;using namespace std::chrono_literals;
LinuxRunitStatus running(unsigned stamp=2,unsigned pid=42){LinuxRunitStatus s;s.changed_at_tai64n[0]=0x40;s.changed_at_tai64n[7]=static_cast<std::uint8_t>(stamp);s.reported_pid=pid;s.state=LinuxRunitState::run;s.want_up=true;return s;}
LinuxRunitStatus down(){auto s=running();s.reported_pid=0;s.state=LinuxRunitState::down;s.want_up=false;return s;}
struct Source {
    Metric<LinuxRunitStatus> value{running(),Support::available,"fixture"};
    std::chrono::steady_clock::time_point time=std::chrono::steady_clock::time_point{}+1s;
    std::chrono::milliseconds read_cost{};unsigned reads{},writes{};bool partial{},fail_write{};
    RunitTransitionApi api(){return {[this]{++reads;time+=read_cost;return value;},[this](auto action,bool auth){assert(auth);++writes;const auto n=action==ServiceAction::restart?3U:1U;return LinuxRunitRequestResult{fail_write?ActionError::system_error:ActionError::none,n,partial?1:n,{true,Support::available,"forged completion must be ignored"},"fixture delivery"};},[this]{return time;}};}
};
#ifdef __linux__
namespace fs=std::filesystem;
void status(const fs::path& p,const LinuxRunitStatus& s){std::array<std::uint8_t,20> b{};std::copy(s.changed_at_tai64n.begin(),s.changed_at_tai64n.end(),b.begin());for(unsigned n=0;n<4;++n)b[12+n]=static_cast<std::uint8_t>(s.reported_pid>>(n*8));b[16]=s.paused;b[17]=s.want_up?'u':'d';b[18]=s.term_sent;b[19]=static_cast<std::uint8_t>(s.state);std::ofstream f(p,std::ios::binary);assert(f);f.write(reinterpret_cast<const char*>(b.data()),b.size());f.close();assert(f);}
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
#endif
int main(){
    const auto base=running();assert(observe_linux_runit_target(ServiceAction::start,base,base).value==true&&observe_linux_runit_target(ServiceAction::restart,base,base).value==false);
    assert(observe_linux_runit_target(ServiceAction::restart,base,running(3,42)).value==true);assert(observe_linux_runit_target(ServiceAction::restart,base,running(2,99)).value==false);assert(observe_linux_runit_target(ServiceAction::restart,base,running(1,99)).value==false);
    auto transient=down();transient.want_up=true;assert(observe_linux_runit_target(ServiceAction::stop,base,transient).value==false&&observe_linux_runit_target(ServiceAction::stop,base,down()).value==true);
    for(unsigned n=0;n<3;++n){auto bad=running(3);if(n==0)bad.paused=true;if(n==1)bad.term_sent=true;if(n==2)bad.want_up=false;assert(observe_linux_runit_target(ServiceAction::restart,base,bad).value==false);}
    {auto bad=base;bad.state=static_cast<LinuxRunitState>(99);assert(!observe_linux_runit_target(ServiceAction::start,base,bad).value);}
    Source s;auto t=RunitTransitionAccess::make(s.api());assert(!t.start(ServiceAction::restart,false)&&s.reads==0&&s.writes==0);assert(!t.start(ServiceAction::restart,true,0ms)&&s.reads==0);
    assert(t.start(ServiceAction::restart,true,100ms)&&s.writes==1&&!t.report().delivery.completed.value&&!t.report().target_state_observed.value);assert(!t.start(ServiceAction::restart,true));
    assert(t.poll()&&t.report().target_state_observed.value==false);s.value.value=running(3);assert(t.poll()&&t.report().state==LinuxRunitTransitionState::observed&&t.report().target_state_observed.value==true&&!t.report().delivery.completed.value);const auto reads=s.reads;assert(t.poll()&&s.reads==reads&&s.writes==1);t.close();assert(t.report().state==LinuxRunitTransitionState::observed);
    for(unsigned mode=0;mode<8;++mode){Source f;auto op=RunitTransitionAccess::make(f.api());if(mode==0)f.partial=true;if(mode==1)f.fail_write=true;if(mode==2)f.read_cost=100ms;
        const auto started=op.start(ServiceAction::restart,true,100ms);
        if(mode<2){assert(!started&&op.report().state==LinuxRunitTransitionState::failed&&f.writes==1&&op.report().delivery.written_bytes>0);continue;}
        if(mode==2){assert(!started&&!f.writes&&!op.report().request_attempted&&op.report().state==LinuxRunitTransitionState::timed_out);continue;}
        assert(started);
        if(mode==3){f.time=op.report().deadline+1ns;assert(!op.poll()&&op.report().state==LinuxRunitTransitionState::timed_out&&!op.report().target_state_observed.value);}
        if(mode==4){f.value.value=running(3);f.read_cost=101ms;assert(!op.poll()&&op.report().state==LinuxRunitTransitionState::timed_out&&op.report().latest.value&&!op.report().target_state_observed.value);}
        if(mode==5){f.value={{},Support::permission_required,"denied"};assert(op.poll()&&!op.report().latest.value&&!op.report().target_state_observed.value);}
        if(mode==6){f.time-=1s;assert(!op.poll()&&op.report().state==LinuxRunitTransitionState::failed&&op.report().error==ActionError::stale_target);}
        if(mode==7){const auto count=f.reads;op.close();assert(f.reads==count&&op.report().state==LinuxRunitTransitionState::abandoned&&op.report().delivery.written_bytes==3);}
        assert(f.writes==1);
    }
    {Source f;auto op=RunitTransitionAccess::make(f.api());assert(op.start(ServiceAction::start,true,100ms)&&op.report().baseline_satisfied.value==true&&!op.report().target_state_observed.value);assert(op.poll()&&op.report().target_state_observed.value==true);}
    {Source f;auto op=RunitTransitionAccess::make(f.api());assert(op.start(ServiceAction::restart,true,100ms));f.time=op.report().deadline;f.value.value=running(3);assert(op.poll()&&op.report().state==LinuxRunitTransitionState::observed);}
    LinuxRunitTransition unavailable(nullptr);assert(!unavailable.start(ServiceAction::start,true)&&unavailable.report().state==LinuxRunitTransitionState::failed);
    for(auto support:{Support::unsupported,Support::estimated,Support::permission_required}){Source f;f.value.support=support;auto op=RunitTransitionAccess::make(f.api());assert(!op.start(ServiceAction::start,true)&&f.writes==0&&!op.report().request_attempted);}
    {Source f;auto op=RunitTransitionAccess::make(f.api());assert(!op.start(ServiceAction::start,true,60001ms)&&f.reads==0&&f.writes==0&&op.report().state==LinuxRunitTransitionState::idle);}
    {Source f;auto op=RunitTransitionAccess::make(f.api());assert(op.start(ServiceAction::stop,true));auto finish=down();finish.state=LinuxRunitState::finish;f.value.value=finish;assert(op.poll()&&op.report().target_state_observed.value==false);f.value.value=transient;assert(op.poll()&&op.report().target_state_observed.value==false);f.value.value=down();assert(op.poll()&&op.report().target_state_observed.value==true);}
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-runit-transition-XXXXXX";const auto d=::mkdtemp(pattern);assert(d);const fs::path root=d;struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};fs::create_directory(root/"supervise");const auto pipe=root/"supervise/control",ok=root/"supervise/ok",file=root/"supervise/status";assert(::mkfifo(pipe.c_str(),0600)==0&&::mkfifo(ok.c_str(),0600)==0);int reader=::open(pipe.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC),okreader=::open(ok.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);assert(reader>=0&&okreader>=0);status(file,down());auto handle=LinuxRunitService::open(root.string());assert(handle.value);
    for(const auto action:{ServiceAction::start,ServiceAction::restart,ServiceAction::stop}){LinuxRunitTransition op(*handle.value);assert(op.start(action,true,30s));char b[4]{};const auto n=::read(reader,b,sizeof b);assert(n==(action==ServiceAction::restart?3:1));
        if(action==ServiceAction::restart){assert(op.poll()&&op.report().target_state_observed.value==false);status(file,running(3,99));}else status(file,action==ServiceAction::start?running():down());
        assert(op.poll()&&op.report().state==LinuxRunitTransitionState::observed&&op.report().target_state_observed.value==true&&!op.report().delivery.completed.value);std::cout<<"Private runit observed state for action"<<static_cast<unsigned>(action)<<" passed; readiness/causal completion UNKNOWN\n";
    }
    handle.value.reset();::close(reader);::close(okreader);assert(fds()==initial);::alarm(0);
#endif
    std::cout<<"Runit transition one-shot/deadline/late-data/unknown/epoch/no-retry/abandon/frozen-report checks passed; NOT real runsv acceptance\n";
}
