#include "windows_version.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winver.h>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <vector>

namespace pcinsight::detail {
namespace {
bool within(const std::vector<unsigned char>& data,const void* ptr,std::size_t bytes) {
    const auto start=reinterpret_cast<std::uintptr_t>(data.data());
    const auto address=reinterpret_cast<std::uintptr_t>(ptr);
    return ptr && address>=start && address-start<=data.size() && bytes<=data.size()-(address-start);
}
}
static std::string file_property(const std::wstring& path,const wchar_t* property) {
    // Never let the version API use its executable/DLL search path.
    if (path.empty() || path.find(L'\0')!=std::wstring::npos || !std::filesystem::path(path).is_absolute()) return {};
    DWORD ignored=0;
    const auto size=GetFileVersionInfoSizeW(path.c_str(),&ignored);
    if (!size || size>4*1024*1024) return {};
    std::vector<unsigned char> data(size);
    if (!GetFileVersionInfoW(path.c_str(),0,size,data.data())) return {};
    void* translation=nullptr; UINT bytes=0;
    if (!VerQueryValueW(data.data(),L"\\VarFileInfo\\Translation",&translation,&bytes) ||
        bytes%4 || bytes>4096 || !within(data,translation,bytes)) return {};
    // Use resource-provided language/codepage pairs, not a hard-coded English ID.
    for (std::size_t offset=0;offset<bytes;offset+=4) {
        WORD pair[2]{};
        std::memcpy(pair,static_cast<const unsigned char*>(translation)+offset,4);
        wchar_t key[64]{};
        if (std::swprintf(key,64,L"\\StringFileInfo\\%04x%04x\\%ls",
            static_cast<unsigned>(pair[0]),static_cast<unsigned>(pair[1]),property)<0) continue;
        void* value=nullptr; UINT chars=0;
        if (!VerQueryValueW(data.data(),key,&value,&chars) || !chars || chars>32768 ||
            !within(data,value,std::size_t(chars)*sizeof(wchar_t))) continue;
        std::wstring text(chars,L'\0');
        std::memcpy(text.data(),value,chars*sizeof(wchar_t));
        const auto end=text.find(L'\0');
        if (end==std::wstring::npos || end==0) continue;
        const int count=static_cast<int>(end);
        const auto utf8_size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),count,nullptr,0,nullptr,nullptr);
        if (!utf8_size) continue;
        std::string result(utf8_size,'\0');
        if (WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),count,result.data(),utf8_size,nullptr,nullptr))
            return result;
    }
    return {};
}
std::string windows_file_description(const std::wstring& path){return file_property(path,L"FileDescription");}
std::string windows_file_company_name(const std::wstring& path){return file_property(path,L"CompanyName");}
}
#endif
