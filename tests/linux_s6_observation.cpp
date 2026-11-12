#include "../src/linux_s6_observation.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#endif
using namespace pcinsight;using namespace pcinsight::detail;using namespace std::chrono_literals;
LinuxS6StatusRecord up(unsigned epoch=2,std::uint64_t pid=42){LinuxS6StatusRecord s;s.changed_at_tai64n[0]=0x40;s.changed_at_tai64n[7]=static_cast<std::uint8_t>(epoch);s.reported_pid=pid;s.want_up=true;return s;}
LinuxS6StatusRecord down(){auto s=up();s.reported_pid=0;s.want_up=false;s.ready_reported=true;return s;}
struct Source {
    Metric<LinuxS6StatusRecord> value{up(),Support::available,"fixture"};
    std::chrono::steady_clock::time_point time=std::chrono::steady_clock::time_point{}+1s;
    std::chrono::milliseconds read_cost{};unsigned reads{},writes{},clocks{};bool partial{},failed_write{},throw_read{};
    S6ObservationApi api(){return {[this]{++reads;time+=read_cost;if(throw_read)throw std::runtime_error("fixture read");return value;},[this](auto action,bool consent){assert(consent);++writes;const auto n=action==ServiceAction::restart?3U:1U;return LinuxS6RequestResult{failed_write?ActionError::system_error:ActionError::none,n,partial?1:n,{true,Support::available,"forged completion"},"fixture delivery"};},[this]{++clocks;return time;}};}
};
#ifdef __linux__
namespace fs=std::filesystem;
void put(const fs::path& p,const LinuxS6StatusRecord& s){std::array<std::uint8_t,35> b{};std::copy(s.changed_at_tai64n.begin(),s.changed_at_tai64n.end(),b.begin());std::copy(s.ready_changed_at_tai64n.begin(),s.ready_changed_at_tai64n.end(),b.begin()+12);for(unsigned i=0;i<8;++i)b[24+i]=static_cast<std::uint8_t>(s.reported_pid>>(56-8*i));b[32]=static_cast<std::uint8_t>(s.reported_wait_status>>8);b[33]=static_cast<std::uint8_t>(s.reported_wait_status);b[34]=static_cast<std::uint8_t>(s.paused|(s.finishing<<1)|(s.want_up<<2)|(s.ready_reported<<3));std::ofstream f(p,std::ios::binary);assert(f);f.write(reinterpret_cast<const char*>(b.data()),b.size());f.close();assert(f);}
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
void command(int reader,const std::string& expected){char b[8]{};const auto n=::read(reader,b,sizeof b);assert(n==static_cast<ssize_t>(expected.size())&&std::string(b,static_cast<std::size_t>(n))==expected);}
#endif
int main(){
    const auto base=up();assert(observe_linux_s6_condition(ServiceAction::start,base,base).value==true);assert(observe_linux_s6_condition(ServiceAction::stop,base,down()).value==true);assert(observe_linux_s6_condition(ServiceAction::start,base,down()).value==false);
    assert(observe_linux_s6_condition(ServiceAction::restart,base,up(3,42)).value==true);assert(observe_linux_s6_condition(ServiceAction::restart,base,up(2,99)).value==false);assert(observe_linux_s6_condition(ServiceAction::restart,base,up(1,99)).value==false);
    auto ready_only=up();ready_only.ready_reported=true;ready_only.ready_changed_at_tai64n[7]=99;assert(observe_linux_s6_condition(ServiceAction::restart,base,ready_only).value==false);
    for(unsigned n=0;n<3;++n){auto s=up(3);if(n==0)s.finishing=true;if(n==1)s.paused=true;if(n==2)s.want_up=false;assert(observe_linux_s6_condition(ServiceAction::restart,base,s).value==false);}
    auto gap=down();gap.want_up=true;assert(observe_linux_s6_condition(ServiceAction::stop,base,gap).value==false);auto finish=down();finish.finishing=true;assert(observe_linux_s6_condition(ServiceAction::stop,base,finish).value==false);
    auto invalid=up();invalid.reported_pid=std::numeric_limits<std::uint64_t>::max();assert(!observe_linux_s6_condition(ServiceAction::start,base,invalid).value);invalid=up();invalid.ready_changed_at_tai64n[8]=255;assert(!observe_linux_s6_condition(ServiceAction::start,base,invalid).value);assert(!observe_linux_s6_condition(static_cast<ServiceAction>(99),base,base).value);
    Source s;auto op=S6ObservationAccess::make(s.api());assert(!op.poll());assert(!op.start(ServiceAction::start,false)&&s.reads==0&&s.clocks==0&&s.writes==0);assert(!op.start(ServiceAction::start,true,0ms)&&s.clocks==0);assert(!op.start(ServiceAction::start,true,60001ms)&&s.clocks==0);
    assert(op.start(ServiceAction::restart,true,100ms)&&s.writes==1&&!op.report().delivery.completed.value);assert(!op.start(ServiceAction::restart,true));assert(op.poll()&&op.report().target_condition_observed.value==false);s.value.value=up(3);assert(op.poll()&&op.report().state==LinuxS6ObservationState::condition_observed&&op.report().target_condition_observed.value==true&&!op.report().delivery.completed.value);const auto reads=s.reads;assert(op.poll()&&s.reads==reads);op.close();assert(op.report().state==LinuxS6ObservationState::condition_observed);
    for(unsigned mode=0;mode<11;++mode){Source f;auto t=S6ObservationAccess::make(f.api());if(mode==0)f.partial=true;if(mode==1)f.failed_write=true;if(mode==2)f.read_cost=100ms;
        const auto result=t.start(ServiceAction::restart,true,100ms);
        if(mode<2){assert(!result&&t.report().state==LinuxS6ObservationState::failed&&f.writes==1&&t.report().delivery.written_bytes>0);continue;}
        if(mode==2){assert(!result&&f.writes==0&&!t.report().request_attempted&&t.report().state==LinuxS6ObservationState::timed_out);continue;}
        assert(result);
        if(mode==3){const auto count=f.reads;f.time=t.report().deadline+1ns;assert(!t.poll()&&f.reads==count&&t.report().state==LinuxS6ObservationState::timed_out&&!t.report().target_condition_observed.value);}
        if(mode==4){f.value.value=up(3);f.read_cost=101ms;assert(!t.poll()&&t.report().latest.value&&t.report().state==LinuxS6ObservationState::timed_out&&!t.report().target_condition_observed.value);}
        if(mode==5){f.value={{},Support::permission_required,"denied"};assert(t.poll()&&!t.report().target_condition_observed.value&&!t.report().latest.value);}
        if(mode==6){f.time-=1s;assert(!t.poll()&&t.report().error==ActionError::stale_target&&t.report().state==LinuxS6ObservationState::failed);}
        if(mode==7){const auto count=f.reads;t.close();assert(t.report().state==LinuxS6ObservationState::abandoned&&f.reads==count&&t.report().delivery.written_bytes==3);}
        if(mode==8){f.throw_read=true;assert(!t.poll()&&t.report().state==LinuxS6ObservationState::failed&&t.report().delivery.written_bytes==3);}
        if(mode==9){f.time=t.report().deadline;f.value.value=up(3);assert(t.poll()&&t.report().state==LinuxS6ObservationState::condition_observed);}
        if(mode==10){f.value={up(3),Support::estimated,"not exact"};assert(t.poll()&&!t.report().target_condition_observed.value);}
        assert(f.writes==1&&!t.report().delivery.completed.value);
    }
    for(auto support:{Support::unsupported,Support::estimated,Support::permission_required}){Source f;f.value.support=support;auto t=S6ObservationAccess::make(f.api());assert(!t.start(ServiceAction::start,true)&&!f.writes&&!t.report().request_attempted);}
    {Source f;f.time=std::chrono::steady_clock::time_point::max();auto t=S6ObservationAccess::make(f.api());assert(!t.start(ServiceAction::start,true)&&!f.reads&&!f.writes);}
    {Source f;auto t=S6ObservationAccess::make(f.api());assert(t.start(ServiceAction::start,true)&&t.report().baseline_satisfied.value==true&&!t.report().target_condition_observed.value);assert(t.poll()&&t.report().target_condition_observed.value==true);}
    LinuxS6Observation missing(nullptr);assert(!missing.start(ServiceAction::start,true)&&missing.report().state==LinuxS6ObservationState::failed);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-s6-observation-XXXXXX";auto d=::mkdtemp(pattern);assert(d);const fs::path root=d,a=root/"a",b=root/"b",alias=root/"alias",file=a/"supervise/status",fifo=a/"supervise/control";struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};fs::create_directories(a/"supervise");fs::create_directories(b/"supervise");assert(::mkfifo(fifo.c_str(),0600)==0);put(file,down());put(b/"supervise/status",up(99));fs::create_directory_symlink(a,alias);
    int reader=::open(fifo.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);assert(reader>=0);auto opened=LinuxS6Control::open(alias.string());assert(opened.value);auto target=*opened.value;
    {LinuxS6Observation t(target);assert(t.start(ServiceAction::start,true,30s));command(reader,"u");fs::remove(alias);fs::create_directory_symlink(b,alias);assert(t.poll()&&t.report().target_condition_observed.value==false);put(file,up());assert(t.poll()&&t.report().state==LinuxS6ObservationState::condition_observed&&!t.report().delivery.completed.value);}
    {LinuxS6Observation t(target);assert(t.start(ServiceAction::restart,true,30s));command(reader,"cru");auto f=up(3,99);f.finishing=true;put(file,f);assert(t.poll()&&t.report().target_condition_observed.value==false);put(file,ready_only);assert(t.poll()&&t.report().target_condition_observed.value==false);put(file,up(3));assert(t.poll()&&t.report().state==LinuxS6ObservationState::condition_observed);}
    {LinuxS6Observation t(target);assert(t.start(ServiceAction::stop,true,30s));command(reader,"d");put(file,finish);assert(t.poll()&&t.report().target_condition_observed.value==false);put(file,gap);assert(t.poll()&&t.report().target_condition_observed.value==false);put(file,down());assert(t.poll()&&t.report().state==LinuxS6ObservationState::condition_observed);}
    fs::remove(file);{LinuxS6Observation t(target);assert(!t.start(ServiceAction::start,true)&&!t.report().request_attempted);}char byte{};assert(::read(reader,&byte,1)<=0);put(file,up());fs::rename(fifo,a/"supervise/old-control");assert(::mkfifo(fifo.c_str(),0600)==0);assert(!target->read_record().value);
    target.reset();opened.value.reset();::close(reader);assert(fds()==initial);::alarm(0);
#endif
    std::cout<<"s6 pinned record observation one-shot/clock/deadline/late-data/flags/epoch/unknown/abandon/no-retry/FD tests passed; NOT life/readiness/causal completion or real s6 acceptance\n";
}
