#include "linux_opencl_fixture_api.hpp"
extern "C" {
pcinsight::detail::ClInt clGetPlatformIDs(pcinsight::detail::ClUint n,void** out,pcinsight::detail::ClUint* count){return opencl_fixture::platforms(n,out,count);}
pcinsight::detail::ClInt clGetDeviceIDs(void* platform,pcinsight::detail::ClType type,pcinsight::detail::ClUint n,void** out,pcinsight::detail::ClUint* count){return opencl_fixture::devices(platform,type,n,out,count);}
pcinsight::detail::ClInt clGetDeviceInfo(void* device,pcinsight::detail::ClUint key,std::size_t size,void* out,std::size_t* returned){return opencl_fixture::info(device,key,size,out,returned);}
}
