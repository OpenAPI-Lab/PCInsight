#include "linux_privileged_identity.hpp"
#include "linux_privileged_execution.hpp"
#include "linux_network_selection.hpp"
#include <cstring>
#include <stdexcept>
#include <set>
#ifdef __linux__
#include <cerrno>
#include <dlfcn.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T>Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
#ifdef __linux__
struct Bus;struct Message;struct Error{const char* name{};const char* message{};int need_free{};};
std::string text(const char* s){if(!s)throw std::runtime_error("Missing bus metadata");const auto n=::strnlen(s,256);if(n>255)throw std::runtime_error("Excessive bus metadata");return {s,n};}
bool unique(const std::string& s){const auto dot=s.find('.',1);if(s.size()<4||s[0]!=':'||dot==s.npos||dot==1||dot+1==s.size())return false;for(std::size_t i=1;i<s.size();++i)if(i!=dot&&(s[i]<'0'||s[i]>'9'))return false;return true;}
struct Api {
    std::shared_ptr<PrivilegedRequestBudget> budget;
    void* library=::dlopen("libsystemd.so.0",RTLD_NOW|RTLD_LOCAL);
    Bus* (*bus)(Message*){};Message* (*current)(Bus*){};const char* (*sender)(Message*){};const char* (*destination)(Message*){};
    int (*type)(Message*,std::uint8_t*){},(*address)(Bus*,const char**) {},(*own)(Bus*,const char**) {},(*fd)(Bus*){},(*ready)(Bus*){},(*client)(Bus*){},(*timeout)(Bus*,std::uint64_t*){};
    int (*call)(Bus*,const char*,const char*,const char*,const char*,Error*,Message**,const char*,...){};
    int (*read)(Message*,const char*,...){};int (*signature)(Message*,const char*){};Message* (*unref)(Message*){};void (*free_error)(Error*){};
    int (*new_call)(Bus*,Message**,const char*,const char*,const char*,const char*){},(*append)(Message*,const char*,...){};
    int (*open_container)(Message*,char,const char*){},(*close_container)(Message*){},(*enter)(Message*,char,const char*){},(*exit)(Message*){},(*at_end)(Message*,int){};
    int (*auto_start)(Message*,int){},(*interactive)(Message*,int){},(*invoke)(Bus*,Message*,std::uint64_t,Error*,Message**){};
    int (*cookie)(Message*,std::uint64_t*){},(*reply_cookie)(Message*,std::uint64_t*){};
    const char* (*path)(Message*){};const char* (*interface)(Message*){};const char* (*member)(Message*){};int (*rewind)(Message*,int){};
    int (*set_timeout)(Bus*,std::uint64_t){};
    explicit Api(std::shared_ptr<PrivilegedRequestBudget> b={}):budget(std::move(b)){if(!library)return;
#define LOAD(field,name) field=reinterpret_cast<decltype(field)>(::dlsym(library,name))
        LOAD(bus,"sd_bus_message_get_bus");LOAD(current,"sd_bus_get_current_message");LOAD(sender,"sd_bus_message_get_sender");LOAD(destination,"sd_bus_message_get_destination");LOAD(type,"sd_bus_message_get_type");LOAD(address,"sd_bus_get_address");LOAD(own,"sd_bus_get_unique_name");LOAD(fd,"sd_bus_get_fd");LOAD(ready,"sd_bus_is_ready");LOAD(client,"sd_bus_is_bus_client");LOAD(timeout,"sd_bus_get_method_call_timeout");LOAD(call,"sd_bus_call_method");LOAD(read,"sd_bus_message_read");LOAD(signature,"sd_bus_message_has_signature");LOAD(unref,"sd_bus_message_unref");LOAD(free_error,"sd_bus_error_free");
        LOAD(new_call,"sd_bus_message_new_method_call");LOAD(append,"sd_bus_message_append");LOAD(open_container,"sd_bus_message_open_container");LOAD(close_container,"sd_bus_message_close_container");LOAD(enter,"sd_bus_message_enter_container");LOAD(exit,"sd_bus_message_exit_container");LOAD(at_end,"sd_bus_message_at_end");LOAD(auto_start,"sd_bus_message_set_auto_start");LOAD(interactive,"sd_bus_message_set_allow_interactive_authorization");LOAD(invoke,"sd_bus_call");LOAD(cookie,"sd_bus_message_get_cookie");LOAD(reply_cookie,"sd_bus_message_get_reply_cookie");
        LOAD(path,"sd_bus_message_get_path");LOAD(interface,"sd_bus_message_get_interface");LOAD(member,"sd_bus_message_get_member");LOAD(rewind,"sd_bus_message_rewind");
        LOAD(set_timeout,"sd_bus_set_method_call_timeout");
#undef LOAD
    }
    ~Api(){if(library)::dlclose(library);}Api(const Api&)=delete;
    explicit operator bool()const{return bus&&current&&sender&&destination&&type&&address&&own&&fd&&ready&&client&&timeout&&call&&read&&signature&&unref&&free_error;}
    bool authorization()const{return bool(*this)&&new_call&&append&&open_container&&close_container&&enter&&exit&&at_end&&auto_start&&interactive&&invoke&&cookie&&reply_cookie;}
    bool receiver()const{return bool(*this)&&path&&interface&&member&&rewind&&at_end;}
};
struct Reply{Api& a;Message* m{};Error error{};~Reply(){if(m)a.unref(m);a.free_error(&error);}};
struct Peer {dev_t dev{};ino_t ino{};pid_t pid{};uid_t uid{};gid_t gid{};bool operator==(const Peer&)const=default;};
Peer peer(int fd,const PrivilegedIdentityConfig& c){
    struct stat st{};struct ucred creds{};socklen_t n=sizeof(creds);sockaddr_un address{};socklen_t length=sizeof(address);int kind{};socklen_t k=sizeof(kind);
    if(fd<0||::fstat(fd,&st)||!S_ISSOCK(st.st_mode)||::getsockopt(fd,SOL_SOCKET,SO_TYPE,&kind,&k)||kind!=SOCK_STREAM||::getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&creds,&n)||n!=sizeof(creds)||creds.pid<=0||creds.uid!=*c.bus_uid||::getpeername(fd,reinterpret_cast<sockaddr*>(&address),&length)||address.sun_family!=AF_UNIX||length<=offsetof(sockaddr_un,sun_path)||address.sun_path[0]=='\0')throw std::runtime_error("Untrusted/missing Unix bus peer");
    if(std::string(address.sun_path,::strnlen(address.sun_path,sizeof address.sun_path))!=c.socket_path)throw std::runtime_error("Bus peer endpoint differs from mechanism configuration");
    return {st.st_dev,st.st_ino,creds.pid,creds.uid,creds.gid};
}
void check_reply(Api& a,Reply& r,int rc,const char* sig){if(rc<0)throw std::runtime_error("Bus identity query failed/disconnected/denied");std::uint8_t kind{};if(!r.m||a.type(r.m,&kind)<0||kind!=2||text(a.sender(r.m))!="org.freedesktop.DBus"||a.signature(r.m,sig)<=0)throw std::runtime_error("Invalid bus daemon reply type/sender/signature");}
std::string query_text(Api& a,Bus* b,const char* method,const char* argument=nullptr){PrivilegedRpcTimeout lease(b,a.timeout,a.set_timeout,a.budget);Reply r{a};const int rc=argument?a.call(b,"org.freedesktop.DBus","/org/freedesktop/DBus","org.freedesktop.DBus",method,&r.error,&r.m,"s",argument):a.call(b,"org.freedesktop.DBus","/org/freedesktop/DBus","org.freedesktop.DBus",method,&r.error,&r.m,"");check_reply(a,r,rc,"s");const char* s{};if(a.read(r.m,"s",&s)<=0)throw std::runtime_error("Missing bus string value");return text(s);}
std::uint32_t query_uid(Api& a,Bus* b,const std::string& name){PrivilegedRpcTimeout lease(b,a.timeout,a.set_timeout,a.budget);Reply r{a};const int rc=a.call(b,"org.freedesktop.DBus","/org/freedesktop/DBus","org.freedesktop.DBus","GetConnectionUnixUser",&r.error,&r.m,"s",name.c_str());check_reply(a,r,rc,"u");std::uint32_t uid{};if(a.read(r.m,"u",&uid)<=0||uid==0xffffffffU)throw std::runtime_error("Missing/invalid bus UID");return uid;}
#endif
}
Metric<PrivilegedAuthorizationContext> observe_native_privileged_identity(void* message,const PrivilegedIdentityConfig& c){
    if(!message||!c.bus_uid||!c.authority_uid||*c.bus_uid==0xffffffffU||*c.authority_uid==0xffffffffU||!c.connection_generation||!c.policy_generation||c.socket_path.empty()||c.socket_path[0]!='/'||c.socket_path.size()>107||c.socket_path.find('\0')!=c.socket_path.npos)return absent<PrivilegedAuthorizationContext>("Invalid incoming message/mechanism trust configuration; no query");
#ifdef __linux__
    try{
        Api a{c.budget};if(!a|| (c.budget&&!c.budget->live()))return absent<PrivilegedAuthorizationContext>("Required native symbols/request time unavailable");
        const auto source=native_network_selection_source_api();const auto before=source.observe(source.context);const auto euid=::geteuid();if(!before.value||before.support!=Support::available)return absent<PrivilegedAuthorizationContext>("Caller context unavailable");
        auto* m=static_cast<Message*>(message);auto* b=a.bus(m);std::uint8_t type{};const char *address{},*own{};std::uint64_t timeout{};
        if(!b||a.current(b)!=m||a.type(m,&type)<0||type!=1||a.ready(b)<=0||a.client(b)<=0||a.address(b,&address)<0||text(address)!="unix:path="+c.socket_path||a.own(b,&own)<0||!unique(text(own))||a.timeout(b,&timeout)<0||!timeout||timeout>5000000)throw std::runtime_error("Not current incoming dispatch/unready/non-client/wrong-address bus or unbounded query timeout");
        const auto own_name=text(own),sender=text(a.sender(m));if(!unique(sender)||text(a.destination(m))!=own_name)throw std::runtime_error("Incoming sender/destination not bound to receiver unique name");
        const auto first_peer=peer(a.fd(b),c);
        PrivilegedAuthorizationContext out;out.system_bus_id=query_text(a,b,"GetId");out.caller_unique_name=sender;out.caller_uid=query_uid(a,b,sender);out.authority_unique_owner=query_text(a,b,"GetNameOwner","org.freedesktop.PolicyKit1");
        if(!unique(out.authority_unique_owner)||out.authority_unique_owner==sender)throw std::runtime_error("Invalid Authority owner");
        out.authority_uid=query_uid(a,b,out.authority_unique_owner);if(out.authority_uid!=*c.authority_uid)throw std::runtime_error("Authority UID does not match mechanism configuration");
        if(out.system_bus_id.size()!=32||out.system_bus_id.find_first_not_of("0123456789abcdef")!=std::string::npos)throw std::runtime_error("Invalid bus ID");
        if(query_text(a,b,"GetId")!=out.system_bus_id||query_text(a,b,"GetNameOwner","org.freedesktop.PolicyKit1")!=out.authority_unique_owner||query_uid(a,b,sender)!=out.caller_uid||query_uid(a,b,out.authority_unique_owner)!=out.authority_uid||peer(a.fd(b),c)!=first_peer)throw std::runtime_error("Bus/caller/Authority identity changed");
        const auto after=source.observe(source.context);const char *last_own{},*last_address{};
        if(!after.value||after.support!=Support::available||*before.value!=*after.value||::geteuid()!=euid||a.current(b)!=m||a.ready(b)<=0||a.own(b,&last_own)<0||text(last_own)!=own_name||a.address(b,&last_address)<0||text(last_address)!="unix:path="+c.socket_path||text(a.sender(m))!=sender||text(a.destination(m))!=own_name)throw std::runtime_error("Native receiver/context changed");
        if(c.budget&&!c.budget->live())throw std::runtime_error("Request expired during identity observation");
        out.connection_generation=c.connection_generation;out.mechanism_policy_generation=c.policy_generation;
        return {out,Support::available,"Native borrowed-message identity observation; no authorization, activation or execution; trusted receiver/config/loader required"};
    }catch(const std::exception& e){return absent<PrivilegedAuthorizationContext>(e.what());}catch(...){return absent<PrivilegedAuthorizationContext>("Native identity observation failed");}
#else
    return absent<PrivilegedAuthorizationContext>("Native bus identity requires Linux");
#endif
}
Metric<PrivilegedAuthorizationReply> check_native_privileged_authorization(void* message,const PrivilegedIdentityConfig& c,const PrivilegedAuthorizationContext& expected,const PrivilegedServiceRequest& request,std::span<const PrivilegedServiceRule> rules,bool allow_interaction){
    const auto plan=preflight_privileged_service(request,rules);
    if(!metric_is_known(plan))return absent<PrivilegedAuthorizationReply>(plan.note,plan.support);
    const auto before=observe_native_privileged_identity(message,c);
    if(!before.value||before.support!=Support::available||*before.value!=expected)return absent<PrivilegedAuthorizationReply>("Native identity mismatch before authorization; no check sent");
#ifdef __linux__
    try{
        Api a{c.budget};if(!a.authorization())return absent<PrivilegedAuthorizationReply>("Native authorization symbols unavailable; no check sent");
        auto* bus=a.bus(static_cast<Message*>(message));Reply outgoing{a},reply{a};
        auto ok=[](int rc){if(rc<0)throw std::runtime_error("Native authorization message/container operation failed");};
        ok(a.new_call(bus,&outgoing.m,expected.authority_unique_owner.c_str(),"/org/freedesktop/PolicyKit1/Authority","org.freedesktop.PolicyKit1.Authority","CheckAuthorization"));
        if(!outgoing.m)throw std::runtime_error("Missing outgoing authorization message");
        ok(a.auto_start(outgoing.m,0));ok(a.interactive(outgoing.m,0));
        ok(a.open_container(outgoing.m,'r',"sa{sv}"));ok(a.append(outgoing.m,"s","system-bus-name"));ok(a.open_container(outgoing.m,'a',"{sv}"));ok(a.open_container(outgoing.m,'e',"sv"));ok(a.append(outgoing.m,"s","name"));ok(a.open_container(outgoing.m,'v',"s"));ok(a.append(outgoing.m,"s",expected.caller_unique_name.c_str()));ok(a.close_container(outgoing.m));ok(a.close_container(outgoing.m));ok(a.close_container(outgoing.m));ok(a.close_container(outgoing.m));
        ok(a.append(outgoing.m,"s",plan.value->policy_action.c_str()));
        ok(a.open_container(outgoing.m,'a',"{ss}"));
        const char* verb=plan.value->operation==ServiceAction::start?"start":plan.value->operation==ServiceAction::stop?"stop":"restart";
        for(const auto& entry:{std::pair<const char*,const char*>{"unit",plan.value->unit.c_str()},{"verb",verb}}){ok(a.open_container(outgoing.m,'e',"ss"));ok(a.append(outgoing.m,"ss",entry.first,entry.second));ok(a.close_container(outgoing.m));}
        ok(a.close_container(outgoing.m));ok(a.append(outgoing.m,"us",allow_interaction?1U:0U,""));
        const int rc=a.invoke(bus,outgoing.m,privileged_rpc_timeout(c.budget),&reply.error,&reply.m);
        if(rc<0)throw std::runtime_error("CheckAuthorization failed/timed out; no retry or authorization accepted");
        PrivilegedAuthorizationReply out;std::uint8_t type{};
        if(!reply.m||a.type(reply.m,&type)<0||type!=2||text(a.sender(reply.m))!=expected.authority_unique_owner||a.signature(reply.m,"(bba{ss})")<=0||a.cookie(outgoing.m,&out.call_serial)<0||!out.call_serial||a.reply_cookie(reply.m,&out.reply_to_serial)<0||out.call_serial!=out.reply_to_serial)throw std::runtime_error("Unbound/malformed authorization reply sender/type/signature/cookie");
        out.sender_unique_owner=expected.authority_unique_owner;
        if(a.enter(reply.m,'r',"bba{ss}")<=0)throw std::runtime_error("Missing authorization result struct");
        int authorized=-1,challenge=-1;if(a.read(reply.m,"bb",&authorized,&challenge)<=0||(authorized!=0&&authorized!=1)||(challenge!=0&&challenge!=1))throw std::runtime_error("Missing/invalid authorization booleans");out.is_authorized=authorized;out.is_challenge=challenge;
        if(a.enter(reply.m,'a',"{ss}")<=0)throw std::runtime_error("Missing authorization details array");
        std::set<std::string> seen;std::size_t count=0,bytes=0;
        for(;;){const auto entry=a.enter(reply.m,'e',"ss");if(entry<0)throw std::runtime_error("Malformed authorization detail entry");if(!entry)break;if(++count>64)throw std::runtime_error("Authorization detail bound64 exceeded");const char *key{},*value{};if(a.read(reply.m,"ss",&key,&value)<=0||!key||!value)throw std::runtime_error("Missing authorization detail");
            const auto key_size=::strnlen(key,129),value_size=::strnlen(value,4097);if(!key_size||key_size>128||value_size>4096||key_size+value_size>16384-bytes)throw std::runtime_error("Authorization detail text/payload bound exceeded");bytes+=key_size+value_size;std::string k(key,key_size),v(value,value_size);if(!seen.insert(k).second)throw std::runtime_error("Duplicate authorization detail key");
            if(k=="polkit.dismissed")out.dismissed=!v.empty();else if(k=="polkit.retains_authorization_after_challenge")out.retains_authorization=!v.empty();else if(k=="polkit.temporary_authorization_id")out.temporary_authorization_id=std::move(v);
            ok(a.exit(reply.m));
        }
        ok(a.exit(reply.m));ok(a.exit(reply.m));if(a.at_end(reply.m,1)<=0)throw std::runtime_error("Trailing authorization result data");
        const auto after=observe_native_privileged_identity(message,c);if(!after.value||after.support!=Support::available||*after.value!=expected)throw std::runtime_error("Native identity changed after authorization; result discarded");
        return {std::move(out),Support::available,"Bound native CheckAuthorization reply only, not execution/grant; apply coordinator rejection policy"};
    }catch(const std::exception& e){return absent<PrivilegedAuthorizationReply>(e.what());}catch(...){return absent<PrivilegedAuthorizationReply>("Native authorization failure");}
#else
    (void)allow_interaction;return absent<PrivilegedAuthorizationReply>("Native authorization requires Linux");
#endif
}
PrivilegedAuthorizationApi native_privileged_authorization_api(void* message,PrivilegedIdentityConfig config,PrivilegedServiceRequest request,std::vector<PrivilegedServiceRule> rules,bool permit_interaction){
    const auto snapshot=preflight_privileged_service(request,rules);
    return {[message,config]{return observe_native_privileged_identity(message,config);},[message,config,request,rules=std::move(rules),snapshot,permit_interaction](const auto& context,const auto& plan,bool interaction){
        if(!metric_is_known(snapshot)||plan.unit!=snapshot.value->unit||plan.operation!=snapshot.value->operation||plan.policy_action!=snapshot.value->policy_action)return absent<PrivilegedAuthorizationReply>("Coordinator request does not match frozen native adapter request");
        return check_native_privileged_authorization(message,config,context,request,rules,interaction&&permit_interaction);
    },config.authority_uid};
}
Metric<PrivilegedServiceRequest> decode_native_privileged_request(void* message,PrivilegedRequestKind kind){
    if(!message||(kind!=PrivilegedRequestKind::authorization_only&&kind!=PrivilegedRequestKind::control))return absent<PrivilegedServiceRequest>("Missing request or invalid closed request kind");
#ifdef __linux__
    try{
        Api a;if(!a.receiver())return absent<PrivilegedServiceRequest>("Native closed receiver symbols unavailable");
        auto* m=static_cast<Message*>(message);auto* bus=a.bus(m);std::uint8_t type{};
        const auto* member=kind==PrivilegedRequestKind::control?"ControlSystemService":"AuthorizeSystemService";
        if(!bus||a.current(bus)!=m||a.type(m,&type)<0||type!=1||text(a.path(m))!="/org/pcinsight/Helper"||text(a.interface(m))!="org.pcinsight.Helper1"||text(a.member(m))!=member||a.signature(m,"uusb")<=0)return absent<PrivilegedServiceRequest>("Wrong current dispatch/method/path/interface/signature; no authorization query");
        std::uint32_t version{},operation{};const char* unit{};int intent=-1;
        if(a.rewind(m,1)<0||a.read(m,"uusb",&version,&operation,&unit,&intent)<=0||a.at_end(m,1)<=0||version!=1||operation<1||operation>3||(intent!=0&&intent!=1)||!unit)return absent<PrivilegedServiceRequest>("Invalid/truncated/version/operation/intent/trailing request");
        const auto size=::strnlen(unit,256);if(size>255)return absent<PrivilegedServiceRequest>("Request unit bound255 exceeded");
        return {PrivilegedServiceRequest{std::string(unit,size),operation==1?ServiceAction::start:operation==2?ServiceAction::stop:ServiceAction::restart,intent==1},Support::available,"Closed request decoded; no authorization/execution"};
    }catch(const std::exception& e){return absent<PrivilegedServiceRequest>(e.what());}catch(...){return absent<PrivilegedServiceRequest>("Native decoder failed");}
#else
    return absent<PrivilegedServiceRequest>("Native decoder requires Linux");
#endif
}
static PrivilegedAuthorizationObservation receive_native_request_impl(void* message,PrivilegedIdentityConfig config,std::vector<PrivilegedServiceRule> rules,bool permit_interaction,const std::function<bool()>& current){
    PrivilegedAuthorizationObservation out;
    auto reject=[&](std::string note){out.state=PrivilegedAuthorizationState::rejected;out.note=std::move(note);out.authorization={{},Support::unsupported,out.note};};
    try{
        const auto decoded=decode_native_privileged_request(message,PrivilegedRequestKind::authorization_only);if(!metric_is_known(decoded)){reject(decoded.note);return out;}const auto request=*decoded.value;
        out.preflight=preflight_privileged_service(request,rules);
        if(!metric_is_known(out.preflight)){out.state=PrivilegedAuthorizationState::rejected;out.note=out.preflight.note;out.authorization={{},out.preflight.support,out.note};return out;}
        auto api=native_privileged_authorization_api(message,std::move(config),request,rules,permit_interaction);
        if(current){const auto observe=api.observe;const auto check=api.check;
            api.observe=[observe,current]{if(!current())return absent<PrivilegedAuthorizationContext>("Policy generation no longer current");auto identity=observe();if(!current())return absent<PrivilegedAuthorizationContext>("Policy changed during identity observation");return identity;};
            api.check=[check,current](const auto& context,const auto& plan,bool interaction){if(!current())return absent<PrivilegedAuthorizationReply>("Policy invalidated before check");auto reply=check(context,plan,interaction);if(!current())return absent<PrivilegedAuthorizationReply>("Policy changed during authorization; reply discarded");return reply;};
        }
        return observe_privileged_authorization(request,rules,api);
    }catch(const std::exception& e){reject(e.what());return out;}catch(...){reject("Native receiver failed; no authorization accepted");return out;}
}
PrivilegedAuthorizationObservation receive_native_privileged_request(void* message,PrivilegedIdentityConfig config,std::vector<PrivilegedServiceRule> rules,bool interaction){return receive_native_request_impl(message,std::move(config),std::move(rules),interaction,{});}
PrivilegedAuthorizationObservation receive_native_privileged_policy_request(void* message,PrivilegedIdentityConfig config,const PrivilegedPolicyStore& policy,bool interaction){
    const auto snapshot=policy.snapshot();
    if(!metric_is_known(snapshot)){PrivilegedAuthorizationObservation out;out.state=PrivilegedAuthorizationState::unavailable;out.note=snapshot.note;out.authorization=absent<bool>(snapshot.note,snapshot.support);return out;}
    const auto generation=snapshot.value->generation;config.policy_generation=generation;
    return receive_native_request_impl(message,std::move(config),snapshot.value->rules,interaction,[&policy,generation]{return policy.is_current(generation);});
}
PrivilegedExecutionResult execute_owned_privileged_request(void* message,const PrivilegedBusConnection& connection,std::uint32_t authority,const PrivilegedPolicyStore& policy,PrivilegedExecutionGate& gate){
    PrivilegedExecutionResult out;
    auto fail=[&](PrivilegedExecutionState state,std::string note){out.state=state;out.note=std::move(note);out.job_path=absent<std::string>(out.note);if(out.job)out.job->invalidate(out.note);};
    if(!gate.enabled.load()){fail(PrivilegedExecutionState::rejected,"Mechanism execution switch OFF; no query or dispatch");return out;}
    if(gate.busy.test_and_set()){fail(PrivilegedExecutionState::rejected,"Execution already in flight; no reentrant dispatch");return out;}
    struct Release{std::atomic_flag& flag;~Release(){flag.clear();}}release{gate.busy};
    const auto admitted=connection.admit_request(message);if(admitted.value!=true){fail(PrivilegedExecutionState::rejected,admitted.note);return out;}
    const auto manager_uid=gate.manager_uid.load();const auto interaction=gate.permit_interaction.load();const auto job_timeout=gate.job_timeout_usec.load();
    const auto request_timeout=gate.request_timeout_usec.load();const auto rpc_limit=gate.rpc_limit.load();out.budget=std::make_shared<PrivilegedRequestBudget>(request_timeout,rpc_limit);
    if(!out.budget->live()){fail(PrivilegedExecutionState::rejected,"Invalid request budget configuration");return out;}
    if(manager_uid==UINT32_MAX||job_timeout<1000||job_timeout>30000000){fail(PrivilegedExecutionState::rejected,"Missing trusted Manager UID or invalid job timeout");return out;}
    try{
        const auto decoded=decode_native_privileged_request(message,PrivilegedRequestKind::control);
        const auto snapshot=policy.snapshot();if(!metric_is_known(decoded)||!metric_is_known(snapshot)){fail(PrivilegedExecutionState::rejected,"Closed control request or current policy unavailable");return out;}
        const auto request=*decoded.value;const auto rules=snapshot.value->rules;const auto epoch=snapshot.value->generation;
        const auto preflight=preflight_privileged_service(request,rules);if(!metric_is_known(preflight)){fail(PrivilegedExecutionState::rejected,preflight.note);return out;}
        auto config=connection.identity_config(authority);config.policy_generation=epoch;config.budget=out.budget;
        const auto handle=connection.native_handle();const auto generation=config.connection_generation;
#ifdef __linux__
        Api a{config.budget};auto* m=static_cast<Message*>(message);auto* bus=static_cast<Bus*>(handle);
        if(!handle||!a.authorization()||a.bus(m)!=bus){fail(PrivilegedExecutionState::unavailable,"No matching owned native connection");return out;}
        auto current=[&]{return out.budget->live()&&gate.enabled.load()&&gate.manager_uid.load()==manager_uid&&gate.permit_interaction.load()==interaction&&gate.job_timeout_usec.load()==job_timeout&&gate.request_timeout_usec.load()==request_timeout&&gate.rpc_limit.load()==rpc_limit&&policy.is_current(epoch)&&connection.native_handle()==handle&&connection.generation()==generation;};
        const auto initial=observe_native_privileged_identity(message,config);if(!metric_is_known(initial)||!current()){fail(PrivilegedExecutionState::unavailable,"Initial native context unavailable");return out;}
        auto manager=[&]{const auto name=query_text(a,bus,"GetNameOwner","org.freedesktop.systemd1");if(!unique(name)||name==initial.value->caller_unique_name||name==initial.value->authority_unique_owner||query_uid(a,bus,name)!=manager_uid)throw std::runtime_error("Untrusted Manager owner/UID");return name;};
        const auto owner=manager();
        auto api=native_privileged_authorization_api(message,config,request,rules,interaction);const auto base=api.observe;
        api.observe=[&]{if(!current())return absent<PrivilegedAuthorizationContext>("Execution policy/connection invalidated");auto observed=base();if(!metric_is_known(observed)||*observed.value!=*initial.value||!current())return absent<PrivilegedAuthorizationContext>("Execution identity/context changed");return observed;};
        const auto authorized=observe_privileged_authorization(request,rules,api);out.authorization_checked=authorized.check_attempted;
        if(authorized.state!=PrivilegedAuthorizationState::authorization_observed||authorized.authorization.value!=true){fail(authorized.state==PrivilegedAuthorizationState::denied?PrivilegedExecutionState::denied:PrivilegedExecutionState::stale,authorized.note);return out;}
        if(manager()!=owner||!metric_is_known(api.observe())||!current()){fail(PrivilegedExecutionState::stale,"Manager/caller/policy/connection changed before dispatch");return out;}
        const auto watched=connection.prepare_job_watch(owner,request.unit,job_timeout,out.budget);if(!metric_is_known(watched)){fail(PrivilegedExecutionState::unavailable,watched.note);return out;}out.job=(*watched.value)->observation();
        Reply outgoing{a},reply{a};auto ok=[](int rc){if(rc<0)throw std::runtime_error("Closed Manager message operation failed");};
        const char* method=request.operation==ServiceAction::start?"StartUnit":request.operation==ServiceAction::stop?"StopUnit":"RestartUnit";
        ok(a.new_call(bus,&outgoing.m,owner.c_str(),"/org/freedesktop/systemd1","org.freedesktop.systemd1.Manager",method));if(!outgoing.m)throw std::runtime_error("Missing Manager message");
        ok(a.auto_start(outgoing.m,0));ok(a.interactive(outgoing.m,0));ok(a.append(outgoing.m,"ss",request.unit.c_str(),"replace"));
        if(manager()!=owner||!metric_is_known(api.observe())||!current()||out.job->snapshot().state!=PrivilegedJobState::pending){fail(PrivilegedExecutionState::stale,"Last pre-dispatch Manager/identity/policy/connection/job deadline check failed");return out;}
        const auto dispatch_timeout=privileged_rpc_timeout(out.budget);
        out.dispatch_attempted=true; // From here errors cannot establish absence of effects.
        const auto rc=a.invoke(bus,outgoing.m,dispatch_timeout,&reply.error,&reply.m);std::uint64_t cookie{},reply_cookie{};std::uint8_t type{};
        if(rc<0||!reply.m||a.type(reply.m,&type)<0||type!=2||text(a.sender(reply.m))!=owner||a.signature(reply.m,"o")<=0||a.cookie(outgoing.m,&cookie)<0||!cookie||a.reply_cookie(reply.m,&reply_cookie)<0||cookie!=reply_cookie)throw std::runtime_error("Manager dispatch reply unavailable/unbound; outcome unknown, no retry");
        const char* path{};if(a.read(reply.m,"o",&path)<=0||!path||a.at_end(reply.m,1)<=0)throw std::runtime_error("Missing/trailing Manager receipt");
        const auto length=::strnlen(path,1025);std::string job(path,length);
        if(length>1024||!valid_privileged_job_path(job))throw std::runtime_error("Invalid canonical uint32 Manager job path; outcome unknown");
        if(!current()||manager()!=owner||!metric_is_known(api.observe()))throw std::runtime_error("Context changed after dispatch; outcome unknown, cannot roll back");
        out.job->select(job);out.state=PrivilegedExecutionState::job_queued;out.job_path={std::move(job),Support::available,"Correlated Manager job receipt only; async job observation determines completion, NOT current service state"};out.note=out.job_path.note;
#else
        (void)interaction;(void)config;(void)handle;(void)generation;fail(PrivilegedExecutionState::unavailable,"Native execution requires Linux");
#endif
    }catch(const std::exception& e){fail(out.dispatch_attempted?PrivilegedExecutionState::dispatch_unknown:PrivilegedExecutionState::unavailable,e.what());}catch(...){fail(out.dispatch_attempted?PrivilegedExecutionState::dispatch_unknown:PrivilegedExecutionState::unavailable,"Closed execution failed; no retry");}
    return out;
}
}
