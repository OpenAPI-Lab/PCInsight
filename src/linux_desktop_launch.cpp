#include "linux_desktop_launch.hpp"
#include <map>
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> unknown(std::string note){return {{},Support::unsupported,std::move(note)};}
std::optional<std::string> unescape(std::string_view raw){
    std::string out;
    for(std::size_t i=0;i<raw.size();++i){char c=raw[i];if(!c)return {};
        if(c=='\\'){if(++i==raw.size())return {};switch(raw[i]){
            case 's':c=' ';break;case 'n':c='\n';break;case 'r':c='\r';break;case 't':c='\t';break;case '\\':c='\\';break;default:return {};
        }}out+=c;
    }return out;
}
struct Token {std::string value;bool quoted{};};
std::optional<std::vector<Token>> tokens(std::string_view raw){
    const auto decoded=unescape(raw);if(!decoded)return {};
    const auto& s=*decoded;std::vector<Token> out;std::size_t i=0;
    while(i<s.size()){
        if(s[i]==' '){++i;continue;}Token token;token.quoted=s[i]=='"';if(token.quoted)++i;bool closed=!token.quoted;
        while(i<s.size()){
            const char c=s[i++];if(static_cast<unsigned char>(c)>=128)return {};
            if(token.quoted){
                if(c=='"'){closed=true;break;}
                if(c=='\\'){if(i==s.size()||std::string_view("\"`$\\").find(s[i])==std::string_view::npos)return {};token.value+=s[i++];}
                else {if(c=='$'||c=='`')return {};token.value+=c;}
            }else{
                if(c==' ')break;
                if(std::string_view("\t\n\r\"'\\><~|&;$*?#()`").find(c)!=std::string_view::npos)return {};
                token.value+=c;
            }
        }
        if(!closed||(token.quoted&&i<s.size()&&s[i]!=' ')||out.size()>=4096)return {};
        out.push_back(std::move(token));
    }return out;
}
bool absolute(const std::string& p){return !p.empty()&&p.front()=='/'&&p.find('\0')==p.npos;}
#ifdef __linux__
bool same_fields(const StartupProperties& a,const StartupProperties& b){
    if(a.entry.id!=b.entry.id||a.fields.size()!=b.fields.size())return false;
    for(std::size_t i=0;i<a.fields.size();++i){const auto& x=a.fields[i];const auto& y=b.fields[i];if(x.group!=y.group||x.key!=y.key||x.raw_value!=y.raw_value)return false;}
    return true;
}
bool eligible(const StartupProperties& p){return p.entry.status==StartupStatus::enabled&&p.entry.xdg_standard_eligible.support==Support::available&&p.entry.xdg_standard_eligible.value==true&&p.entry.autostart_condition_satisfied.support==Support::available&&p.entry.autostart_condition_satisfied.value==true;}
#endif
}
Metric<LinuxDesktopRoute> resolve_linux_desktop_route(const StartupProperties& properties){
    if(properties.fields.size()>65536||properties.entry.id.size()>32768||!absolute(properties.entry.id)||!properties.entry.id.ends_with(".desktop"))return unknown<LinuxDesktopRoute>("Invalid desktop snapshot ID/size");
    std::map<std::string,std::string> keys;std::size_t bytes=0;
    for(const auto& f:properties.fields){
        if(f.group.size()>4096||f.key.empty()||f.key.size()>4096||f.raw_value.size()>32768||f.group.find('\0')!=f.group.npos||f.key.find('\0')!=f.key.npos||f.raw_value.find('\0')!=f.raw_value.npos)return unknown<LinuxDesktopRoute>("Invalid desktop fields");
        bytes+=f.group.size()+f.key.size()+f.raw_value.size();if(bytes>1048576)return unknown<LinuxDesktopRoute>("Desktop snapshot exceeds payload bound");
        if(f.group=="Desktop Entry"&&!keys.emplace(f.key,f.raw_value).second)return unknown<LinuxDesktopRoute>("Duplicate main-group key is ambiguous");
    }
    if(keys["Type"]!="Application"||keys["Name"].empty()||keys["Exec"].empty())return unknown<LinuxDesktopRoute>("Need Application/Name/Exec");
    for(const auto key:{"Terminal","DBusActivatable","PrefersNonDefaultGPU"})if(keys.contains(key)&&keys[key]!="false")return unknown<LinuxDesktopRoute>("Unsupported terminal/D-Bus/GPU launch route");
    for(const auto& [key,value]:keys){(void)value;if(key.starts_with("X-")&&key!="X-GNOME-Autostart-enabled")return unknown<LinuxDesktopRoute>("Vendor extension not supported by controlled route");}
    if((keys.contains("Hidden")&&keys["Hidden"]!="false")||(keys.contains("X-GNOME-Autostart-enabled")&&keys["X-GNOME-Autostart-enabled"]!="true"))return unknown<LinuxDesktopRoute>("Disabled/invalid startup entry");
    const auto parsed=tokens(keys["Exec"]);if(!parsed||parsed->empty())return unknown<LinuxDesktopRoute>("Exec quoting/escaping outside supported grammar");
    std::vector<std::string> args;unsigned file_codes=0;
    for(std::size_t n=0;n<parsed->size();++n){const auto& t=(*parsed)[n];std::string value;
        if(t.quoted&&t.value.find('%')!=t.value.npos)return unknown<LinuxDesktopRoute>("Field codes in quoted tokens unsupported");
        if(n&&t.value.size()==2&&t.value[0]=='%'&&std::string_view("fFuUdDnNvm").find(t.value[1])!=std::string_view::npos){
            if(std::string_view("fFuU").find(t.value[1])!=std::string_view::npos&&++file_codes>1)return unknown<LinuxDesktopRoute>("Repeated file/URL field code");
            continue; // No file/URL inputs in autostart; deprecated standalone codes omitted.
        }
        for(std::size_t i=0;i<t.value.size();++i){if(t.value[i]=='%'){if(i+1==t.value.size()||t.value[++i]!='%')return unknown<LinuxDesktopRoute>("Unsupported/embedded field code");}value+=t.value[i];}
        args.push_back(std::move(value));
    }
    if(args.empty()||!absolute(args.front())||args.front().find('=')!=args.front().npos)return unknown<LinuxDesktopRoute>("Executable must be absolute, no PATH/assignment");
    LaunchRequest request;request.executable=std::move(args.front());args.erase(args.begin());request.arguments=std::move(args);
    if(keys.contains("Path")){const auto path=unescape(keys["Path"]);if(!path||(!path->empty()&&!absolute(*path)))return unknown<LinuxDesktopRoute>("Path must be absolute or empty");request.working_directory=*path;}
    return {LinuxDesktopRoute{properties.entry.id,std::move(request)},Support::available,"Resolved restricted direct Exec snapshot, NOT current file/app/automatic cause"};
}
ActionResult launch_linux_desktop_entry(const std::string& id,bool authorized,const LinuxDesktopArm& arm){
    if(!authorized)return {ActionError::permission_denied,"Explicit desktop launch authorization required"};
#ifdef __linux__
    const auto source=get_startup_properties(id);if(!source)return {source.error,source.message};
    if(!eligible(*source.value))return {ActionError::not_supported,"Startup disabled/ineligible/condition unknown"};
    const auto route=resolve_linux_desktop_route(*source.value);if(!route.value)return {ActionError::not_supported,route.note};
    auto revalidate=[&](){const auto current=get_startup_properties(id);return current&&eligible(*current.value)&&same_fields(*source.value,*current.value);};
    return launch_linux_gated(route.value->request,true,[&](const LinuxGatedLaunchTarget& target){
        if(!revalidate())return ActionResult{ActionError::stale_target,"Effective desktop snapshot/eligibility changed before arm"};
        if(arm){const auto result=arm(*route.value,target);if(!result)return result;}
        if(!revalidate())return ActionResult{ActionError::stale_target,"Effective desktop snapshot/eligibility changed during arm"};
        return ActionResult{};
    });
#else
    (void)id;(void)arm;return {ActionError::not_supported,"Controlled XDG launch requires Linux"};
#endif
}
}
