#include "../src/linux_nvml.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace pcinsight;
using namespace pcinsight::detail;
int main() {
    int initialized = 0, stopped = 0, init_error = 0, memory_error = 0, use_error = 0;
    unsigned count = 1, usage = 0, temperature = 0;
    bool bad_memory = false, bad_name = false, missing_uuid = false, bad_pci = false;
    NvmlApi api;
    api.init = [&] { ++initialized; return init_error; };
    api.shutdown = [&] { ++stopped; return 0; };
    api.count = [&](unsigned* n) { *n = count; return 0; };
    api.handle = [](unsigned i, NvmlDevice* h) { *h = reinterpret_cast<void*>(static_cast<std::uintptr_t>(i + 1)); return 0; };
    api.uuid = [&](NvmlDevice h, char* b, unsigned n) {
        if (missing_uuid) return 3;
        const auto s = "GPU-fixture-" + std::to_string(reinterpret_cast<std::uintptr_t>(h));
        assert(s.size() + 1 < n); std::memcpy(b, s.c_str(), s.size() + 1); return 0;
    };
    api.name = [&](NvmlDevice, char* b, unsigned n) {
        if (bad_name) std::memset(b, 'X', n);
        else std::strcpy(b, "NVIDIA fixture");
        return 0;
    };
    api.driver = [](char* b, unsigned) { std::strcpy(b, "fixture.1"); return 0; };
    api.pci = [&](NvmlDevice, NvmlPci* p) {
        std::strcpy(p->bus_id, bad_pci ? "00000000:01:ff.0" : "00000000:01:00.0"); return 0;
    };
    api.memory = [&](NvmlDevice, NvmlMemory* m) {
        *m = {1000, bad_memory ? 900ULL : 800ULL, 200}; return memory_error;
    };
    api.utilization = [&](NvmlDevice, NvmlUtilization* u) { *u = {usage, 100}; return use_error; };
    api.temperature = [&](NvmlDevice, unsigned sensor, unsigned* t) { assert(sensor == 0); *t = temperature; return 0; };
    assert(read_linux_nvml({}).gpus.empty());
    assert(nvml_pci_identity("0000:0a:1f.7") == nvml_pci_identity("00000000:0A:1F.7"));
    assert(nvml_pci_identity("0000:00:20.0").empty());
    assert(nvml_pci_identity("0000:00:00.8").empty());
    assert(nvml_pci_identity("not PCI").empty());
    auto s = read_linux_nvml(api);
    assert(initialized == 1 && stopped == 1 && s.support == Support::available && s.gpus.size() == 1);
    auto g = s.gpus.front();
    assert(g.id == "nvml:GPU-fixture-1" && g.model == "NVIDIA fixture" && g.vendor == "NVIDIA");
    assert(g.utilization_percent.value == 0 && g.temperature_celsius.value == 0);
    assert(g.memory_used_bytes.value == 200 && g.memory_available_bytes.value == 800 && g.dedicated_memory_bytes.value == 1000);
    assert(g.engines.empty() && !g.shared_memory_bytes.value && !g.driver_date.value);
    std::vector<GpuInfo> drm(1);
    drm[0].id = "card0";
    drm[0].physical_location = {"0000:01:00.0", Support::available, "DRM"};
    merge_linux_nvml(drm, s);
    assert(drm.size() == 1 && drm[0].id == "card0" && drm[0].model == "NVIDIA fixture");
    assert(drm[0].memory_used_bytes.value == 200);
    assert(drm[0].dedicated_memory_used_bytes.value == 200 && g.dedicated_memory_used_bytes.value==200 && !g.shared_memory_used_bytes.value);
    {
        auto estimated=drm;estimated[0].physical_location.support=Support::estimated;
        merge_linux_nvml(estimated,s);assert(estimated.size()==2); // not a proven PCI identity
        auto partial=s;partial.gpus[0].memory_available_bytes={};
        auto original=drm;original[0].dedicated_memory_bytes={4000,Support::available,"DRM"};original[0].memory_used_bytes={500,Support::available,"DRM"};original[0].memory_available_bytes={3500,Support::estimated,"DRM headroom"};
        merge_linux_nvml(original,partial);assert(original[0].dedicated_memory_bytes.value==4000 && original[0].memory_used_bytes.value==500 && original[0].memory_available_bytes.value==3500);
    }
    std::vector<GpuInfo> ambiguous{drm[0], drm[0]};
    ambiguous[0].model = ambiguous[1].model = "unchanged";
    merge_linux_nvml(ambiguous, s);
    assert(ambiguous.size() == 3 && ambiguous[0].model == "unchanged" && ambiguous[1].model == "unchanged");
    usage = 101; temperature = 256; bad_memory = true; bad_name = true; bad_pci = true;
    s = read_linux_nvml(api);
    assert(s.gpus.size() == 1);
    assert(!s.gpus[0].utilization_percent.value && !s.gpus[0].temperature_celsius.value);
    assert(!s.gpus[0].memory_used_bytes.value && s.gpus[0].model.empty() && !s.gpus[0].physical_location.value);
    bad_memory = false; bad_name = false; bad_pci = false; usage = 100; temperature = 60;
    memory_error = 4; use_error = 3;
    s = read_linux_nvml(api);
    assert(!s.gpus[0].memory_used_bytes.value && s.gpus[0].memory_used_bytes.support == Support::permission_required);
    assert(!s.gpus[0].utilization_percent.value && s.gpus[0].temperature_celsius.value == 60);
    memory_error = use_error = 0;
    count = 2;
    s = read_linux_nvml(api);
    assert(s.gpus.size() == 2);
    drm.resize(1); merge_linux_nvml(drm, s);
    assert(drm.size() == 3); // two NVML rows sharing PCI are not blindly joined
    count = 0;
    s = read_linux_nvml(api);
    assert(s.gpus.empty() && s.support == Support::available);
    count = 4097;
    s = read_linux_nvml(api);
    assert(s.gpus.empty() && s.support == Support::unsupported);
    count = 1; missing_uuid = true;
    s = read_linux_nvml(api);
    assert(s.gpus[0].id == "nvml:pci:0:1:0.0");
    bad_pci = true;
    s = read_linux_nvml(api);
    assert(s.gpus.empty() && s.support == Support::estimated);
    init_error = 4;
    const int before = stopped;
    s = read_linux_nvml(api);
    assert(s.support == Support::permission_required && stopped == before);
    drm.resize(1); drm[0].vendor = "0x10de";
    drm[0].utilization_percent = {};
    drm[0].temperature_celsius = {23.0, Support::available, "hwmon"};
    merge_linux_nvml(drm, s);
    assert(drm[0].utilization_percent.support == Support::permission_required);
    assert(drm[0].temperature_celsius.value == 23.0); // valid DRM fallback is not discarded
    init_error = 0;
    auto throwing = api;
    throwing.count = [](unsigned*) -> int { throw std::runtime_error("injected"); };
    try { (void)read_linux_nvml(throwing); assert(false); } catch (const std::runtime_error&) {}
    assert(stopped == before + 1); // balances initialization on unwinding
    missing_uuid = bad_pci = false;
    auto optional = api;
    optional.memory = {}; optional.utilization = {}; optional.temperature = {}; optional.driver = {};
    s = read_linux_nvml(optional);
    assert(s.gpus.size() == 1 && !s.gpus[0].memory_used_bytes.value);
    assert(!s.gpus[0].utilization_percent.value && !s.gpus[0].driver_version.value);
    assert(s.gpus[0].model == "NVIDIA fixture");
    auto failed_count = api;
    failed_count.count = [](unsigned*) { return 4; };
    s = read_linux_nvml(failed_count);
    assert(s.gpus.empty() && s.support == Support::permission_required);
    auto lost = api;
    lost.handle = [](unsigned, NvmlDevice*) { return 15; };
    s = read_linux_nvml(lost);
    assert(s.gpus.empty() && s.support == Support::estimated);
    auto sentinel = api;
    sentinel.memory = [](NvmlDevice, NvmlMemory* m) {
        *m = {1000, 1000, std::numeric_limits<unsigned long long>::max()}; return 0;
    };
    s = read_linux_nvml(sentinel);
    assert(s.gpus.size() == 1 && !s.gpus[0].memory_used_bytes.value && !s.gpus[0].memory_available_bytes.value);
#ifdef __linux__
    const auto native = read_linux_nvml(load_linux_nvml());
    std::cout << "Native NVML: " << native.note << "; devices=" << native.gpus.size() << '\n';
    for (const auto& device : native.gpus) {
        std::cout << device.model << "; utilization=";
        if (device.utilization_percent.value) std::cout << *device.utilization_percent.value;
        else std::cout << "unavailable";
        std::cout << "; temperature=";
        if (device.temperature_celsius.value) std::cout << *device.temperature_celsius.value;
        else std::cout << "unavailable";
        std::cout << "; memory=";
        if (device.memory_used_bytes.value) std::cout << *device.memory_used_bytes.value;
        else std::cout << "unavailable";
        std::cout << '\n';
    }
    Monitor monitor;
    SampleOptions options; options.include_gpus = false;
    assert(monitor.sample(options).gpus.empty());
    options.include_gpus = true;
    const auto combined = monitor.sample(options);
    for (const auto& device : native.gpus) {
        const auto found = std::find_if(combined.gpus.begin(), combined.gpus.end(), [&](const GpuInfo& row) {
            return row.id == device.id || (row.physical_location.value && device.physical_location.value &&
                nvml_pci_identity(*row.physical_location.value) == nvml_pci_identity(*device.physical_location.value));
        });
        assert(found != combined.gpus.end());
    }
#endif
}
