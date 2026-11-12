#include "pcinsight/sampler.hpp"
#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>
#include <atomic>

void require(bool ok) { if (!ok) throw std::runtime_error("sampler lifecycle regression"); }
void wait_sample(pcinsight::Sampler& s) {
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(!s.latest() && s.running() && std::chrono::steady_clock::now()<deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    require(s.latest().has_value());
}
int main() {
    using namespace std::chrono_literals;
    pcinsight::Sampler source(1h,2), destination(1h,2);
    source.start(); destination.start();
    wait_sample(source); wait_sample(destination);
    // Destroying the old Impl during move assignment must wake its sleeping worker.
    auto moved=std::async(std::launch::async,[&]{ destination=std::move(source); });
    require(moved.wait_for(2s)==std::future_status::ready); moved.get();
    destination.stop(); require(!destination.running());
    destination.start(); require(destination.running()); destination.stop();
    require(destination.last_error().empty());
    pcinsight::Sampler concurrent(10ms,2);
    std::jthread a([&]{for(int i=0;i<10;++i){concurrent.start();concurrent.stop();}});
    std::jthread b([&]{for(int i=0;i<10;++i){concurrent.start();(void)concurrent.running();concurrent.stop();}});
    a.join(); b.join(); concurrent.stop();
    require(!concurrent.running());
    // No BPF setup: updates can never authorize a capture by themselves. Public
    // lifecycle/collection/update/read calls may race, but move/destruction may not.
    pcinsight::SampleOptions options;
    options.include_processes=options.include_disks=options.include_networks=options.include_gpus=false;
    options.include_linux_process_network=true;
    pcinsight::Sampler updating(10ms,16,options);
    require(!updating.update_linux_process_network_selection(true,{}).value);
    require(updating.update_linux_process_network_selection(false,{}).support==pcinsight::Support::permission_required);
    auto changed=std::move(updating);
    require(!updating.update_linux_process_network_selection(true,{}).value);
    std::atomic<unsigned> observed{0};
    std::jthread lifecycle([&]{for(unsigned i=0;i<8;++i){changed.start();changed.stop();}});
    std::jthread updates([&]{for(unsigned i=0;i<40;++i){
        require(!changed.update_linux_process_network_selection(true,{}).value);
        require(changed.update_linux_process_network_selection(false,{}).support==pcinsight::Support::permission_required);
        if(const auto latest=changed.latest())require(!latest->linux_process_network.value);
        (void)changed.timed_history();++observed;
    }});
    lifecycle.join();updates.join();changed.stop();require(observed==40&&!changed.running()&&changed.last_error().empty());
    changed.start();wait_sample(changed);
    require(!changed.update_linux_process_network_selection(true,{}).value&&changed.running());
    require(!changed.latest()->linux_process_network.value);changed.stop();
    pcinsight::Sampler disabled(1h,2);
    disabled.start();require(!disabled.update_linux_process_network_selection(true,{}).value);disabled.stop();
}
