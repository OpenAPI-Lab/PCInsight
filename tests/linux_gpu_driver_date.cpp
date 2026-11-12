#include "../src/linux_gpu_driver_date.hpp"
#include "../src/linux_nvml.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
void unknown(const Metric<std::string>& date,const char* source) {
    assert(!date.value && date.support==Support::unsupported && !metric_is_known(date));
    assert(date.note.starts_with("Linux driver date unavailable:") && date.note.find(source)!=std::string::npos);
    assert(date.note.find("not inferred from BIOS")!=std::string::npos);
}
int main() {
    unknown(linux_gpu_driver_date_unavailable(LinuxGpuDriverDateProvider::drm_sysfs),"DRM/sysfs");
    unknown(linux_gpu_driver_date_unavailable(LinuxGpuDriverDateProvider::nvml),"NVML");
    unknown(linux_gpu_driver_date_unavailable(LinuxGpuDriverDateProvider::drm_nvml),"merged by PCI");
    unknown(linux_gpu_driver_date_unavailable(static_cast<LinuxGpuDriverDateProvider>(99)),"no registered Linux");
    NvmlApi api;int opens=0,closes=0;
    api.init=[&]{++opens;return 0;};api.shutdown=[&]{++closes;return 0;};
    api.count=[](unsigned* n){*n=1;return 0;};
    api.handle=[](unsigned n,NvmlDevice* h){assert(n==0);*h=reinterpret_cast<void*>(1);return 0;};
    api.uuid=[](NvmlDevice,char* text,unsigned capacity){const char* value="GPU-date-fixture";assert(capacity>std::strlen(value));std::strcpy(text,value);return 0;};
    api.pci=[](NvmlDevice,NvmlPci* p){std::strcpy(p->bus_id,"00000000:01:00.0");return 0;};
    for(const char* version:{"550.00", "2026-10-17", "550.00 build 2026-10-17", "20150101", "0"}) {
        api.driver=[&](char* out,unsigned capacity){assert(capacity>std::strlen(version));std::strcpy(out,version);return 0;};
        auto nvml=read_linux_nvml(api);assert(nvml.gpus.size()==1 && opens==closes);
        assert(nvml.gpus[0].driver_version.value==version);unknown(nvml.gpus[0].driver_date,"NVML");
        std::vector<GpuInfo> rows(1);rows[0].id="card0";
        rows[0].physical_location={std::string("0000:01:00.0"),Support::available,"DRM fixture"};
        rows[0].driver_date=linux_gpu_driver_date_unavailable(LinuxGpuDriverDateProvider::drm_sysfs);
        merge_linux_nvml(rows,nvml);assert(rows.size()==1 && rows[0].driver_version.value==version);
        unknown(rows[0].driver_date,"merged by PCI"); // reason used to be lost on successful merge
        rows.clear();merge_linux_nvml(rows,nvml);assert(rows.size()==1);unknown(rows[0].driver_date,"NVML");
        rows.push_back(rows.front());rows[0].driver_date=rows[1].driver_date=linux_gpu_driver_date_unavailable(LinuxGpuDriverDateProvider::drm_sysfs);
        merge_linux_nvml(rows,nvml);assert(rows.size()==3);
        unknown(rows[0].driver_date,"DRM/sysfs");unknown(rows[1].driver_date,"DRM/sysfs");unknown(rows[2].driver_date,"NVML");
    }
    api.init=[] {return 4;};const auto denied=read_linux_nvml(api);assert(denied.gpus.empty());
    std::vector<GpuInfo> rows(1);rows[0].vendor="0x10de";
    rows[0].driver_date=linux_gpu_driver_date_unavailable(LinuxGpuDriverDateProvider::drm_sysfs);
    merge_linux_nvml(rows,denied);unknown(rows[0].driver_date,"DRM/sysfs");
    // Failure of a different query must not label date support as a permission issue.
    assert(rows[0].driver_version.support==Support::permission_required);
#ifdef __linux__
    Monitor monitor;SampleOptions options;options.include_gpus=false;
    assert(monitor.sample(options).gpus.empty());
    options.include_gpus=true;const auto snapshot=monitor.sample(options);
    for(const auto& gpu:snapshot.gpus)unknown(gpu.driver_date,"Linux driver date unavailable:");
    std::cout<<"Native Linux GPU rows="<<snapshot.gpus.size()<<"; no release/installation date inferred\n";
#endif
    std::cout<<"Linux GPU driver date capability/merge tests passed\n";
}
