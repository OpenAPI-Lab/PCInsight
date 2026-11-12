#pragma once
#ifdef _WIN32
#include "windows_disks.hpp"
#include <chrono>
#include <functional>
#include <oleauto.h>
namespace pcinsight::detail {
struct DiskMediaRow { DiskHardwareIdentity identity; unsigned media_type{}; };
struct DiskMediaInventory {
    std::vector<DiskMediaRow> rows;
    Support support=Support::unsupported;
    std::string note="Storage media inventory unavailable or not queried";
};
std::optional<DiskMediaRow> decode_windows_media_row(const VARIANT&,const VARIANT&,const VARIANT&,const VARIANT&);
DiskMediaInventory read_windows_disk_media();
void apply_windows_disk_media(std::vector<DiskInfo>&,const std::map<std::string,DiskHardwareIdentity>&,
                             const DiskMediaInventory&);
class WindowsDiskMedia {
public:
    using Provider=std::function<DiskMediaInventory()>;
    explicit WindowsDiskMedia(Provider provider=read_windows_disk_media) : provider_(std::move(provider)) {}
    void fill(std::vector<DiskInfo>&,const std::map<std::string,DiskHardwareIdentity>&,
              std::chrono::steady_clock::time_point now=std::chrono::steady_clock::now());
    void reset() { primed_=false; identities_.clear(); inventory_={}; }
private:
    Provider provider_;
    bool primed_=false;
    std::chrono::steady_clock::time_point captured_;
    std::map<std::string,DiskHardwareIdentity> identities_;
    DiskMediaInventory inventory_;
};
}
#endif
