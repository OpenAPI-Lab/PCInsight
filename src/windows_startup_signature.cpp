#include "windows_startup_signature.hpp"
#include "windows_startup_publisher.hpp"
#include <map>
#include <stdexcept>
namespace pcinsight::detail {
WindowsSignatureStatus windows_signature_status(std::int32_t code){
    using S=WindowsSignatureStatus;
    switch(static_cast<std::uint32_t>(code)){
    case 0:return S::trusted_local_policy;
    case 0x800b0100U:return S::no_embedded_signature; // TRUST_E_NOSIGNATURE, this file-mode check only.
    case 0x80096010U:case 0x800b0004U:case 0x800b0109U:case 0x800b0101U:
    case 0x800b010cU:case 0x800b0111U:case 0x800b0110U:return S::rejected;
    default:return S::unavailable;
    }
}
void fill_windows_startup_signatures(std::vector<StartupEntry>& rows,const StartupSignatureRead& read){
    if(rows.size()>50000)throw std::length_error("Startup signature inventory exceeds 50000 rows");
    std::map<std::string,Metric<WindowsStartupSignature>> cache;std::size_t output=0;
    for(auto& row:rows){
        if(!row.id.starts_with("winrun:") && !row.id.starts_with("winfolder:"))continue;
        row.windows_signature={};const auto path=windows_startup_executable_candidate(row);
        if(path.empty()){row.windows_signature.note="No supported explicit executable candidate; no PATH, shell or URL handler lookup";continue;}
        auto found=cache.find(path);
        if(found==cache.end()){
            if(!read || cache.size()>=16){row.windows_signature.note="Signature provider/budget unavailable (16 distinct path strings per sample)";continue;}
            auto value=read(path);
            if(value.value && value.support==Support::available){
                auto& v=*value.value;
                if(!v.native_status.value || v.native_status.support!=Support::available){value={{},Support::unsupported,"Signature result lacks native status"};}
                else {
                    v.status=windows_signature_status(*v.native_status.value); // Ignore injected derived classification.
                    if(v.status!=WindowsSignatureStatus::trusted_local_policy)v.signer_name={};
                    if(v.signer_name.value && (v.signer_name.support!=Support::available || v.signer_name.value->empty() || v.signer_name.value->size()>16384 || v.signer_name.value->find('\0')!=std::string::npos))v.signer_name={};
                    v.source_executable=path;v.policy="authenticode-file/local-policy/cache-only/no-revocation/default-signature";
                    v.native_status.note="WinVerifyTrust LONG; only exact zero means accepted for this policy";
                    v.signer_name.note="Certificate display name only after local-policy acceptance; not CompanyName, safety or launch authorization";
                }
            }else value.value.reset();
            found=cache.emplace(path,std::move(value)).first;
        }
        const auto& value=found->second;
        if(!value.value){row.windows_signature={{},value.support,value.note};continue;}
        const auto& v=*value.value;const auto cost=v.source_executable.size()+v.policy.size()+(v.signer_name.value?v.signer_name.value->size():0);
        if(cost>16*1024*1024-output){row.windows_signature.note="Signature text output budget exceeds 16 MiB";continue;}
        output+=cost;row.windows_signature={v,Support::available,"Authenticode file-mode policy result only; no online revocation, catalog discovery, malware or execution safety guarantee"};
    }
}
void fill_windows_startup_signatures(std::vector<StartupEntry>& rows){fill_windows_startup_signatures(rows,read_windows_startup_signature);}
}
#ifdef _WIN32
#include <algorithm>
namespace pcinsight::detail {
Metric<WindowsStartupSignature> evaluate_windows_signature(const std::wstring& path,HANDLE file,const WindowsSignatureApi& api){
    if(!api.verify || !api.provider || !api.signer || !api.name || !file || file==INVALID_HANDLE_VALUE)return {{},Support::unsupported,"Incomplete signature API/file handle"};
    WINTRUST_FILE_INFO info{};info.cbStruct=sizeof(info);info.pcwszFilePath=path.c_str();info.hFile=file;
    WINTRUST_DATA data{};data.cbStruct=sizeof(data);data.dwUIChoice=WTD_UI_NONE;data.fdwRevocationChecks=WTD_REVOKE_NONE;
    data.dwUnionChoice=WTD_CHOICE_FILE;data.pFile=&info;data.dwStateAction=WTD_STATEACTION_VERIFY;
    data.dwProvFlags=WTD_CACHE_ONLY_URL_RETRIEVAL|WTD_REVOCATION_CHECK_NONE|WTD_DISABLE_MD2_MD4;
    GUID action=WINTRUST_ACTION_GENERIC_VERIFY_V2;
    struct State{const WindowsSignatureApi& api;GUID& action;WINTRUST_DATA& data;~State(){data.dwStateAction=WTD_STATEACTION_CLOSE;api.verify(reinterpret_cast<HWND>(INVALID_HANDLE_VALUE),&action,&data);}} cleanup{api,action,data};
    const auto code=api.verify(reinterpret_cast<HWND>(INVALID_HANDLE_VALUE),&action,&data);
    WindowsStartupSignature out;out.native_status={static_cast<std::int32_t>(code),Support::available,"Native WinVerifyTrust status"};out.status=windows_signature_status(static_cast<std::int32_t>(code));
    out.policy="authenticode-file/local-policy/cache-only/no-revocation/default-signature";
    out.signer_name.note="No accepted signer display name available";
    if(code==0){
        auto* provider=api.provider(data.hWVTStateData);
        if(provider && provider->csSigners && provider->csSigners<=64){
            auto* signer=api.signer(provider,0,FALSE,0);
            if(signer && signer->csCertChain && signer->csCertChain<=64 && signer->pasCertChain && signer->pasCertChain[0].pCert){
                const auto cert=signer->pasCertChain[0].pCert;
                const auto count=api.name(cert,CERT_NAME_SIMPLE_DISPLAY_TYPE,0,nullptr,nullptr,0);
                if(count>1 && count<=4097){
                    std::vector<wchar_t> text(count,L'\0');
                    const auto read=api.name(cert,CERT_NAME_SIMPLE_DISPLAY_TYPE,0,nullptr,text.data(),count);
                    if(read==count && text.back()==L'\0' && std::find(text.begin(),text.end()-1,L'\0')==text.end()-1){
                        const auto bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(count-1),nullptr,0,nullptr,nullptr);
                        if(bytes>0 && bytes<=16384){std::string name(bytes,'\0');if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(count-1),name.data(),bytes,nullptr,nullptr)==bytes)out.signer_name={std::move(name),Support::available,"Leaf certificate display name of first signer accepted by selected WinTrust policy; not revocation-fresh or execution authorization"};}
                    }
                }
            }
        }
    }
    return {std::move(out),Support::available,"WinTrust local-policy result, cache-only and revocation disabled"};
}
Metric<WindowsStartupSignature> read_windows_startup_signature(const std::string& path){
    StartupEntry check;check.id="winfolder:signature";check.name="candidate.exe";check.source=path;
    if(windows_startup_executable_candidate(check)!=path || path.empty())return {{},Support::unsupported,"Unsupported literal signature candidate path"};
    auto fail=[](DWORD code,const char* stage)->Metric<WindowsStartupSignature>{return {{},code==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported,std::string(stage)+"; Win32 error="+std::to_string(code)};};
    const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path.data(),static_cast<int>(path.size()),nullptr,0);if(!count)return fail(GetLastError(),"Signature path invalid UTF-8");
    std::wstring native(count,L'\0');if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path.data(),static_cast<int>(path.size()),native.data(),count))return fail(GetLastError(),"Signature path conversion failed");
    const auto file=CreateFileW(native.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(file==INVALID_HANDLE_VALUE)return fail(GetLastError(),"Signature file open failed");
    struct Close{HANDLE h;~Close(){CloseHandle(h);}} close{file};BY_HANDLE_FILE_INFORMATION before{};
    if(GetFileType(file)!=FILE_TYPE_DISK)return fail(ERROR_INVALID_DATA,"Signature candidate is not a disk file");
    if(!GetFileInformationByHandle(file,&before))return fail(GetLastError(),"Signature file information unavailable");
    if(before.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT) || before.nFileSizeHigh || before.nFileSizeLow>128U*1024*1024)return fail(ERROR_INVALID_DATA,"Signature candidate kind/size unsupported (128 MiB maximum)");
    auto result=evaluate_windows_signature(native,file,{});
    if(result.value)result.value->source_executable=path;
    return result;
}
}
#else
namespace pcinsight::detail {Metric<WindowsStartupSignature> read_windows_startup_signature(const std::string&){return {{},Support::unsupported,"Native WinTrust provider requires Windows"};}}
#endif
