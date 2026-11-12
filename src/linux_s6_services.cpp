#include "linux_s6_services.hpp"
#include <set>
namespace pcinsight {
namespace {
template<class T>Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support==Support::permission_required?support:Support::unsupported,std::move(note)};}
bool absolute(const std::string& s){return !s.empty()&&s.front()=='/'&&s.size()<=32768&&s.find('\0')==s.npos;}
bool filename(const std::string& s){return !s.empty()&&s.front()!='.'&&s.size()<=255&&s.find('/')==s.npos&&s.find('\0')==s.npos;}
std::string id(const std::string& name){constexpr char hex[]="0123456789abcdef";std::string out="s6-record:";for(const unsigned char c:name){out+=hex[c>>4];out+=hex[c&15];}return out;}
bool timestamp(const std::array<std::uint8_t,12>& t){std::uint32_t ns=0;for(unsigned i=8;i<12;++i)ns=(ns<<8)|t[i];return ns<1000000000;}
Metric<std::string> report(const Metric<LinuxS6StatusRecord>& m){
    if(!m.value||m.support!=Support::available)return absent<std::string>("No exact s6 record: "+m.note,m.support);
    const auto& s=*m.value;if(!timestamp(s.changed_at_tai64n)||!timestamp(s.ready_changed_at_tai64n))return absent<std::string>("Invalid s6 TAI nanoseconds");
    std::string label=s.reported_pid?"record-pid-present":"record-pid-zero";
    if(s.finishing)label+="/finishing";
    if(s.paused)label+="/paused";
    return {std::move(label),Support::estimated,"Historical s6 record fields ONLY, not current supervisor/process life, readiness, namespace or authenticated peer proof"};
}
}
Metric<LinuxS6ServiceInventory> detail::project_s6_services(Metric<LinuxS6Inventory> input){
    if(!metric_is_known(input))return absent<LinuxS6ServiceInventory>(input.note,input.support);
    const auto& source=*input.value;if(!absolute(source.root)||source.entries.size()>4096)return absent<LinuxS6ServiceInventory>("Invalid s6 source root/bound");
    std::set<std::string> names,parents;std::size_t payload=0;
    for(const auto& e:source.entries){
        const bool valid=e.logger?(filename(e.parent_name)&&e.name==e.parent_name+"/log"):(filename(e.name)&&e.parent_name.empty());
        const auto expected=source.root+(source.root.back()=='/'?"":"/")+e.name;
        if(!valid||e.directory!=expected||!names.insert(e.name).second)return absent<LinuxS6ServiceInventory>("Invalid/duplicate s6 entry/logger shape or source directory");
        if(!e.logger)parents.insert(e.name);
        const auto cost=e.name.size()+e.directory.size()+e.parent_name.size();if(cost>8*1024*1024-payload)return absent<LinuxS6ServiceInventory>("s6 source payload exceeds8MiB");payload+=cost;
    }
    for(const auto& e:source.entries)if(e.logger&&!parents.contains(e.parent_name))return absent<LinuxS6ServiceInventory>("Orphan s6 logger, no direct parent in this inventory");
    const bool coherent=source.complete&&input.support==Support::available;LinuxS6ServiceInventory out;out.services.reserve(source.entries.size());
    for(const auto& e:source.entries){
        ServiceInfo row;row.id=id(e.name);row.name=e.name;row.status="unknown";LinuxServiceDetails d;d.provider="s6-status-record-inventory";
        d.name={e.name,coherent?Support::available:Support::estimated,"Observed scan-relative name (including direct logger), not authenticated daemon identity"};
        d.status=coherent?report(e.status_record):absent<std::string>("Partial/unstable s6 inventory; record projection unavailable");
        if(metric_is_known(d.status))row.status=*d.status.value;
        d.main_pid=absent<std::uint32_t>("Raw u64 PID retained only in source; no current host process/namespace/birth identity verified");
        d.description=absent<std::string>("No standard description observed; run/finish scripts not read or evaluated");
        d.load_state=d.active_state=d.sub_state=d.unit_file_state=d.slice=absent<std::string>("No manager property/group/effective enablement observed; logger association is not a group");
        row.linux_details={std::move(d),Support::estimated,"Historical s6 file-record projection, not active-manager enumeration or control authority"};out.services.push_back(std::move(row));
    }
    out.source=std::move(*input.value);
    return {std::move(out),coherent?Support::available:Support::estimated,"Explicit s6 tree projected to ServiceInfo; record/FIFO presence NOT current life or control capability"};
}
Metric<LinuxS6ServiceInventory> query_linux_s6_service_inventory(const std::string& root,std::uint32_t maximum){return detail::project_s6_services(query_linux_s6_inventory(root,maximum));}
}
