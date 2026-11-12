#include "linux_disk_properties.hpp"
#include <charconv>
#include <limits>
namespace pcinsight::detail {
namespace {
std::string_view trim_end(std::string_view text){
    while(!text.empty()&&(text.back()==' '||text.back()=='\t'||text.back()=='\r'||text.back()=='\n'))text.remove_suffix(1);
    return text;
}
}
Metric<std::uint64_t> linux_disk_capacity(std::string_view text){
    if(text.size()>128)return {{},Support::unsupported,"Invalid/oversized sysfs block size"};
    text=trim_end(text);
    if(text.empty())return {{},Support::unsupported,"Sysfs block size unavailable"};
    std::uint64_t sectors{};
    const auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),sectors);
    if(error!=std::errc{}||end!=text.data()+text.size())return {{},Support::unsupported,"Invalid sysfs block size"};
    if(sectors>std::numeric_limits<std::uint64_t>::max()/512)return {{},Support::unsupported,"Block capacity in bytes exceeds uint64 range"};
    return {sectors*512,Support::available,"Visible block-device size in 512-byte sectors; not manufacturer raw capacity"};
}
void apply_linux_disk_properties(DiskInfo& disk,std::string_view sectors,std::string_view rotational,std::string_view subsystem){
    disk.capacity_bytes=linux_disk_capacity(sectors);
    disk.rotational={std::nullopt,Support::unsupported,"Sysfs queue/rotational unavailable or invalid"};
    disk.media_type="unknown";
    if(rotational.size()<=128){
        rotational=trim_end(rotational);
        if(rotational=="0"||rotational=="1"){
            const bool value=rotational=="1";
            disk.rotational={value,Support::available,"Kernel queue/rotational hint; not proof of the physical medium behind virtual/stacked devices"};
            disk.media_type=value?"rotational":"non-rotational";
        }
    }
    disk.reported_media_type={std::nullopt,Support::unsupported,"Queue rotation hint does not establish HDD/SSD/SCM medium; no physical-medium provider selected"};
    // The native subsystem is a reported category, not a guessed SATA/SAS transport.
    disk.bus_type="unknown";
    if(!subsystem.empty()&&subsystem.size()<=128&&subsystem.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")==subsystem.npos)
        disk.bus_type=subsystem=="nvme"?"NVMe":std::string(subsystem);
}
}
