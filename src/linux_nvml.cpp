#include "linux_nvml.hpp"
#include "linux_gpu_driver_date.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <map>
#include <set>
#ifdef __linux__
#include <dlfcn.h>
#endif

namespace pcinsight::detail {
static_assert(sizeof(unsigned) == 4 && sizeof(unsigned long long) == 8);
static_assert(sizeof(NvmlMemory) == 24 && sizeof(NvmlUtilization) == 8 && sizeof(NvmlPci) == 68);
namespace {
constexpr int success = 0, no_permission = 4;
template<class T> Metric<T> unavailable(int error, const char* what) {
    return {std::nullopt, error == no_permission ? Support::permission_required : Support::unsupported,
        std::string("NVML ") + what + ": error " + std::to_string(error)};
}
template<class T> Metric<T> available(T value, const char* note) {
    return {std::move(value), Support::available, note};
}
template<std::size_t N> std::optional<std::string> text(const std::array<char, N>& buffer) {
    auto end = std::find(buffer.begin(), buffer.end(), '\0');
    if (end == buffer.end() || end == buffer.begin()) return {};
    if (std::any_of(buffer.begin(), end, [](char c) { return static_cast<unsigned char>(c) < 32; })) return {};
    return std::string(buffer.begin(), end);
}
}
std::string nvml_pci_identity(std::string_view value) {
    // Four/eight hex domain digits, two bus/device digits, one function digit.
    const auto colon = value.find(':');
    if ((colon != 4 && colon != 8) || value.size() != colon + 8 ||
        value[colon + 3] != ':' || value[colon + 6] != '.') return {};
    unsigned parts[4]{};
    const std::string_view fields[]{value.substr(0, colon), value.substr(colon + 1, 2),
        value.substr(colon + 4, 2), value.substr(colon + 7, 1)};
    for (unsigned i = 0; i < 4; ++i) {
        const auto result = std::from_chars(fields[i].data(), fields[i].data() + fields[i].size(), parts[i], 16);
        if (result.ec != std::errc{} || result.ptr != fields[i].data() + fields[i].size()) return {};
    }
    if (parts[2] > 31 || parts[3] > 7) return {};
    return std::to_string(parts[0]) + ":" + std::to_string(parts[1]) + ":" +
        std::to_string(parts[2]) + "." + std::to_string(parts[3]);
}
namespace {
template<class T> void overlay(Metric<T>& destination, const Metric<T>& source) {
    if (source.value || !destination.value) destination = source;
}
}

NvmlApi load_linux_nvml() {
    NvmlApi api;
#ifdef __linux__
    // Driver-installed SONAME, resolved by the system loader. No SDK, subprocess,
    // install, global symbol export, or absolute vendor path required.
    void* library = dlopen("libnvidia-ml.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!library) return api;
    api.library = std::shared_ptr<void>(library, [](void* p) { dlclose(p); });
    api.init = reinterpret_cast<int(*)()>(dlsym(library, "nvmlInit_v2"));
    api.shutdown = reinterpret_cast<int(*)()>(dlsym(library, "nvmlShutdown"));
    api.count = reinterpret_cast<int(*)(unsigned*)>(dlsym(library, "nvmlDeviceGetCount_v2"));
    api.handle = reinterpret_cast<int(*)(unsigned, NvmlDevice*)>(dlsym(library, "nvmlDeviceGetHandleByIndex_v2"));
    api.name = reinterpret_cast<int(*)(NvmlDevice, char*, unsigned)>(dlsym(library, "nvmlDeviceGetName"));
    api.uuid = reinterpret_cast<int(*)(NvmlDevice, char*, unsigned)>(dlsym(library, "nvmlDeviceGetUUID"));
    api.driver = reinterpret_cast<int(*)(char*, unsigned)>(dlsym(library, "nvmlSystemGetDriverVersion"));
    api.pci = reinterpret_cast<int(*)(NvmlDevice, NvmlPci*)>(dlsym(library, "nvmlDeviceGetPciInfo_v3"));
    api.memory = reinterpret_cast<int(*)(NvmlDevice, NvmlMemory*)>(dlsym(library, "nvmlDeviceGetMemoryInfo"));
    api.utilization = reinterpret_cast<int(*)(NvmlDevice, NvmlUtilization*)>(dlsym(library, "nvmlDeviceGetUtilizationRates"));
    api.temperature = reinterpret_cast<int(*)(NvmlDevice, unsigned, unsigned*)>(dlsym(library, "nvmlDeviceGetTemperature"));
#endif
    return api;
}

NvmlSnapshot read_linux_nvml(const NvmlApi& api) {
    NvmlSnapshot result;
    if (!api.init || !api.shutdown || !api.count || !api.handle) {
        result.note = "NVML no instalado o faltan símbolos de inventario v2";
        return result;
    }
    int error = api.init();
    if (error != success) {
        result.support = error == no_permission ? Support::permission_required : Support::unsupported;
        result.note = "NVML init: error " + std::to_string(error);
        return result;
    }
    struct Session { const NvmlApi& api; ~Session() { api.shutdown(); } } session{api};
    unsigned count{};
    error = api.count(&count);
    if (error != success || count > 4096) {
        result.support = error == no_permission ? Support::permission_required : Support::unsupported;
        result.note = "NVML inventario fallido o excesivo: error " + std::to_string(error);
        return result;
    }
    Metric<std::string> driver = unavailable<std::string>(13, "driver: símbolo ausente");
    if (api.driver) {
        std::array<char, 256> buffer{};
        error = api.driver(buffer.data(), static_cast<unsigned>(buffer.size())); // fixed 256-byte array
        auto value = text(buffer);
        driver = error == success && value ? available(*value, "NVML versión del driver") :
            unavailable<std::string>(error == success ? 2 : error, "driver");
    }
    std::set<std::string> ids;
    for (unsigned i = 0; i < count; ++i) {
        NvmlDevice handle{};
        error = api.handle(i, &handle);
        if (error != success || !handle) {
            result.note = "NVML inventario parcial: uno o más dispositivos inaccesibles";
            continue;
        }
        GpuInfo g;
        g.vendor = "NVIDIA";
        g.driver_version = driver;
        g.physical_location = unavailable<std::string>(13, "PCI: símbolo ausente");
        if (api.uuid) {
            std::array<char, 128> buffer{};
            error = api.uuid(handle, buffer.data(), static_cast<unsigned>(buffer.size())); // fixed 128-byte array
            if (auto value = text(buffer); error == success && value) g.id = "nvml:" + *value;
        }
        if (api.pci) {
            NvmlPci pci{};
            error = api.pci(handle, &pci);
            std::array<char, 32> buffer{};
            std::copy(std::begin(pci.bus_id), std::end(pci.bus_id), buffer.begin());
            auto value = text(buffer);
            g.physical_location = error == success && value && !nvml_pci_identity(*value).empty()
                ? available(*value, "NVML PCI domain:bus:device.function")
                : unavailable<std::string>(error == success ? 2 : error, "PCI");
        }
        if (g.id.empty() && g.physical_location.value) g.id = "nvml:pci:" + nvml_pci_identity(*g.physical_location.value);
        if (g.id.empty() || !ids.insert(g.id).second) {
            result.note = "NVML inventario parcial: identidad ausente o duplicada";
            continue; // a transient enumeration index is not a stable identity
        }
        if (api.name) {
            std::array<char, 256> buffer{};
            error = api.name(handle, buffer.data(), static_cast<unsigned>(buffer.size())); // fixed 256-byte array
            if (auto value = text(buffer); error == success && value) g.model = *value;
        }
        g.utilization_percent = unavailable<double>(13, "utilización: símbolo ausente");
        if (api.utilization) {
            NvmlUtilization use{};
            error = api.utilization(handle, &use);
            g.utilization_percent = error == success && use.gpu <= 100
                ? available(static_cast<double>(use.gpu), "NVML tiempo con kernels activos; no es motor 3D/copy")
                : unavailable<double>(error == success ? 2 : error, "utilización");
        }
        // Memory-interface activity is not allocated memory or a copy engine.
        g.memory_used_bytes = unavailable<std::uint64_t>(13, "memoria: símbolo ausente");
        g.memory_available_bytes = g.dedicated_memory_bytes = g.memory_used_bytes;
        if (api.memory) {
            NvmlMemory memory{};
            error = api.memory(handle, &memory);
            const auto sentinel = std::numeric_limits<unsigned long long>::max();
            const bool valid = memory.total != sentinel && memory.free != sentinel && memory.used != sentinel &&
                memory.free <= memory.total && memory.used <= memory.total && memory.free == memory.total - memory.used;
            if (error == success && valid) {
                g.dedicated_memory_bytes = available<std::uint64_t>(memory.total, "NVML memoria framebuffer reportada");
                g.memory_used_bytes = available<std::uint64_t>(memory.used, "NVML v1: incluye memoria reservada por driver");
                g.memory_available_bytes = available<std::uint64_t>(memory.free, "NVML framebuffer sin asignar");
            } else {
                g.memory_used_bytes = unavailable<std::uint64_t>(error == success ? 2 : error, "memoria");
                g.memory_available_bytes = g.dedicated_memory_bytes = g.memory_used_bytes;
            }
        }
        g.temperature_celsius = unavailable<double>(13, "temperatura: símbolo ausente");
        if (api.temperature) {
            unsigned temperature{};
            error = api.temperature(handle, 0, &temperature); // NVML_TEMPERATURE_GPU
            g.temperature_celsius = error == success && temperature <= 255
                ? available(static_cast<double>(temperature), "NVML temperatura GPU Celsius")
                : unavailable<double>(error == success ? 2 : error, "temperatura");
        }
        g.shared_memory_bytes = {std::nullopt, Support::unsupported, "NVML no proporciona reserva de memoria compartida estilo Windows"};
        g.driver_date = linux_gpu_driver_date_unavailable(LinuxGpuDriverDateProvider::nvml);
        g.dedicated_memory_used_bytes = g.memory_used_bytes;
        g.shared_memory_used_bytes = {std::nullopt,Support::unsupported,"NVML does not provide Windows-style shared GPU usage"};
        result.gpus.push_back(std::move(g));
    }
    result.support = result.note.empty() ? Support::available : Support::estimated;
    if (result.note.empty()) result.note = "inventario NVML completo";
    return result;
}

void merge_linux_nvml(std::vector<GpuInfo>& drm, const NvmlSnapshot& nvml) {
    if (nvml.gpus.empty() && nvml.support != Support::available) {
        for (auto& gpu : drm) {
            if (gpu.vendor != "0x10de" && gpu.vendor != "NVIDIA" &&
                !(gpu.hardware_vendor_id.support == Support::available && gpu.hardware_vendor_id.value == 0x10deU)) continue;
            auto missing = [&](auto& metric) {
                if (!metric.value) { metric.support = nvml.support; metric.note = nvml.note; }
            };
            missing(gpu.utilization_percent);
            missing(gpu.memory_used_bytes);
            missing(gpu.dedicated_memory_used_bytes);
            missing(gpu.memory_available_bytes);
            missing(gpu.dedicated_memory_bytes);
            missing(gpu.temperature_celsius);
            missing(gpu.driver_version);
        }
    }
    std::map<std::string, std::size_t> drm_counts, nvml_counts;
    for (const auto& g : drm) if (g.physical_location.value && g.physical_location.support==Support::available) ++drm_counts[nvml_pci_identity(*g.physical_location.value)];
    for (const auto& g : nvml.gpus) if (g.physical_location.value && g.physical_location.support==Support::available) ++nvml_counts[nvml_pci_identity(*g.physical_location.value)];
    const auto original = drm.size();
    for (const auto& g : nvml.gpus) {
        const auto key = g.physical_location.value && g.physical_location.support==Support::available ? nvml_pci_identity(*g.physical_location.value) : "";
        GpuInfo* target = nullptr;
        if (!key.empty() && drm_counts[key] == 1 && nvml_counts[key] == 1) {
            for (std::size_t i = 0; i < original; ++i)
                if (drm[i].physical_location.value && drm[i].physical_location.support==Support::available && nvml_pci_identity(*drm[i].physical_location.value) == key) { target = &drm[i]; break; }
        }
        if (!target) {
            // Ambiguous PCI mappings stay separate rather than overwrite another GPU.
            drm.push_back(g);
            continue;
        }
        target->vendor = g.vendor;
        target->driver_date = linux_gpu_driver_date_unavailable(LinuxGpuDriverDateProvider::drm_nvml);
        if (!g.model.empty()) target->model = g.model;
        overlay(target->driver_version, g.driver_version);
        overlay(target->utilization_percent, g.utilization_percent);
        const auto& total=g.dedicated_memory_bytes;const auto& used=g.memory_used_bytes;const auto& free=g.memory_available_bytes;
        if(total.value && used.value && free.value && total.support==Support::available &&
           used.support==Support::available && free.support==Support::available && *used.value<=*total.value && *free.value==*total.value-*used.value){
            target->dedicated_memory_bytes=total;target->memory_used_bytes=used;
            target->dedicated_memory_used_bytes=used;target->memory_available_bytes=free;
        }else{
            // Never mix capacities from one provider with usage/free from another.
            if(!target->dedicated_memory_bytes.value)target->dedicated_memory_bytes={{},total.support,total.note};
            if(!target->memory_used_bytes.value)target->memory_used_bytes={{},used.support,used.note};
            if(!target->dedicated_memory_used_bytes.value)target->dedicated_memory_used_bytes={{},used.support,used.note};
            if(!target->memory_available_bytes.value)target->memory_available_bytes={{},free.support,free.note};
        }
        overlay(target->temperature_celsius, g.temperature_celsius);
        // Existing DRM engines remain independent; NVML overall != 3D/compute/copy.
    }
}
}
