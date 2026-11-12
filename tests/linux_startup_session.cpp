#include "../src/linux_startup_session.hpp"
#include "../src/linux_startup_eligibility.hpp"
#include "../src/linux_service_actions.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#ifdef __linux__
#include <atomic>
#include <cstring>
#include <dlfcn.h>
#include <mutex>
#include <thread>
#include <unistd.h>
struct Fixture {
    pcinsight::detail::SystemdActionApi api{pcinsight::detail::load_systemd_action_api()};
    pcinsight::detail::ActionBus* bus{};pcinsight::detail::ActionSlot* slot{};
    std::atomic<bool> stop{};std::thread thread;std::mutex mutex;
    std::string name{"gnome-fixture"};int mode{};unsigned calls{};
    int (*reply)(pcinsight::detail::ActionMessage*,const char*,...){};
    int (*reply_error)(pcinsight::detail::ActionMessage*,const pcinsight::detail::ActionBusError*){};
    const char* (*member)(pcinsight::detail::ActionMessage*){};
    const char* (*interface)(pcinsight::detail::ActionMessage*){};
    std::string service="org.pcinsight.SessionFixture.p"+std::to_string(getpid());
    static int callback(pcinsight::detail::ActionMessage* message,void* data,pcinsight::detail::ActionBusError*) noexcept {
        auto& f=*static_cast<Fixture*>(data);
        try {
            const auto* m=f.member(message);const auto* i=f.interface(message);
            if(!m || !i || std::strcmp(m,"Get") || std::strcmp(i,"org.freedesktop.DBus.Properties")) return 0;
            const char* requested_interface{};const char* requested_property{};
            if(f.api.read(message,"ss",&requested_interface,&requested_property)<=0 ||
                !requested_interface || !requested_property || std::strcmp(requested_interface,"org.gnome.SessionManager") ||
                std::strcmp(requested_property,"SessionName")) return 0;
            std::lock_guard lock(f.mutex);++f.calls;
            if(f.mode==1) return f.reply(message,"v","u",123U);
            if(f.mode==2) {
                pcinsight::detail::ActionBusError e{"org.freedesktop.DBus.Error.AccessDenied","private fixture denial",0};
                return f.reply_error(message,&e);
            }
            return f.reply(message,"v","s",f.name.c_str());
        } catch(...) {return -1;}
    }
    bool start() {
        if(!api) return false;
        auto* l=api.library.get();
        const auto open=reinterpret_cast<int(*)(pcinsight::detail::ActionBus**)>(dlsym(l,"sd_bus_open_user"));
        const auto request=reinterpret_cast<int(*)(pcinsight::detail::ActionBus*,const char*,std::uint64_t)>(dlsym(l,"sd_bus_request_name"));
        const auto add=reinterpret_cast<int(*)(pcinsight::detail::ActionBus*,pcinsight::detail::ActionSlot**,const char*,decltype(&callback),void*)>(dlsym(l,"sd_bus_add_object"));
        reply=reinterpret_cast<decltype(reply)>(dlsym(l,"sd_bus_reply_method_return"));
        reply_error=reinterpret_cast<decltype(reply_error)>(dlsym(l,"sd_bus_reply_method_error"));
        member=reinterpret_cast<decltype(member)>(dlsym(l,"sd_bus_message_get_member"));
        interface=reinterpret_cast<decltype(interface)>(dlsym(l,"sd_bus_message_get_interface"));
        if(!open || !request || !add || !reply || !reply_error || !member || !interface || open(&bus)<0) return false;
        // Own unique test name only, flags0: never replace another name owner.
        assert(request(bus,service.c_str(),0)>0);
        assert(add(bus,&slot,"/org/gnome/SessionManager",callback,this)>=0);
        thread=std::thread([this] {
            while(!stop) {const auto n=api.process(bus,nullptr);assert(n>=0);if(!n)assert(api.wait(bus,100000)>=0);}
        });
        return true;
    }
    ~Fixture(){stop=true;if(thread.joinable())thread.join();if(slot)api.slot_unref(slot);if(bus)api.unref(bus);}
};
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
int main() {
    const std::string base="[Desktop Entry]\nType=Application\nName=Session test\nExec=/never-run\n";
    XdgEligibilityContext context;context.desktop="GNOME";
    auto check=[&](const std::string& value){return autostart_condition_satisfied(base+"AutostartCondition="+value+"\n",context);};
    assert(!check("GNOME3 if-session gnome").value); // desktop name is not session name
    unsigned calls=0;
    context.session_name=[&]{++calls;return Metric<std::string>{"ubuntu",Support::available,"injected"};};
    assert(check("GNOME3 if-session ubuntu").value==true);
    assert(check("GNOME3 unless-session ubuntu").value==false);
    assert(check("GNOME3 if-session Ubuntu").value==false); // name is case-sensitive
    assert(check("gnome3 UNLESS-SESSION gnome").value==true);
    const auto before=calls;
    for(const auto* s:{"GNOME3","GNOME3 if-session","GNOME3 unknown gnome","GNOME3 if-session ","GNOME3 if-session gnome\\n"}) assert(!check(s).value);
    assert(!check("GNOME3 if-session "+std::string(1025,'a')).value && calls==before);
    context.session_name=[] {return Metric<std::string>{std::nullopt,Support::permission_required,"denied"};};
    assert(!check("GNOME3 unless-session gnome").value && check("GNOME3 unless-session gnome").support==Support::permission_required);
    context.session_name=[] {return Metric<std::string>{"",Support::available,"invalid"};};
    assert(!check("GNOME3 unless-session gnome").value);
#ifdef __linux__
    const auto* raw=getenv("DBUS_SESSION_BUS_ADDRESS");const std::optional<std::string> old=raw?std::optional<std::string>(raw):std::nullopt;
    struct Restore {std::optional<std::string> old;~Restore(){if(old)setenv("DBUS_SESSION_BUS_ADDRESS",old->c_str(),1);else unsetenv("DBUS_SESSION_BUS_ADDRESS");}} restore{old};
    assert(setenv("DBUS_SESSION_BUS_ADDRESS","unixexec:path=/never-execute",1)==0);
    assert(!read_native_gnome_session_name().value); // rejects BEFORE opening transport
    if(old)assert(setenv("DBUS_SESSION_BUS_ADDRESS",old->c_str(),1)==0);else assert(unsetenv("DBUS_SESSION_BUS_ADDRESS")==0);
    Fixture fixture;
    if(!fixture.start()){std::cout<<"SKIP native fixture: local user bus unavailable\n";return 0;}
    auto read=[&]{return read_native_gnome_session_name(fixture.service);};
    auto session=read();assert(session.value=="gnome-fixture" && session.support==Support::available);
    context.session_name=read;
    assert(check("GNOME3 if-session gnome-fixture").value==true);
    {std::lock_guard lock(fixture.mutex);fixture.name="ubuntu-fixture";}
    assert(check("GNOME3 if-session gnome-fixture").value==false);
    assert(check("GNOME3 unless-session gnome-fixture").value==true);
    {std::lock_guard lock(fixture.mutex);fixture.mode=1;}
    assert(!read().value); // wrong variant type
    {std::lock_guard lock(fixture.mutex);fixture.mode=2;}
    assert(!read().value && read().support==Support::permission_required);
    {std::lock_guard lock(fixture.mutex);fixture.mode=0;fixture.name=std::string(1025,'x');}
    assert(!read().value);
    {std::lock_guard lock(fixture.mutex);fixture.name="bad\nname";}
    assert(!read().value);
    assert(!read_native_gnome_session_name("org.pcinsight.AbsentSessionManager.p"+std::to_string(getpid())).value);
    std::cout<<"Native private SessionName service verified: true/false, wrong variant, denied, oversized/control values; no real GNOME session modified\n";
#else
    assert(!read_native_gnome_session_name().value);
#endif
}
