#include "linux_sysv_services.hpp"
#include <set>
namespace pcinsight {
namespace {
template<class T>Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support==Support::permission_required?support:Support::unsupported,std::move(note)};}
bool absolute(const std::string& s){return !s.empty()&&s.front()=='/'&&s.size()<=32768&&s.find('\0')==s.npos;}
bool filename(const std::string& s){return !s.empty()&&s.front()!='.'&&s.size()<=255&&s.find('/')==s.npos&&s.find('\0')==s.npos;}
std::string id(const std::string& name){constexpr char hex[]="0123456789abcdef";std::string out="sysv-file:";for(const unsigned char c:name){out+=hex[c>>4];out+=hex[c&15];}return out;}
Metric<std::string> description(const Metric<LinuxSysVLsbMetadata>& m){
    if(!metric_is_known(m))return absent<std::string>("LSB metadata unavailable: "+m.note,m.support);
    const auto& fields=*m.value;
    if(metric_is_known(fields.short_description)){auto out=fields.short_description;out.note="Declared LSB Short-Description, NOT live/effective manager metadata; "+out.note;return out;}
    if(metric_is_known(fields.description)){auto out=fields.description;out.note="Declared LSB Description fallback, NOT live/effective manager metadata; "+out.note;return out;}
    return absent<std::string>("No declared descriptive field: "+fields.short_description.note+"; "+fields.description.note,fields.short_description.support==Support::permission_required||fields.description.support==Support::permission_required?Support::permission_required:Support::unsupported);
}
}
Metric<LinuxSysVServiceInventory> detail::project_sysv_services(Metric<LinuxSysVDescribedCatalog> input){
    if(!metric_is_known(input))return absent<LinuxSysVServiceInventory>(input.note,input.support);
    const auto& source=*input.value;const auto& catalog=source.catalog;
    if(!absolute(catalog.init_directory)||!absolute(catalog.runlevel_root)||catalog.scripts.size()>4096||source.metadata.size()!=catalog.scripts.size())return absent<LinuxSysVServiceInventory>("Invalid SysV source scope/bounds/metadata alignment; no name join attempted");
    std::set<std::string> names;
    for(const auto& script:catalog.scripts)if(!filename(script.name)||!names.insert(script.name).second)return absent<LinuxSysVServiceInventory>("Invalid/duplicate SysV candidate filename");
    LinuxSysVServiceInventory out;const bool coherent=catalog.complete&&input.support==Support::available;
    for(std::size_t i=0;i<catalog.scripts.size();++i){
        const auto& script=catalog.scripts[i];ServiceInfo row;row.id=id(script.name);row.name=script.name;row.status="unknown";
        LinuxServiceDetails details;details.provider="sysv-file-catalog";
        details.name={script.name,coherent?Support::available:Support::estimated,"Observed candidate filename bytes, NOT manager-verified daemon/service name"};
        details.description=coherent?description(source.metadata[i]):absent<std::string>("Partial/unstable SysV catalog; bound description unavailable");
        details.status=absent<std::string>("SysV file/link inventory provides no execution state");
        details.main_pid=absent<std::uint32_t>("No native verified daemon PID; no pidfile/proc-name guessing");
        details.load_state=details.active_state=details.sub_state=details.unit_file_state=details.slice=absent<std::string>("No corresponding manager property observed; links/mode bits are not effective policy/group/state");
        if(metric_is_known(details.description))row.description=*details.description.value;
        row.linux_details={std::move(details),Support::estimated,"Filesystem candidate projection only, not running-manager enumeration/control authority"};
        out.services.push_back(std::move(row));
    }
    out.source=std::move(*input.value);
    return {std::move(out),coherent?Support::available:Support::estimated,"Explicit SysV filesystem scope projected to ServiceInfo; per-field availability, not live-service coverage or control"};
}
Metric<LinuxSysVServiceInventory> query_linux_sysv_service_inventory(const std::string& init,const std::string& rc,std::uint32_t maximum,std::uint64_t budget){return detail::project_sysv_services(query_linux_sysv_described_catalog(init,rc,maximum,budget));}
}
