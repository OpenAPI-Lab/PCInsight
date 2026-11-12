#include "linux_opencl_fixture_api.hpp"
#include "pcinsight/monitor.hpp"
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace f=opencl_fixture;
LinuxOpenClApi api{f::platforms,f::devices,f::info};
std::vector<GpuInfo> targets(){std::vector<GpuInfo> g(3);g[0].physical_location={std::string("0000:03:00.0"),Support::available,"DRM"};g[1].physical_location={std::string("00000001:03:00.0"),Support::available,"NVML"};g[2].physical_location={std::string("0000:04:00.0"),Support::available,"unmatched"};for(auto& gpu:g)gpu.opencl_version={std::string("stale platform version"),Support::available,"old"};return g;}
int main(int argc,char**){
    if(argc>1){auto g=targets();fill_linux_gpu_opencl(g);assert(g[0].opencl_version.value=="OpenCL 1.2 fixture-A" && g[1].opencl_version.value=="OpenCL 3.0 fixture-B" && !g[2].opencl_version.value);std::cout<<"OpenCL loader fixture passed\n";return 0;}
    f::reset(0);auto g=targets();fill_linux_gpu_opencl(g,api);assert(g[0].opencl_version.value=="OpenCL 1.2 fixture-A" && g[1].opencl_version.value=="OpenCL 3.0 fixture-B" && !g[2].opencl_version.value);
    for(int m=1;m<=31;++m){f::reset(m);g=targets();fill_linux_gpu_opencl(g,api);assert(!g[0].opencl_version.value);if(m==21 || m==22 || m==19)assert(f::pci_calls==0);if(m==28)assert(g[1].opencl_version.value);}
    f::reset(0);g=targets();g.push_back(g[0]);fill_linux_gpu_opencl(g,api);assert(!g[0].opencl_version.value && !g[3].opencl_version.value && g[1].opencl_version.value);
    g=targets();g[0].physical_location.value="bad pci";fill_linux_gpu_opencl(g,api);assert(!g[0].opencl_version.value && g[1].opencl_version.value);
    g=targets();g[0].physical_location.support=Support::estimated;fill_linux_gpu_opencl(g,api);assert(!g[0].opencl_version.value);
    g=targets();auto missing=api;missing.info=nullptr;f::reset(0);fill_linux_gpu_opencl(g,missing);assert(!g[0].opencl_version.value && f::platform_calls==0);
    g=targets();for(auto& gpu:g)gpu.physical_location.value.reset();fill_linux_gpu_opencl(g,api);assert(f::platform_calls==0);
    g=targets();fill_linux_gpu_opencl(g);for(const auto& gpu:g)std::cout<<*gpu.physical_location.value<<" | OpenCL="<<gpu.opencl_version.value.value_or("unavailable")<<'\n';
    Monitor monitor;SampleOptions options;options.include_gpus=false;assert(monitor.sample(options).gpus.empty());options.include_gpus=true;for(const auto& gpu:monitor.sample(options).gpus)if(gpu.opencl_version.value)assert(gpu.opencl_version.note.starts_with("CL_DEVICE_VERSION"));
    std::cout<<"OpenCL per-device tests passed\n";
}
