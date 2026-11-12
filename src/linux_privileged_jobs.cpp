#include "linux_privileged_jobs.hpp"
#include "linux_service_actions.hpp"
#include <charconv>
#include <cstring>
#include <stdexcept>
#ifdef __linux__
#include <dlfcn.h>
#endif
namespace pcinsight::detail {
namespace {
bool job_path(std::string_view path,std::uint32_t* id=nullptr){constexpr std::string_view prefix="/org/freedesktop/systemd1/job/";if(path.size()>1024||!path.starts_with(prefix))return false;path.remove_prefix(prefix.size());std::uint32_t number{};const auto parsed=std::from_chars(path.data(),path.data()+path.size(),number);if(parsed.ec!=std::errc{}||parsed.ptr!=path.data()+path.size()||!number||path!=std::to_string(number))return false;if(id)*id=number;return true;}
}
bool valid_privileged_job_path(std::string_view path){return job_path(path);}
PrivilegedJobObservation::PrivilegedJobObservation(std::string unit,Clock::time_point deadline):unit_(std::move(unit)),deadline_(deadline){if(!valid_systemd_service_name(unit_))unknown_locked("Invalid job unit");}
void PrivilegedJobObservation::unknown_locked(std::string note){state_.state=PrivilegedJobState::unknown;state_.succeeded={{},Support::unsupported,note};state_.note=std::move(note);early_.clear();}
void PrivilegedJobObservation::invalidate(std::string note){std::lock_guard lock(mutex_);unknown_locked(std::move(note));}
void PrivilegedJobObservation::close_pending(std::string note){std::lock_guard lock(mutex_);if(state_.state==PrivilegedJobState::pending)unknown_locked(std::move(note));}
PrivilegedJobSnapshot PrivilegedJobObservation::snapshot(Clock::time_point now){std::lock_guard lock(mutex_);if(state_.state==PrivilegedJobState::pending&&now>=deadline_)unknown_locked("Job observation deadline; operation NOT canceled, outcome unknown");return state_;}
void PrivilegedJobObservation::apply_locked(const PrivilegedJobEvent& event){
    if(event.path!=path_)return;
    if(event.unit!=unit_){unknown_locked("Matching job path has unexpected unit; alias/identity mismatch");return;}
    if(state_.result.value){if(*state_.result.value!=event.result)unknown_locked("Conflicting completion events");return;}
    state_.result={event.result,Support::available,"Raw correlated JobRemoved result"};
    if(event.result=="done"){state_.state=PrivilegedJobState::completed;state_.succeeded={true,Support::available,"Job done; NOT current service state"};}
    else if(event.result=="canceled"||event.result=="timeout"||event.result=="failed"||event.result=="dependency"||event.result=="skipped"){state_.state=PrivilegedJobState::failed;state_.succeeded={false,Support::available,"Job ended without done; NOT proof of no effects"};}
    else {unknown_locked("Unrecognized Manager job result");return;}
    state_.note=state_.succeeded.note;
}
void PrivilegedJobObservation::select(std::string path){std::lock_guard lock(mutex_);if(state_.state==PrivilegedJobState::unknown)return;if(Clock::now()>=deadline_){unknown_locked("Job observation expired before receipt");return;}if(!path_.empty()||!job_path(path)){unknown_locked("Invalid/repeated job receipt selection");return;}path_=std::move(path);auto early=std::move(early_);early_.clear();for(const auto& event:early){apply_locked(event);if(state_.state==PrivilegedJobState::unknown)break;}}
void PrivilegedJobObservation::accept(PrivilegedJobEvent event){std::lock_guard lock(mutex_);if(state_.state==PrivilegedJobState::unknown)return;
    if(!path_.empty()&&event.path!=path_)return;
    if(state_.state==PrivilegedJobState::pending&&Clock::now()>=deadline_){unknown_locked("Job event arrived after deadline; outcome unknown");return;}
    std::uint32_t id{};if(++events_>4096||!job_path(event.path,&id)||id!=event.id||event.unit.size()>255||event.unit.empty()||event.result.empty()||event.result.size()>64||event.result.find('\0')!=event.result.npos){unknown_locked("Malformed/bounded job event");return;}
    if(path_.empty()){if(early_.size()>=64){unknown_locked("Early job event bound64 exceeded");return;}early_.push_back(std::move(event));}else apply_locked(event);
}
#ifdef __linux__
namespace {
struct Bus;struct Message;struct Slot;struct Error{const char* name{};const char* message{};int need_free{};};
struct Api {
    void* lib=dlopen("libsystemd.so.0",RTLD_NOW|RTLD_LOCAL);
    int (*match)(Bus*,Slot**,const char*,const char*,const char*,const char*,int(*)(Message*,void*,Error*),void*){};Slot* (*slot_unref)(Slot*){};
    int (*read)(Message*,const char*,...) {},(*type)(Message*,std::uint8_t*){},(*signature)(Message*,const char*){},(*at_end)(Message*,int){};const char* (*sender)(Message*){};Bus* (*bus)(Message*){};
    int (*new_call)(Bus*,Message**,const char*,const char*,const char*,const char*){},(*auto_start)(Message*,int){},(*interactive)(Message*,int){},(*invoke)(Bus*,Message*,std::uint64_t,Error*,Message**){};
    int (*cookie)(Message*,std::uint64_t*){},(*reply_cookie)(Message*,std::uint64_t*){};Message* (*unref)(Message*){};void (*free_error)(Error*){};
    int (*get_timeout)(Bus*,std::uint64_t*){},(*set_timeout)(Bus*,std::uint64_t){};
    Api(){if(!lib)return;
#define L(f,s) f=reinterpret_cast<decltype(f)>(dlsym(lib,s))
        L(match,"sd_bus_match_signal");L(slot_unref,"sd_bus_slot_unref");L(read,"sd_bus_message_read");L(type,"sd_bus_message_get_type");L(signature,"sd_bus_message_has_signature");L(at_end,"sd_bus_message_at_end");L(sender,"sd_bus_message_get_sender");L(bus,"sd_bus_message_get_bus");L(new_call,"sd_bus_message_new_method_call");L(auto_start,"sd_bus_message_set_auto_start");L(interactive,"sd_bus_message_set_allow_interactive_authorization");L(invoke,"sd_bus_call");L(cookie,"sd_bus_message_get_cookie");L(reply_cookie,"sd_bus_message_get_reply_cookie");L(unref,"sd_bus_message_unref");L(free_error,"sd_bus_error_free");
        L(get_timeout,"sd_bus_get_method_call_timeout");L(set_timeout,"sd_bus_set_method_call_timeout");
#undef L
    }
    explicit operator bool()const{return match&&slot_unref&&read&&type&&signature&&at_end&&sender&&bus&&new_call&&auto_start&&interactive&&invoke&&cookie&&reply_cookie&&unref&&free_error;}
    ~Api(){if(lib)dlclose(lib);}
};
struct Reply{Api& a;Message* m{};Error error{};~Reply(){if(m)a.unref(m);a.free_error(&error);}};
}
#endif
struct PrivilegedNativeJobWatch::Impl {
    std::shared_ptr<PrivilegedJobObservation> record;
#ifdef __linux__
    Api api;Bus* bus{};Slot* slot{};std::string owner;
    ~Impl(){if(record)record->close_pending("Job watch/connection removed; not canceled");if(slot)api.slot_unref(slot);}
    static int removed(Message* message,void* data,Error*)noexcept{auto& self=*static_cast<Impl*>(data);auto& a=self.api;
        try{const char* sender=a.sender(message);if(a.bus(message)!=self.bus||!sender||self.owner!=sender)return 0;std::uint8_t type{};
            if(a.type(message,&type)<0||type!=4||a.signature(message,"uoss")<=0)throw std::runtime_error("Invalid trusted JobRemoved signature/type");
            std::uint32_t id{};const char *path{},*unit{},*result{};if(a.read(message,"uoss",&id,&path,&unit,&result)<=0||!path||!unit||!result||a.at_end(message,1)<=0||strnlen(path,1025)>1024||strnlen(unit,256)>255||strnlen(result,65)>64)throw std::runtime_error("Truncated/oversized/trailing JobRemoved");
            self.record->accept({id,path,unit,result});
        }catch(...){try{self.record->invalidate("Invalid trusted job signal; outcome unknown");}catch(...){}}return 0;
    }
#endif
};
PrivilegedNativeJobWatch::PrivilegedNativeJobWatch(std::unique_ptr<Impl> p):impl_(std::move(p)){}
PrivilegedNativeJobWatch::~PrivilegedNativeJobWatch()=default;
std::shared_ptr<PrivilegedJobObservation> PrivilegedNativeJobWatch::observation()const{return impl_->record;}
Metric<std::shared_ptr<PrivilegedNativeJobWatch>> create_privileged_job_watch(void* bus,std::string owner,std::string unit,std::uint64_t timeout,bool subscribe,std::shared_ptr<PrivilegedRequestBudget> budget){
    auto no=[](std::string note)->Metric<std::shared_ptr<PrivilegedNativeJobWatch>>{return {{},Support::unsupported,std::move(note)};};
    if(!bus||owner.size()<4||owner.size()>255||owner[0]!=':'||!valid_systemd_service_name(unit)||timeout<1000||timeout>30000000)return no("Invalid native job watch configuration");
#ifdef __linux__
    try{auto p=std::make_unique<PrivilegedNativeJobWatch::Impl>();auto& a=p->api;if(!a)return no("Native job match symbols unavailable");p->bus=static_cast<Bus*>(bus);p->owner=std::move(owner);p->record=std::make_shared<PrivilegedJobObservation>(std::move(unit),PrivilegedJobObservation::Clock::now()+std::chrono::microseconds(timeout));
        {PrivilegedRpcTimeout lease(p->bus,a.get_timeout,a.set_timeout,budget);if(a.match(p->bus,&p->slot,p->owner.c_str(),"/org/freedesktop/systemd1","org.freedesktop.systemd1.Manager","JobRemoved",PrivilegedNativeJobWatch::Impl::removed,p.get())<0||!p->slot)return no("Cannot install job signal match");}
        if(subscribe){Reply request{a},reply{a};if(a.new_call(p->bus,&request.m,p->owner.c_str(),"/org/freedesktop/systemd1","org.freedesktop.systemd1.Manager","Subscribe")<0||!request.m||a.auto_start(request.m,0)<0||a.interactive(request.m,0)<0)return no("Cannot build closed Subscribe");
            const int rc=a.invoke(p->bus,request.m,privileged_rpc_timeout(budget),&reply.error,&reply.m);std::uint8_t type{};std::uint64_t cookie{},reply_cookie{};
            if(rc<0||!reply.m||a.type(reply.m,&type)<0||type!=2||!a.sender(reply.m)||p->owner!=a.sender(reply.m)||a.signature(reply.m,"")<=0||a.cookie(request.m,&cookie)<0||!cookie||a.reply_cookie(reply.m,&reply_cookie)<0||cookie!=reply_cookie)return no("Subscribe reply unavailable/unbound; no service dispatch");
        }
        return {std::shared_ptr<PrivilegedNativeJobWatch>(new PrivilegedNativeJobWatch(std::move(p))),Support::available,"Installed bounded match before dispatch; caller owns native lifetime"};
    }catch(const std::exception& e){return no(e.what());}
#else
    (void)subscribe;(void)budget;return no("Native job observation requires Linux");
#endif
}
}
