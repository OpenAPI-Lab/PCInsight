#include "windows_disks.hpp"
#ifdef _WIN32
#include <cstring>
#include <cwchar>
#include <set>
#include <array>
#include <system_error>
namespace pcinsight::detail {
namespace {
constexpr GUID disk_guid{0x53f56307,0xb6bf,0x11d0,{0x94,0xf2,0x00,0xa0,0xc9,0x1e,0xfb,0x8b}};
struct Devices {
    HDEVINFO value; DiskApi& api;
    ~Devices() { if (value != INVALID_HANDLE_VALUE) api.destroy(value); }
};
struct Handle {
    HANDLE value; DiskApi& api;
    ~Handle() { if (value != INVALID_HANDLE_VALUE) api.close(value); }
};
[[noreturn]] void failure(const char* what,DWORD error) {
    throw std::system_error(static_cast<int>(error),std::system_category(),what);
}
std::string utf8(const std::wstring& text) {
    const auto n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
    if (!n) failure("disk path Unicode",ERROR_INVALID_DATA);
    std::string result(n,'\0');
    if (!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),result.data(),n,nullptr,nullptr))
        failure("disk path Unicode conversion",GetLastError());
    return result;
}
std::string bus_name(STORAGE_BUS_TYPE type) {
    switch(type) {
        case BusTypeScsi: return "SCSI"; case BusTypeAta: return "ATA";
        case BusTypeSata: return "SATA"; case BusTypeSas: return "SAS";
        case BusTypeNvme: return "NVMe"; case BusTypeUsb: return "USB";
        case BusTypeRAID: return "RAID"; case BusTypeVirtual: return "Virtual";
        case BusTypeFileBackedVirtual: return "File-backed virtual";
        case BusTypeSpaces: return "Storage Spaces";
        default: return {};
    }
}
std::string descriptor_string(const std::vector<unsigned char>& bytes,DWORD offset) {
    if (!offset) return {};
    constexpr auto base=offsetof(STORAGE_DEVICE_DESCRIPTOR,RawDeviceProperties);
    if (offset < base || offset >= bytes.size()) return {};
    auto begin=reinterpret_cast<const char*>(bytes.data()+offset);
    auto end=static_cast<const char*>(std::memchr(begin,0,bytes.size()-offset));
    if (!end) return {};
    for (auto p=begin;p!=end;++p) if (static_cast<unsigned char>(*p)>=128) return {};
    while(begin!=end && *begin==' ') ++begin;
    while(end!=begin && end[-1]==' ') --end;
    return {begin,end};
}
void metadata(DiskInfo& disk,HANDLE handle,DiskApi& api,DiskHardwareIdentity* identity) {
    STORAGE_PROPERTY_QUERY request{}; request.PropertyId=StorageDeviceProperty; request.QueryType=PropertyStandardQuery;
    STORAGE_DESCRIPTOR_HEADER header{}; DWORD returned=0;
    if (api.query(handle,IOCTL_STORAGE_QUERY_PROPERTY,&request,sizeof request,&header,sizeof header,&returned,nullptr) &&
        returned>=sizeof header && header.Size>=sizeof(STORAGE_DEVICE_DESCRIPTOR) && header.Size<=1024*1024) {
        std::vector<unsigned char> buffer(header.Size);
        if (api.query(handle,IOCTL_STORAGE_QUERY_PROPERTY,&request,sizeof request,buffer.data(),static_cast<DWORD>(buffer.size()),&returned,nullptr) &&
            returned>=sizeof(STORAGE_DEVICE_DESCRIPTOR) && returned<=buffer.size()) {
            STORAGE_DEVICE_DESCRIPTOR descriptor{}; std::memcpy(&descriptor,buffer.data(),sizeof descriptor);
            if (descriptor.Size>=sizeof descriptor && descriptor.Size<=returned) {
                buffer.resize(descriptor.Size);
                auto vendor=descriptor_string(buffer,descriptor.VendorIdOffset);
                auto product=descriptor_string(buffer,descriptor.ProductIdOffset);
                disk.model=vendor.empty()?product:(product.empty()?vendor:vendor+" "+product);
                disk.bus_type=bus_name(descriptor.BusType);
                if(identity) {
                    identity->serial=descriptor_string(buffer,descriptor.SerialNumberOffset);
                    identity->bus=static_cast<unsigned>(descriptor.BusType);
                }
            }
        }
    }
    // Report the property literally: no seek penalty alone does not prove SSD media.
    request.PropertyId=StorageDeviceSeekPenaltyProperty;
    DEVICE_SEEK_PENALTY_DESCRIPTOR seek{};
    if (api.query(handle,IOCTL_STORAGE_QUERY_PROPERTY,&request,sizeof request,&seek,sizeof seek,&returned,nullptr) &&
        returned>=sizeof seek && seek.Size>=sizeof seek && seek.Version>=sizeof seek && seek.IncursSeekPenalty<=1)
        disk.media_type=seek.IncursSeekPenalty ? "seek-penalty" : "no-seek-penalty";
    DISK_GEOMETRY_EX geometry{};
    std::array<unsigned char,4096> geometry_buffer{};
    returned=0;
    const auto geometry_ok=api.query(handle,IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,nullptr,0,
        geometry_buffer.data(),static_cast<DWORD>(geometry_buffer.size()),&returned,nullptr);
    std::memcpy(&geometry,geometry_buffer.data(),sizeof geometry);
    if (geometry_ok && returned<=geometry_buffer.size() &&
        returned>=offsetof(DISK_GEOMETRY_EX,Data) && geometry.DiskSize.QuadPart>0)
        disk.capacity_bytes={static_cast<std::uint64_t>(geometry.DiskSize.QuadPart),Support::available,
            "OS-reported device size via disk geometry; not formatted volume capacity"};
    else {
        const auto error=geometry_ok?ERROR_INVALID_DATA:GetLastError();
        disk.capacity_bytes={std::nullopt,error==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported,
            "Disk geometry unavailable, incomplete or invalid; Win32 "+std::to_string(error)};
    }
    if(identity && disk.capacity_bytes.value) identity->capacity=*disk.capacity_bytes.value;
}
}
std::vector<DiskInfo> read_windows_disks(DiskApi api,std::map<std::uint32_t,std::string>* device_numbers,
    std::map<std::string,DiskHardwareIdentity>* hardware_identities) {
    Devices devices{api.devices(&disk_guid,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE),api};
    if (devices.value==INVALID_HANDLE_VALUE) failure("SetupDiGetClassDevs disks",GetLastError());
    std::vector<DiskInfo> disks; std::set<std::wstring> seen;
    std::map<std::uint32_t,std::string> numbers; std::set<std::uint32_t> ambiguous;
    std::map<std::string,DiskHardwareIdentity> hardware;
    for (DWORD i=0;i<4096;++i) {
        SP_DEVICE_INTERFACE_DATA device_interface{}; device_interface.cbSize=sizeof device_interface;
        if (!api.enumerate(devices.value,nullptr,&disk_guid,i,&device_interface)) {
            const auto error=GetLastError();
            if (error==ERROR_NO_MORE_ITEMS) {
                if (device_numbers) *device_numbers=std::move(numbers);
                if (hardware_identities) *hardware_identities=std::move(hardware);
                return disks;
            }
            failure("SetupDiEnumDeviceInterfaces disks",error);
        }
        DWORD bytes=0;
        if (api.detail(devices.value,&device_interface,nullptr,0,&bytes,nullptr) || GetLastError()!=ERROR_INSUFFICIENT_BUFFER ||
            bytes<sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) || bytes>65536)
            failure("disk interface detail size",ERROR_INVALID_DATA);
        std::vector<unsigned char> buffer(bytes);
        auto detail=reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buffer.data());
        detail->cbSize=sizeof *detail;
        DWORD required=0;
        if (!api.detail(devices.value,&device_interface,detail,bytes,&required,nullptr)) failure("disk interface detail",GetLastError());
        constexpr auto offset=offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_W,DevicePath);
        if (required>bytes || required<offset+sizeof(wchar_t)) failure("invalid disk detail size",ERROR_INVALID_DATA);
        const auto count=(required-offset)/sizeof(wchar_t);
        const auto end=std::wmemchr(detail->DevicePath,L'\0',count);
        if (!end || end==detail->DevicePath) failure("invalid disk interface path",ERROR_INVALID_DATA);
        std::wstring path(detail->DevicePath,end);
        if (!seen.insert(path).second) failure("duplicate disk interface path",ERROR_INVALID_DATA);
        DiskInfo disk; disk.id=utf8(path);
        disk.formatted_capacity_bytes.note="Volume mapping not yet implemented";
        disk.system_disk.note="System volume mapping not yet implemented";
        disk.contains_page_file.note="Page-file volume mapping not yet implemented";
        disk.active_percent.note=disk.average_response_ms.note="Disk performance provider not yet implemented";
        disk.read_bytes_per_second.note=disk.write_bytes_per_second.note=disk.transfer_bytes_per_second.note="Requires a disk performance provider";
        Handle handle{api.open(path.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr),api};
        if (handle.value==INVALID_HANDLE_VALUE) {
            const auto error=GetLastError();
            disk.capacity_bytes={std::nullopt,error==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported,
                "Disk open failed; Win32 "+std::to_string(error)};
        } else {
            DiskHardwareIdentity identity;
            metadata(disk,handle.value,api,hardware_identities?&identity:nullptr);
            if(hardware_identities) hardware.emplace(disk.id,std::move(identity));
            if (device_numbers) {
                STORAGE_DEVICE_NUMBER number{}; DWORD returned=0;
                if (api.query(handle.value,IOCTL_STORAGE_GET_DEVICE_NUMBER,nullptr,0,&number,sizeof number,&returned,nullptr) &&
                    returned>=sizeof number && number.DeviceType==FILE_DEVICE_DISK && number.DeviceNumber!=0xffffffffu) {
                    if (!ambiguous.contains(number.DeviceNumber)) {
                        if (!numbers.emplace(number.DeviceNumber,disk.id).second) {
                            numbers.erase(number.DeviceNumber); ambiguous.insert(number.DeviceNumber);
                        }
                    }
                }
            }
        }
        disks.push_back(std::move(disk));
    }
    failure("disk interface inventory limit",ERROR_MORE_DATA);
}
}
#endif
