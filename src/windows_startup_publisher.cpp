#include "windows_startup_publisher.hpp"
#include <map>
#include <stdexcept>
#include <string_view>
namespace pcinsight::detail {
namespace {
bool extension(std::string_view path,std::string_view ext){
    if(path.size()<ext.size())return false;
    path.remove_prefix(path.size()-ext.size());
    for(std::size_t k=0;k<path.size();++k){auto c=static_cast<unsigned char>(path[k]);if(c>='A' && c<='Z')c+='a'-'A';if(c!=static_cast<unsigned char>(ext[k]))return false;}return true;
}
bool literal_exe(std::string_view path){
    if(path.size()<7 || path.size()>32768 || !extension(path,".exe"))return false;
    if(!((path[0]>='A' && path[0]<='Z') || (path[0]>='a' && path[0]<='z')) || path[1]!=':' || path[2]!='\\')return false;
    for(std::size_t k=2;k<path.size();++k){const auto c=static_cast<unsigned char>(path[k]);if(c<32 || c==127 || c=='%' || c=='"' || c=='*' || c=='?' || c=='<' || c=='>' || c=='|' || c==':' || c=='/')return false;}
    return true;
}
std::string candidate(const StartupEntry& row){
    std::string_view path;
    if(row.id.starts_with("winrun:")){
        if(row.command.size()>131072)return {};
        std::string_view command(row.command);const auto first=command.find_first_not_of(" \t");if(first==command.npos)return {};command.remove_prefix(first);
        if(command[0]=='"'){
            const auto end=command.find('"',1);if(end==command.npos || (end+1<command.size() && command[end+1]!=' ' && command[end+1]!='\t'))return {};
            path=command.substr(1,end-1);
        }else path=command.substr(0,command.find_first_of(" \t"));
    }else if(row.id.starts_with("winfolder:")){
        if(extension(row.name,".exe"))path=row.source;
        else if(extension(row.name,".lnk") && row.windows_shortcut.value && row.windows_shortcut.support==Support::available && row.windows_shortcut.value->stored_local_target.value && row.windows_shortcut.value->stored_local_target.support==Support::available)path=*row.windows_shortcut.value->stored_local_target.value;
    }
    return literal_exe(path)?std::string(path):std::string{};
}
}
std::string windows_startup_executable_candidate(const StartupEntry& row){return candidate(row);}
void fill_windows_startup_publishers(std::vector<StartupEntry>& rows,const StartupCompanyRead& read){
    if(rows.size()>50000)throw std::length_error("Startup publisher inventory exceeds 50000 rows");
    std::map<std::string,Metric<std::string>> cache;std::size_t output=0;
    for(auto& row:rows){
        if(!row.id.starts_with("winrun:") && !row.id.starts_with("winfolder:"))continue;
        row.publisher.clear();row.publisher_label={};row.publisher_source_executable={};
        const auto path=candidate(row);
        if(path.empty()){row.publisher_label.note="No unambiguous literal drive-absolute .exe resource candidate; no PATH, expansion, URL or script fallback";continue;}
        auto found=cache.find(path);
        if(found==cache.end()){
            if(!read || cache.size()>=128){row.publisher_label.note="Publisher read API/budget unavailable (128 distinct path strings per sample)";continue;}
            auto label=read(path);
            if(label.value && (label.value->empty() || label.value->size()>16384 || label.value->find('\0')!=std::string::npos))label={{},Support::unsupported,"CompanyName missing/invalid/exceeds 16 KiB"};
            if(label.support!=Support::available && label.support!=Support::estimated)label.value.reset();
            found=cache.emplace(path,std::move(label)).first;
        }
        const auto& label=found->second;
        if(!label.value){row.publisher_label={{},label.support,label.note};continue;}
        const auto cost=2*label.value->size()+path.size();
        if(cost>16*1024*1024-output){row.publisher_label.note="Publisher metadata output budget exceeds 16 MiB";continue;}
        output+=cost;row.publisher=*label.value;
        row.publisher_label={label.value,Support::estimated,"Self-reported VERSIONINFO CompanyName; NOT verified signer, trust, effective launch identity or publisher of payloads loaded by a launcher"};
        row.publisher_source_executable={path,Support::estimated,"Selected resource candidate only; literal Run prefix, direct .exe or stored .lnk local target, not resolved or launch-verified"};
    }
}
void fill_windows_startup_publishers(std::vector<StartupEntry>& rows){fill_windows_startup_publishers(rows,read_windows_startup_company);}
}
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "windows_version.hpp"
namespace pcinsight::detail {
Metric<std::string> read_windows_startup_company(const std::string& path){
    if(!literal_exe(path))return {{},Support::unsupported,"No supported literal executable resource path"};
    const auto size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path.data(),static_cast<int>(path.size()),nullptr,0);
    if(!size)return {{},Support::unsupported,"Invalid UTF-8 resource path"};
    std::wstring native(size,L'\0');if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path.data(),static_cast<int>(path.size()),native.data(),size))return {{},Support::unsupported,"Resource path conversion failed"};
    const auto label=windows_file_company_name(native);
    if(label.empty())return {{},Support::unsupported,"VERSIONINFO CompanyName unavailable: absent, unsupported or unreadable resource; cause not classified"};
    return {label,Support::estimated,"Self-reported CompanyName; no signature verification"};
}
}
#else
namespace pcinsight::detail {Metric<std::string> read_windows_startup_company(const std::string&){return {{},Support::unsupported,"Native VERSIONINFO reader requires Windows"};}}
#endif
