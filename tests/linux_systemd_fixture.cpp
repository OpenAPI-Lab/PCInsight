// Isolated dynamic-loader fixture: never connects to the host's D-Bus.
#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
struct Message;struct Error;struct Slot{};
struct Bus{int sockets[2]{-1,-1};bool address{},client{},timeout{},interactive{},emitted{};unsigned owners{};
    int(*callback)(Message*,void*,Error*){};void* userdata{};};
struct Message{std::string kind;unsigned reads{};std::string text;};
struct Error{const char* name{};const char* message{};int need_free{};};
static std::string mode(){const auto* m=std::getenv("PCINSIGHT_SYSTEMD_FIXTURE");return m?m:"ok";}
extern "C" {
int sd_bus_open_system(Bus**){return -EACCES;}
int sd_bus_new(Bus** b){*b=new Bus;return socketpair(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0,(*b)->sockets)<0?-errno:0;}
Bus* sd_bus_unref(Bus* b){for(int fd:b->sockets)if(fd>=0)close(fd);delete b;return nullptr;}
int sd_bus_set_address(Bus* b,const char* s){b->address=s=="unix:path=/run/user/"+std::to_string(geteuid())+"/bus";assert(b->address);return 0;}
int sd_bus_set_bus_client(Bus* b,int c){assert(c==1);b->client=true;return 0;}
int sd_bus_set_method_call_timeout(Bus* b,std::uint64_t t){assert(t==5000000);b->timeout=true;return 0;}
int sd_bus_set_allow_interactive_authorization(Bus* b,int a){assert(a==0);b->interactive=true;return 0;}
int sd_bus_start(Bus* b){assert(b->address&&b->client&&b->timeout&&b->interactive);return mode()=="busdenied"?-EACCES:0;}
int sd_bus_get_fd(Bus* b){return b->sockets[0];}
Message* sd_bus_message_unref(Message* m){delete m;return nullptr;}
void sd_bus_error_free(Error*){}
Slot* sd_bus_slot_unref(Slot* s){delete s;return nullptr;}
int sd_bus_match_signal(Bus* b,Slot** slot,const char* sender,const char* path,const char* iface,const char* signal,
    int(*callback)(Message*,void*,Error*),void* userdata){
    assert(std::string(sender)==":1.77"&&std::string(path)=="/org/freedesktop/systemd1"&&
        std::string(iface)=="org.freedesktop.systemd1.Manager"&&std::string(signal)=="JobRemoved");
    b->callback=callback;b->userdata=userdata;*slot=new Slot;return 0;
}
int sd_bus_process(Bus* b,Message**){
    if(mode()=="waiterror")return -ECONNRESET;
    if(!b->emitted){assert(b->callback);b->emitted=true;Message signal{"JobRemoved",0,""};b->callback(&signal,b->userdata,nullptr);return 1;}
    return 0;
}
int sd_bus_wait(Bus*,std::uint64_t){assert(false);return -EPROTO;}
int sd_bus_get_property_trivial(Bus*,const char*,const char*,const char*,const char*,Error*,char,void*){assert(false);return -EPROTO;}
int sd_bus_call_method(Bus* b,const char* dest,const char* path,const char* iface,const char* member,Error* e,Message** out,const char* signature,...){
    const std::string method=member;
    assert(b->address&&b->client&&b->timeout&&b->interactive);
    va_list args;va_start(args,signature);
    if(method=="GetNameOwner"||method=="GetConnectionUnixUser"){
        assert(std::string(dest)=="org.freedesktop.DBus"&&std::string(path)=="/org/freedesktop/DBus"&&std::string(iface)=="org.freedesktop.DBus");
        assert(std::string(signature)=="s");const std::string arg=va_arg(args,const char*);
        assert(arg==(method=="GetNameOwner"?"org.freedesktop.systemd1":":1.77"));
    }else{
        assert(std::string(dest)==":1.77"&&std::string(path)=="/org/freedesktop/systemd1"&&std::string(iface)=="org.freedesktop.systemd1.Manager");
        if(method=="ListUnits"||method=="ListUnitFiles"||method=="Subscribe")assert(std::string(signature).empty());
        else{
            assert(method=="StartUnit"||method=="StopUnit"||method=="RestartUnit");
            assert(std::string(signature)=="ss"&&std::string(va_arg(args,const char*))=="fixture.service"&&std::string(va_arg(args,const char*))=="replace");
        }
    }
    va_end(args);
    if(mode()=="denied"&&method=="GetConnectionUnixUser"){e->name="org.freedesktop.DBus.Error.AccessDenied";return -EACCES;}
    if(mode()=="missing"&&method=="GetNameOwner"){e->name="org.freedesktop.DBus.Error.NameHasNoOwner";return -ENXIO;}
    if(mode()=="broken"&&method=="ListUnitFiles")return -EPROTO;
    const bool action=method=="StartUnit"||method=="StopUnit"||method=="RestartUnit";
    if(action&&mode()=="actiondenied"){e->name="org.freedesktop.DBus.Error.AccessDenied";return -EACCES;}
    if(action&&mode()=="nosuchunit"){e->name="org.freedesktop.systemd1.NoSuchUnit";return -ENOENT;}
    *out=new Message{method,0,""};
    if(method=="GetNameOwner")(*out)->text=mode()=="ownerchange"&&b->owners++?":1.78":":1.77";
    if(action&&mode()=="early"){assert(b->callback);b->emitted=true;Message signal{"JobRemoved",0,""};b->callback(&signal,b->userdata,nullptr);}
    return 1;
}
int sd_bus_message_enter_container(Message*,char,const char*){return 1;}
int sd_bus_message_exit_container(Message*){return 1;}
int sd_bus_message_read(Message* m,const char* sig,...){
    va_list args;va_start(args,sig);int rc=1;
    if(m->kind=="GetNameOwner")*va_arg(args,const char**)=m->text.c_str();
    else if(m->kind=="GetConnectionUnixUser")*va_arg(args,std::uint32_t*)=static_cast<std::uint32_t>(geteuid())+(mode()=="wronguid"?1:0);
    else if(m->kind=="ListUnits")rc=0;
    else if(m->kind=="ListUnitFiles"){
        if(m->reads++||mode()=="empty")rc=0;
        else{assert(std::string(sig)=="(ss)");*va_arg(args,const char**)="/usr/lib/systemd/user/fixture.service";*va_arg(args,const char**)="disabled";}
    }else if(m->kind=="StartUnit"||m->kind=="StopUnit"||m->kind=="RestartUnit"){
        assert(std::string(sig)=="o");*va_arg(args,const char**)=mode()=="badjob"?"/wrong/job":"/org/freedesktop/systemd1/job/17";
    }else if(m->kind=="JobRemoved"){
        assert(std::string(sig)=="uoss");*va_arg(args,unsigned*)=17;
        *va_arg(args,const char**)="/org/freedesktop/systemd1/job/17";
        *va_arg(args,const char**)="fixture.service";
        *va_arg(args,const char**)=mode()=="failed"?"failed":"done";
    }else{assert(false);rc=-EPROTO;}
    va_end(args);return rc;
}
}
