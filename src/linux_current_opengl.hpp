#pragma once
#include "pcinsight/types.hpp"
#include <memory>
#include <string_view>
#include <functional>
namespace pcinsight::detail {
using EglInt=std::int32_t;
using EglBool=std::uint32_t;
using EglProc=void(*)();
struct CurrentEglApi {
    void* (*display)(){};
    void* (*context)(){};
    EglBool (*query_context)(void*,void*,EglInt,EglInt*){};
    const char* (*query_string)(void*,EglInt){};
    EglBool (*query_display)(void*,EglInt,std::intptr_t*){};
    const char* (*query_device)(void*,EglInt){};
    const unsigned char* (*gl_string)(std::uint32_t){};
    Metric<std::string> (*pci_identity)(const char*){};
    std::shared_ptr<void> library;
};
CurrentEglApi load_linux_current_egl();
void fill_linux_current_opengl(std::vector<GpuInfo>&,const CurrentEglApi&);
void fill_linux_current_opengl(std::vector<GpuInfo>&);
// Mesa device-info ABI v1 prefix; request version1, never use a newer version
// with this shortened struct. Seven uint32 fields, no pointer/native-long ABI.
struct MesaGlInteropDeviceV1 {
    std::uint32_t version{1}, pci_segment_group{}, pci_bus{}, pci_device{}, pci_function{}, vendor_id{}, device_id{};
    bool operator==(const MesaGlInteropDeviceV1&) const = default;
};
static_assert(sizeof(MesaGlInteropDeviceV1)==28);
struct CurrentGlxApi {
    void* (*display)(){};
    void* (*context)(){};
    int (*query_context)(void*,void*,int,int*){};
    const char* (*extensions)(void*,int){};
    int (*is_direct)(void*,void*){};
    int (*query_version)(void*,int*,int*){};
    int (*device_info)(void*,void*,MesaGlInteropDeviceV1*){};
    int (*renderer_integer)(int,unsigned*){};
    const unsigned char* (*gl_string)(std::uint32_t){};
    // Native sysfs confirms BDF and PCI vendor/device, not just GL strings.
    Metric<std::string> (*pci_identity)(const MesaGlInteropDeviceV1&){};
    std::shared_ptr<void> library;
};
CurrentGlxApi load_linux_current_glx();
Metric<std::string> read_linux_glx_pci(const MesaGlInteropDeviceV1&,const std::string& sysfs_root,
    const std::function<void()>& after_read={});
void fill_linux_current_glx(std::vector<GpuInfo>&,const CurrentGlxApi&);
void fill_linux_current_opengl(std::vector<GpuInfo>&,const CurrentEglApi&,const CurrentGlxApi&);
std::optional<std::string> bounded_gl_text(const char*,std::size_t);
bool valid_desktop_gl_version(std::string_view);
}
