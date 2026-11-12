#include <pcinsight/app_history.hpp>
#include <pcinsight/sampler.hpp>
#include <pcinsight/sdk_headers.hpp>

#include <chrono>
#include <iostream>
#include <thread>

int main() {
    using namespace std::chrono_literals;
    pcinsight::require_matching_sdk_headers();
    pcinsight::SampleOptions options;
    options.include_app_history = true;
    options.include_processes = false;
    options.include_disks = options.include_networks = options.include_gpus = false;
    pcinsight::Monitor monitor;
    auto snapshot = monitor.sample(options);
    (void)pcinsight::app_history_view(snapshot); // May be unknown in this context.

    pcinsight::Sampler sampler(50ms, 4, options);
    sampler.start();
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    while (!sampler.latest_timed() && sampler.running() &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(20ms);
    }
    sampler.stop();
    const auto last = sampler.latest_timed();
    const auto window = sampler.timed_history();
    if (sampler.running() || !sampler.last_error().empty() || !last || window.empty() ||
        last->started_at > last->finished_at || window.back().finished_at != last->finished_at) {
        std::cerr << "Read consumer did not receive a consistent sampler window: "
                  << sampler.last_error() << '\n';
        return 1;
    }
}
