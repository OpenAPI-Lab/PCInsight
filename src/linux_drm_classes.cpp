#include "pcinsight/linux_drm.hpp"
namespace pcinsight {
Metric<LinuxDrmEngineClass> classify_linux_drm_engine(std::string_view driver,std::string_view engine){
    using C=LinuxDrmEngineClass;
    auto known=[](C c,const char* source)->Metric<C>{return {c,Support::available,std::string("Native engine capability class from ")+source+"; NOT observed workload type, exclusive 3D utilization, or proof that counters exist"};};
    if(driver.empty() || driver.size()>128 || engine.empty() || engine.size()>128)return {{},Support::unsupported,"Invalid or unbounded DRM driver/engine name"};
    if(driver=="amdgpu"){
        constexpr auto source="amdgpu fdinfo IP-name mapping (Linux v6.12), graphics/compute IP semantics";
        if(engine=="gfx")return known(C::graphics_compute,source);
        if(engine=="compute")return known(C::compute,source);
        if(engine=="dma")return known(C::copy,source);
        if(engine=="dec")return known(C::video_decode,source);
        if(engine=="enc" || engine=="enc_1")return known(C::video_encode,source);
        if(engine=="jpeg")return known(C::jpeg,source);
        if(engine=="vpe")return known(C::video_processing,source);
    }else if(driver=="i915"){
        constexpr auto source="i915 fdinfo uabi_class_names and Intel engine-class documentation";
        if(engine=="render")return known(C::graphics_compute,source);
        if(engine=="compute")return known(C::compute,source);
        if(engine=="copy")return known(C::copy,source);
        if(engine=="video")return known(C::video_codec,source);
        if(engine=="video-enhance")return known(C::video_processing,source);
    }else if(driver=="xe"){
        // Taxonomy only. Current Xe cycle-only records still need a cycle reader.
        constexpr auto source="Xe fdinfo class names and Intel command-streamer documentation";
        if(engine=="rcs")return known(C::graphics_compute,source);
        if(engine=="ccs")return known(C::compute,source);
        if(engine=="bcs")return known(C::copy,source);
        if(engine=="vcs")return known(C::video_codec,source);
        if(engine=="vecs")return known(C::video_processing,source);
    }
    return {{},Support::unsupported,"No exact documented mapping for this DRM driver/engine pair; retain native name and counters"};
}
}
