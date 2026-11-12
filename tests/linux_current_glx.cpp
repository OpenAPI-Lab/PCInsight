#include "linux_glx_fixture_api.hpp"
#include "linux_egl_fixture_api.hpp"
#include "pcinsight/monitor.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdlib>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace f=glx_fixture;
void sysfs_tests(){
    char pattern[]="/tmp/pcinsight-glx-pci-XXXXXX";const char* raw=::mkdtemp(pattern);assert(raw);
    const std::filesystem::path root(raw),node=root/"0000:03:00.0";
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code ec;std::filesystem::remove_all(p,ec);}} cleanup{root};
    const MesaGlInteropDeviceV1 d{1,0,3,0,0,0x1002,0x731f};
    assert(!read_linux_glx_pci(d,root.string()).value);
    std::filesystem::create_directory(node);
    auto write=[&](const char* name,const std::string& value){std::ofstream out(node/name);out<<value;assert(out.good());};
    write("vendor","0x1002\n");write("device","0x731f\n");
    assert(read_linux_glx_pci(d,root.string()).value=="0:3:0.0");
    write("device","0x731e\n");assert(!read_linux_glx_pci(d,root.string()).value);
    write("device",std::string(64,'1'));assert(!read_linux_glx_pci(d,root.string()).value);
    write("device","0x731fgarbage\n");assert(!read_linux_glx_pci(d,root.string()).value);
    write("device","0x731f\n");
    assert(!read_linux_glx_pci(d,root.string(),[&]{write("vendor","0x1003\n");}).value);
    write("vendor","0x1002\n");
    assert(!read_linux_glx_pci(d,root.string(),[&]{
        std::filesystem::rename(node,root/"old");std::filesystem::create_directory(node);
        write("vendor","0x1002\n");write("device","0x731f\n");
    }).value);
    std::filesystem::remove(node/"device");std::filesystem::create_symlink(node/"vendor",node/"device");
    assert(!read_linux_glx_pci(d,root.string()).value);
}
std::vector<GpuInfo> targets(){
    std::vector<GpuInfo> rows(2);
    rows[0].physical_location={std::string("0000:03:00.0"),Support::available,"fixture"};
    rows[1].physical_location={std::string("0001:03:00.0"),Support::available,"other PCI domain"};
    for(auto& row:rows){row.hardware_vendor_id={0x1002,Support::available,"fixture"};row.hardware_device_id={0x731f,Support::available,"fixture"};row.opengl_version={std::string("stale"),Support::available,"stale"};}
    return rows;
}
void success(const std::vector<GpuInfo>& rows){
    assert(rows[0].opengl_version.value=="4.2 (Core Profile) GLX fixture" && !rows[1].opengl_version.value);
    assert(rows[0].linux_opengl_details.value && !rows[1].linux_opengl_details.value);
    const auto& gl=*rows[0].linux_opengl_details.value;
    assert(gl.context_api=="GLX" && gl.context_version.value==rows[0].opengl_version.value);
    assert(gl.reported_max_core_profile_version.value=="4.6" && gl.reported_max_compatibility_profile_version.value=="4.5");
    assert(gl.core_profile_supported.value==true && gl.compatibility_profile_supported.value==true);
}
int main(int argc,char**){
    if(argc>1){f::reset();auto api=load_linux_current_glx();assert(api.library);api.pci_identity=f::pci;auto rows=targets();fill_linux_current_glx(rows,api);success(rows);std::cout<<"GLX dynamic query-only loader fixture passed\n";return 0;}
    sysfs_tests();
    f::reset();auto rows=targets();fill_linux_current_glx(rows,f::api());success(rows);assert(f::identities==2 && f::device_reads==2);
    for(int m=1;m<=42;++m){
        f::reset(m);rows=targets();fill_linux_current_glx(rows,f::api());
        if(m>=38){
            assert(rows[0].opengl_version.value && rows[0].linux_opengl_details.value);
            const auto& gl=*rows[0].linux_opengl_details.value;
            if(m==38 || m==40 || m==42){assert(!gl.core_profile_supported.value && !gl.reported_max_core_profile_version.value);}
            if(m==39){assert(gl.core_profile_supported.value==false && gl.core_profile_supported.support==Support::available && !gl.reported_max_core_profile_version.value);}
            if(m==41){assert(!gl.compatibility_profile_supported.value && !gl.reported_max_compatibility_profile_version.value && gl.reported_max_core_profile_version.value=="4.6");}
        }else assert(!rows[0].opengl_version.value && !rows[0].linux_opengl_details.value);
        assert(!rows[1].opengl_version.value && !rows[1].linux_opengl_details.value);
        if(m<=24 || m==33 || m==35 || m==36 || m==37)assert(!f::gl_reads);
        if(m==24)assert(rows[0].opengl_version.support==Support::permission_required);
    }
    {f::reset();rows=targets();rows.push_back(rows[0]);fill_linux_current_glx(rows,f::api());for(const auto& row:rows)assert(!row.opengl_version.value);}
    {f::reset();rows=targets();rows[0].physical_location.support=Support::estimated;fill_linux_current_glx(rows,f::api());assert(!rows[0].linux_opengl_details.value);}
    {f::reset();rows=targets();rows[0].hardware_device_id.value=1;fill_linux_current_glx(rows,f::api());assert(!rows[0].opengl_version.value);}
    {f::reset();rows=targets();auto api=f::api();api.device_info=nullptr;fill_linux_current_glx(rows,api);assert(!f::displays && !rows[0].opengl_version.value);}
    // If both APIs claim a current context, do not guess GL dispatch ownership.
    {f::reset();egl_fixture::reset(0);rows=targets();fill_linux_current_opengl(rows,egl_fixture::api(),f::api());assert(!rows[0].opengl_version.value && !f::gl_reads && !egl_fixture::gl_calls);}
    {f::reset();egl_fixture::reset(2);rows=targets();auto glx=f::api();glx.context=[]()->void*{return reinterpret_cast<void*>(2);};fill_linux_current_opengl(rows,egl_fixture::api(),glx);success(rows);}
    {f::reset(2);egl_fixture::reset(0);rows=targets();fill_linux_current_opengl(rows,egl_fixture::api(),f::api());assert(rows[0].opengl_version.value && rows[0].linux_opengl_details.value->context_api=="EGL");assert(!rows[0].linux_opengl_details.value->reported_max_core_profile_version.value);}
    auto native=load_linux_current_glx();rows=targets();fill_linux_current_glx(rows,native);
    std::cout<<"Native caller GLX context="<<(native.context && native.context()?"present":"absent")<<"; attributed="<<rows[0].opengl_version.value.has_value()<<'\n';
    SampleOptions options;assert(!options.include_current_opengl_context);options.include_current_opengl_context=true;options.include_gpus=false;
    Monitor monitor;assert(monitor.sample(options).gpus.empty());
    std::cout<<"GLX current context/profile capability tests passed\n";
}
