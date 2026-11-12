#include "../src/linux_nvml.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
using namespace pcinsight;
using namespace pcinsight::detail;
int main() {
    auto api = load_linux_nvml();
    assert(api.library && api.init && api.shutdown && api.count && api.handle);
    auto copy = api;
    api = {}; // copied function bindings must retain the shared object
    for (int i = 0; i < 3; ++i) {
        const auto snapshot = read_linux_nvml(copy);
        assert(snapshot.support == Support::available && snapshot.gpus.size() == 2);
        for (const auto& gpu : snapshot.gpus) {
            assert(gpu.model.starts_with("Synthetic NVIDIA"));
            assert(gpu.utilization_percent.value == 25 && gpu.temperature_celsius.value == 42);
            assert(gpu.memory_used_bytes.value == 1024 && gpu.memory_available_bytes.value == 3072);
            assert(gpu.driver_version.value == "synthetic.1");
        }
    }
    unsigned count = 0;
    assert(copy.count(&count) == 1); // no leaked initialization reference
    Monitor monitor;
    const auto sample = monitor.sample();
    unsigned synthetic = 0;
    for (const auto& gpu : sample.gpus) if (gpu.model.starts_with("Synthetic NVIDIA")) ++synthetic;
    assert(synthetic == 2);
    SampleOptions options; options.include_gpus = false;
    assert(monitor.sample(options).gpus.empty());
    assert(copy.count(&count) == 1);
}
