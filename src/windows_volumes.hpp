#pragma once
#ifdef _WIN32
#include "windows_disks.hpp"
#include <psapi.h>
namespace pcinsight::detail {
struct VolumeApi {
    decltype(&FindFirstVolumeW) first=&FindFirstVolumeW;
    decltype(&FindNextVolumeW) next=&FindNextVolumeW;
    decltype(&FindVolumeClose) finish=&FindVolumeClose;
    decltype(&GetDiskFreeSpaceExW) space=&GetDiskFreeSpaceExW;
    decltype(&GetWindowsDirectoryW) windows=&GetWindowsDirectoryW;
    decltype(&GetVolumePathNameW) root=&GetVolumePathNameW;
    decltype(&GetVolumeNameForVolumeMountPointW) name=&GetVolumeNameForVolumeMountPointW;
    decltype(&EnumPageFilesW) pages=&EnumPageFilesW;
    decltype(&CreateFileW) open=&CreateFileW;
    decltype(&CloseHandle) close=&CloseHandle;
    decltype(&DeviceIoControl) query=&DeviceIoControl;
};
void fill_windows_volumes(std::vector<DiskInfo>&,const std::map<std::uint32_t,std::string>&,VolumeApi={});
}
#endif
