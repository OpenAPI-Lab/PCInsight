#include "backend.hpp"
#include "windows_network.hpp"
#include "windows_disks.hpp"
#include "windows_disk_performance.hpp"
#include "windows_volumes.hpp"
#include "windows_disk_media.hpp"
#include "windows_gpus.hpp"
#include "windows_gpu_devices.hpp"
#include "windows_gpu_vulkan.hpp"
#include "windows_gpu_opencl.hpp"
#include "windows_current_opengl.hpp"
#include "windows_gpu_temperature.hpp"
#include "windows_gpu_memory.hpp"
#include "windows_nvml_memory.hpp"
#include "windows_gpu_engines.hpp"
#include "smbios.hpp"
#include "windows_pdh.hpp"
#include "windows_compression.hpp"
#include "windows_processes.hpp"
#include "windows_app_history.hpp"
#include "windows_services.hpp"
#include "windows_users.hpp"
#include "windows_user_resources.hpp"
#include "windows_startup.hpp"
#include "windows_shortcut.hpp"
#include "windows_startup_publisher.hpp"
#include "windows_startup_signature.hpp"
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#include <powrprof.h>
#include <cstddef>
#include <cstring>
#include <limits>

namespace pcinsight::detail {
namespace {
// Documented PROCESSOR_POWER_INFORMATION layout; some SDKs omit its declaration.
struct ProcessorPowerInfo {
    ULONG Number, MaxMhz, CurrentMhz, MhzLimit, MaxIdleState, CurrentIdleState;
};
static_assert(sizeof(ProcessorPowerInfo) == 6*sizeof(ULONG));
template<class T> Metric<T> measured(T value, std::string note = {}) {
    return {value, Support::available, std::move(note)};
}
std::uint64_t ticks(FILETIME t) {
    return (std::uint64_t(t.dwHighDateTime) << 32) | t.dwLowDateTime;
}
std::string registry_string(const wchar_t* name) {
    wchar_t buffer[512]{};
    DWORD bytes = sizeof(buffer);
    if (RegGetValueW(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", name,
        RRF_RT_REG_SZ, nullptr, buffer, &bytes) != ERROR_SUCCESS) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return {};
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, buffer, -1, result.data(), size, nullptr, nullptr);
    result.pop_back();
    return result;
}
void topology(CpuInfo& cpu) {
    DWORD bytes = 0;
    GetLogicalProcessorInformationEx(RelationAll, nullptr, &bytes);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || !bytes || bytes > 16*1024*1024) return;
    std::vector<unsigned char> buffer(bytes);
    if (!GetLogicalProcessorInformationEx(RelationAll,
        reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &bytes)) return;
    unsigned cores = 0, sockets = 0;
    std::uint64_t cache[3]{};
    std::size_t offset = 0;
    constexpr auto header = offsetof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, Processor);
    while (offset < bytes) {
        if (bytes - offset < header) return;
        // Copy avoids alignment assumptions about variable-sized records.
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX entry{};
        std::memcpy(&entry, buffer.data()+offset, header);
        if (entry.Size < header || entry.Size > bytes-offset) return;
        if (entry.Relationship == RelationProcessorCore) ++cores;
        if (entry.Relationship == RelationProcessorPackage) ++sockets;
        if (entry.Relationship == RelationCache) {
            constexpr auto needed = offsetof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, Cache)
                + offsetof(CACHE_RELATIONSHIP, CacheSize) + sizeof(DWORD);
            if (entry.Size < needed) return;
            std::memcpy(&entry, buffer.data()+offset, needed);
            if (entry.Cache.Level >= 1 && entry.Cache.Level <= 3)
                cache[entry.Cache.Level-1] += entry.Cache.CacheSize;
        }
        offset += entry.Size;
    }
    if (cores) cpu.physical_cores = measured(cores);
    if (sockets) cpu.sockets = measured(sockets);
    if (cache[0]) cpu.l1_bytes = measured(cache[0]);
    if (cache[1]) cpu.l2_bytes = measured(cache[1]);
    if (cache[2]) cpu.l3_bytes = measured(cache[2]);
}
class WindowsBackend final : public Backend {
    // Firmware inventory is static for this Monitor; recreate it after hardware changes.
    SmbiosInventory firmware_ = read_windows_smbios();
    WindowsCpuCounter grouped_cpu_;
    WindowsProcesses processes_;
    WindowsAppHistory app_history_;
    WindowsNetwork networks_;
    WindowsDiskPerformance disk_performance_;
    WindowsDiskMedia disk_media_;
    WindowsGpuMemory gpu_memory_;
    WindowsGpuEngines gpu_engines_;
    bool has_previous_ = false;
    std::uint64_t idle_ = 0, kernel_ = 0, user_ = 0;
public:
    SystemSnapshot sample(const SampleOptions& options) override {
        SystemSnapshot s;
        s.captured_at = std::chrono::system_clock::now();
        auto& cpu = s.cpu;
        cpu.model = registry_string(L"ProcessorNameString");
        cpu.vendor = registry_string(L"VendorIdentifier");
        if(!cpu.vendor.empty())cpu.reported_vendor={cpu.vendor,Support::estimated,"Windows registry CentralProcessor/0 VendorIdentifier; representative CPU, not all-package identity"};
        if(!cpu.model.empty())cpu.reported_model={cpu.model,Support::estimated,"Windows registry CentralProcessor/0 ProcessorNameString; representative CPU, not all-package identity"};
        const auto count = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
        if (count) cpu.logical_processors = measured<unsigned>(count);
        topology(cpu);
        cpu.uptime = measured(std::chrono::seconds(GetTickCount64()/1000));
        cpu.virtualization_enabled = measured(IsProcessorFeaturePresent(PF_VIRT_FIRMWARE_ENABLED) != FALSE,
            "Windows firmware virtualization feature flag; not hypervisor activity");
        cpu.base_ghz = smbios_metric(firmware_, firmware_.firmware_cpu_ghz);
        if (firmware_.firmware_cpu_ghz)
            cpu.base_ghz = {firmware_.firmware_cpu_ghz, Support::estimated,
                "SMBIOS Current Speed at boot; not guaranteed to be nominal base frequency"};
        cpu.current_ghz.note = "Processor power information unavailable";
        // This API is not group-addressable; do not label one group as the whole machine.
        if (GetActiveProcessorGroupCount() == 1) {
            grouped_cpu_.reset();
            FILETIME idle{}, kernel{}, user{};
            if (GetSystemTimes(&idle, &kernel, &user)) {
                const auto i=ticks(idle), k=ticks(kernel), u=ticks(user);
                cpu.utilization_percent.note = "Requires two samples with elapsed CPU time";
                if (has_previous_ && i>=idle_ && k>=kernel_ && u>=user_) {
                    const auto total=(k-kernel_)+(u-user_), inactive=i-idle_;
                    if (total && inactive<=total)
                        cpu.utilization_percent = measured(100.0*double(total-inactive)/double(total),
                            "Busy scheduler time; not frequency-weighted Task Manager utility");
                }
                idle_=i; kernel_=k; user_=u; has_previous_=true;
            } else {
                has_previous_=false;
                cpu.utilization_percent.note="GetSystemTimes failed";
            }
            if (count && count <= 64) {
                std::vector<ProcessorPowerInfo> power(count);
                if (CallNtPowerInformation(ProcessorInformation, nullptr, 0, power.data(),
                    static_cast<ULONG>(power.size()*sizeof(power[0]))) == 0) {
                    double mhz=0;
                    bool valid=true;
                    for (const auto& p : power) { mhz+=p.CurrentMhz; valid &= p.CurrentMhz>0; }
                    if (valid) cpu.current_ghz={mhz/count/1000.0, Support::estimated,
                        "Mean OS-reported throttled frequency; not measured instantaneous turbo clock"};
                }
            }
        } else {
            has_previous_=false;
            cpu.utilization_percent=grouped_cpu_.sample(count,GetActiveProcessorGroupCount());
        }
        auto& mem=s.memory;
        PERFORMANCE_INFORMATION p{};
        p.cb=sizeof(p);
        if (GetPerformanceInfo(&p, sizeof(p)) && p.PageSize && p.PhysicalTotal &&
            std::uint64_t(p.PhysicalTotal)<=std::numeric_limits<std::uint64_t>::max()/std::uint64_t(p.PageSize)) {
            const auto page=std::uint64_t(p.PageSize);
            mem.total_bytes=std::uint64_t(p.PhysicalTotal)*page;
            mem.reported_total_bytes=measured(mem.total_bytes,"OS-visible RAM from PSAPI PhysicalTotal * PageSize");
            mem.available_bytes=measured(std::uint64_t(p.PhysicalAvailable)*page);
            if (*mem.available_bytes.value<=mem.total_bytes) {
                mem.in_use_bytes=measured(mem.total_bytes-*mem.available_bytes.value);
                mem.utilization_percent=measured(100.0*double(*mem.in_use_bytes.value)/double(mem.total_bytes));
            }
            mem.committed_bytes=measured(std::uint64_t(p.CommitTotal)*page);
            mem.commit_limit_bytes=measured(std::uint64_t(p.CommitLimit)*page);
            mem.cached_bytes={std::uint64_t(p.SystemCache)*page, Support::estimated,
                "PSAPI SystemCache: standby plus system working set, not Task Manager Cached"};
            mem.paged_pool_bytes=measured(std::uint64_t(p.KernelPaged)*page);
            mem.nonpaged_pool_bytes=measured(std::uint64_t(p.KernelNonpaged)*page);
            cpu.processes=measured<std::uint64_t>(p.ProcessCount);
            cpu.threads=measured<std::uint64_t>(p.ThreadCount);
            cpu.handles=measured<std::uint64_t>(p.HandleCount);
            ULONGLONG installed_kib=0;
            if (GetPhysicallyInstalledSystemMemory(&installed_kib) &&
                installed_kib<=std::numeric_limits<std::uint64_t>::max()/1024 &&
                installed_kib*1024>=mem.total_bytes)
                mem.hardware_reserved_bytes={installed_kib*1024-mem.total_bytes, Support::estimated,
                    "Firmware installed RAM minus OS-visible RAM; may include boot configuration limits"};
        } else {
            mem.available_bytes.note="GetPerformanceInfo unavailable";
            mem.reported_total_bytes.note="GetPerformanceInfo total unavailable or outside uint64 range";
            mem.in_use_bytes.note=mem.available_bytes.note;
            mem.utilization_percent.note=mem.available_bytes.note;
        }
        mem.compressed_bytes=read_windows_compression();
        mem.type=smbios_metric(firmware_, firmware_.memory_type);
        mem.form_factor=smbios_metric(firmware_, firmware_.memory_form_factor);
        mem.speed_mts=smbios_metric(firmware_, firmware_.memory_speed_mts);
        mem.slots_used=smbios_metric(firmware_, firmware_.slots_used);
        mem.slots_total=smbios_metric(firmware_, firmware_.slots_total);
        if (options.include_processes || options.include_user_sessions || options.include_app_history) {
            try{s.processes=processes_.sample();}catch(...){app_history_.break_continuity();throw;}
        }
        else processes_.reset();
        if(options.include_app_history)s.windows_app_history=app_history_.sample(s.processes,s.captured_at);
        else{app_history_.reset();s.windows_app_history.note="Windows app CPU history disabled; capture resets when disabled";}
        if (options.include_services) s.services=read_windows_services();
        if (options.include_startup_entries) {
            s.startup_entries=read_windows_startup_inventory();
            if(options.include_windows_startup_shortcut_metadata)fill_windows_startup_shortcuts(s.startup_entries);
            if(options.include_windows_startup_publisher)fill_windows_startup_publishers(s.startup_entries);
            if(options.include_windows_startup_signature)fill_windows_startup_signatures(s.startup_entries);
        }
        if (options.include_user_sessions) {
            s.user_sessions=read_windows_users();
            fill_windows_user_resources(s.user_sessions,s.processes);
        }
        if (!options.include_processes) s.processes.clear();
        if (options.include_networks) s.networks=networks_.sample();
        else networks_.reset();
        if (options.include_disks) {
            std::map<std::uint32_t,std::string> numbers;
            std::map<std::string,DiskHardwareIdentity> identities;
            try { s.disks=read_windows_disks({},&numbers,&identities); }
            catch (...) { disk_performance_.reset(); disk_media_.reset(); throw; }
            disk_performance_.fill(s.disks,numbers);
            fill_windows_volumes(s.disks,numbers);
            if(options.include_disk_media) disk_media_.fill(s.disks,identities);
            else {
                disk_media_.reset();
                for(auto& disk:s.disks) disk.reported_media_type.note="Storage-provider media metadata disabled";
            }
        } else { disk_performance_.reset(); disk_media_.reset(); }
        if(options.include_gpus){s.gpus=read_windows_gpus();fill_windows_gpu_devices(s.gpus);fill_windows_gpu_temperature(s.gpus);fill_windows_gpu_vulkan(s.gpus);fill_windows_gpu_opencl(s.gpus);gpu_memory_.fill(s.gpus);fill_windows_nvml_memory(s.gpus);gpu_engines_.fill(s.gpus);if(options.include_current_opengl_context)fill_windows_current_opengl(s.gpus);}
        else {gpu_memory_.reset();gpu_engines_.reset();}
        return s;
    }
};
}
std::unique_ptr<Backend> make_backend() { return std::make_unique<WindowsBackend>(); }
}
#endif
