#include "pcinsight/monitor.hpp"
#include "../src/windows_processes.hpp"
#include "../src/windows_version.hpp"
#include "../src/windows_startup_publisher.hpp"
#include <windows.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <thread>

int main() {
    using namespace pcinsight::detail;
    const auto now=std::chrono::steady_clock::now();
    ProcessCpuSample before{100,0,now,4};
    ProcessCpuSample after{100,10000000,now+std::chrono::seconds(1),4};
    assert(process_cpu_delta(before,after).value==25);
    after.created=101; assert(!process_cpu_delta(before,after).value);
    after.created=100; after.processors=8; assert(!process_cpu_delta(before,after).value);
    after.processors=4; after.captured=now; assert(!process_cpu_delta(before,after).value);
    after.captured=now+std::chrono::seconds(1); after.ticks=0;
    assert(process_cpu_delta(before,after).value==0);
    before.ticks=1; assert(!process_cpu_delta(before,after).value);
    before.ticks=0; after.ticks=50000000;
    assert(!process_cpu_delta(before,after).value); // impossible >100% total
    after.processors=0; assert(!process_cpu_delta(before,after).value);
    after.processors=4; after.created=0; assert(!process_cpu_delta(before,after).value);
    const auto self=[](const pcinsight::SystemSnapshot& snapshot) -> const pcinsight::ProcessInfo& {
        const auto it=std::find_if(snapshot.processes.begin(),snapshot.processes.end(),
            [](const auto& p) { return p.pid==GetCurrentProcessId(); });
        assert(it!=snapshot.processes.end()); return *it;
    };
    pcinsight::Monitor monitor;
    auto first=monitor.sample();
    const auto& process=self(first);
    assert(process.description=="PCInsight descripci\xc3\xb3n de prueba");
    const std::string expected_company="PCInsight corporaci\xc3\xb3n de prueba";
    wchar_t module[32768]{};const auto module_size=GetModuleFileNameW(nullptr,module,32768);assert(module_size && module_size<32768);
    assert(windows_file_company_name(module)==expected_company);
    assert(windows_file_company_name(L"relative.exe").empty());
    assert(windows_file_company_name(L"C:\\pcinsight-nonexistent-fixture\\absent.exe").empty());
    pcinsight::StartupEntry own_startup;own_startup.id="winrun:fixture";own_startup.command="\""+process.executable+"\" --not-executed";
    std::vector<pcinsight::StartupEntry> own_rows{own_startup};fill_windows_startup_publishers(own_rows);
    assert(own_rows[0].publisher==expected_company && own_rows[0].publisher_label.support==pcinsight::Support::estimated && own_rows[0].publisher_source_executable.value==process.executable);
    pcinsight::Monitor startup_monitor;pcinsight::SampleOptions startup_options;startup_options.include_startup_entries=true;startup_options.include_windows_startup_publisher=true;startup_options.include_processes=false;
    for(const auto& entry:startup_monitor.sample(startup_options).startup_entries)if(!entry.publisher.empty())assert(entry.publisher_label.value==entry.publisher && entry.publisher_label.support==pcinsight::Support::estimated && entry.publisher_source_executable.value);
    startup_options.include_windows_startup_publisher=false;
    startup_options.include_windows_startup_signature=true;
    for(const auto& entry:startup_monitor.sample(startup_options).startup_entries){
        assert(entry.publisher.empty() && !entry.publisher_label.value);
        if(entry.windows_signature.value){
            const auto& signature=*entry.windows_signature.value;
            assert(signature.native_status.value);
            const bool accepted=signature.status==pcinsight::WindowsSignatureStatus::trusted_local_policy;
            assert(accepted==(*signature.native_status.value==0));
            assert(!signature.signer_name.value || accepted);
            assert(!signature.source_executable.empty());
        }
    }
    startup_options.include_windows_startup_signature=false;
    for(const auto& entry:startup_monitor.sample(startup_options).startup_entries)assert(!entry.windows_signature.value);
    assert(windows_file_description(L"relative.exe").empty());
    assert(windows_file_description(L"").empty());
    assert(windows_file_description(L"C:\\pcinsight-nonexistent-fixture\\absent.exe").empty());
    assert(!process.name.empty() && !process.executable.empty() && !process.username.empty());
    assert(process.status=="active" && process.thread_count>0);
    assert(process.memory_bytes.value && *process.memory_bytes.value>0);
    FILETIME created{}, exited{}, kernel{}, user{};
    assert(GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user));
    const auto native_created = (std::uint64_t(created.dwHighDateTime)<<32) | created.dwLowDateTime;
    assert(native_created && process.windows_creation_time_100ns.value == native_created);
    assert(process.windows_creation_time_100ns.support == pcinsight::Support::available);
    assert(!process.cpu_percent.value && !process.disk_bytes_per_second.value);
    assert(!first.cpu.utilization_percent.value);
    assert(first.memory.total_bytes>0);
    assert(first.memory.reported_total_bytes.value==first.memory.total_bytes);
    assert(first.memory.reported_total_bytes.support==pcinsight::Support::available);
    assert(first.cpu.logical_processors.value && *first.cpu.logical_processors.value>0);
    assert(!first.cpu.linux_kvm_api_version.value);
    assert(first.cpu.reported_vendor.value==first.cpu.vendor&&first.cpu.reported_model.value==first.cpu.model);
    assert(first.cpu.physical_cores.value && *first.cpu.physical_cores.value>0);
    assert(first.cpu.sockets.value && *first.cpu.sockets.value>0);
    assert(*first.cpu.physical_cores.value<=*first.cpu.logical_processors.value);
    assert(first.cpu.processes.value && *first.cpu.processes.value>0);
    assert(first.cpu.threads.value && *first.cpu.threads.value>=*first.cpu.processes.value);
    assert(first.memory.available_bytes.value && first.memory.in_use_bytes.value);
    assert(*first.memory.available_bytes.value+*first.memory.in_use_bytes.value==first.memory.total_bytes);
    assert(first.memory.committed_bytes.value && first.memory.commit_limit_bytes.value);
    assert(*first.memory.committed_bytes.value<=*first.memory.commit_limit_bytes.value);
    assert(first.memory.paged_pool_bytes.value && first.memory.nonpaged_pool_bytes.value);
    if (first.memory.compressed_bytes.value)
        assert(first.memory.compressed_bytes.support==pcinsight::Support::estimated);
    if (first.cpu.base_ghz.value) {
        assert(*first.cpu.base_ghz.value>0);
        assert(first.cpu.base_ghz.support==pcinsight::Support::estimated);
    }
    if (first.memory.slots_used.value && first.memory.slots_total.value)
        assert(*first.memory.slots_used.value<=*first.memory.slots_total.value);
    bool saw_utilization=false;
    bool saw_process_cpu=false;
    for (int attempt=0; attempt<5; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const auto next=monitor.sample();
        assert(self(next).description==process.description);
        assert(self(next).windows_creation_time_100ns.value == process.windows_creation_time_100ns.value);
        if (self(next).cpu_percent.value) saw_process_cpu=true;
        assert(next.memory.type.value==first.memory.type.value);
        assert(next.memory.speed_mts.value==first.memory.speed_mts.value);
        assert(next.memory.slots_used.value==first.memory.slots_used.value);
        assert(next.cpu.uptime.value && *next.cpu.uptime.value>=*first.cpu.uptime.value);
        if (next.cpu.utilization_percent.value) {
            auto value=*next.cpu.utilization_percent.value;
            assert(std::isfinite(value) && value>=0 && value<=100);
            saw_utilization=true;
        }
    }
    assert(saw_utilization);
    assert(saw_process_cpu);
    pcinsight::SampleOptions disabled;
    disabled.include_processes=false;
    assert(monitor.sample(disabled).processes.empty());
    assert(!self(monitor.sample()).cpu_percent.value);
    pcinsight::Monitor independent;
    assert(!independent.sample().cpu.utilization_percent.value);
    std::cout << first.cpu.model << ": " << *first.cpu.logical_processors.value
              << " logical CPUs, " << first.memory.total_bytes << " usable RAM bytes\n";
    std::cout << "Firmware RAM: " << first.memory.type.value.value_or("unavailable")
              << ", " << first.memory.form_factor.value.value_or("unavailable") << '\n';
    if (first.memory.speed_mts.value) std::cout << *first.memory.speed_mts.value << " MT/s\n";
    if (first.memory.slots_used.value && first.memory.slots_total.value)
        std::cout << *first.memory.slots_used.value << '/' << *first.memory.slots_total.value << " firmware memory devices populated\n";
}
