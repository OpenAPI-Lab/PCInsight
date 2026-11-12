#include "linux_service_selection.hpp"
#include "pcinsight/linux_services.hpp"
#include "pcinsight/linux_sysv_services.hpp"
#include "pcinsight/linux_runit_services.hpp"
#include "pcinsight/linux_s6_services.hpp"
#include "linux_openrc_services.hpp"
#include <exception>
namespace pcinsight::detail {
namespace {
using Rows=Metric<std::vector<ServiceInfo>>;
Rows absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
bool absolute(const std::string& s){return !s.empty()&&s.front()=='/'&&s.size()<=32768&&s.find('\0')==s.npos;}
bool bound(const LinuxServiceSelection& s){return s.max_entries&&s.max_entries<=4096;}
template<class T>Rows rows(Metric<T> result){if(!metric_is_known(result))return absent(result.note,result.support);return {std::move(result.value->services),result.support,result.note};}
Rows systemd(LinuxServiceScope scope){return rows(query_linux_service_inventory(scope));}
Rows configured(const LinuxServiceSelection& s){
    auto catalog=query_linux_openrc_catalog(s.init_directory,s.runlevel_directory,s.max_entries);
    const auto support=catalog.support;const auto note=catalog.note;const bool known=metric_is_known(catalog);const bool complete=known&&catalog.value->complete&&support==Support::available;
    auto result=project_openrc_services(std::move(catalog),{{},Support::unsupported,"Runtime marks not selected; no query"});
    if(!known||!result.value)return absent(result.note,known?result.support:support);
    return {std::move(result.value->configured_services),complete?Support::available:Support::estimated,note};
}
Rows marked(const LinuxServiceSelection& s){
    auto markers=query_linux_openrc_markers(s.root,s.max_entries);
    const auto support=markers.support;const auto note=markers.note;const bool known=metric_is_known(markers);const bool complete=known&&markers.value->complete&&support==Support::available;
    auto result=project_openrc_services({{},Support::unsupported,"Configured candidates not selected; no query"},std::move(markers));
    if(!known||!result.value)return absent(result.note,known?result.support:support);
    return {std::move(result.value->marked_services),complete?Support::available:Support::estimated,note};
}
}
LinuxServiceSelectionApi native_service_selection_api(){return {
    [](const auto&){return systemd(LinuxServiceScope::system);},[](const auto&){return systemd(LinuxServiceScope::current_user);},
    [](const auto& s){return rows(query_linux_sysv_service_inventory(s.init_directory,s.runlevel_directory,s.max_entries,s.sysv_prefix_budget));},
    [](const auto& s){return rows(query_linux_runit_service_inventory(s.root,s.max_entries));},
    [](const auto& s){return rows(query_linux_s6_service_inventory(s.root,s.max_entries));},configured,marked};}
void fill_linux_selected_services(SystemSnapshot& snapshot,const LinuxServiceSelection& selection,const LinuxServiceSelectionApi& api){
    snapshot.services.clear();snapshot.linux_service_inventory_source={{},Support::unsupported,"Invalid/legacy explicit selection; no query or fallback"};
    const SelectedServiceQuery* query=nullptr;std::string provider,scope;bool valid=true;
    switch(selection.provider){
    case LinuxServiceProvider::systemd_system:query=&api.systemd_system;provider="systemd-system-manager";scope="system manager";break;
    case LinuxServiceProvider::systemd_current_user:query=&api.systemd_user;provider="systemd-user-manager";scope="current effective UID manager";break;
    case LinuxServiceProvider::sysv:query=&api.sysv;provider="sysv-file-catalog";valid=bound(selection)&&absolute(selection.init_directory)&&absolute(selection.runlevel_directory)&&selection.sysv_prefix_budget<=16*1024*1024;scope=selection.init_directory+"; "+selection.runlevel_directory;break;
    case LinuxServiceProvider::runit:query=&api.runit;provider="runit-reported-inventory";valid=bound(selection)&&absolute(selection.root);scope=selection.root;break;
    case LinuxServiceProvider::s6:query=&api.s6;provider="s6-status-record-inventory";valid=bound(selection)&&absolute(selection.root);scope=selection.root;break;
    case LinuxServiceProvider::openrc_configured:query=&api.openrc_configured;provider="openrc-file-catalog";valid=bound(selection)&&absolute(selection.init_directory)&&absolute(selection.runlevel_directory);scope=selection.init_directory+"; "+selection.runlevel_directory;break;
    case LinuxServiceProvider::openrc_marked:query=&api.openrc_marked;provider="openrc-runtime-marks";valid=bound(selection)&&absolute(selection.root);scope=selection.root;break;
    default:return;
    }
    const auto prefix="Selected "+provider+" ["+scope+"]; no fallback: ";
    if(!valid||!query||!*query){snapshot.linux_service_inventory_source.note=prefix+"Invalid selection arguments or unavailable query";return;}
    try{auto result=(*query)(selection);snapshot.linux_service_inventory_source={{},result.support,prefix+result.note};if(metric_is_known(result)){snapshot.services=std::move(*result.value);snapshot.linux_service_inventory_source.value=provider;}}
    catch(const std::exception& e){snapshot.services.clear();snapshot.linux_service_inventory_source={{},Support::unsupported,prefix+e.what()};}
    catch(...){snapshot.services.clear();snapshot.linux_service_inventory_source={{},Support::unsupported,prefix+"Query exception; no rows published"};}
}
void fill_linux_selected_services(SystemSnapshot& snapshot,const LinuxServiceSelection& selection){fill_linux_selected_services(snapshot,selection,native_service_selection_api());}
}
