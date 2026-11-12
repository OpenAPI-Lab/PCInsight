#include "linux_cpu_identity.hpp"
#include "cpu_identity_text.hpp"
#include <array>
#include <charconv>
#include <fstream>
#include <map>
namespace pcinsight::detail {
namespace {
Metric<std::string> missing(std::string note){return {{},Support::unsupported,std::move(note)};}
std::string_view trim_names(std::string_view s){const auto first=s.find_first_not_of(" \t\r"),last=s.find_last_not_of(" \t\r");return first==s.npos?std::string_view{}:s.substr(first,last-first+1);}
struct Field {std::optional<std::string> value;std::string key;bool seen{},invalid{};};
struct Row {Field vendor,model;};
void set_name(Field& f,std::string_view key,std::string_view value){
    if(f.seen)f.invalid=true;
    f.seen=true;f.key=key;f.value=cpu_identity_text(value);
    if(!value.empty()&&!f.value)f.invalid=true;
}
std::pair<Metric<std::string>,bool> summarize(const std::map<std::uint32_t,Row>& rows,bool vendor){
    std::optional<std::string> common,key;std::size_t present=0;
    for(const auto& [cpu,row]:rows){
        (void)cpu;const auto& field=vendor?row.vendor:row.model;
        if(field.invalid)return {missing("Duplicate/invalid CPU name field; no first-row or firmware substitution"),false};
        if(!field.value)continue;
        ++present;
        if((common&&common!=field.value)||(key&&*key!=field.key))return {missing("CPU name fields differ across visible processor records; no single identity inferred"),false};
        common=field.value;key=field.key;
    }
    if(!present)return {missing("No textual CPU name in selected proc fields"),true};
    if(present!=rows.size())return {missing("Incomplete CPU name coverage across visible processor records"),false};
    return {{*common,Support::available,"Common /proc/cpuinfo "+*key+" across visible processor records; reported text, not hardware attestation/commercial-name lookup"},false};
}
}
Metric<LinuxCpuNames> parse_linux_cpu_names(std::string_view text){
    auto fail=[]()->Metric<LinuxCpuNames>{return {{},Support::unsupported,"CPU identity inventory empty, malformed or over bounds"};};
    if(text.empty()||text.size()>16*1024*1024)return fail();
    std::map<std::uint32_t,Row> rows;Row* current=nullptr;unsigned lines=0;
    while(!text.empty()){
        const auto newline=text.find('\n');const auto raw=text.substr(0,newline);
        if(++lines>250000||raw.size()>65536||raw.find('\0')!=raw.npos)return fail();
        const auto line=trim_names(raw);
        if(line.empty())current=nullptr;
        else if(const auto colon=line.find(':');colon!=line.npos){
            const auto key=trim_names(line.substr(0,colon)),value=trim_names(line.substr(colon+1));
            if(key=="processor"){
                if(value.empty())return fail();
                std::uint32_t cpu{};const auto [end,ec]=std::from_chars(value.data(),value.data()+value.size(),cpu);
                if(ec!=std::errc{}||end!=value.data()+value.size()||rows.size()>=4096)return fail();
                const auto [it,inserted]=rows.emplace(cpu,Row{});if(!inserted)return fail();current=&it->second;
            }else if(current){
                if(key=="vendor_id")set_name(current->vendor,key,value);
                else if(key=="model name"||key=="cpu model"||key=="cpu")set_name(current->model,key,value);
                // Hardware/Model/system type/machine identify a board/platform,
                // and numeric implementer/part codes are not textual CPU names.
            }
        }
        if(newline==text.npos)break;
        text.remove_prefix(newline+1);
    }
    LinuxCpuNames out;
    const auto vendor=summarize(rows,true),model=summarize(rows,false);
    out.vendor=vendor.first;out.vendor_fallback=vendor.second;out.model=model.first;out.model_fallback=model.second;
    for(const auto& [id,row]:rows){(void)row;out.cpu_ids.push_back(id);}
    return {std::move(out),Support::available,"Bounded proc CPU text inventory, not an atomic online topology snapshot"};
}
Metric<LinuxCpuNames> read_linux_cpu_names(const std::filesystem::path& path){
#ifdef __linux__
    std::ifstream in(path,std::ios::binary);if(!in)return {{},Support::unsupported,"CPU info unavailable/unreadable; firmware may provide an independent reported identity"};
    std::string text;std::array<char,8192> buffer{};
    do{
        in.read(buffer.data(),static_cast<std::streamsize>(buffer.size()));const auto count=static_cast<std::size_t>(in.gcount());
        if(in.bad()||count>16*1024*1024-text.size())return {{},Support::unsupported,"CPU info read failure/size limit"};
        text.append(buffer.data(),count);
    }while(in);
    return parse_linux_cpu_names(text);
#else
    (void)path;return {{},Support::unsupported,"Native proc CPU names reader is Linux-only"};
#endif
}
void apply_linux_cpu_names(CpuInfo& cpu,const Metric<LinuxCpuNames>& input,const SmbiosInventory& firmware){
    auto choose=[&](bool vendor){
        const bool fallback=!input.value||(vendor?input.value->vendor_fallback:input.value->model_fallback);
        auto metric=input.value?(vendor?input.value->vendor:input.value->model):missing(input.note);
        if(metric.value||!fallback)return metric;
        const auto& raw=vendor?firmware.cpu_vendor:firmware.cpu_model;
        if(raw){
            const auto value=cpu_identity_text(*raw);
            if(value&&(firmware.support==Support::available||firmware.support==Support::estimated))return Metric<std::string>{*value,Support::estimated,"SMBIOS populated central-processor "+std::string(vendor?"Manufacturer":"Version")+" common across reported records; independent firmware scope, not online-CPU correlation or hardware attestation"};
        }
        if(firmware.support==Support::permission_required)return Metric<std::string>{{},Support::permission_required,"No proc CPU name; firmware identity requires access: "+firmware.note};
        return metric;
    };
    cpu.reported_vendor=choose(true);cpu.reported_model=choose(false);
    cpu.vendor=cpu.reported_vendor.value.value_or("");cpu.model=cpu.reported_model.value.value_or("");
}
void fill_linux_cpu_names(CpuInfo& cpu,const SmbiosInventory& firmware,const std::filesystem::path& path){apply_linux_cpu_names(cpu,read_linux_cpu_names(path),firmware);}
}
