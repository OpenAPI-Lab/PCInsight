#include "pcinsight/linux_drm.hpp"
#include "../src/linux_drm_client.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;
using C=LinuxDrmEngineClass;
struct Case {const char* driver;const char* name;C category;};
int main(){
    const Case cases[]{
        {"amdgpu","gfx",C::graphics_compute},{"amdgpu","compute",C::compute},{"amdgpu","dma",C::copy},
        {"amdgpu","dec",C::video_decode},{"amdgpu","enc",C::video_encode},{"amdgpu","enc_1",C::video_encode},
        {"amdgpu","jpeg",C::jpeg},{"amdgpu","vpe",C::video_processing},
        {"i915","render",C::graphics_compute},{"i915","compute",C::compute},{"i915","copy",C::copy},
        {"i915","video",C::video_codec},{"i915","video-enhance",C::video_processing},
        {"xe","rcs",C::graphics_compute},{"xe","ccs",C::compute},{"xe","bcs",C::copy},
        {"xe","vcs",C::video_codec},{"xe","vecs",C::video_processing}
    };
    for(const auto& item:cases){
        const auto result=classify_linux_drm_engine(item.driver,item.name);
        assert(result.value==item.category && result.support==Support::available && result.note.find("NOT observed workload")!=std::string::npos);
        const auto text="drm-driver: "+std::string(item.driver)+"\ndrm-pdev: 0000:03:00.0\ndrm-client-id: 1\ndrm-engine-"+item.name+": 100 ns\n";
        auto parsed=pcinsight::detail::parse_linux_drm_fdinfo(text);assert(parsed && parsed->engines.size()==1 && parsed->engines[0].name==item.name && parsed->engines[0].engine_class.value==item.category && !parsed->engines[0].utilization_percent.value);
        parsed->pid=42;parsed->descriptor=5;parsed->process_start_ticks=100;parsed->captured_at={};
        // User-supplied class is ignored; the aggregate recomputes it from the driver/name.
        parsed->engines[0].engine_class={C::jpeg,Support::available,"forged class"};
        LinuxDrmGpuSampler sampler;auto first=sampler.sample({{*parsed,Support::available,"fixture"}});
        assert(first.value->front().engines.front().engine_class.value==item.category);
        parsed->captured_at+=std::chrono::seconds(1);parsed->engines[0].busy_nanoseconds+=100000000;
        const auto second=sampler.sample({{*parsed,Support::available,"fixture"}});
        const auto& engine=second.value->front().engines.front();assert(engine.engine_class.value==item.category && engine.name==item.name && engine.summed_client_active_percent.value==10);
    }
    for(const auto* driver:{"", "nvidia", "nouveau", "AMDGPU", "amd", "amdgpu_suffix", "i915 ", "xe2"})for(const auto* name:{"gfx","copy","compute","rcs"})assert(!classify_linux_drm_engine(driver,name).value);
    for(const auto* name:{"", "GFX", "gfx0", "gfx_suffix", "compute0", "compute1", "compute_0", "sdma", "dma0", "enc_10", "3D", "copy", "render", "gfx "})assert(!classify_linux_drm_engine("amdgpu",name).value);
    for(const auto* name:{"render0","copy0","compute0","rcs","gfx"})assert(!classify_linux_drm_engine("i915",name).value);
    for(const auto* name:{"other","rcs0","ccs0","render","copy","compute"})assert(!classify_linux_drm_engine("xe",name).value);
    assert(!classify_linux_drm_engine(std::string(129,'x'),"gfx").value && !classify_linux_drm_engine("amdgpu",std::string(129,'x')).value);
    assert(!classify_linux_drm_engine("amdgpu",std::string("gfx\0",4)).value);
    // GPU-cycle records remain a distinct basis, not fabricated nanosecond samples.
    auto xe=pcinsight::detail::parse_linux_drm_fdinfo("drm-driver: xe\ndrm-pdev: 0000:03:00.0\ndrm-client-id: 1\ndrm-cycles-rcs: 100\ndrm-total-cycles-rcs: 1000\n");
    assert(xe && xe->engines[0].counter_kind==LinuxDrmCounterKind::cycles && !xe->engines[0].utilization_percent.value);
    const auto raw=pcinsight::detail::parse_linux_drm_fdinfo("drm-driver: future\ndrm-pdev: 0000:03:00.0\ndrm-client-id: 1\ndrm-engine-compute: 10 ns\n");assert(raw && !raw->engines[0].engine_class.value && raw->engines[0].busy_nanoseconds==10);
    std::cout<<"DRM native engine class tests passed\n";
}
