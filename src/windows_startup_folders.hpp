#pragma once
#ifdef _WIN32
#include "pcinsight/types.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
namespace pcinsight::detail {
struct WindowsStartupFoldersApi {
    decltype(&SHGetKnownFolderPath) path=&SHGetKnownFolderPath;
    decltype(&CoTaskMemFree) free=&CoTaskMemFree;
    decltype(&FindFirstFileW) first=&FindFirstFileW;
    decltype(&FindNextFileW) next=&FindNextFileW;
    decltype(&FindClose) close=&FindClose;
};
std::vector<StartupEntry> read_windows_startup_folders(WindowsStartupFoldersApi api = {});
}
#endif
