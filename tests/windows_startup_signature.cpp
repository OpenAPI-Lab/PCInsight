#include "../src/windows_startup_signature.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <algorithm>
using namespace pcinsight;
using namespace pcinsight::detail;
using Status=WindowsSignatureStatus;
StartupEntry entry(std::string command="C:\\App.exe --never-run"){
    StartupEntry row;row.id="winrun:fixture";row.command=std::move(command);
    row.publisher="Self-reported company";return row;
}
Metric<WindowsStartupSignature> result(std::int32_t code){
    WindowsStartupSignature value;value.status=Status::trusted_local_policy;
    value.native_status={code,Support::available,"fixture"};
    value.signer_name={std::string("Fixture signer"),Support::available,"fixture"};
    return {value,Support::available,"fixture"};
}
#ifdef _WIN32
namespace {
LONG returned=0;unsigned verifies=0,closes=0,providers=0,names=0;
bool missing_provider=false,bad_name=false,throw_name=false;
CRYPT_PROVIDER_DATA provider_data{};CRYPT_PROVIDER_SGNR signer_data{};
CRYPT_PROVIDER_CERT provider_cert{};CERT_CONTEXT cert{};
HANDLE token=reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(123));
LONG WINAPI verify(HWND window,GUID* action,LPVOID raw){
    const GUID expected=WINTRUST_ACTION_GENERIC_VERIFY_V2;
    assert(window==reinterpret_cast<HWND>(INVALID_HANDLE_VALUE));
    assert(IsEqualGUID(*action,expected));
    auto& data=*static_cast<WINTRUST_DATA*>(raw);
    assert(data.cbStruct==sizeof(data) && data.dwUIChoice==WTD_UI_NONE);
    assert(data.fdwRevocationChecks==WTD_REVOKE_NONE && data.dwUnionChoice==WTD_CHOICE_FILE);
    assert(data.dwProvFlags==(WTD_CACHE_ONLY_URL_RETRIEVAL|WTD_REVOCATION_CHECK_NONE|WTD_DISABLE_MD2_MD4));
    assert(data.pFile && data.pFile->hFile==token && std::wstring(data.pFile->pcwszFilePath)==L"C:\\fixture.exe");
    if(data.dwStateAction==WTD_STATEACTION_CLOSE){++closes;assert(data.hWVTStateData==token);data.hWVTStateData=nullptr;return 0;}
    assert(data.dwStateAction==WTD_STATEACTION_VERIFY);++verifies;data.hWVTStateData=token;return returned;
}
CRYPT_PROVIDER_DATA* WINAPI provider(HANDLE state){++providers;assert(state==token);return missing_provider?nullptr:&provider_data;}
CRYPT_PROVIDER_SGNR* WINAPI signer(CRYPT_PROVIDER_DATA* data,DWORD index,BOOL counter,DWORD counter_index){
    assert(data==&provider_data && index==0 && counter==FALSE && counter_index==0);return &signer_data;
}
DWORD WINAPI name(PCCERT_CONTEXT context,DWORD type,DWORD flags,void* argument,LPWSTR output,DWORD capacity){
    ++names;assert(context==&cert && type==CERT_NAME_SIMPLE_DISPLAY_TYPE && flags==0 && !argument);
    if(throw_name)throw std::runtime_error("fixture extraction failure");
    if(bad_name)return 5000;
    constexpr wchar_t text[]=L"Fixture signer";constexpr DWORD count=sizeof(text)/sizeof(wchar_t);
    if(output){assert(capacity==count);std::copy(std::begin(text),std::end(text),output);}return count;
}
std::string utf8(const std::wstring& text){
    auto count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
    assert(count>0);std::string out(count,'\0');assert(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),out.data(),count,nullptr,nullptr)==count);return out;
}
void native_tests(){
    provider_data.csSigners=1;signer_data.csCertChain=1;signer_data.pasCertChain=&provider_cert;provider_cert.pCert=&cert;
    WindowsSignatureApi api;api.verify=verify;api.provider=provider;api.signer=signer;api.name=name;
    for(LONG code:{0L,1L,static_cast<LONG>(TRUST_E_BAD_DIGEST),static_cast<LONG>(TRUST_E_NOSIGNATURE),static_cast<LONG>(TRUST_E_PROVIDER_UNKNOWN)}){
        returned=code;verifies=closes=providers=names=0;
        auto value=evaluate_windows_signature(L"C:\\fixture.exe",token,api);
        assert(value.value && value.value->native_status.value==code && verifies==1 && closes==1);
        assert((value.value->status==Status::trusted_local_policy)==(code==0));
        assert((providers!=0)==(code==0) && (names!=0)==(code==0));
        assert(value.value->signer_name.value.has_value()==(code==0));
    }
    returned=0;missing_provider=true;closes=0;
    auto value=evaluate_windows_signature(L"C:\\fixture.exe",token,api);
    assert(value.value && value.value->status==Status::trusted_local_policy && !value.value->signer_name.value && closes==1);
    missing_provider=false;bad_name=true;closes=0;
    value=evaluate_windows_signature(L"C:\\fixture.exe",token,api);assert(value.value && !value.value->signer_name.value && closes==1);
    bad_name=false;throw_name=true;closes=0;bool threw=false;
    try{evaluate_windows_signature(L"C:\\fixture.exe",token,api);}catch(const std::runtime_error&){threw=true;}
    assert(threw && closes==1);throw_name=false;
    api.verify=nullptr;assert(!evaluate_windows_signature(L"C:\\fixture.exe",token,api).value);
    wchar_t module[32768]{};auto count=GetModuleFileNameW(nullptr,module,32768);assert(count>0 && count<32768);
    value=read_windows_startup_signature(utf8(std::wstring(module,count)));
    assert(value.value && value.value->native_status.value && value.value->status==Status::no_embedded_signature && !value.value->signer_name.value);
    std::cout<<"Native unsigned test executable: no embedded signature\n";
    wchar_t directory[MAX_PATH]{};count=GetWindowsDirectoryW(directory,MAX_PATH);assert(count>0 && count<MAX_PATH);
    value=read_windows_startup_signature(utf8(std::wstring(directory,count)+L"\\explorer.exe"));
    if(value.value){
        const auto& signature=*value.value;
        assert(signature.native_status.value && ((signature.status==Status::trusted_local_policy)==(*signature.native_status.value==0)));
        assert(!signature.signer_name.value || signature.status==Status::trusted_local_policy);
        std::cout<<"Native explorer.exe: status="<<static_cast<int>(signature.status)<<" code="<<*signature.native_status.value<<" signer="<<signature.signer_name.value.value_or("unavailable")<<'\n';
}else std::cout<<"Native explorer.exe unavailable: "<<value.note<<'\n';
}
}
#endif
int main(){
    assert(windows_signature_status(0)==Status::trusted_local_policy);
    assert(windows_signature_status(1)==Status::unavailable);
    assert(windows_signature_status(static_cast<std::int32_t>(0x800b0100U))==Status::no_embedded_signature);
    for(auto code:{0x80096010U,0x800b0004U,0x800b0109U,0x800b0101U,0x800b010cU,0x800b0111U,0x800b0110U})assert(windows_signature_status(static_cast<std::int32_t>(code))==Status::rejected);
    assert(windows_signature_status(static_cast<std::int32_t>(0x800b0001U))==Status::unavailable);
    unsigned calls=0;
    StartupSignatureRead reader=[&](const std::string&)->Metric<WindowsStartupSignature>{++calls;return result(0);};
    std::vector<StartupEntry> rows{entry(),entry()};fill_windows_startup_signatures(rows,reader);
    assert(calls==1 && rows[0].windows_signature.value && rows[0].windows_signature.value->signer_name.value=="Fixture signer");
    assert(rows[0].windows_signature.value->source_executable=="C:\\App.exe" && rows[0].publisher=="Self-reported company" && !rows[0].publisher_label.value);
    for(auto code:{1U,0x80096010U,0x800b0100U,0x800b0001U}){
        rows={entry()};fill_windows_startup_signatures(rows,[&](const std::string&){return result(static_cast<std::int32_t>(code));});
        assert(rows[0].windows_signature.value && rows[0].windows_signature.value->status!=Status::trusted_local_policy && !rows[0].windows_signature.value->signer_name.value);
    }
    for(auto support:{Support::unsupported,Support::permission_required}){
        rows={entry()};fill_windows_startup_signatures(rows,[&](const std::string&)->Metric<WindowsStartupSignature>{auto r=result(0);r.support=support;return r;});
        assert(!rows[0].windows_signature.value && rows[0].windows_signature.support==support);
    }
    rows={entry()};fill_windows_startup_signatures(rows,[](const std::string&){auto r=result(0);r.value->native_status={};return r;});assert(!rows[0].windows_signature.value);
    for(auto name:std::vector<std::string>{"",std::string("a\0b",3),std::string(16385,'x')}){
        rows={entry()};fill_windows_startup_signatures(rows,[&](const std::string&){auto r=result(0);r.value->signer_name.value=name;return r;});
        assert(rows[0].windows_signature.value && !rows[0].windows_signature.value->signer_name.value);
    }
    calls=0;rows.clear();for(unsigned k=0;k<17;++k)rows.push_back(entry("C:\\App"+std::to_string(k)+".exe"));
    fill_windows_startup_signatures(rows,reader);assert(calls==16 && rows[15].windows_signature.value && !rows[16].windows_signature.value);
    calls=0;rows={entry(),entry()};fill_windows_startup_signatures(rows,[&](const std::string&)->Metric<WindowsStartupSignature>{++calls;return {{},Support::permission_required,"denied"};});assert(calls==1 && !rows[1].windows_signature.value);
    rows.assign(1200,entry());calls=0;fill_windows_startup_signatures(rows,[&](const std::string&){++calls;auto r=result(0);r.value->signer_name.value=std::string(16384,'x');return r;});assert(calls==1 && rows[0].windows_signature.value && !rows.back().windows_signature.value);
    for(const auto* command:{"app.exe","C:\\Program Files\\app.exe","%APP%\\app.exe","C:\\run.cmd"}){
        calls=0;rows={entry(command)};fill_windows_startup_signatures(rows,reader);assert(calls==0 && !rows[0].windows_signature.value);
    }
    rows={entry()};fill_windows_startup_signatures(rows,StartupSignatureRead{});assert(!rows[0].windows_signature.value);
    bool threw=false;rows.assign(50001,entry());try{fill_windows_startup_signatures(rows,reader);}catch(const std::length_error&){threw=true;}assert(threw);
    assert(!read_windows_startup_signature("").value);
    assert(!SampleOptions{}.include_windows_startup_signature);
#ifdef _WIN32
    native_tests();
#endif
    std::cout<<"Startup signature classification, independent confidence, cache, limits and failures passed\n";
}
