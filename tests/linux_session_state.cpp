#include "../src/linux_logout.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace pcinsight;
using namespace pcinsight::detail;
template<class T> Metric<T> value(T v){return {std::move(v),Support::available,"fixture"};}
LogindSessionObservation observation(){
    return {"2","alice",1000,{":1.42","/org/freedesktop/login1/session/_32",0},
        {123,ActionError::none},{123,ActionError::none},value(std::string("online")),value(false),value(true),
        value(std::string("2")),value(std::string("alice")),value<std::uint32_t>(1000)};
}
int main(){
    auto o=observation();auto row=make_logind_login(o);
    assert(row.id=="login1:2"&&row.status=="online"&&row.details.value&&parse_logout_token(row.logout_token));
    const auto& d=*row.details.value;
    assert(d.provider=="login1"&&d.unix_uid.value==1000&&d.state.value=="online");
    assert(d.active.value==false&&d.remote.value==true); // known false is not absence or disconnected
    for(const auto& state:{"opening","online","active","closing","future_state"}){
        o=observation();o.state=value(std::string(state));o.active=value(false);
        row=make_logind_login(o);assert(row.status==state&&row.details.value->active.value==false);
    } // preserve raw provider state; don't derive Active from a different property read
    for(unsigned mode=0;mode<10;++mode){
        o=observation();
        if(mode==0)o.before.timestamp=0;
        if(mode==1)++o.after.timestamp;
        if(mode==2)o.before.error=ActionError::permission_denied;
        if(mode==3)o.after.error=ActionError::not_found;
        if(mode==4)o.object_id=value(std::string("3"));
        if(mode==5)o.object_username=value(std::string("bob"));
        if(mode==6)o.object_uid=value<std::uint32_t>(0);
        if(mode==7)o.object_uid={{},Support::permission_required,"denied"};
        if(mode==8)o.object_username.support=Support::unsupported;
        if(mode==9)o.after.error=ActionError::system_error;
        row=make_logind_login(o);assert(row.status=="unknown"&&row.logout_token.empty()&&row.details.value);
        assert(!row.details.value->state.value&&!row.details.value->active.value&&!row.details.value->remote.value);
        assert(row.details.value->unix_uid.value==1000); // enumeration report, not verified live object
        assert(row.details.value->state.support==((mode==2||mode==7)?Support::permission_required:Support::unsupported));
    }
    for(const auto& bad:std::vector<std::string>{"","active\n",std::string("active\0x",8),std::string(129,'a')}){
        o=observation();o.state=value(bad);row=make_logind_login(o);
        assert(row.status=="unknown"&&!row.details.value->state.value&&row.details.value->active.value==false&&!row.logout_token.empty());
    }
    o=observation();o.state={{},Support::permission_required,"denied"};o.active={true,Support::unsupported,"ignore value"};o.remote={false,Support::permission_required,"ignore value"};
    row=make_logind_login(o);assert(!row.details.value->state.value&&row.details.value->state.support==Support::permission_required);
    assert(!row.details.value->active.value&&!row.details.value->remote.value&&row.details.value->remote.support==Support::permission_required);
    for(unsigned mode=0;mode<4;++mode){o=observation();if(mode==0)o.id="";if(mode==1)o.username=std::string("a\0b",3);if(mode==2)o.identity.owner="bad";if(mode==3)o.identity.path="/self";bool threw=false;try{(void)make_logind_login(o);}catch(const std::invalid_argument&){threw=true;}assert(threw);}
    auto a=make_logind_login(observation()),b=a;
    auto rows=aggregate_linux_users(std::vector{a,b},{});assert(rows.size()==1&&rows[0].linux_details.value->active.value==false);
    b.details.value->unix_uid.value=0;bool threw=false;try{(void)aggregate_linux_users(std::vector{a,b},{});}catch(const std::invalid_argument&){threw=true;}assert(threw);
#ifdef __linux__
    const auto native=read_logind_logins();unsigned states=0,active=0;
    if(native)for(const auto& r:*native){
        assert(r.details.value&&r.details.value->provider=="login1"&&r.details.value->unix_uid.value);
        const auto& details=*r.details.value;
        if(details.state.value){++states;assert(r.status==*details.state.value&&!r.logout_token.empty());}
        else assert(r.status=="unknown");
        if(details.active.value)++active;
        if(!r.logout_token.empty())assert(parse_logout_token(r.logout_token));
    }
    std::cout<<"Native login1 read-only: "<<(native?std::to_string(native->size()):"unavailable")<<" sessions, "<<states<<" known states, "<<active<<" known foreground flags\n";
    SampleOptions options;options.include_user_sessions=true;options.include_processes=false;
    options.include_disks=options.include_networks=options.include_gpus=false;
    Monitor monitor;const auto s=monitor.sample(options);assert(s.processes.empty());
    if(s.linux_session_inventory_source.value=="login1")for(const auto& u:s.user_sessions){assert(u.linux_details.value&&u.linux_details.value->provider=="login1");assert(u.status==u.linux_details.value->state.value.value_or("unknown"));}
    options.include_user_sessions=false;const auto disabled=monitor.sample(options);
    assert(disabled.user_sessions.empty()&&!disabled.linux_session_inventory_source.value);
#else
    assert(!read_linux_login_records().value&&!read_logind_logins());
#endif
    std::cout<<"Session state: native semantics, known false/unknown, identity continuity, property denial and duplicate metadata checks passed\n";
}
