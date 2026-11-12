#include "pcinsight/monitor.hpp"
#include "pcinsight/actions.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main(){
    pcinsight::Monitor monitor;
    auto first=monitor.sample();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto second=monitor.sample();
    assert(second.cpu.model==second.cpu.reported_model.value.value_or(""));
    assert(second.cpu.vendor==second.cpu.reported_vendor.value.value_or(""));
    if(second.cpu.reported_model.value)assert(!second.cpu.model.empty()); // heterogeneous/unknown is not an invented first CPU name
    if(!second.cpu.utilization_percent.value)
        std::cerr<<"Second Monitor CPU sample rejected: "<<second.cpu.utilization_percent.note<<std::endl;
    assert(second.cpu.utilization_percent.value);
    assert(second.memory.total_bytes>0);
    assert(second.memory.utilization_percent.value);
    assert(!second.processes.empty());
    assert(second.cpu.processes.value && *second.cpu.processes.value > 0);
    assert(second.cpu.threads.value && *second.cpu.threads.value > 0);
    assert(second.cpu.handles.value && *second.cpu.handles.value > 0);
    assert(second.cpu.handles.support == pcinsight::Support::estimated);
    pcinsight::SampleOptions disabled;
    disabled.include_processes=false;
    const auto without_processes = monitor.sample(disabled);
    assert(without_processes.processes.empty());
    assert(without_processes.cpu.processes.value && *without_processes.cpu.processes.value > 0);
    for (const auto& process : monitor.sample().processes) assert(!process.cpu_percent.value);
    for (const auto& process : second.processes) {
        assert(!process.windows_creation_time_100ns.value);
        assert(process.linux_start_ticks.value);
        assert(process.efficiency_mode_available.support == pcinsight::Support::available);
        assert(process.efficiency_mode_available.value == false);
    }
    // Invalid identity must not fall back to PID-only kill.
    assert(pcinsight::make_system_actions()->terminate_process_checked(0, 1234, true).error
        == pcinsight::ActionError::invalid_argument);
    assert(pcinsight::make_system_actions()->set_efficiency_mode_checked(0,1234,true).error
        == pcinsight::ActionError::not_supported);
}
