#include "windows_nvml_loader.hpp"
#ifdef _WIN32
#include <shlobj.h>
#include <cwchar>
#include <iterator>
namespace pcinsight::detail {
namespace {
constexpr std::size_t maximum_path=32767;
constexpr wchar_t suffix[]=L"\\NVIDIA Corporation\\NVSMI\\nvml.dll";
bool valid_root(std::wstring& path){
    if(path.size()<3 || path.size()>maximum_path-std::size(suffix) ||
       !((path[0]>=L'A' && path[0]<=L'Z') || (path[0]>=L'a' && path[0]<=L'z')) || path[1]!=L':' || path[2]!=L'\\')return false;
    // Restrict to ordinary absolute drive paths. No UNC/device namespaces,
    // environment expansions, alternate streams, Win32 normalization or traversal.
    for(std::size_t i=2;i<path.size();++i){const auto c=path[i];if(c<32 || c==L':' || c==L'/' || c==L'"' || c==L'<' || c==L'>' || c==L'|' || c==L'?' || c==L'*' || c==L'%')return false;}
    if(path.size()>3 && path.back()==L'\\')path.pop_back();
    if(path.size()==3)return false; // never treat an entire drive as Program Files
    std::size_t offset=3;
    while(offset<path.size()){
        const auto end=path.find(L'\\',offset);const auto count=(end==std::wstring::npos?path.size():end)-offset;
        if(!count || path[offset+count-1]==L'.' || path[offset+count-1]==L' ')return false;
        offset=end==std::wstring::npos?path.size():end+1;
    }
    return path.back()!=L'\\';
}
HRESULT program_files(std::wstring& out){
    struct Text {PWSTR value{};~Text(){CoTaskMemFree(value);}} text;
    const auto status=SHGetKnownFolderPath(FOLDERID_ProgramFilesX64,KF_FLAG_DONT_VERIFY,nullptr,&text.value);
    if(status!=S_OK || !text.value)return status==S_OK?E_UNEXPECTED:status;
    // SHGetKnownFolderPath guarantees its owned output is NUL terminated.
    const auto count=std::wcslen(text.value);if(count>=maximum_path)return E_INVALIDARG;
    out.assign(text.value,count);return S_OK;
}
bool absent(DWORD error){return error==ERROR_MOD_NOT_FOUND || error==ERROR_DLL_NOT_FOUND || error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND;}
}
WindowsNvmlLibrary load_windows_nvml_library(WindowsNvmlLoaderApi api){
    if(!api.load)return {nullptr,ERROR_PROC_NOT_FOUND,"NVML loader API unavailable"};
    if(auto handle=api.load(L"nvml.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32))return {handle,ERROR_SUCCESS,"NVML loaded from System32 search"};
    const auto system_error=GetLastError();
    // Never bypass access denial or a bad image with another installed copy.
    if(!absent(system_error))return {nullptr,system_error?system_error:ERROR_GEN_FAILURE,"NVML System32 load failed; legacy fallback not permitted for this error"};
    if(!api.program_files_x64)return {nullptr,ERROR_PROC_NOT_FOUND,"ProgramFilesX64 resolver unavailable"};
    std::wstring root;const auto status=api.program_files_x64(root);
    if(status!=S_OK)return {nullptr,static_cast<DWORD>(status==E_ACCESSDENIED?ERROR_ACCESS_DENIED:ERROR_PATH_NOT_FOUND),"NVML legacy ProgramFilesX64 folder unavailable"};
    if(!valid_root(root))return {nullptr,ERROR_INVALID_NAME,"NVML legacy ProgramFilesX64 path rejected"};
    root+=suffix;
    if(auto handle=api.load(root.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32))return {handle,ERROR_SUCCESS,"NVML loaded from ProgramFilesX64/NVIDIA Corporation/NVSMI"};
    const auto error=GetLastError();return {nullptr,error?error:ERROR_GEN_FAILURE,"NVML legacy ProgramFilesX64/NVIDIA Corporation/NVSMI load failed"};
}
WindowsNvmlLibrary load_windows_nvml_library(){
    return load_windows_nvml_library({&LoadLibraryExW,program_files});
}
}
#endif
