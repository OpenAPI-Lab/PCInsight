// Isolated libsystemd ABI fixture. Real private Unix socket, NO host bus/polkit.
#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
struct Message;
struct Bus {int listener{-1},client{-1},server{-1};std::string path,address,mode;unsigned calls{},owners{},ids{},caller_uids{},authority_uids{},authcalls{};Message* incoming{};};
struct Message {Bus* bus{};std::string kind,text,signature,sender;std::uint32_t uid{};std::uint64_t cookie{},reply_cookie{};std::vector<std::string> wire;std::vector<std::pair<std::string,std::string>> details;unsigned depth{},index{};};
struct Error{const char* name{};const char* message{};int need_free{};};
extern "C" {
Message* identity_fixture_create(const char* path,const char* mode){
    auto* b=new Bus;b->path=path;b->address="unix:path="+b->path;b->mode=mode;
    b->listener=socket(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0);b->client=socket(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0);assert(b->listener>=0&&b->client>=0);
    sockaddr_un a{};a.sun_family=AF_UNIX;assert(b->path.size()<sizeof a.sun_path);std::memcpy(a.sun_path,path,b->path.size()+1);assert(bind(b->listener,reinterpret_cast<sockaddr*>(&a),sizeof a)==0&&listen(b->listener,1)==0&&connect(b->client,reinterpret_cast<sockaddr*>(&a),sizeof a)==0);b->server=accept4(b->listener,nullptr,nullptr,SOCK_CLOEXEC);assert(b->server>=0);
    b->incoming=new Message{b,"incoming",{}, {},":1.23",0};return b->incoming;
}
void identity_fixture_destroy(Message* m){auto* b=m->bus;close(b->listener);close(b->client);close(b->server);unlink(b->path.c_str());delete m;delete b;}
unsigned identity_fixture_calls(Message* m){return m->bus->calls;}
unsigned identity_fixture_authcalls(Message* m){return m->bus->authcalls;}
Bus* sd_bus_message_get_bus(Message* m){return m->bus;}
Message* sd_bus_get_current_message(Bus* b){return b->mode=="not-current"?nullptr:b->incoming;}
const char* sd_bus_message_get_sender(Message* m){if(m->kind=="incoming"&&m->bus->mode=="bad-sender")return "org.client.Payload";return m->sender.c_str();}
const char* sd_bus_message_get_destination(Message* m){return m->bus->mode=="wrong-destination"?":1.88":":1.50";}
int sd_bus_message_get_type(Message* m,std::uint8_t* type){*type=m->kind=="incoming"?1:2;if(m->bus->mode=="wrong-type"||(m->kind=="auth-reply"&&m->bus->mode=="auth-reply-type"))*type=4;return 0;}
int sd_bus_get_address(Bus* b,const char** s){*s=b->mode=="wrong-address"?"unix:path=/another/bus":b->address.c_str();return 0;}
int sd_bus_get_unique_name(Bus*,const char** s){*s=":1.50";return 0;}
int sd_bus_get_fd(Bus* b){return b->client;}
int sd_bus_is_ready(Bus* b){return b->mode=="disconnected"?0:1;}
int sd_bus_is_bus_client(Bus* b){return b->mode=="not-client"?0:1;}
int sd_bus_get_method_call_timeout(Bus* b,std::uint64_t* timeout){*timeout=b->mode=="unbounded"?0:5000000;return 0;}
int sd_bus_call_method(Bus* b,const char* dest,const char* path,const char* iface,const char* method,Error*,Message** out,const char* signature,...){
    ++b->calls;assert(std::string(dest)=="org.freedesktop.DBus"&&std::string(path)=="/org/freedesktop/DBus"&&std::string(iface)=="org.freedesktop.DBus");
    if(b->mode=="missing-authority"&&std::string(method)=="GetNameOwner")return -ENOENT;
    auto* r=new Message{b,"reply",{},"s","org.freedesktop.DBus",0};*out=r;
    va_list args;va_start(args,signature);
    if(std::string(method)=="GetId"){assert(std::string(signature).empty());++b->ids;r->text="0123456789abcdef0123456789abcdef";if(b->mode=="changed-id"&&b->ids>1)r->text[0]='f';}
    else if(std::string(method)=="GetNameOwner"){assert(std::string(signature)=="s"&&std::string(va_arg(args,const char*))=="org.freedesktop.PolicyKit1");++b->owners;r->text=(b->mode=="changed-owner"&&b->owners>1)||(b->mode=="auth-owner-change"&&b->authcalls)?":1.8":":1.7";}
    else {assert(std::string(method)=="GetConnectionUnixUser"&&std::string(signature)=="s");std::string name=va_arg(args,const char*);r->signature="u";if(name==":1.23"){++b->caller_uids;r->uid=1000+(b->mode=="changed-caller"&&b->caller_uids>1?1:0);}else{assert(name==":1.7"||name==":1.8");++b->authority_uids;r->uid=100+(b->mode=="changed-authority-uid"&&b->authority_uids>1?1:0);}}
    va_end(args);if(b->mode=="spoof-reply")r->sender=":1.99";if(b->mode=="bad-signature")r->signature="su";if(b->mode=="truncated")r->kind="truncated";return 1;
}
int sd_bus_message_has_signature(Message* m,const char* signature){return m->signature==signature;}
int sd_bus_message_read(Message* m,const char* signature,...){if(m->kind=="truncated")return 0;va_list args;va_start(args,signature);
    if(m->kind=="auth-reply"){
        if(std::string(signature)=="bb"){assert(m->depth==1);if(m->bus->mode=="auth-truncated"){va_end(args);return 0;}*va_arg(args,int*)=m->bus->mode=="auth-deny"?0:m->bus->mode=="auth-bad-bool"?2:1;*va_arg(args,int*)=m->bus->mode=="auth-challenge"?1:0;}
        else {assert(std::string(signature)=="ss"&&m->depth==3&&m->index<m->details.size());*va_arg(args,const char**)=m->details[m->index].first.c_str();*va_arg(args,const char**)=m->details[m->index].second.c_str();}
    }else{assert(m->signature==signature);if(m->signature=="s")*va_arg(args,const char**)=m->text.c_str();else *va_arg(args,std::uint32_t*)=m->uid;}
    va_end(args);return 1;}
int sd_bus_message_new_method_call(Bus* b,Message** m,const char* destination,const char* path,const char* interface,const char* method){assert(std::string(destination)==":1.7"&&std::string(path)=="/org/freedesktop/PolicyKit1/Authority"&&std::string(interface)=="org.freedesktop.PolicyKit1.Authority"&&std::string(method)=="CheckAuthorization");*m=new Message{b,"outgoing",{}, {},{},0};return 0;}
int sd_bus_message_set_auto_start(Message* m,int flag){assert(flag==0);m->wire.push_back("autostart0");return 0;}
int sd_bus_message_set_allow_interactive_authorization(Message* m,int flag){assert(flag==0);m->wire.push_back("bus-interactive0");return 0;}
int sd_bus_message_open_container(Message* m,char type,const char* contents){if(m->bus->mode=="auth-build-fail")return -EINVAL;m->wire.push_back(std::string(1,type)+":"+contents);return 0;}
int sd_bus_message_close_container(Message* m){m->wire.push_back("close");return 0;}
int sd_bus_message_append(Message* m,const char* types,...){va_list args;va_start(args,types);std::string s=types;if(s=="s")m->wire.push_back("s:"+std::string(va_arg(args,const char*)));else if(s=="ss"){std::string key=va_arg(args,const char*);std::string value=va_arg(args,const char*);m->wire.push_back("ss:"+key+"="+value);}else{assert(s=="us");unsigned flags=va_arg(args,unsigned);const auto* cancellation=va_arg(args,const char*);assert(std::string(cancellation).empty());m->wire.push_back("flags:"+std::to_string(flags));}va_end(args);return 0;}
int sd_bus_call(Bus* b,Message* outgoing,std::uint64_t timeout,Error*,Message** reply){
    ++b->authcalls;assert(timeout==5000000);const std::vector<std::string> expected{"autostart0","bus-interactive0","r:sa{sv}","s:system-bus-name","a:{sv}","e:sv","s:name","v:s","s::1.23","close","close","close","close","s:org.pcinsight.helper.systemd.start","a:{ss}","e:ss","ss:unit=private.service","close","e:ss","ss:verb=start","close","close",b->mode=="auth-interaction"?"flags:1":"flags:0"};assert(outgoing->wire==expected);outgoing->cookie=77;
    if(b->mode=="auth-call-error")return -ETIMEDOUT;
    auto* r=new Message{b,"auth-reply",{},"(bba{ss})",":1.7",0};*reply=r;r->reply_cookie=b->mode=="auth-bad-cookie"?78:77;
    if(b->mode=="auth-spoof")r->sender=":1.99";if(b->mode=="auth-signature")r->signature="bba{ss}";
    if(b->mode=="auth-dismissed")r->details={{"polkit.dismissed","1"}};if(b->mode=="auth-retained")r->details={{"polkit.retains_authorization_after_challenge","1"}};if(b->mode=="auth-temporary")r->details={{"polkit.temporary_authorization_id","opaque"}};
    if(b->mode=="auth-duplicate")r->details={{"x","one"},{"x","two"}};if(b->mode=="auth-long-key")r->details={{std::string(129,'x'),"v"}};if(b->mode=="auth-long-value")r->details={{"x",std::string(4097,'v')}};
    if(b->mode=="auth-many")for(unsigned i=0;i<65;++i)r->details.push_back({std::to_string(i),"v"});if(b->mode=="auth-payload")for(unsigned i=0;i<5;++i)r->details.push_back({std::to_string(i),std::string(4096,'v')});
    if(b->mode=="auth-max-details")for(unsigned i=0;i<64;++i)r->details.push_back({std::to_string(i),"v"});
    if(b->mode=="auth-max-payload")for(unsigned i=0;i<4;++i)r->details.push_back({std::to_string(i),std::string(4095,'v')});
    if(b->mode=="auth-unknown")r->details={{"extension","ignored"}};return 0;
}
int sd_bus_message_get_cookie(Message* m,std::uint64_t* c){*c=m->cookie;return 0;}
int sd_bus_message_get_reply_cookie(Message* m,std::uint64_t* c){*c=m->reply_cookie;return 0;}
int sd_bus_message_enter_container(Message* m,char type,const char* contents){assert(m->kind=="auth-reply");if(type=='r'){assert(m->depth==0&&std::string(contents)=="bba{ss}");if(m->bus->mode=="auth-no-struct")return 0;m->depth=1;return 1;}if(type=='a'){assert(m->depth==1&&std::string(contents)=="{ss}");if(m->bus->mode=="auth-no-array")return 0;m->depth=2;return 1;}assert(type=='e'&&m->depth==2&&std::string(contents)=="ss");if(m->bus->mode=="auth-bad-entry")return -EINVAL;if(m->index>=m->details.size())return 0;m->depth=3;return 1;}
int sd_bus_message_exit_container(Message* m){assert(m->depth);if(m->depth==3)++m->index;--m->depth;return 0;}
int sd_bus_message_at_end(Message* m,int complete){assert(complete==1&&m->depth==0);return m->bus->mode=="auth-trailing"?0:1;}
Message* sd_bus_message_unref(Message* m){assert(m->kind!="incoming");delete m;return nullptr;}
void sd_bus_error_free(Error*){}
}
