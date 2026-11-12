#pragma once
#ifdef _WIN32
#include "pcinsight/startup.hpp"
#include "windows_startup_folders.hpp"
#include <functional>
namespace pcinsight::detail {
struct WindowsStartupFilePropertiesApi {
    decltype(&CreateFileW) open=&CreateFileW;
    decltype(&CloseHandle) close=&CloseHandle;
    decltype(&GetFileType) type=&GetFileType;
    decltype(&GetFileInformationByHandle) info=&GetFileInformationByHandle;
    decltype(&ReadFile) read=&ReadFile;
};
using StartupFolderInventory=std::function<std::vector<StartupEntry>()>;
StartupPropertiesResult get_windows_startup_file_properties(const std::string&,const StartupFolderInventory&,WindowsStartupFilePropertiesApi = {});
StartupPropertiesResult get_windows_startup_file_properties(const std::string&);
}
#endif
