#pragma once
#include "pcinsight/types.hpp"
#include <cstddef>
#include <string_view>
namespace pcinsight::detail {
// Minimal documented OpenCL C ABI; no SDK headers or link dependency.
// Root platform/device IDs are borrowed; no contexts/subdevices are created.
#ifdef _WIN32
#define PCINSIGHT_CL_CALL __stdcall
#else
#define PCINSIGHT_CL_CALL
#endif
using ClPlatform=void*;
using ClDevice=void*;
using ClInt=std::int32_t;
using ClUint=std::uint32_t;
using ClType=std::uint64_t;
struct ClPci {ClUint domain,bus,device,function;};
inline constexpr ClUint cl_type=0x1000,cl_version=0x102f,cl_extensions=0x1030,cl_platform=0x1031,cl_pci=0x410f;
inline constexpr ClType cl_gpu=4;
inline constexpr ClUint cl_luid_valid=0x106c,cl_luid=0x106d,cl_node_mask=0x106e;
struct LinuxOpenClApi {
    ClInt (PCINSIGHT_CL_CALL *platforms)(ClUint,ClPlatform*,ClUint*){};
    ClInt (PCINSIGHT_CL_CALL *devices)(ClPlatform,ClType,ClUint,ClDevice*,ClUint*){};
    ClInt (PCINSIGHT_CL_CALL *info)(ClDevice,ClUint,std::size_t,void*,std::size_t*){};
};
enum class OpenClIdentity {linux_pci,windows_luid};
void fill_opencl_gpus(std::vector<GpuInfo>&,LinuxOpenClApi,OpenClIdentity);
// Internal bounded helpers shared by the Windows interop association provider.
bool enumerate_opencl_platforms(LinuxOpenClApi,std::vector<ClPlatform>&);
bool enumerate_opencl_gpus(LinuxOpenClApi,ClPlatform,std::vector<ClDevice>&);
std::optional<std::string> read_opencl_text(LinuxOpenClApi,ClDevice,ClUint,std::size_t);
bool has_opencl_extension(std::string_view,std::string_view);
Metric<std::string> read_opencl_device_version(LinuxOpenClApi,ClDevice);
void fill_linux_gpu_opencl(std::vector<GpuInfo>&,LinuxOpenClApi);
void fill_linux_gpu_opencl(std::vector<GpuInfo>&);
}
