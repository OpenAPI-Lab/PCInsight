#include "../src/linux_gpu_sysfs.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#ifdef __linux__
#include <cstdlib>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
Metric<std::string> value(std::string s){return {std::move(s),Support::available,"fixture"};}
LinuxGpuSysfsInputs inputs(){
    LinuxGpuSysfsInputs in;in.vendor=value("0x1002");in.device=value("0x744c");in.location=value("0000:03:00.0");
    in.driver=value("amdgpu");in.module=value("different_module");in.module_version=value("version.1");
    in.busy=value("0");in.vram_total=value("1000");in.vram_used=value("200");
    in.gtt_total=value("4000");in.gtt_used=value("500");in.sensors.push_back({"hwmon0.temp1",value("edge"),value("42500")});return in;
}
int main(){
    auto in=inputs();auto g=parse_linux_gpu_sysfs("card0",in);
    assert(g.id=="card0" && g.driver_version.value=="version.1" && g.physical_location.value=="0000:03:00.0");
    assert(g.utilization_percent.value==0 && g.engines.size()==1 && g.temperature_celsius.value==42.5);
    assert(g.memory_used_bytes.value==200 && g.dedicated_memory_used_bytes.value==200 && g.dedicated_memory_bytes.value==1000);
    assert(g.memory_available_bytes.value==800 && g.memory_available_bytes.support==Support::estimated);
    assert(!g.shared_memory_bytes.value && !g.shared_memory_used_bytes.value);
    assert(g.linux_sysfs_details.value->gtt_total_bytes.value==4000 && g.linux_sysfs_details.value->gtt_used_bytes.value==500);
    assert(g.linux_sysfs_details.value->gtt_headroom_bytes.value==3500);
    for(const char* bad:{"", "-1", "+1", "12suffix", "1.5", "NaN", "inf", "18446744073709551616", "1\n2"}){
        in=inputs();in.vram_used=value(bad);in.busy=value(bad);in.sensors[0].millidegrees=value(bad);
        g=parse_linux_gpu_sysfs("card0",in);assert(!g.memory_used_bytes.value && !g.memory_available_bytes.value && !g.utilization_percent.value);
        if(std::string_view(bad)=="-1")assert(g.temperature_celsius.value==-0.001); // signed millidegrees are valid
        else assert(!g.temperature_celsius.value);
        assert(g.dedicated_memory_bytes.value==1000 && g.linux_sysfs_details.value->gtt_used_bytes.value==500);
    }
    in=inputs();in.vram_total=value("0");in.vram_used=value("0");in.gtt_used=value("0");in.sensors[0].millidegrees=value("0");
    g=parse_linux_gpu_sysfs("card0",in);assert(g.memory_available_bytes.value==0 && g.memory_used_bytes.value==0 && g.temperature_celsius.value==0);
    in=inputs();in.vram_used=value("1001");in.busy=value("101");g=parse_linux_gpu_sysfs("card0",in);assert(!g.memory_used_bytes.value && !g.memory_available_bytes.value && !g.utilization_percent.value);
    in=inputs();in.vram_used={{},Support::permission_required,"denied"};g=parse_linux_gpu_sysfs("card0",in);assert(!g.memory_used_bytes.value && g.memory_used_bytes.support==Support::permission_required);
    in=inputs();in.module_version={};g=parse_linux_gpu_sysfs("card0",in);assert(!g.driver_version.value && g.linux_sysfs_details.value->kernel_driver_name.value=="amdgpu");
    in=inputs();in.location=value("not PCI");g=parse_linux_gpu_sysfs("card0",in);assert(!g.physical_location.value);
    in=inputs();in.sensors[0].millidegrees=value("-5000");g=parse_linux_gpu_sysfs("card0",in);assert(g.temperature_celsius.value==-5.0);
    in.sensors[0].millidegrees=value("-273151");g=parse_linux_gpu_sysfs("card0",in);assert(!g.temperature_celsius.value);
    in=inputs();in.sensors[0].label=value("junction");g=parse_linux_gpu_sysfs("card0",in);assert(!g.temperature_celsius.value && g.linux_sysfs_details.value->temperature_sensors[0].celsius.value==42.5);
    in=inputs();in.sensors.push_back({"hwmon1.temp1",value("edge"),value("50000")});g=parse_linux_gpu_sysfs("card0",in);assert(!g.temperature_celsius.value && g.linux_sysfs_details.value->temperature_sensors.size()==2);
    in=inputs();in.sensors.push_back(in.sensors[0]);g=parse_linux_gpu_sysfs("card0",in);assert(!g.temperature_celsius.value && g.linux_sysfs_details.value->temperature_sensors.empty());
    in=inputs();in.sensors[0].label={};g=parse_linux_gpu_sysfs("card0",in);assert(g.temperature_celsius.value==42.5);
    in.driver=value("other");g=parse_linux_gpu_sysfs("card0",in);assert(!g.temperature_celsius.value);
    in=inputs();in.sensors[0].millidegrees={{},Support::permission_required,"denied"};g=parse_linux_gpu_sysfs("card0",in);assert(!g.temperature_celsius.value && g.temperature_celsius.support==Support::permission_required);
    const auto drm=value("DRM/sysfs");auto source=linux_gpu_inventory_source(drm,Support::unsupported,"missing NVML");assert(source.value && source.support==Support::estimated);
    source=linux_gpu_inventory_source(drm,Support::available,"empty NVML");assert(source.value && source.support==Support::available);
    source=linux_gpu_inventory_source({},Support::permission_required,"denied");assert(!source.value && source.support==Support::permission_required);
#ifdef __linux__
    char pattern[]="/tmp/pcinsight-gpu-sysfs-XXXXXX";const char* raw=::mkdtemp(pattern);assert(raw);const std::filesystem::path root(raw);
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code ec;std::filesystem::remove_all(p,ec);}}cleanup{root};
    const auto card=root/"card0",dev=root/"0000:03:00.0",driver=root/"driver",module=root/"different_module";
    std::filesystem::create_directories(card);std::filesystem::create_directories(dev/"hwmon/hwmon0");
    std::filesystem::create_directory(driver);std::filesystem::create_directory(module);
    std::filesystem::create_directory_symlink(dev,card/"device");std::filesystem::create_directory_symlink(driver,dev/"driver");std::filesystem::create_directory_symlink(module,driver/"module");
    auto write=[](const auto& path,const auto& text){std::ofstream f(path);f<<text;assert(f.good());};
    write(dev/"vendor","0x1002\n");write(dev/"device","0x744c\n");write(module/"version","module.2\n");
    write(dev/"gpu_busy_percent","80\n");write(dev/"mem_info_vram_total","1000\n");write(dev/"mem_info_vram_used","200\n");
    write(dev/"mem_info_gtt_total","4000\n");write(dev/"mem_info_gtt_used","500\n");
    write(dev/"hwmon/hwmon0/temp1_input","42500\n");write(dev/"hwmon/hwmon0/temp1_label","edge\n");
    auto row=read_linux_gpu_sysfs_card(card);assert(row.value && row.value->driver_version.value=="module.2" && row.value->temperature_celsius.value==42.5 && !row.value->physical_location.value); // PCI-shaped directory alone is not proof
    write(dev/"gpu_busy_percent","999garbage\n");write(dev/"mem_info_vram_used","-1\n");
    row=read_linux_gpu_sysfs_card(card);assert(row.value && !row.value->utilization_percent.value && !row.value->memory_used_bytes.value && row.value->temperature_celsius.value==42.5);
    write(dev/"mem_info_vram_used",std::string(4097,'1'));row=read_linux_gpu_sysfs_card(card);assert(row.value && !row.value->memory_used_bytes.value);
    std::filesystem::remove(dev/"mem_info_vram_used");std::filesystem::create_symlink(dev/"mem_info_vram_total",dev/"mem_info_vram_used");row=read_linux_gpu_sysfs_card(card);assert(row.value && !row.value->memory_used_bytes.value);
    assert(!read_linux_gpu_sysfs_card(card,[&]{write(module/"version","changed\n");}).value);
    assert(!read_linux_gpu_sysfs_card(card,[&]{write(dev/"vendor","0x10de\n");}).value);write(dev/"vendor","0x1002\n");
    assert(!read_linux_gpu_sysfs_card(card,[&]{std::filesystem::rename(dev,root/"old");std::filesystem::create_directory(dev);}).value);
    assert(!read_linux_gpu_sysfs_card(root/"cardjunk").value);
    std::vector<GpuInfo> rows;source=fill_linux_gpu_sysfs(rows,root/"missing");assert(!source.value && rows.empty());
    const auto empty=root/"empty";std::filesystem::create_directory(empty);source=fill_linux_gpu_sysfs(rows,empty);assert(source.value && source.support==Support::available && rows.empty());
    Monitor monitor;SampleOptions options;options.include_gpus=false;auto snapshot=monitor.sample(options);assert(snapshot.gpus.empty() && !snapshot.linux_gpu_inventory_source.value && snapshot.linux_gpu_inventory_source.note=="GPU inventory disabled");
    options.include_gpus=true;snapshot=monitor.sample(options);std::cout<<"Native GPU rows="<<snapshot.gpus.size()<<"; source="<<snapshot.linux_gpu_inventory_source.value.value_or("unavailable")<<"; "<<snapshot.linux_gpu_inventory_source.note<<'\n';
#endif
    std::cout<<"Linux GPU sysfs telemetry tests passed\n";
}
