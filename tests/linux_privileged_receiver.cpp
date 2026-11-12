#include "../src/linux_privileged_execution.hpp"
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <atomic>
#include <chrono>
#include <csignal>
#include <dlfcn.h>
#include <dirent.h>
#include <fstream>
#include <fcntl.h>
#include <mutex>
#include <thread>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
std::size_t fds(){auto* d=opendir("/proc/self/fd");assert(d);std::size_t n{};while(auto* e=readdir(d))if(e->d_name[0]!='.')++n;closedir(d);return n;}
struct Bus;struct Message;struct Slot;struct Error{const char* name{};const char* message{};int need_free{};};
struct Api {
    void* lib=dlopen("libsystemd.so.0",RTLD_NOW|RTLD_LOCAL);
    int (*create)(Bus**){};Bus* (*unref)(Bus*){};
    int (*address)(Bus*,const char*){},(*client)(Bus*,int){},(*start)(Bus*){},(*timeout)(Bus*,std::uint64_t){},(*own)(Bus*,const char**){};
    int (*filter)(Bus*,Slot**,int(*)(Message*,void*,Error*),void*){},(*process)(Bus*,Message**) {},(*wait)(Bus*,std::uint64_t){},(*request_name)(Bus*,const char*,std::uint64_t){};
    int (*new_call)(Bus*,Message**,const char*,const char*,const char*,const char*){},(*new_reply)(Message*,Message**){};
    Message* (*unref_message)(Message*){};int (*append)(Message*,const char*,...) {},(*read)(Message*,const char*,...){};
    int (*open)(Message*,char,const char*){},(*close)(Message*){},(*enter)(Message*,char,const char*){},(*exit)(Message*){};
    int (*send)(Bus*,Message*,std::uint64_t*){},(*call)(Bus*,Message*,std::uint64_t,Error*,Message**){};
    Bus* (*bus)(Message*){};int (*type)(Message*,std::uint8_t*){},(*set_sender)(Message*,const char*){};
    const char* (*member)(Message*){};void (*free_error)(Error*){};
    int (*can_send)(Bus*,char){},(*interactive)(Bus*){},(*exit_disconnect)(Bus*){};
    int (*error_reply)(Message*,Message**,const char*,const char*,...){};
    int (*new_signal)(Bus*,Message**,const char*,const char*,const char*){};
    int (*get_timeout)(Bus*,std::uint64_t*){};
    Api(){assert(lib);
#define L(field,symbol) field=reinterpret_cast<decltype(field)>(dlsym(lib,symbol));assert(field)
        L(create,"sd_bus_new");L(unref,"sd_bus_unref");L(address,"sd_bus_set_address");L(client,"sd_bus_set_bus_client");L(start,"sd_bus_start");L(timeout,"sd_bus_set_method_call_timeout");L(own,"sd_bus_get_unique_name");L(filter,"sd_bus_add_filter");L(process,"sd_bus_process");L(wait,"sd_bus_wait");L(request_name,"sd_bus_request_name");
        L(new_call,"sd_bus_message_new_method_call");L(new_reply,"sd_bus_message_new_method_return");L(unref_message,"sd_bus_message_unref");L(append,"sd_bus_message_append");L(read,"sd_bus_message_read");L(open,"sd_bus_message_open_container");L(close,"sd_bus_message_close_container");L(enter,"sd_bus_message_enter_container");L(exit,"sd_bus_message_exit_container");L(send,"sd_bus_send");L(call,"sd_bus_call");L(bus,"sd_bus_message_get_bus");L(type,"sd_bus_message_get_type");L(set_sender,"sd_bus_message_set_sender");L(member,"sd_bus_message_get_member");L(free_error,"sd_bus_error_free");
        L(can_send,"sd_bus_can_send");L(interactive,"sd_bus_get_allow_interactive_authorization");L(exit_disconnect,"sd_bus_get_exit_on_disconnect");
        L(error_reply,"sd_bus_message_new_method_errorf");
        L(new_signal,"sd_bus_message_new_signal");
        L(get_timeout,"sd_bus_get_method_call_timeout");
#undef L
    }
    ~Api(){dlclose(lib);}
    Bus* connect(const std::string& path){Bus* b{};assert(create(&b)>=0);assert(address(b,("unix:path="+path).c_str())>=0);assert(client(b,1)>=0);assert(timeout(b,2000000)>=0);assert(start(b)>=0);const char* n{};assert(own(b,&n)>=0&&n);return b;}
};
// Private broker only: no service dirs, activation, inherited host bus, or polkit.
struct Broker {
    std::string directory,path;pid_t pid{};
    explicit Broker(const char* executable,const std::string& limits_file={}){char temp[]="/tmp/pcinsight-receiver-XXXXXX";const auto* d=mkdtemp(temp);assert(d);directory=d;path=directory+"/bus";
        const auto config=directory+"/bus.conf";
        {std::ofstream out(config);out<<"<busconfig><type>session</type><listen>unix:path="<<path<<"</listen><auth>EXTERNAL</auth>";if(!limits_file.empty()){std::ifstream limits(limits_file);assert(limits.good());out<<limits.rdbuf();assert(!limits.bad());}out<<"<policy context=\"default\"><allow user=\"*\"/><allow own=\"*\"/><allow send_destination=\"*\"/><allow receive_sender=\"*\"/></policy></busconfig>";assert(out.good());}
        const auto argument="--config-file="+config;const auto parent=getpid();pid=fork();assert(pid>=0);if(pid==0){if(prctl(PR_SET_PDEATHSIG,SIGTERM)<0||getppid()!=parent)_exit(127);execl(executable,executable,argument.c_str(),"--nofork","--nopidfile",static_cast<char*>(nullptr));_exit(127);}
        for(int i=0;i<200&&access(path.c_str(),F_OK);++i)std::this_thread::sleep_for(std::chrono::milliseconds(5));assert(access(path.c_str(),F_OK)==0);
    }
    ~Broker(){assert(kill(pid,SIGTERM)==0);int status{};assert(waitpid(pid,&status,0)==pid);unlink(path.c_str());unlink((directory+"/bus.conf").c_str());unlink((directory+"/policy.conf").c_str());assert(rmdir(directory.c_str())==0);}
};
struct State {
    Api& a;PrivilegedIdentityConfig config;std::string expected_caller;
    std::atomic<unsigned> checks{},last_operation{};std::atomic<bool> allow{true};std::mutex lock;PrivilegedAuthorizationObservation last;
    PrivilegedPolicyStore policy;std::atomic<bool> revoke_during_check{};
    std::shared_ptr<PrivilegedBusConnection> connection;std::string directory;std::atomic<bool> compromise_directory{};
    PrivilegedExecutionGate execution_gate;PrivilegedExecutionResult execution;std::atomic<unsigned> manager_calls{},manager_mode{},manager_operation{};
    std::atomic<unsigned> subscribe_calls{};std::atomic<bool> subscribe_fail{};
    std::atomic<bool> hold_authority{},holding_authority{},hold_manager{},holding_manager{};
    std::atomic<unsigned> incoming_calls{};
};
int receiver(Message* m,void* user,Error*){
    auto& s=*static_cast<State*>(user);std::uint8_t type{};assert(s.a.type(m,&type)>=0);if(type!=1)return 0;
    ++s.incoming_calls;
    int accepted{};
    if(std::string(s.a.member(m))=="ControlSystemService"){
        auto result=execute_owned_privileged_request(m,*s.connection,*s.config.authority_uid,s.policy,s.execution_gate);accepted=result.state==PrivilegedExecutionState::job_queued;
        {std::lock_guard guard(s.lock);s.execution=std::move(result);}
    }else{auto observed=receive_owned_privileged_request(m,*s.connection,*s.config.authority_uid,s.policy);accepted=observed.state==PrivilegedAuthorizationState::authorization_observed&&observed.authorization.value.value_or(false);
        {std::lock_guard guard(s.lock);s.last=observed;}}
    std::uint64_t restored{};assert(s.a.get_timeout(s.a.bus(m),&restored)>=0&&restored==2000000);
    Message* reply{};assert(s.a.new_reply(m,&reply)>=0);assert(s.a.append(reply,"b",accepted)>=0);assert(s.a.send(s.a.bus(m),reply,nullptr)>=0);s.a.unref_message(reply);return 1;
}
int manager(Message* m,void* user,Error*){
    auto& s=*static_cast<State*>(user);auto& a=s.a;std::uint8_t type{};assert(a.type(m,&type)>=0);if(type!=1)return 0;
    const std::string method=a.member(m);if(method=="Subscribe"){++s.subscribe_calls;Message* reply{};if(s.subscribe_fail)assert(a.error_reply(m,&reply,"org.freedesktop.DBus.Error.Failed","private Subscribe refusal")>=0);else assert(a.new_reply(m,&reply)>=0);assert(a.send(a.bus(m),reply,nullptr)>=0);a.unref_message(reply);return 1;}
    const unsigned operation=method=="StartUnit"?1:method=="StopUnit"?2:method=="RestartUnit"?3:0;assert(operation);
    const char* unit{};const char* mode{};assert(a.read(m,"ss",&unit,&mode)>0&&std::string(unit)=="private.service"&&std::string(mode)=="replace");
    const auto id=37U+(++s.manager_calls);const auto path="/org/freedesktop/systemd1/job/"+std::to_string(id);s.manager_operation=operation;Message* reply{};
    const auto behavior=s.manager_mode.load();if(behavior==3)s.policy.invalidate();
    if(behavior==13){s.holding_manager=true;for(unsigned i=0;i<1000&&s.hold_manager;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));s.holding_manager=false;}
    auto emit=[&](const char* unit,const char* result,bool bad_signature=false){Message* signal{};assert(a.new_signal(a.bus(m),&signal,"/org/freedesktop/systemd1","org.freedesktop.systemd1.Manager","JobRemoved")>=0);if(bad_signature)assert(a.append(signal,"s","bad signature")>=0);else assert(a.append(signal,"uoss",id,path.c_str(),unit,result)>=0);assert(a.send(a.bus(m),signal,nullptr)>=0);a.unref_message(signal);};
    if(behavior==0||behavior==9)emit("private.service","done");
    if(behavior==5)emit("private.service","failed");if(behavior==7)emit("alias.service","done");if(behavior==8)emit("private.service","future-result");
    if(behavior==9)emit("private.service","done");if(behavior==11){emit("private.service","done");emit("private.service","failed");}if(behavior==12)emit("private.service","done",true);
    if(behavior==4)assert(a.error_reply(m,&reply,"org.freedesktop.DBus.Error.Failed","private failure after simulated acceptance")>=0);
    else {assert(a.new_reply(m,&reply)>=0);assert(a.append(reply,behavior==2?"s":"o",behavior==1?"/not/a/job":path.c_str())>=0);}
    assert(a.send(a.bus(m),reply,nullptr)>=0);a.unref_message(reply);return 1;
}
int authority(Message* m,void* user,Error*){
    auto& s=*static_cast<State*>(user);auto& a=s.a;std::uint8_t type{};assert(a.type(m,&type)>=0);if(type!=1)return 0;
    assert(std::string(a.member(m))=="CheckAuthorization");
    assert(a.enter(m,'r',"sa{sv}")>0);const char* kind{};assert(a.read(m,"s",&kind)>0&&std::string(kind)=="system-bus-name");
    assert(a.enter(m,'a',"{sv}")>0);assert(a.enter(m,'e',"sv")>0);const char* key{};assert(a.read(m,"s",&key)>0&&std::string(key)=="name");
    assert(a.enter(m,'v',"s")>0);const char* name{};assert(a.read(m,"s",&name)>0);{std::lock_guard guard(s.lock);assert(std::string(name)==s.expected_caller);}
    assert(a.exit(m)>=0);assert(a.exit(m)>=0);assert(a.exit(m)>=0);assert(a.exit(m)>=0);
    const char* action{};assert(a.read(m,"s",&action)>0);const std::string id=action;const unsigned op=id=="org.pcinsight.helper.systemd.start"?1:id=="org.pcinsight.helper.systemd.stop"?2:id=="org.pcinsight.helper.systemd.restart"?3:0;assert(op);const std::string verb=op==1?"start":op==2?"stop":"restart";s.last_operation=op;
    assert(a.enter(m,'a',"{ss}")>0);unsigned details{};while(a.enter(m,'e',"ss")>0){const char* k{};const char* v{};assert(a.read(m,"ss",&k,&v)>0);assert((std::string(k)=="unit"&&std::string(v)=="private.service")||(std::string(k)=="verb"&&std::string(v)==verb));++details;assert(a.exit(m)>=0);}assert(details==2);assert(a.exit(m)>=0);
    std::uint32_t flags{};const char* cancellation{};assert(a.read(m,"us",&flags,&cancellation)>0&&flags==0&&std::string(cancellation).empty());
    ++s.checks;if(s.hold_authority){s.holding_authority=true;for(unsigned i=0;i<1000&&s.hold_authority;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));s.holding_authority=false;}if(s.revoke_during_check)s.policy.invalidate();if(s.compromise_directory)assert(chmod(s.directory.c_str(),0777)==0);Message* reply{};assert(a.new_reply(m,&reply)>=0);assert(a.open(reply,'r',"bba{ss}")>=0);assert(a.append(reply,"bb",int(s.allow.load()),0)>=0);assert(a.open(reply,'a',"{ss}")>=0);assert(a.close(reply)>=0);assert(a.close(reply)>=0);assert(a.send(a.bus(m),reply,nullptr)>=0);a.unref_message(reply);return 1;
}
void loop(Api& a,Bus* b,std::atomic<bool>& stop){while(!stop.load()){const int rc=a.process(b,nullptr);assert(rc>=0);if(rc==0)assert(a.wait(b,10000)>=0);}}
void exercise_transport(const std::string& socket_path,bool admission_case,bool transport_case){
    alarm(20);const auto initial=fds();{
    const auto directory=socket_path.substr(0,socket_path.rfind('/'));const auto opened=open_privileged_directory(directory,geteuid(),true);if(!opened.value)std::cerr<<opened.note<<'\n';assert(opened.value);
    PrivilegedBusEndpoint endpoint{*opened.value,"bus",std::uint32_t(geteuid()),std::uint32_t(geteuid())};
    if(admission_case){endpoint.admission.global_burst=3;endpoint.admission.global_per_second=1;endpoint.admission.sender_burst=2;endpoint.admission.sender_per_second=1;}
    else {endpoint.admission.global_burst=endpoint.admission.sender_burst=256;endpoint.admission.global_per_second=endpoint.admission.sender_per_second=512;} // Explicit regression profile, not production defaults.
    const auto connected=connect_privileged_bus(endpoint);if(!connected.value)std::cerr<<connected.note<<'\n';assert(connected.value);auto connection=*connected.value;
    {const auto another=connect_privileged_bus(endpoint);assert(another.value&&(*another.value)->generation()>connection->generation());(*another.value)->disconnect();assert(!(*another.value)->native_handle()&&(*another.value)->generation()==0&&(*another.value)->identity_config(geteuid()).socket_path.empty());}
    {auto wrong=endpoint;wrong.bus_peer_uid=geteuid()==0?1:0;assert(!connect_privileged_bus(wrong).value);wrong=endpoint;wrong.socket_owner_uid=geteuid()==0?1:0;assert(!connect_privileged_bus(wrong).value);}
    Api a;auto* server=static_cast<Bus*>(connection->native_handle());assert(server);Bus* auth=a.connect(socket_path);Bus* client=a.connect(socket_path);Bus* service_manager=a.connect(socket_path);Bus* second_client=(admission_case||transport_case)?a.connect(socket_path):nullptr;
    assert(a.can_send(server,'h')==0&&a.interactive(server)==0&&a.exit_disconnect(server)==0);
    const char* own{};assert(a.own(server,&own)>=0);const std::string destination=own;assert(a.own(client,&own)>=0);const std::string caller=own;
    State state{a,connection->identity_config(geteuid()),caller};state.connection=connection;state.directory=directory;
    const auto policy_file=directory+"/policy.conf";
    {std::ofstream out(policy_file);out<<"PCINSIGHT-SERVICE-POLICY 1\nprivate.service 7\n";out.close();assert(out.good());}assert(chmod(policy_file.c_str(),0600)==0);
    const int anchor=open(directory.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);assert(anchor>=0);assert(state.policy.refresh(anchor,"policy.conf",geteuid()).value);
    assert(a.request_name(auth,"org.freedesktop.PolicyKit1",0)>=0);assert(a.request_name(service_manager,"org.freedesktop.systemd1",0)>=0);assert(a.filter(server,nullptr,receiver,&state)>=0);assert(a.filter(auth,nullptr,authority,&state)>=0);assert(a.filter(service_manager,nullptr,manager,&state)>=0);
    std::atomic<bool> stop{};std::thread serving([&]{loop(a,server,stop);});std::thread authorizing([&]{loop(a,auth,stop);});std::thread managing([&]{loop(a,service_manager,stop);});
    auto invoke=[&](std::uint32_t version,std::uint32_t op,const char* unit,int intent,bool extra=false,bool forged_sender=false,const char* path="/org/pcinsight/Helper",const char* iface="org.pcinsight.Helper1",const char* method="AuthorizeSystemService"){
        Message* request{};assert(a.new_call(client,&request,destination.c_str(),path,iface,method)>=0);assert(a.append(request,"uusb",version,op,unit,intent)>=0);if(extra)assert(a.append(request,"u",0U)>=0);if(forged_sender)assert(a.set_sender(request,":1.999999")>=0);
        Message* reply{};Error error{};const int rc=a.call(client,request,5000000,&error,&reply);if(rc<0)std::cerr<<"private call: "<<(error.message?error.message:"")<<'\n';assert(rc>=0);int accepted=-1;assert(a.read(reply,"b",&accepted)>0);a.unref_message(reply);a.unref_message(request);a.free_error(&error);
        {std::lock_guard guard(state.lock);if(!accepted&&state.last.check_attempted&&state.allow)std::cerr<<"receiver: "<<state.last.note<<" / "<<state.last.authorization.note<<'\n';}
        return accepted;
    };
    if(transport_case){
        assert(invoke(1,1,"private.service",1)==1&&state.incoming_calls==1&&state.checks==1);
        Message* oversized{};assert(a.new_call(client,&oversized,destination.c_str(),"/org/pcinsight/Helper","org.pcinsight.Helper1","AuthorizeSystemService")>=0);const std::string large(65536,'x');assert(a.append(oversized,"uusb",1U,1U,large.c_str(),1)>=0);
        Message* reply{};Error error{};const int rc=a.call(client,oversized,2000000,&error,&reply);if(reply)a.unref_message(reply);a.unref_message(oversized);a.free_error(&error);assert(rc<0);
        assert(state.incoming_calls==1&&state.checks==1&&state.manager_calls==0);
        std::swap(client,second_client);const char* new_caller{};assert(a.own(client,&new_caller)>=0);{std::lock_guard guard(state.lock);state.expected_caller=new_caller;}
        assert(invoke(1,1,"private.service",1)==1&&state.incoming_calls==2&&state.checks==2&&state.manager_calls==0);
    }else if(admission_case){
        state.execution_gate.enabled=true;state.execution_gate.manager_uid=geteuid();
        assert(invoke(1,1,"private.service",1)==1&&state.checks==1);
        assert(invoke(1,1,"private.service",1,false,false,"/org/pcinsight/Helper","org.pcinsight.Helper1","ControlSystemService")==1&&state.checks==2&&state.manager_calls==1);
        assert(invoke(1,1,"private.service",1,false,false,"/org/pcinsight/Helper","org.pcinsight.Helper1","ControlSystemService")==0&&state.checks==2&&state.manager_calls==1);
        {std::lock_guard guard(state.lock);assert(!state.execution.budget&&!state.execution.dispatch_attempted&&state.execution.note.find("Admission")!=std::string::npos);}
        std::swap(client,second_client);assert(invoke(1,1,"private.service",1)==0&&state.checks==2);std::swap(client,second_client);
        assert(invoke(1,1,"private.service",1)==0&&state.checks==2&&state.manager_calls==1);
        {std::lock_guard guard(state.lock);assert(!state.last.check_attempted&&state.last.note.find("Admission")!=std::string::npos);}
    }else{
    assert(invoke(1,1,"private.service",1)==1);assert(state.checks==1);
    // Broker replaces a forged sender header; authority must see authenticated caller.
    assert(invoke(1,1,"private.service",1,false,true)==1);assert(state.checks==2);
    assert(invoke(1,2,"private.service",1)==1);assert(state.checks==3&&state.last_operation==2);
    assert(invoke(1,3,"private.service",1)==1);assert(state.checks==4&&state.last_operation==3);
    assert(invoke(2,1,"private.service",1)==0);assert(invoke(1,0,"private.service",1)==0);assert(invoke(1,4,"private.service",1)==0);
    assert(invoke(1,1,"private.service",0)==0);assert(invoke(1,1,"other.service",1)==0);assert(invoke(1,1,"../private.service",1)==0);
    assert(invoke(1,1,std::string(256,'x').c_str(),1)==0);assert(invoke(1,1,"private.service",1,true)==0);
    assert(invoke(1,1,"private.service",1,false,false,"/org/pcinsight/Wrong")==0);
    assert(invoke(1,1,"private.service",1,false,false,"/org/pcinsight/Helper","org.pcinsight.Wrong")==0);
    assert(invoke(1,1,"private.service",1,false,false,"/org/pcinsight/Helper","org.pcinsight.Helper1","StartUnit")==0);assert(state.checks==4);
    state.allow=false;assert(invoke(1,1,"private.service",1)==0);assert(state.checks==5);
    state.allow=true;state.revoke_during_check=true;assert(invoke(1,1,"private.service",1)==0);assert(state.checks==6);
    {std::lock_guard guard(state.lock);assert(state.last.check_attempted&&!state.last.authorization.value&&state.last.state!=PrivilegedAuthorizationState::denied);}
    assert(invoke(1,1,"private.service",1)==0&&state.checks==6);state.revoke_during_check=false;
    assert(state.policy.refresh(anchor,"policy.conf",geteuid()).value);assert(invoke(1,1,"private.service",1)==1&&state.checks==7);
    assert(chmod(policy_file.c_str(),0666)==0);assert(!state.policy.refresh(anchor,"policy.conf",geteuid()).value);assert(invoke(1,1,"private.service",1)==0&&state.checks==7);
    assert(chmod(policy_file.c_str(),0600)==0);assert(state.policy.refresh(anchor,"policy.conf",geteuid()).value);assert(invoke(1,1,"private.service",1)==1&&state.checks==8);
    auto control=[&](unsigned op=1,const char* unit="private.service",int intent=1,bool extra=false){return invoke(1,op,unit,intent,extra,false,"/org/pcinsight/Helper","org.pcinsight.Helper1","ControlSystemService");};
    assert(control()==0&&state.checks==8&&state.manager_calls==0);state.execution_gate.enabled=true;
    assert(control()==0&&state.checks==8&&state.manager_calls==0);state.execution_gate.manager_uid=geteuid();
    assert(!state.execution_gate.busy.test_and_set());assert(control()==0&&state.checks==8);state.execution_gate.busy.clear();
    assert(control(4)==0&&control(1,"other.service")==0&&control(1,"private.service",0)==0&&control(1,"private.service",1,true)==0);assert(state.checks==8&&state.manager_calls==0);
    state.execution_gate.manager_uid=geteuid()==0?1:0;assert(control()==0&&state.checks==8&&state.manager_calls==0);state.execution_gate.manager_uid=geteuid();
    state.allow=false;assert(control()==0&&state.checks==9&&state.manager_calls==0);state.allow=true;
    auto job_record=[&]{std::lock_guard guard(state.lock);assert(state.execution.job);return state.execution.job;};
    auto wait_job=[&](std::shared_ptr<PrivilegedJobObservation> job,PrivilegedJobState expected){for(unsigned i=0;i<1000;++i){if(job->snapshot().state==expected)return;std::this_thread::sleep_for(std::chrono::milliseconds(1));}assert(false&&"job observation did not reach expected state");};
    state.subscribe_fail=true;assert(control()==0&&state.manager_calls==0&&state.checks==10);state.subscribe_fail=false;
    std::shared_ptr<PrivilegedJobObservation> completed;
    for(unsigned op=1;op<=3;++op){assert(control(op)==1&&state.manager_calls==op&&state.manager_operation==op);{std::lock_guard guard(state.lock);assert(state.execution.dispatch_attempted&&state.execution.authorization_checked&&state.execution.job_path.value=="/org/freedesktop/systemd1/job/"+std::to_string(37+op));}completed=job_record();wait_job(completed,PrivilegedJobState::completed);assert(completed->snapshot().succeeded.value==true);}
    assert(invoke(1,1,"private.service",1)==1&&state.manager_calls==3); // Authorization-only is never an executor.
    state.revoke_during_check=true;assert(control()==0&&state.manager_calls==3);state.revoke_during_check=false;
    assert(state.policy.refresh(anchor,"policy.conf",geteuid()).value);
    for(unsigned mode=1;mode<=4;++mode){state.manager_mode=mode;const auto before=state.manager_calls.load();assert(control()==0&&state.manager_calls==before+1);{std::lock_guard guard(state.lock);assert(state.execution.dispatch_attempted&&state.execution.state==PrivilegedExecutionState::dispatch_unknown&&!state.execution.job_path.value);}if(mode==3)assert(state.policy.refresh(anchor,"policy.conf",geteuid()).value);}
    state.manager_mode=0;assert(control()==1&&state.manager_calls==8);
    for(unsigned mode:{5,7,8,9,11,12}){state.manager_mode=mode;assert(control()==1);wait_job(job_record(),mode==5?PrivilegedJobState::failed:mode==9?PrivilegedJobState::completed:PrivilegedJobState::unknown);}
    state.manager_mode=6;state.execution_gate.job_timeout_usec=50000;assert(control()==1);auto timed=job_record();
    {Message* spoof{};assert(a.new_signal(client,&spoof,"/org/freedesktop/systemd1","org.freedesktop.systemd1.Manager","JobRemoved")>=0);const auto id=37U+state.manager_calls.load();const auto path="/org/freedesktop/systemd1/job/"+std::to_string(id);assert(a.append(spoof,"uoss",id,path.c_str(),"private.service","done")>=0);assert(a.send(client,spoof,nullptr)>=0);a.unref_message(spoof);}wait_job(timed,PrivilegedJobState::unknown);assert(!timed->snapshot().succeeded.value);
    state.execution_gate.job_timeout_usec=30000000;assert(control()==1);auto pending=job_record();assert(pending->snapshot().state==PrivilegedJobState::pending);
    assert(state.manager_calls==16&&state.subscribe_calls==2&&state.checks==28);
    state.execution_gate.request_timeout_usec=0;assert(control()==0&&state.checks==28&&state.manager_calls==16);state.execution_gate.request_timeout_usec=5000000;
    state.execution_gate.rpc_limit=0;assert(control()==0&&state.checks==28);state.execution_gate.rpc_limit=1;assert(control()==0&&state.checks==28&&state.manager_calls==16);{std::lock_guard guard(state.lock);assert(!state.execution.dispatch_attempted&&state.execution.budget->used()==1);}state.execution_gate.rpc_limit=96;
    state.hold_authority=true;state.execution_gate.request_timeout_usec=200000;assert(control()==0&&state.manager_calls==16&&state.holding_authority);{std::lock_guard guard(state.lock);assert(state.execution.authorization_checked&&!state.execution.dispatch_attempted);}state.hold_authority=false;state.execution_gate.request_timeout_usec=5000000;
    for(unsigned i=0;i<1000&&state.holding_authority;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));assert(!state.holding_authority);
    state.manager_mode=13;state.hold_manager=true;state.execution_gate.request_timeout_usec=200000;assert(control()==0&&state.manager_calls==17&&state.holding_manager);{std::lock_guard guard(state.lock);assert(state.execution.dispatch_attempted&&state.execution.state==PrivilegedExecutionState::dispatch_unknown);}state.hold_manager=false;state.execution_gate.request_timeout_usec=5000000;state.manager_mode=0;
    assert(control()==1&&state.manager_calls==18);{std::lock_guard guard(state.lock);assert(state.execution.budget->used()<=96);}
    assert(state.checks==31);state.compromise_directory=true;assert(invoke(1,1,"private.service",1)==0&&state.checks==32);
    {std::lock_guard guard(state.lock);assert(state.last.check_attempted&&!state.last.authorization.value&&state.last.state==PrivilegedAuthorizationState::stale);}
    assert(chmod(directory.c_str(),0700)==0);
    stop=true;serving.join();authorizing.join();managing.join();assert(!connection->native_handle()&&connection->generation()==0);assert(pending->snapshot().state==PrivilegedJobState::unknown);assert(completed->snapshot().state==PrivilegedJobState::completed);
    }
    if(admission_case||transport_case){stop=true;serving.join();authorizing.join();managing.join();}
    a.unref(client);if(second_client)a.unref(second_client);a.unref(auth);a.unref(service_manager);connection->disconnect();assert(close(anchor)==0);assert(unlink(policy_file.c_str())==0);
    }assert(fds()==initial);alarm(0);
}
void real_transport(const char* executable,bool admission_case,const std::string& limits_file){
    alarm(25);bool passed{};
    {Broker broker(executable,limits_file);const auto worker=fork();assert(worker>=0);if(worker==0){exercise_transport(broker.path,admission_case,!limits_file.empty());_exit(0);}int status{};assert(waitpid(worker,&status,0)==worker);passed=WIFEXITED(status)&&WEXITSTATUS(status)==0;}
    // Parent cleans private broker and files even if the assertion worker failed.
    alarm(0);assert(passed);
}
}
#endif
int main(int argc,char** argv){
    const auto no=pcinsight::detail::receive_native_privileged_request(nullptr,{},{});assert(no.state==pcinsight::detail::PrivilegedAuthorizationState::rejected&&!no.check_attempted&&!no.authorization.value);
#ifdef __linux__
    if(argc>=2)real_transport(argv[1],argc==3&&std::string(argv[2])=="--admission",argc==4&&std::string(argv[2])=="--transport"?argv[3]:"");
#else
    (void)argc;(void)argv;
#endif
}
