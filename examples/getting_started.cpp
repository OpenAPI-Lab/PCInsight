// Build against an installed PCInsight SDK; see docs/GETTING_STARTED_CPP.md.
#include <pcinsight/monitor.hpp>
#include <pcinsight/sdk_headers.hpp>
#include <chrono>
#include <iostream>
#include <thread>

template<class T>
void show(const char* label,const pcinsight::Metric<T>& value,const char* unit) {
    std::cout<<label<<": ";
    if(pcinsight::metric_is_known(value)) {
        std::cout<<*value.value<<' '<<unit;
        if(value.support==pcinsight::Support::estimated)std::cout<<" (estimated)";
    }else std::cout<<"unknown: "<<value.note;
    std::cout<<'\n';
}

int main() {
    pcinsight::require_matching_sdk_headers();
    pcinsight::Monitor monitor;
    pcinsight::SampleOptions options;
    options.include_processes=true;
    // Keep the introduction small. Enable these to read those inventories too.
    options.include_disks=options.include_networks=options.include_gpus=false;
    monitor.sample(options); // Baseline, not a measured zero-percent sample.
    for(int i=0;i<3;++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        const auto snapshot=monitor.sample(options);
        show("CPU",snapshot.cpu.utilization_percent,"%");
        show("RAM in use",snapshot.memory.in_use_bytes,"bytes");
        show("RAM available",snapshot.memory.available_bytes,"bytes");
        std::cout<<"Visible process rows: "<<snapshot.processes.size()<<"\n\n";
    }
}
