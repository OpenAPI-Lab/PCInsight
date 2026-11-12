#include "../src/windows_user_resources.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pcinsight;
using namespace pcinsight::detail;
UserSession user(std::string id="wts:1",std::string name="DOMAIN\\alice"){
    UserSession r;r.id=std::move(id);r.username=std::move(name);r.status="active";r.resource_scope="before";return r;
}
ProcessInfo process(std::uint32_t pid=42,std::uint32_t session=1,std::string account="DOMAIN\\alice"){
    ProcessInfo p;p.pid=pid;p.username=std::move(account);p.windows_session_id={session,Support::available,"fixture"};p.windows_creation_time_100ns={100,Support::available,"fixture"};
    p.cpu_percent={10,Support::available,"fixture"};p.memory_bytes={100,Support::available,"fixture"};return p;
}
int main(){
    std::vector<UserSession> rows{user(),user("wts:2"),user("wts:3","DOMAIN\\bob")};
    auto a=process(),b=process(43),c=process(44,2);
    fill_windows_user_resources(rows,{std::vector<ProcessInfo>{a,b,c}});
    assert(rows[0].cpu_percent.value==20 && rows[0].memory_bytes.value==200 && rows[1].cpu_percent.value==10 && rows[1].memory_bytes.value==100);
    assert(!rows[2].cpu_percent.value && !rows[2].memory_bytes.value && !rows[0].disk_bytes_per_second.value && !rows[0].network_bytes_per_second.value);
    assert(rows[0].cpu_percent.support==Support::estimated && rows[0].resource_scope=="windows-visible-processes-by-wts-session-and-account");
    b.cpu_percent={{},Support::permission_required,"denied"};b.memory_bytes={{},Support::unsupported,"unavailable"};
    fill_windows_user_resources(rows,{std::vector<ProcessInfo>{a,b}});assert(rows[0].cpu_percent.value==10 && rows[0].cpu_percent.note.find("1 missing")!=std::string::npos);
    fill_windows_user_resources(rows,{std::vector<ProcessInfo>{b}});assert(!rows[0].cpu_percent.value && rows[0].cpu_percent.support==Support::permission_required);
    a.cpu_percent.value=0;a.memory_bytes.value=0;fill_windows_user_resources(rows,{std::vector<ProcessInfo>{a}});assert(rows[0].cpu_percent.value==0 && rows[0].memory_bytes.value==0);
    for(double invalid:{-1.0,101.0,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
        a.cpu_percent.value=invalid;fill_windows_user_resources(rows,{std::vector<ProcessInfo>{a}});assert(!rows[0].cpu_percent.value && rows[0].memory_bytes.value==0);
    }
    a=process();b=process(43);a.cpu_percent.value=60;b.cpu_percent.value=50;a.memory_bytes.value=std::numeric_limits<std::uint64_t>::max();b.memory_bytes.value=1;
    fill_windows_user_resources(rows,{std::vector<ProcessInfo>{a,b}});assert(!rows[0].cpu_percent.value && !rows[0].memory_bytes.value);
    a=process();a.disk_bytes_per_second={12,Support::estimated,"injected provider"};a.network_bytes_per_second={3,Support::available,"injected provider"};fill_windows_user_resources(rows,{std::vector<ProcessInfo>{a}});assert(rows[0].disk_bytes_per_second.value==12 && rows[0].network_bytes_per_second.value==3);
    // No username-only fallback, no guessed session or creation identity.
    for(unsigned mode=0;mode<8;++mode){
        a=process();
        if(mode==0)a.windows_session_id.value.reset();
        if(mode==1)a.windows_session_id.support=Support::estimated;
        if(mode==2)a.windows_creation_time_100ns.value=0;
        if(mode==3)a.windows_creation_time_100ns.support=Support::unsupported;
        if(mode==4)a.username="DOMAIN\\bob";
        if(mode==5)a.windows_session_id.value=99;
        if(mode==6)a.pid=0;
        if(mode==7)a.username="domain\\ALICE"; // Exact-match guard is conservative.
        fill_windows_user_resources(rows,{std::vector<ProcessInfo>{a}});assert(!rows[0].memory_bytes.value && !rows[1].memory_bytes.value);
    }
    a=process();a.cpu_percent.support=Support::unsupported;fill_windows_user_resources(rows,{std::vector<ProcessInfo>{a}});assert(!rows[0].cpu_percent.value);
    auto fails=[&](std::vector<UserSession> input,std::vector<ProcessInfo> processes){
        const auto old=input.empty()?std::string{}:input[0].resource_scope;bool failed=false;
        try{fill_windows_user_resources(input,processes);}catch(const std::exception&){failed=true;}
        assert(failed && (input.empty() || input[0].resource_scope==old));
    };
    fails({user(),user()},{process()});fails({user()},{process(),process()});
    for(const auto* id:{"wts:","1","wts:-1","wts:+1","wts:01","wts:4294967295","wts:4294967296","wts:1x"})fails({user(id)},{process()});
    fails({user("wts:1","")},{process()});fails({user()},{process(42,1,std::string("a\0b",3))});
    fails(std::vector<UserSession>(10001,user()),{});fails({user()},std::vector<ProcessInfo>(100001,process()));
    std::vector<UserSession> empty;fill_windows_user_resources(empty,{});assert(empty.empty());
    rows={user()};fill_windows_user_resources(rows,{});assert(!rows[0].cpu_percent.value && !rows[0].memory_bytes.value);
    std::cout<<"WTS session/account resource aggregation tests passed\n";
}
