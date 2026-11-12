#include "windows_shortcut.hpp"
#include <algorithm>
#include <stdexcept>
#include <string_view>
namespace pcinsight::detail {
namespace {
constexpr std::size_t max_file=1024*1024;
using Bytes=std::span<const std::uint8_t>;
Bytes cut(Bytes b,std::size_t offset,std::size_t length){if(offset>b.size() || length>b.size()-offset)throw std::invalid_argument("truncated/overflowing Shell Link range");return b.subspan(offset,length);}
std::uint32_t u32(Bytes b,std::size_t p){const auto s=cut(b,p,4);return std::uint32_t(s[0])|(std::uint32_t(s[1])<<8)|(std::uint32_t(s[2])<<16)|(std::uint32_t(s[3])<<24);}
std::uint16_t u16(Bytes b,std::size_t p){const auto s=cut(b,p,2);return static_cast<std::uint16_t>(s[0]|(unsigned(s[1])<<8));}
void point(std::string& out,std::uint32_t c){
    if(c<0x80)out.push_back(static_cast<char>(c));
    else if(c<0x800){out.push_back(static_cast<char>(0xc0|(c>>6)));out.push_back(static_cast<char>(0x80|(c&63)));}
    else if(c<0x10000){out.push_back(static_cast<char>(0xe0|(c>>12)));out.push_back(static_cast<char>(0x80|((c>>6)&63)));out.push_back(static_cast<char>(0x80|(c&63)));}
    else {out.push_back(static_cast<char>(0xf0|(c>>18)));out.push_back(static_cast<char>(0x80|((c>>12)&63)));out.push_back(static_cast<char>(0x80|((c>>6)&63)));out.push_back(static_cast<char>(0x80|(c&63)));}
}
std::string decode(Bytes b,bool unicode){
    std::string out;if(unicode && b.size()%2)throw std::invalid_argument("odd UTF-16 length");
    for(std::size_t p=0;p<b.size();p+=unicode?2:1){
        std::uint32_t c=unicode?u16(b,p):b[p];
        if(!c)throw std::invalid_argument("embedded NUL in Shell Link string");
        if(!unicode && c>=128)throw std::invalid_argument("legacy code page not guessed; only ASCII or UTF-16 supported");
        if(unicode && c>=0xd800 && c<=0xdbff){const auto low=u16(b,p+2);if(low<0xdc00 || low>0xdfff)throw std::invalid_argument("invalid surrogate pair");c=0x10000+((c-0xd800)<<10)+(low-0xdc00);p+=2;}
        else if(c>=0xdc00 && c<=0xdfff)throw std::invalid_argument("unpaired low surrogate");
        point(out,c);
    }return out;
}
std::string terminated(Bytes b,std::size_t offset,bool unicode,std::size_t header){
    if(offset<header || offset>=b.size())throw std::invalid_argument("Shell Link string offset outside payload");
    std::size_t end=offset;
    while(unicode?u16(b,end)!=0:b[end]!=0){end+=unicode?2:1;if(end>=b.size() || end-offset>65534)throw std::invalid_argument("unterminated/oversized Shell Link string");}
    return decode(cut(b,offset,end-offset),unicode);
}
Metric<std::string> stored(std::string s){return {std::move(s),Support::available,"Stored Shell Link metadata only; not resolved, executable, validated or authorized"};}
bool lnk(std::string_view name){if(name.size()<4)return false;name.remove_prefix(name.size()-4);return name[0]=='.' && (name[1]=='l'||name[1]=='L') && (name[2]=='n'||name[2]=='N') && (name[3]=='k'||name[3]=='K');}
bool ascii_equal(std::string_view a,std::string_view b){
    if(a.size()!=b.size())return false;
    for(std::size_t k=0;k<a.size();++k){auto c=static_cast<unsigned char>(a[k]);if(c>='A' && c<='Z')c+='a'-'A';if(c!=static_cast<unsigned char>(b[k]))return false;}return true;
}
bool url_file(std::string_view name){return name.size()>=4 && ascii_equal(name.substr(name.size()-4),".url");}
std::string_view trim_ini(std::string_view s){const auto first=s.find_first_not_of(" \t\r");return first==s.npos?std::string_view{}:s.substr(first,s.find_last_not_of(" \t\r")-first+1);}
std::string url_text(Bytes b){
    if(b.size()>=2 && b[0]==0xff && b[1]==0xfe)return decode(b.subspan(2),true);
    if(b.size()>=2 && b[0]==0xfe && b[1]==0xff)throw std::invalid_argument("UTF-16BE Internet Shortcut not supported");
    if(b.size()<3 || b[0]!=0xef || b[1]!=0xbb || b[2]!=0xbf)return decode(b,false); // No encoding guess for legacy bytes.
    b=b.subspan(3); // Strict UTF-8 with explicit BOM.
    for(std::size_t p=0;p<b.size();){
        const auto lead=b[p++];if(!lead)throw std::invalid_argument("NUL in Internet Shortcut text");if(lead<128)continue;
        const unsigned n=lead>=0xc2 && lead<=0xdf?1:lead>=0xe0 && lead<=0xef?2:lead>=0xf0 && lead<=0xf4?3:0;
        if(!n || n>b.size()-p)throw std::invalid_argument("invalid/truncated UTF-8 Internet Shortcut");
        std::uint32_t c=lead&((1U<<(6-n))-1);
        for(unsigned k=0;k<n;++k){const auto next=b[p++];if((next&0xc0)!=0x80)throw std::invalid_argument("invalid UTF-8 continuation");c=(c<<6)|(next&63);}
        if(c<(n==1?0x80U:n==2?0x800U:0x10000U) || c>0x10ffff || (c>=0xd800 && c<=0xdfff))throw std::invalid_argument("non-scalar/overlong UTF-8 Internet Shortcut");
    }
    return {reinterpret_cast<const char*>(b.data()),b.size()};
}
}
Metric<WindowsShortcutInfo> parse_windows_internet_shortcut(Bytes bytes){
    try{
        if(bytes.empty() || bytes.size()>max_file)throw std::invalid_argument("Internet Shortcut size unsupported");
        const auto text=url_text(bytes);std::string_view remaining(text);bool section=false,seen_section=false,seen_url=false;std::string url;unsigned lines=0;
        while(!remaining.empty()){
            if(++lines>4096)throw std::invalid_argument("Internet Shortcut exceeds 4096 lines");
            const auto end=remaining.find('\n');auto line=remaining.substr(0,end);
            if(line.size()>65536)throw std::invalid_argument("Internet Shortcut line exceeds 64 KiB");
            line=trim_ini(line);
            if(!line.empty() && line[0]!=';' && line[0]!='#'){
                if(line[0]=='['){
                    if(line.size()<3 || line.back()!=']')throw std::invalid_argument("invalid Internet Shortcut INI section");
                    section=ascii_equal(trim_ini(line.substr(1,line.size()-2)),"internetshortcut");
                    if(section && seen_section)throw std::invalid_argument("duplicate InternetShortcut section");
                    seen_section|=section;
                }else if(section){
                    const auto equal=line.find('=');if(equal==line.npos || trim_ini(line.substr(0,equal)).empty())throw std::invalid_argument("invalid Internet Shortcut INI key");
                    if(ascii_equal(trim_ini(line.substr(0,equal)),"url")){
                        if(seen_url)throw std::invalid_argument("duplicate Internet Shortcut URL key");
                        seen_url=true;
                        url=trim_ini(line.substr(equal+1));
                    }
                }
            }
            if(end==remaining.npos)break;
            remaining.remove_prefix(end+1);
        }
        const auto colon=url.find(':');
        auto letter=[](unsigned char c){return (c>='A' && c<='Z') || (c>='a' && c<='z');};
        if(!seen_url || url.empty() || url.size()>32768 || colon==url.npos || !colon || colon+1==url.size() || !letter(url[0]))throw std::invalid_argument("missing/empty/unsupported Internet Shortcut URI");
        for(std::size_t k=1;k<colon;++k){const auto c=static_cast<unsigned char>(url[k]);if(!letter(c) && !(c>='0' && c<='9') && c!='+' && c!='-' && c!='.')throw std::invalid_argument("invalid URI scheme");}
        if(std::any_of(url.begin(),url.end(),[](unsigned char c){return c<=32 || c==127;}))throw std::invalid_argument("URI contains unescaped whitespace/control");
        WindowsShortcutInfo out;out.stored_url={std::move(url),Support::available,"Stored URL text, NOT validated safe/reachable or authorized to open; arbitrary schemes possible"};
        out.stored_local_target.note="Internet Shortcut is not a local executable path";
        out.arguments.note=out.working_directory.note="Not collected from Internet Shortcut";
        return {std::move(out),Support::available,"Parsed conservative INI InternetShortcut/URL subset; no shell, icons, network or handlers"};
    }catch(const std::invalid_argument& e){return {{},Support::unsupported,e.what()};}
}
Metric<WindowsShortcutInfo> parse_windows_shortcut(Bytes bytes){
    try{
        if(bytes.size()>max_file || bytes.size()<80 || u32(bytes,0)!=76)throw std::invalid_argument("Shell Link size/header unsupported");
        constexpr std::uint8_t clsid[]{1,0x14,2,0,0,0,0,0,0xc0,0,0,0,0,0,0,0x46};
        if(!std::equal(std::begin(clsid),std::end(clsid),bytes.begin()+4))throw std::invalid_argument("not a Shell Link CLSID");
        const auto flags=u32(bytes,20);std::size_t p=76;WindowsShortcutInfo out;
        out.stored_local_target.note="No supported stored local LinkInfo target; IDList/network/environment not resolved";
        out.arguments=stored("");out.working_directory.note="No working directory StringData stored";
        if(flags&1){const auto size=u16(bytes,p);p+=2;cut(bytes,p,size);if(size<2 || u16(bytes,p+size-2)!=0)throw std::invalid_argument("invalid IDList extent/terminal");p+=size;}
        if(flags&2){
            const auto size=u32(bytes,p);const auto info=cut(bytes,p,size);const auto header=u32(info,4),kind=u32(info,8);
            if((header!=28 && header<36) || header>size)throw std::invalid_argument("invalid LinkInfo header");
            for(std::size_t k:{12U,16U,20U,24U}){const auto off=u32(info,k);if(off && (off<header || off>=size))throw std::invalid_argument("invalid LinkInfo offset");}
            if(header>=36)for(std::size_t k:{28U,32U}){const auto off=u32(info,k);if(off && (off<header || off>=size))throw std::invalid_argument("invalid Unicode LinkInfo offset");}
            if(kind==1 && !(flags&0x100)){
                const auto volume=u32(info,12);if(!volume || u32(info,volume)<16)throw std::invalid_argument("invalid VolumeID extent");cut(info,volume,u32(info,volume));
                const bool unicode=header>=36 && u32(info,28) && u32(info,32);
                if(header>=36 && !unicode)throw std::invalid_argument("missing Unicode local LinkInfo fields");
                const auto base=terminated(info,u32(info,unicode?28:16),unicode,header);
                const auto suffix=terminated(info,u32(info,unicode?32:24),unicode,header);
                const auto target=base+suffix;
                if(target.size()>=3 && ((target[0]>='A'&&target[0]<='Z') || (target[0]>='a'&&target[0]<='z')) && target[1]==':' && target[2]=='\\')out.stored_local_target=stored(target);
            }
            p+=size;
        }
        for(unsigned k=0;k<5;++k)if(flags&(4U<<k)){
            const auto count=u16(bytes,p);p+=2;
            if(k!=3 && count>260)throw std::invalid_argument("non-argument StringData exceeds 260 units");
            const auto n=std::size_t(count)*((flags&128)?2:1);const auto value=decode(cut(bytes,p,n),(flags&128)!=0);p+=n;
            if(k==2)out.working_directory=stored(value);
            if(k==3)out.arguments=stored(value);
        }
        unsigned blocks=0;
        for(;;){const auto size=u32(bytes,p);if(!size){p+=4;break;}if(size<8 || ++blocks>1024)throw std::invalid_argument("invalid/excessive ExtraData blocks");cut(bytes,p,size);p+=size;}
        if(p!=bytes.size())throw std::invalid_argument("trailing Shell Link data");
        return {std::move(out),Support::available,"Parsed stored metadata subset; IDList/ExtraData payloads not decoded, no shell resolution or execution"};
    }catch(const std::invalid_argument& e){return {{},Support::unsupported,e.what()};}
}
void fill_windows_startup_shortcuts(std::vector<StartupEntry>& rows,const ShortcutRead& read){
    std::size_t files=0,total=0,output=0;
    for(auto& row:rows){
        row.windows_shortcut={};
        if(!row.id.starts_with("winfolder:") || (!lnk(row.name) && !url_file(row.name)))continue;
        if(!read || files>=128 || total>=16*1024*1024){row.windows_shortcut.note="Shortcut metadata read API/budget unavailable (128 files, 16 MiB per sample)";continue;}
        ++files;const auto bytes=read(row.source);
        if(!bytes.value || bytes.support!=Support::available){row.windows_shortcut={{},bytes.support,bytes.note};continue;}
        if(bytes.value->size()>max_file || bytes.value->size()>16*1024*1024-total){row.windows_shortcut.note="Shortcut metadata byte budget exceeded";continue;}
        total+=bytes.value->size();row.windows_shortcut=url_file(row.name)?parse_windows_internet_shortcut(*bytes.value):parse_windows_shortcut(*bytes.value);
        if(row.windows_shortcut.value){const auto& v=*row.windows_shortcut.value;
            const auto cost=v.stored_local_target.value.value_or("").size()+v.arguments.value.value_or("").size()+v.working_directory.value.value_or("").size()+v.stored_url.value.value_or("").size();
            if(cost>16*1024*1024-output)row.windows_shortcut={{},Support::unsupported,"Shortcut metadata text output exceeds 16 MiB budget"};
            else output+=cost;
        }
    }
}
void fill_windows_startup_shortcuts(std::vector<StartupEntry>& rows){fill_windows_startup_shortcuts(rows,read_windows_shortcut_file);}
}
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
namespace pcinsight::detail {
Metric<std::vector<std::uint8_t>> read_windows_shortcut_file(const std::string& path){
    auto fail=[](DWORD e,const char* stage)->Metric<std::vector<std::uint8_t>>{return {{},e==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported,std::string(stage)+"; Win32 error="+std::to_string(e)};};
    if(path.empty() || path.size()>131072 || path.find('\0')!=path.npos)return fail(ERROR_INVALID_PARAMETER,"Invalid shortcut path");
    const auto length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path.data(),static_cast<int>(path.size()),nullptr,0);if(!length)return fail(GetLastError(),"Invalid UTF-8 shortcut path");
    std::wstring native(length,L'\0');if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path.data(),static_cast<int>(path.size()),native.data(),length))return fail(GetLastError(),"Path conversion failed");
    const auto handle=CreateFileW(native.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if(handle==INVALID_HANDLE_VALUE)return fail(GetLastError(),"Shortcut open failed");
    struct Close{HANDLE h;~Close(){CloseHandle(h);}} close{handle};BY_HANDLE_FILE_INFORMATION before{},after{};
    if(GetFileType(handle)!=FILE_TYPE_DISK)return fail(ERROR_INVALID_DATA,"Shortcut is not a disk file");
    if(!GetFileInformationByHandle(handle,&before))return fail(GetLastError(),"Shortcut file metadata unavailable");
    if(before.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY) || before.nFileSizeHigh || before.nFileSizeLow>max_file)return fail(ERROR_INVALID_DATA,"Shortcut file kind/size unsupported");
    std::vector<std::uint8_t> bytes(before.nFileSizeLow);DWORD got=0;
    if(!ReadFile(handle,bytes.data(),static_cast<DWORD>(bytes.size()),&got,nullptr))return fail(GetLastError(),"Shortcut read failed");
    if(got!=bytes.size() || !GetFileInformationByHandle(handle,&after) || before.nFileSizeLow!=after.nFileSizeLow || before.nFileSizeHigh!=after.nFileSizeHigh || CompareFileTime(&before.ftLastWriteTime,&after.ftLastWriteTime)!=0)return fail(ERROR_INVALID_DATA,"Shortcut changed during read");
    return {std::move(bytes),Support::available,"Read shortcut file only; no target access, no shell, non-atomic metadata checks"};
}
}
#else
namespace pcinsight::detail {Metric<std::vector<std::uint8_t>> read_windows_shortcut_file(const std::string&){return {{},Support::unsupported,"Native Windows shortcut file reader requires Windows"};}}
#endif
