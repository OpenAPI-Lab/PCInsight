#include "linux_runit_services.hpp"
#include <set>
namespace pcinsight {
namespace {
template<class T>Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support==Support::permission_required?support:Support::unsupported,std::move(note)};}
bool absolute(const std::string& s){return !s.empty()&&s.front()=='/'&&s.size()<=32768&&s.find('\0')==s.npos;}
bool filename(const std::string& s){return !s.empty()&&s.front()!='.'&&s.size()<=255&&s.find('/')==s.npos&&s.find('\0')==s.npos;}
std::string id(const std::string& s){constexpr char hex[]="0123456789abcdef";std::string out="runit-report:";for(const unsigned char c:s){out+=hex[c>>4];out+=hex[c&15];}return out;}
Metric<std::string> report(const Metric<LinuxRunitStatus>& m){
    if(!m.value||m.support!=Support::available)return absent<std::string>("No exact runit record: "+m.note,m.support);
    const auto& s=*m.value;std::string label;
    std::uint32_t ns=0;for(unsigned i=8;i<12;++i)ns=(ns<<8)|s.changed_at_tai64n[i];
    if(ns>=1000000000||s.reported_pid>2147483647U)return absent<std::string>("Invalid runit TAI/PID range");
    switch(s.state){
    case LinuxRunitState::down:if(s.reported_pid)return absent<std::string>("Invalid down/PID record");label="reported-down";break;
    case LinuxRunitState::run:if(!s.reported_pid)return absent<std::string>("Invalid run/PID record");label="reported-run";break;
    case LinuxRunitState::finish:label="reported-finish";break;
    default:return absent<std::string>("Invalid runit state");
    }
    if(s.paused)label+="/paused";
    return {std::move(label),Support::estimated,"Display of runit record/paused flag ONLY, not kernel liveness/readiness, PID namespace or authenticated peer proof"};
}
}
Metric<LinuxRunitServiceInventory> detail::project_runit_services(Metric<LinuxRunitInventory> input){
    if(!metric_is_known(input))return absent<LinuxRunitServiceInventory>(input.note,input.support);
    const auto& source=*input.value;if(!absolute(source.root)||source.services.size()>4096)return absent<LinuxRunitServiceInventory>("Invalid runit source root/bound");
    std::set<std::string> names;std::size_t payload=0;
    for(const auto& s:source.services){const auto expected=source.root+(source.root.back()=='/'?"":"/")+s.name;
        if(!filename(s.name)||s.directory!=expected||!names.insert(s.name).second)return absent<LinuxRunitServiceInventory>("Invalid/duplicate runit candidate or mismatched source directory");
        const auto cost=s.name.size()+s.directory.size();if(cost>8*1024*1024-payload)return absent<LinuxRunitServiceInventory>("Runit source payload exceeds8MiB");payload+=cost;
    }
    const bool coherent=source.complete&&input.support==Support::available;LinuxRunitServiceInventory out;out.services.reserve(source.services.size());
    for(const auto& s:source.services){
        ServiceInfo row;row.id=id(s.name);row.name=s.name;row.status="unknown";LinuxServiceDetails d;d.provider="runit-reported-inventory";
        d.name={s.name,coherent?Support::available:Support::estimated,"Observed tree entry name, not authenticated supervisor/daemon identity"};
        d.status=coherent?report(s.status):absent<std::string>("Partial/unstable runit inventory; status projection unavailable");
        if(metric_is_known(d.status))row.status=*d.status.value;
        d.description=absent<std::string>("No standard runit description metadata; "+s.description.note,s.description.support);
        d.main_pid=absent<std::uint32_t>("Reported PID is only in source, namespace/process birth identity unverified; not a usable host PID");
        d.load_state=d.active_state=d.sub_state=d.unit_file_state=d.slice=absent<std::string>("No corresponding manager property/group/effective enablement observed");
        row.linux_details={std::move(d),Support::estimated,"Runit protocol report projection, not kernel process identity/control authority"};out.services.push_back(std::move(row));
    }
    out.source=std::move(*input.value);
    return {std::move(out),coherent?Support::available:Support::estimated,"Explicit runit tree projected to ServiceInfo; report availability separate from census completeness"};
}
Metric<LinuxRunitServiceInventory> query_linux_runit_service_inventory(const std::string& root,std::uint32_t max){return detail::project_runit_services(query_linux_runit_inventory(root,max));}
}
