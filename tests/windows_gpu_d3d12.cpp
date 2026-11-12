#include "../src/windows_gpu_d3d12.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
int token,calls,mode;D3D_FEATURE_LEVEL maximum;
HRESULT WINAPI create(IUnknown* adapter,D3D_FEATURE_LEVEL level,REFIID iid,void** out){
    assert(adapter==reinterpret_cast<IUnknown*>(&token));assert(!out);assert(IsEqualGUID(iid,__uuidof(ID3D12Device)));++calls;
    if(mode==1)return E_ACCESSDENIED;
    if(mode==2)return DXGI_ERROR_DEVICE_REMOVED;
    if(mode==3)return S_OK; // invalid success for validation-only request
    if(mode==4 && level==D3D_FEATURE_LEVEL_12_2)return E_INVALIDARG;
    if(mode==5)return E_FAIL;
    return level<=maximum?S_FALSE:DXGI_ERROR_UNSUPPORTED;
}
Metric<std::string> run(D3D_FEATURE_LEVEL level,int scenario=0){maximum=level;mode=scenario;calls=0;return probe_windows_gpu_d3d12(&token,create);}
}
int main(){
    const char* expected[]={"11_0","11_1","12_0","12_1","12_2"};
    const D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_12_0,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_2};
    for(int i=0;i<5;++i){auto metric=run(levels[i]);assert(metric.value==expected[i] && metric.support==Support::available && calls==5-i);}
    auto metric=run(D3D_FEATURE_LEVEL_10_0);assert(!metric.value && calls==5);
    metric=run(D3D_FEATURE_LEVEL_12_2,1);assert(!metric.value && metric.support==Support::permission_required && calls==1);
    for(int m:{2,3,5}){metric=run(D3D_FEATURE_LEVEL_12_2,m);assert(!metric.value && calls==1);}
    metric=run(D3D_FEATURE_LEVEL_12_1,4);assert(metric.value=="12_1" && metric.support==Support::estimated && calls==2);
    calls=0;assert(!probe_windows_gpu_d3d12(nullptr,create).value && calls==0);
    assert(!probe_windows_gpu_d3d12(&token,nullptr).value);
    for(const auto& gpu:read_windows_gpus())std::cout<<gpu.model<<" | D3D12="<<gpu.direct3d12_feature_level.value.value_or("unavailable")<<'\n';
    Monitor monitor;SampleOptions options;options.include_gpus=false;assert(monitor.sample(options).gpus.empty());
    options.include_gpus=true;
    for(const auto& gpu:monitor.sample(options).gpus){if(gpu.direct3d12_feature_level.value){bool known=false;for(auto text:expected)if(*gpu.direct3d12_feature_level.value==text)known=true;assert(known);}}
    options.include_gpus=false;assert(monitor.sample(options).gpus.empty());
    std::cout<<"D3D12 validation-only capability tests passed\n";
}
