#pragma once
#include "types.hpp"
namespace pcinsight {
// EXPERIMENTAL, explicit call only; never used by Monitor/Sampler. The AMD
// runtime can initialize GPU heap resources while answering this query.
// These are runtime-reported estimates, NOT global free/allocatable memory.
struct WindowsAmdRuntimeMemory {
    std::string adapter_id; // session-local DXGI ID, observed D3D11/CL association
    Metric<std::uint64_t> reported_free_bytes, reported_largest_block_bytes;
    Metric<std::string> opencl_driver_version;
};
struct WindowsAmdRuntimeMemorySnapshot {
    Support support{Support::unsupported};
    std::string note;
    std::vector<WindowsAmdRuntimeMemory> gpus;
};
// Windows64 + optional system OpenCL loader/AMD extension. Other platforms
// return unsupported. DXGI inventory errors may throw std::system_error.
WindowsAmdRuntimeMemorySnapshot query_experimental_windows_amd_runtime_memory();
}
