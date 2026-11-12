#pragma once
#ifdef _WIN32
#include "windows_gpus.hpp"
#include <string>
namespace pcinsight::detail {
struct WindowsNvmlLoaderApi {
    decltype(&LoadLibraryExW) load{};
    HRESULT (*program_files_x64)(std::wstring&){};
};
// Caller owns a successful module reference and must FreeLibrary it.
struct WindowsNvmlLibrary {
    HMODULE handle{};
    DWORD error{};
    const char* note{};
};
WindowsNvmlLibrary load_windows_nvml_library(WindowsNvmlLoaderApi);
WindowsNvmlLibrary load_windows_nvml_library();
}
#endif
