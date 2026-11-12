#include "linux_runit.hpp"
#include <limits>
#include <mutex>
namespace pcinsight {
namespace {
bool valid(const LinuxRunitStatus& s){std::uint32_t ns=0;for(unsigned n=8;n<12;++n)ns=(ns<<8)|s.changed_at_tai64n[n];return ns<1000000000&&s.reported_pid<=2147483647U&&static_cast<unsigned>(s.state)<=2&&!(s.state==LinuxRunitState::down&&s.reported_pid)&&!(s.state==LinuxRunitState::run&&!s.reported_pid);}
Metric<bool> unknown(std::string note){return {{},Support::unsupported,std::move(note)};}
bool action_valid(ServiceAction a){return a==ServiceAction::start||a==ServiceAction::stop||a==ServiceAction::restart;}
}
Metric<bool> observe_linux_runit_target(ServiceAction action,const LinuxRunitStatus& before,const LinuxRunitStatus& after){
    if(!action_valid(action)||!valid(before)||!valid(after))return unknown("Invalid runit action/status; no transition inference");
    bool matched=false;
    if(action==ServiceAction::stop)matched=after.state==LinuxRunitState::down&&!after.reported_pid&&!after.want_up;
    else {matched=after.state==LinuxRunitState::run&&after.reported_pid&&after.want_up&&!after.paused&&!after.term_sent;
        if(action==ServiceAction::restart)matched=matched&&after.changed_at_tai64n>before.changed_at_tai64n;
    }
    return {matched,Support::available,"Supervisor-reported target predicate only; newer TAI run epoch for restart, NOT readiness/causal request completion"};
}
struct LinuxRunitTransition::Impl {
    mutable std::mutex mutex;detail::RunitTransitionApi api;LinuxRunitTransitionReport status;
    std::chrono::steady_clock::time_point last{};
    explicit Impl(detail::RunitTransitionApi a):api(std::move(a)){}
    void release()noexcept{api.read=nullptr;api.request=nullptr;api.now=nullptr;}
    ActionResult fail(ActionError error){status.state=LinuxRunitTransitionState::failed;status.error=error;status.target_state_observed=unknown("Observation failed; delivered request is not undone or completed");release();return {error,"Runit observation failed; inspect retained delivery byte counts"};}
    ActionResult timeout(){status.state=LinuxRunitTransitionState::timed_out;status.error=ActionError::system_error;status.target_state_observed=unknown("No in-window target observation; timeout is NOT proof the request failed");release();return {ActionError::system_error,"Runit observation deadline reached, no service completion claim"};}
};
LinuxRunitTransition::LinuxRunitTransition(std::unique_ptr<Impl> p,int):impl_(std::move(p)){}
LinuxRunitTransition::LinuxRunitTransition(std::shared_ptr<LinuxRunitService> service):impl_(std::make_unique<Impl>(detail::RunitTransitionApi{})){
    if(service){impl_->api.read=[service]{return service->read();};impl_->api.request=[service](auto a,bool consent){return service->request(a,consent);};impl_->api.now=[]{return std::chrono::steady_clock::now();};}
}
LinuxRunitTransition::~LinuxRunitTransition()=default;
ActionResult LinuxRunitTransition::start(ServiceAction action,bool authorized,std::chrono::milliseconds timeout){
    if(!authorized)return {ActionError::permission_denied,"Explicit runit transition control consent required"};
    std::lock_guard lock(impl_->mutex);auto& i=*impl_;
    if(i.status.state!=LinuxRunitTransitionState::idle)return {ActionError::invalid_argument,"One request per runit transition instance"};
    if(!action_valid(action)||timeout.count()<=0||timeout>std::chrono::minutes(1))return {ActionError::invalid_argument,"Invalid runit action/timeout1..60000ms"};
    if(!i.api.read||!i.api.request||!i.api.now)return i.fail(ActionError::not_supported);
    try{
        const auto now=i.api.now();const auto span=std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout);
        if(now.time_since_epoch().count()>std::numeric_limits<std::chrono::steady_clock::duration::rep>::max()-span.count())return i.fail(ActionError::invalid_argument);
        i.status.action=action;i.status.started_at=i.last=now;i.status.deadline=now+span;
        i.status.before=i.api.read();if(!i.status.before.value||i.status.before.support!=Support::available||!valid(*i.status.before.value))return i.fail(i.status.before.support==Support::permission_required?ActionError::permission_denied:ActionError::not_supported);
        i.status.baseline_satisfied=observe_linux_runit_target(action,*i.status.before.value,*i.status.before.value);
        i.status.target_state_observed=unknown("No post-delivery observation yet");
        ActionResult result{ActionError::none,"Runit request delivered; target still needs explicit poll"};
        std::string failed="Runit delivery failed/partial; retained counts, no automatic retry";
        std::string completion="Readiness/causal completion unknown even if target becomes observed";
        const auto ready=i.api.now();if(ready<i.last)return i.fail(ActionError::stale_target);if(ready>=i.status.deadline)return i.timeout();
        i.last=ready;i.status.state=LinuxRunitTransitionState::pending;i.status.request_attempted=true;
        i.status.delivery=i.api.request(action,true);
        // No allocations/reads after delivery; retain byte counts before returning.
        i.status.delivery.completed.value.reset();i.status.delivery.completed.support=Support::unsupported;i.status.delivery.completed.note.swap(completion);
        const auto expected=action==ServiceAction::restart?3U:1U;
        if(!i.status.delivery||i.status.delivery.requested_bytes!=expected||i.status.delivery.written_bytes!=expected){i.status.error=i.status.delivery.error==ActionError::none?ActionError::system_error:i.status.delivery.error;i.status.state=LinuxRunitTransitionState::failed;i.release();result.error=i.status.error;result.message.swap(failed);}
        return result;
    }catch(...){return i.fail(ActionError::system_error);}
}
ActionResult LinuxRunitTransition::poll(){
    std::lock_guard lock(impl_->mutex);auto& i=*impl_;
    if(i.status.state==LinuxRunitTransitionState::observed)return {};
    if(i.status.state!=LinuxRunitTransitionState::pending)return {ActionError::invalid_argument,"Runit transition is not pending"};
    try{
        const auto now=i.api.now();if(now<i.last)return i.fail(ActionError::stale_target);i.last=now;if(now>i.status.deadline)return i.timeout();
        auto sample=i.api.read();const auto sampled=i.api.now();if(sampled<i.last)return i.fail(ActionError::stale_target);
        i.last=sampled;i.status.latest=std::move(sample);i.status.sampled_at=sampled;
        if(sampled>i.status.deadline)return i.timeout();
        if(i.status.latest.value&&i.status.latest.support==Support::available&&valid(*i.status.latest.value))i.status.target_state_observed=observe_linux_runit_target(i.status.action,*i.status.before.value,*i.status.latest.value);
        else i.status.target_state_observed=unknown("Latest status unavailable/non-exact; no last-good value substituted");
        if(i.status.target_state_observed.value==true){i.status.state=LinuxRunitTransitionState::observed;i.release();return {};}
        if(sampled==i.status.deadline)return i.timeout();
        return {};
    }catch(...){return i.fail(ActionError::system_error);}
}
LinuxRunitTransitionReport LinuxRunitTransition::report()const{std::lock_guard lock(impl_->mutex);return impl_->status;}
void LinuxRunitTransition::close(){std::lock_guard lock(impl_->mutex);if(impl_->status.state==LinuxRunitTransitionState::idle||impl_->status.state==LinuxRunitTransitionState::pending)impl_->status.state=LinuxRunitTransitionState::abandoned;impl_->release();}
}
namespace pcinsight::detail {
LinuxRunitTransition RunitTransitionAccess::make(RunitTransitionApi a){return LinuxRunitTransition(std::make_unique<LinuxRunitTransition::Impl>(std::move(a)),0);}
}
