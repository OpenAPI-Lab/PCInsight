#include "../src/windows_nvml_loader.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace pcinsight::detail;
namespace {
std::wstring root;
DWORD first_error,second_error;
HRESULT folder_status;
unsigned loads,folders;
bool first_success,second_success;
std::vector<std::wstring> paths;
HMODULE module=reinterpret_cast<HMODULE>(static_cast<std::uintptr_t>(1));
HMODULE WINAPI load(LPCWSTR path,HANDLE file,DWORD flags){
    assert(file==nullptr);++loads;paths.emplace_back(path);
    if(loads==1){assert(paths.back()==L"nvml.dll" && flags==LOAD_LIBRARY_SEARCH_SYSTEM32);SetLastError(first_error);return first_success?module:nullptr;}
    assert(loads==2 && flags==(LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32));
    SetLastError(second_error);return second_success?module:nullptr;
}
HRESULT folder(std::wstring& out){++folders;out=root;SetLastError(ERROR_ACCESS_DENIED);return folder_status;}
void reset(){root=L"D:\\Program Files";first_error=ERROR_MOD_NOT_FOUND;second_error=ERROR_MOD_NOT_FOUND;folder_status=S_OK;loads=folders=0;first_success=false;second_success=true;paths.clear();}
WindowsNvmlLoaderApi api{load,folder};
}
int main(){
    reset();auto result=load_windows_nvml_library(api);
    assert(result.handle==module && result.error==0 && loads==2 && folders==1);
    assert(paths[1]==L"D:\\Program Files\\NVIDIA Corporation\\NVSMI\\nvml.dll");
    reset();first_success=true;result=load_windows_nvml_library(api);assert(result.handle==module && !folders && loads==1);
    for(DWORD error:{ERROR_MOD_NOT_FOUND,ERROR_DLL_NOT_FOUND,ERROR_FILE_NOT_FOUND,ERROR_PATH_NOT_FOUND}){
        reset();first_error=error;result=load_windows_nvml_library(api);assert(result.handle==module && folders==1 && loads==2);
    }
    for(DWORD error:std::vector<DWORD>{ERROR_ACCESS_DENIED,ERROR_BAD_EXE_FORMAT,ERROR_INVALID_PARAMETER,ERROR_SHARING_VIOLATION,ERROR_GEN_FAILURE,0}){
        reset();first_error=error;result=load_windows_nvml_library(api);assert(!result.handle && result.error==(error?error:ERROR_GEN_FAILURE) && !folders && loads==1);
    }
    for(DWORD error:std::vector<DWORD>{ERROR_MOD_NOT_FOUND,ERROR_ACCESS_DENIED,ERROR_BAD_EXE_FORMAT,0}){
        reset();second_success=false;second_error=error;result=load_windows_nvml_library(api);assert(!result.handle && result.error==(error?error:ERROR_GEN_FAILURE) && folders==1 && loads==2);
    }
    for(HRESULT error:{E_FAIL,E_ACCESSDENIED,S_FALSE}){
        reset();folder_status=error;result=load_windows_nvml_library(api);assert(!result.handle && result.error==(error==E_ACCESSDENIED?ERROR_ACCESS_DENIED:ERROR_PATH_NOT_FOUND) && folders==1 && loads==1);
    }
    for(const auto& path:std::vector<std::wstring>{L"",L"C:",L"C:\\",L"C:relative",L"relative",L"\\\\server\\share",L"\\\\?\\C:\\Program Files",L"C:/Program Files",L"1:\\Program Files",L"C:\\Program Files\\..",L"C:\\Program Files\\.\\folder",L"C:\\Program Files ",L"C:\\Program Files.",L"C:\\Program Files:stream",L"C:\\Program Files\\\\folder",L"C:\\%ProgramFiles%",L"C:\\bad?path",L"C:\\bad|path",L"C:\\bad\"path",L"C:\\bad<path",L"C:\\bad>path",L"C:\\bad*path",L"C:\\Program Files\\\\",std::wstring(L"C:\\bad\0path",11),L"C:\\bad\tpath",L"C:\\"+std::wstring(32767,L'a')}){
        reset();root=path;result=load_windows_nvml_library(api);assert(!result.handle && result.error==ERROR_INVALID_NAME && loads==1 && folders==1);
    }
    for(const auto& path:std::vector<std::wstring>{L"C:\\Program Files\\",L"e:\\Programs x64",L"C:\\Programas\\Aplicaciones",L"C:\\Program\u00e1s"}){
        reset();root=path;result=load_windows_nvml_library(api);assert(result.handle==module && loads==2);
        auto normalized=path;if(normalized.back()==L'\\')normalized.pop_back();assert(paths[1]==normalized+L"\\NVIDIA Corporation\\NVSMI\\nvml.dll");
    }
    reset();auto missing=api;missing.load=nullptr;result=load_windows_nvml_library(missing);assert(!result.handle && result.error==ERROR_PROC_NOT_FOUND && !loads && !folders);
    reset();missing=api;missing.program_files_x64=nullptr;result=load_windows_nvml_library(missing);assert(!result.handle && result.error==ERROR_PROC_NOT_FOUND && loads==1 && !folders);
    // Read-only native location probe. No nvmlInit/CUDA calls, files or installs.
    result=load_windows_nvml_library();
    std::cout<<"Native NVML location probe: "<<result.note<<" | Win32="<<result.error<<'\n';
    if(result.handle)assert(FreeLibrary(result.handle));
    std::cout<<"Windows NVML loader tests passed\n";
}
