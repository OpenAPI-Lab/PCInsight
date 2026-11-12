#include "pcinsight/sampler.hpp"
#include "sampler_history.hpp"
#include "linux_network_admission_monitor.hpp"

#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <atomic>

namespace pcinsight {
struct Sampler::Impl {
    Monitor monitor;
    std::chrono::steady_clock::duration interval;
    SampleOptions options;
    mutable std::mutex mutex;
    mutable std::mutex lifecycle_mutex;
    std::mutex monitor_mutex; // lifecycle -> Monitor -> history; worker never takes lifecycle.
    std::condition_variable monitor_ready;
    std::atomic<bool> update_pending{false};
    std::condition_variable wake;
    mutable detail::SamplerHistory samples;
    std::jthread worker;
    std::atomic<bool> active{false};
    bool visible_setup=false; // Protected by monitor_mutex, including worker exit.
    std::string error;

    Impl(std::chrono::milliseconds i,std::size_t c,SampleOptions o)
        :interval(detail::sampler_interval(i)),options(o),samples(detail::sampler_capacity(i,c)){}
    ~Impl(){stop();}
    void start(){
        std::lock_guard lifecycle_lock(lifecycle_mutex);
        if(active.load())return;
        if(worker.joinable())worker.join();
        { std::lock_guard lock(mutex); error.clear(); samples.clear(); }
        active.store(true);
        try {
        worker=std::jthread([this](std::stop_token stop){
            try {
            while(!stop.stop_requested()){
                std::chrono::steady_clock::time_point next;
                {
                std::unique_lock monitor_lock(monitor_mutex);
                monitor_ready.wait(monitor_lock,[&]{return !update_pending.load()||stop.stop_requested();});
                if(stop.stop_requested())break;
                const auto started=std::chrono::steady_clock::now();
                next=detail::sampler_deadline(started,interval);
                auto snapshot=monitor.sample(options);
                const auto finished=std::chrono::steady_clock::now();
                std::lock_guard lock(mutex);
                samples.append({started,finished,std::move(snapshot)});
                } // Commit the old sample before an update can complete.
                std::unique_lock lock(mutex);
                wake.wait_until(lock,next,[&]{return stop.stop_requested();});
            }
            } catch(const std::exception& e) {
                std::lock_guard lock(mutex); error=e.what();
            } catch(...) {
                std::lock_guard lock(mutex); error="unknown collection error";
            }
            std::lock_guard monitor_lock(monitor_mutex);
            monitor.stop_linux_process_network();visible_setup=false;active.store(false);
        });
        } catch(...) { std::lock_guard monitor_lock(monitor_mutex);active.store(false); monitor.stop_linux_process_network();visible_setup=false; throw; }
    }
    void stop(){
        std::lock_guard lifecycle_lock(lifecycle_mutex);
        if(worker.joinable()){
            { std::lock_guard lock(mutex); worker.request_stop(); }
            wake.notify_all();monitor_ready.notify_all();worker.join();
        }
        std::lock_guard monitor_lock(monitor_mutex);
        active.store(false);
        monitor.stop_linux_process_network();
        visible_setup=false;
    }
};

Sampler::Sampler(std::chrono::milliseconds i,std::size_t c,SampleOptions o):impl_(std::make_unique<Impl>(i,c,o)){}
Sampler::~Sampler(){if(impl_)impl_->stop();}
Sampler::Sampler(Sampler&&) noexcept=default;
Sampler& Sampler::operator=(Sampler&&) noexcept=default;
void Sampler::start(){impl_->start();}
void Sampler::stop(){impl_->stop();}
Metric<std::uint64_t> Sampler::start_linux_process_network(bool authorized,std::span<const LinuxProcessNetworkSelection> selection){
    return start_linux_process_network(authorized,selection,LinuxProcessNetworkSelectionMode::strict_single_namespace);
}
Metric<std::uint64_t> Sampler::start_linux_process_network(bool authorized,std::span<const LinuxProcessNetworkSelection> selection,LinuxProcessNetworkSelectionMode mode){
    if(!authorized)return {{},Support::permission_required,"Explicit selected BPF capture authorization required"};
    if(!impl_)return {{},Support::unsupported,"Moved-from Sampler has no Monitor"};
    std::lock_guard lock(impl_->lifecycle_mutex);
    if(impl_->active.load())return {{},Support::unsupported,"Stop Sampler before starting/replacing selected capture"};
    if(impl_->worker.joinable())impl_->worker.join();
    if(!impl_->options.include_linux_process_network)return {{},Support::unsupported,"Enable include_linux_process_network before explicit capture setup"};
    std::lock_guard monitor_lock(impl_->monitor_mutex);
    return impl_->monitor.start_linux_process_network(true,selection,mode);
}
Metric<std::uint64_t> Sampler::update_linux_process_network_selection(bool authorized,std::span<const LinuxProcessNetworkSelection> selection){
    if(!authorized)return {{},Support::permission_required,"Explicit authorization required for each background selected capture update"};
    if(!impl_)return {{},Support::unsupported,"Moved-from Sampler cannot update capture"};
    std::lock_guard lifecycle_lock(impl_->lifecycle_mutex);
    if(!impl_->options.include_linux_process_network)return {{},Support::unsupported,"Selected process network report is disabled"};
    // Publish the request BEFORE waiting for Monitor: a busy worker must yield
    // at its next boundary instead of repeatedly reacquiring an unfair mutex.
    struct PendingUpdate {
        Impl& impl;
        std::unique_lock<std::mutex> lock;
        explicit PendingUpdate(Impl& i):impl(i),lock(i.monitor_mutex,std::defer_lock){
            impl.update_pending.store(true);
            try{lock.lock();}catch(...){impl.update_pending.store(false);impl.monitor_ready.notify_all();throw;}
        }
        // Clear/notify WHILE owning Monitor: do not lose a wakeup between the
        // worker's predicate check and its condition-variable wait.
        ~PendingUpdate(){impl.update_pending.store(false);impl.monitor_ready.notify_all();}
    } pending(*impl_);
    if(!impl_->active.load())return {{},Support::unsupported,"Selection updates require a running Sampler and active capture; setup while stopped first"};
    return impl_->monitor.update_linux_process_network_selection(true,selection);
}
Metric<LinuxNetworkAdmissionChange> Sampler::start_linux_visible_process_network(bool authorized,LinuxNetworkAdmissionPolicy policy){
    if(!authorized)return {{},Support::permission_required,"Explicit visible admission setup authorization required"};
    if(!impl_)return {{},Support::unsupported,"Moved-from Sampler cannot prepare admission"};
    std::lock_guard lifecycle_lock(impl_->lifecycle_mutex);
    if(impl_->active.load())return {{},Support::unsupported,"Stop Sampler before visible admission setup"};
    if(impl_->worker.joinable())impl_->worker.join();
    if(!impl_->options.include_linux_process_network)return {{},Support::unsupported,"Enable include_linux_process_network before explicit admission setup"};
    std::lock_guard monitor_lock(impl_->monitor_mutex);
    auto r=impl_->monitor.refresh_linux_visible_process_network(true,policy);
    // A failed setup cannot erase a previously prepared visible route, e.g.
    // denied mode mixing. Runtime refresh failures remain explicitly retryable.
    if(r.value&&r.support==Support::available)impl_->visible_setup=true;
    return r;
}
Metric<LinuxNetworkAdmissionChange> Sampler::refresh_linux_visible_process_network(bool authorized,LinuxNetworkAdmissionPolicy policy){
    if(!authorized)return {{},Support::permission_required,"Fresh authorization required for EACH worker admission refresh"};
    if(!impl_)return {{},Support::unsupported,"Moved-from Sampler cannot refresh admission"};
    std::lock_guard lifecycle_lock(impl_->lifecycle_mutex);
    if(!impl_->options.include_linux_process_network)return {{},Support::unsupported,"Process network report disabled"};
    // Same request-before-lock handoff as manual updates. Never wait for the
    // worker while owning history; old sample commits before refresh returns.
    struct Pending {
        Impl& i;std::unique_lock<std::mutex> lock;
        explicit Pending(Impl& value):i(value),lock(i.monitor_mutex,std::defer_lock){i.update_pending.store(true);try{lock.lock();}catch(...){i.update_pending.store(false);i.monitor_ready.notify_all();throw;}}
        ~Pending(){i.update_pending.store(false);i.monitor_ready.notify_all();}
    } pending(*impl_);
    if(!impl_->active.load()||!impl_->visible_setup)return {{},Support::unsupported,"Running worker and previously prepared visible route required; refresh cannot implicitly set up capture"};
    return impl_->monitor.refresh_linux_visible_process_network(true,policy);
}
bool Sampler::running()const{return impl_->active.load();}
std::string Sampler::last_error()const{std::lock_guard lock(impl_->mutex);return impl_->error;}
std::optional<SystemSnapshot> Sampler::latest()const{std::lock_guard lock(impl_->mutex);return impl_->samples.latest();}
std::optional<SamplerSample> Sampler::latest_timed()const{std::lock_guard lock(impl_->mutex);return impl_->samples.latest_timed();}
std::vector<SystemSnapshot> Sampler::history()const{std::lock_guard lock(impl_->mutex);return impl_->samples.history(std::chrono::steady_clock::now());}
std::vector<SamplerSample> Sampler::timed_history()const{std::lock_guard lock(impl_->mutex);return impl_->samples.timed_history(std::chrono::steady_clock::now());}
}
namespace pcinsight::detail {
Sampler SamplerAdmissionAccess::make(Monitor&& monitor,std::chrono::milliseconds interval,std::size_t capacity,SampleOptions options){
    Sampler result(interval,capacity,options);result.impl_->monitor=std::move(monitor);return result;
}
}
