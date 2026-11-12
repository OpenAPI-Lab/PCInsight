#include "linux_s6_observation.hpp"
#include <limits>
#include <mutex>
namespace pcinsight {
namespace {
bool eligible(const LinuxS6StatusRecord& s){if(s.reported_pid>2147483647U)return false;for(const auto* stamp:{&s.changed_at_tai64n,&s.ready_changed_at_tai64n}){std::uint32_t ns=0;for(unsigned i=8;i<12;++i)ns=(ns<<8)|(*stamp)[i];if(ns>=1000000000)return false;}return true;}
bool valid_action(ServiceAction a){return a==ServiceAction::start||a==ServiceAction::stop||a==ServiceAction::restart;}
Metric<bool> unknown(std::string note){return {{},Support::unsupported,std::move(note)};}
}
Metric<bool> observe_linux_s6_condition(ServiceAction action,const LinuxS6StatusRecord& before,const LinuxS6StatusRecord& after){
    if(!valid_action(action)||!eligible(before)||!eligible(after))return unknown("Invalid/ineligible s6 record/action; raw u64 PID is not assumed a Linux lifecycle PID");
    bool matched=!after.finishing&&!after.paused;
    if(action==ServiceAction::stop)matched=matched&&!after.reported_pid&&!after.want_up;
    else {matched=matched&&after.reported_pid&&after.want_up;if(action==ServiceAction::restart)matched=matched&&after.changed_at_tai64n>before.changed_at_tai64n;}
    return {matched,Support::available,"Raw record condition ONLY; ready flag ignored, restart needs newer execution TAI, no life/readiness/causal completion proof"};
}
struct LinuxS6Observation::Impl {
    mutable std::mutex mutex;detail::S6ObservationApi api;LinuxS6ObservationReport status;
    std::chrono::steady_clock::time_point last{};
    explicit Impl(detail::S6ObservationApi a):api(std::move(a)){}
    void release()noexcept{api.read=nullptr;api.request=nullptr;api.now=nullptr;}
    ActionResult fail(ActionError e){status.state=LinuxS6ObservationState::failed;status.error=e;status.target_condition_observed=unknown("Observation failed; previous delivery NOT undone or completed");release();return {e,"s6 observation failed; retained delivery counts"};}
    ActionResult timeout(){status.state=LinuxS6ObservationState::timed_out;status.error=ActionError::system_error;status.target_condition_observed=unknown("No in-window condition evidence; NOT proof the service request failed");release();return {ActionError::system_error,"s6 observation deadline reached without in-window confirmation"};}
};
LinuxS6Observation::LinuxS6Observation(std::unique_ptr<Impl> p,int):impl_(std::move(p)){}
LinuxS6Observation::LinuxS6Observation(std::shared_ptr<LinuxS6Control> target):impl_(std::make_unique<Impl>(detail::S6ObservationApi{})){
    if(target){impl_->api.read=[target]{return target->read_record();};impl_->api.request=[target](auto action,bool consent){return target->request(action,consent);};impl_->api.now=[]{return std::chrono::steady_clock::now();};}
}
LinuxS6Observation::~LinuxS6Observation()=default;
ActionResult LinuxS6Observation::start(ServiceAction action,bool authorized,std::chrono::milliseconds timeout){
    if(!authorized)return {ActionError::permission_denied,"Explicit s6 observation control consent required"};
    std::lock_guard lock(impl_->mutex);auto& i=*impl_;
    if(i.status.state!=LinuxS6ObservationState::idle)return {ActionError::invalid_argument,"One request per s6 observation instance"};
    if(!valid_action(action)||timeout.count()<=0||timeout>std::chrono::minutes(1))return {ActionError::invalid_argument,"Invalid s6 action/timeout1..60000ms"};
    if(!i.api.read||!i.api.request||!i.api.now)return i.fail(ActionError::not_supported);
    try{
        const auto now=i.api.now();const auto span=std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout);
        if(now.time_since_epoch().count()>std::numeric_limits<std::chrono::steady_clock::duration::rep>::max()-span.count())return i.fail(ActionError::invalid_argument);
        i.status.action=action;i.status.started_at=i.last=now;i.status.deadline=now+span;
        i.status.before=i.api.read();if(!i.status.before.value||i.status.before.support!=Support::available||!eligible(*i.status.before.value))return i.fail(i.status.before.support==Support::permission_required?ActionError::permission_denied:ActionError::not_supported);
        i.status.baseline_satisfied=observe_linux_s6_condition(action,*i.status.before.value,*i.status.before.value);
        i.status.target_condition_observed=unknown("No post-delivery raw condition observation yet");
        ActionResult result{ActionError::none,"s6 request delivered; call poll explicitly to observe raw conditions"};std::string failed="s6 delivery failed/partial; retained counts, NO retry";std::string completion="Completion/readiness UNKNOWN even if raw condition becomes observed";
        const auto ready=i.api.now();if(ready<i.last)return i.fail(ActionError::stale_target);if(ready>=i.status.deadline)return i.timeout();i.last=ready;
        i.status.state=LinuxS6ObservationState::pending;i.status.request_attempted=true;i.status.delivery=i.api.request(action,true);
        // Native source prepares its result before write; no allocating work here after delivery.
        i.status.delivery.completed.value.reset();i.status.delivery.completed.support=Support::unsupported;i.status.delivery.completed.note.swap(completion);
        const auto expected=action==ServiceAction::restart?3U:1U;
        if(!i.status.delivery||i.status.delivery.requested_bytes!=expected||i.status.delivery.written_bytes!=expected){i.status.error=i.status.delivery.error==ActionError::none?ActionError::system_error:i.status.delivery.error;i.status.state=LinuxS6ObservationState::failed;i.release();result.error=i.status.error;result.message.swap(failed);}
        return result;
    }catch(...){return i.fail(ActionError::system_error);}
}
ActionResult LinuxS6Observation::poll(){
    std::lock_guard lock(impl_->mutex);auto& i=*impl_;
    if(i.status.state==LinuxS6ObservationState::condition_observed)return {};
    if(i.status.state!=LinuxS6ObservationState::pending)return {ActionError::invalid_argument,"s6 observation is not pending"};
    try{
        const auto now=i.api.now();if(now<i.last)return i.fail(ActionError::stale_target);i.last=now;if(now>i.status.deadline)return i.timeout();
        auto sample=i.api.read();const auto sampled=i.api.now();if(sampled<i.last)return i.fail(ActionError::stale_target);i.last=sampled;i.status.latest=std::move(sample);i.status.sampled_at=sampled;
        if(sampled>i.status.deadline)return i.timeout();
        if(i.status.latest.value&&i.status.latest.support==Support::available&&eligible(*i.status.latest.value))i.status.target_condition_observed=observe_linux_s6_condition(i.status.action,*i.status.before.value,*i.status.latest.value);
        else i.status.target_condition_observed=unknown("Latest record unavailable/ineligible/non-exact; no last-good replacement");
        if(i.status.target_condition_observed.value==true){i.status.state=LinuxS6ObservationState::condition_observed;i.release();return {};}
        if(sampled==i.status.deadline)return i.timeout();
        return {};
    }catch(...){return i.fail(ActionError::system_error);}
}
LinuxS6ObservationReport LinuxS6Observation::report()const{std::lock_guard lock(impl_->mutex);return impl_->status;}
void LinuxS6Observation::close(){std::lock_guard lock(impl_->mutex);if(impl_->status.state==LinuxS6ObservationState::idle||impl_->status.state==LinuxS6ObservationState::pending)impl_->status.state=LinuxS6ObservationState::abandoned;impl_->release();}
}
namespace pcinsight::detail {
LinuxS6Observation S6ObservationAccess::make(S6ObservationApi api){return LinuxS6Observation(std::make_unique<LinuxS6Observation::Impl>(std::move(api)),0);}
}
