#include "linux_openrc_services.hpp"
#include <set>
namespace pcinsight {
namespace {
template<class T>Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support==Support::permission_required?support:Support::unsupported,std::move(note)};}
bool absolute(const std::string& s){return !s.empty()&&s.front()=='/'&&s.size()<=32768&&s.find('\0')==s.npos;}
bool filename(const std::string& s){return !s.empty()&&s.front()!='.'&&s.size()<=255&&s.find('/')==s.npos&&s.find('\0')==s.npos;}
std::string id(std::string prefix,const std::string& s){constexpr char hex[]="0123456789abcdef";for(const unsigned char c:s){prefix+=hex[c>>4];prefix+=hex[c&15];}return prefix;}
ServiceInfo row(const std::string& name,bool mark,bool coherent){
    ServiceInfo r;r.id=id(mark?"openrc-mark:":"openrc-file:",name);r.name=name;r.status="unknown";LinuxServiceDetails d;
    d.provider=mark?"openrc-runtime-marks":"openrc-file-catalog";
    d.name={name,coherent?Support::available:Support::estimated,mark?"Observed runtime marker name, NOT identity of an init.d candidate":"Observed init.d candidate name, NOT runtime service identity"};
    d.status=absent<std::string>(mark?"No unique available primary mark":"Init.d/runlevel files do not establish execution state");
    d.main_pid=absent<std::uint32_t>("No process identity observed; marker targets not followed and no pidfile/proc guessing");
    d.description=absent<std::string>("No description observed; scripts/librc not evaluated");
    d.load_state=d.active_state=d.sub_state=d.unit_file_state=d.slice=absent<std::string>("No corresponding manager property/effective enablement/group observed");
    r.linux_details={std::move(d),Support::estimated,"Filesystem observation only; not verified process life, installed-manager census or control authority"};return r;
}
Metric<std::string> label(const LinuxOpenrcMarkerEntry& e,bool coherent){
    if(!coherent)return absent<std::string>("Partial/unstable runtime mark inventory");
    const auto parsed=decode_linux_openrc_markers(e.marker_mask);
    if(!parsed.value||!e.marked_state.value||e.marked_state.support!=Support::available||parsed.value!=e.marked_state.value)return absent<std::string>("No consistent unique primary mark: "+e.marked_state.note,e.marked_state.support);
    std::string s;switch(*parsed.value){case LinuxOpenrcMarkedState::started:s="marked-started";break;case LinuxOpenrcMarkedState::stopping:s="marked-stopping";break;case LinuxOpenrcMarkedState::starting:s="marked-starting";break;case LinuxOpenrcMarkedState::inactive:s="marked-inactive";break;}
    return {std::move(s),Support::estimated,"Unique filesystem primary mark ONLY; not running/stopped/readiness/crashed or binding to a configured script"};
}
}
Metric<LinuxOpenrcServiceInventory> detail::project_openrc_services(Metric<LinuxOpenrcCatalog> catalog,Metric<LinuxOpenrcMarkerInventory> markers){
    const bool have_catalog=metric_is_known(catalog),have_marks=metric_is_known(markers);
    if(!have_catalog&&!have_marks)return absent<LinuxOpenrcServiceInventory>("Catalog: "+catalog.note+"; markers: "+markers.note,catalog.support==Support::permission_required||markers.support==Support::permission_required?Support::permission_required:Support::unsupported);
    LinuxOpenrcServiceInventory out;bool catalog_complete=false,marks_complete=false;
    if(have_catalog){
        const auto& c=*catalog.value;if(!absolute(c.init_directory)||!absolute(c.runlevel_directory)||c.scripts.size()>4096)return absent<LinuxOpenrcServiceInventory>("Invalid OpenRC catalog scope/bound");
        std::set<std::string> names;catalog_complete=c.complete&&catalog.support==Support::available;
        for(const auto& e:c.scripts){if(!filename(e.name)||!names.insert(e.name).second)return absent<LinuxOpenrcServiceInventory>("Invalid/duplicate configured candidate");out.configured_services.push_back(row(e.name,false,catalog_complete));}
    }
    if(have_marks){
        const auto& m=*markers.value;if(!absolute(m.root)||m.entries.size()>4096)return absent<LinuxOpenrcServiceInventory>("Invalid OpenRC runtime scope/bound");
        std::set<std::string> names;marks_complete=m.complete&&markers.support==Support::available;
        for(const auto& e:m.entries){if(!filename(e.name)||!names.insert(e.name).second)return absent<LinuxOpenrcServiceInventory>("Invalid/duplicate runtime mark name");auto r=row(e.name,true,marks_complete);auto status=label(e,marks_complete);if(metric_is_known(status))r.status=*status.value;r.linux_details.value->status=std::move(status);out.marked_services.push_back(std::move(r));}
    }
    out.catalog=std::move(catalog);out.markers=std::move(markers);
    return {std::move(out),catalog_complete&&marks_complete?Support::available:Support::estimated,"Independent OpenRC file/marker projections; NO cross-source identity/name join or atomic combined census"};
}
Metric<LinuxOpenrcServiceInventory> query_linux_openrc_service_inventory(const std::string& init,const std::string& levels,const std::string& runtime,std::uint32_t maximum){
    if(!absolute(init)||!absolute(levels)||!absolute(runtime)||!maximum||maximum>4096)return absent<LinuxOpenrcServiceInventory>("Invalid explicit OpenRC roots/bound1..4096 per source");
    auto catalog=query_linux_openrc_catalog(init,levels,maximum);auto markers=query_linux_openrc_markers(runtime,maximum);
    return detail::project_openrc_services(std::move(catalog),std::move(markers));
}
}
