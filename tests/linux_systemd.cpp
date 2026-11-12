#include "../src/linux_systemd.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <limits>

using namespace pcinsight;
using namespace pcinsight::detail;
int main() {
    std::vector<SystemdUnit> loaded{
        {"daemon.service", "Daemon description", "loaded", "active", "running", "/org/freedesktop/systemd1/unit/daemon"},
        {"oneshot.service", "Completed one shot", "loaded", "active", "exited", "/org/freedesktop/systemd1/unit/oneshot"},
        {"broken.service", "Failure", "loaded", "failed", "failed", "/org/freedesktop/systemd1/unit/broken"},
        {"stopped.service", "Stopped", "loaded", "inactive", "dead", "/org/freedesktop/systemd1/unit/stopped"},
        {"starting.service", "Starting", "loaded", "activating", "start", "/org/freedesktop/systemd1/unit/starting"},
        {"root.mount", "Not a service", "loaded", "active", "mounted", "/org/freedesktop/systemd1/unit/root"}
    };
    std::vector<SystemdUnitFile> files{
        {"/usr/lib/systemd/system/daemon.service", "enabled"},
        {"/etc/systemd/system/unloaded.service", "disabled"},
        {"/usr/lib/systemd/system/template@.service", "static"},
        {"/etc/systemd/system/alias.service", "alias"},
        {"/etc/systemd/system/masked.service", "masked"},
        {"/usr/lib/systemd/system/root.mount", "static"}
    };
    unsigned queried = 0;
    SystemdServiceReader reader;
    reader.loaded = [&] { return loaded; };
    reader.installed = [&] { return files; };
    reader.main_pid = [&](const std::string& path) -> Metric<std::uint32_t> {
        ++queried;
        if (path.ends_with("daemon")) return {123,Support::available,"fixture"};
        if (path.ends_with("starting")) return {{},Support::permission_required,"denied"};
        return {0,Support::available,"no main process"};
    };
    reader.unit_id=[&](const std::string& path)->Metric<std::string>{
        for(const auto& unit:loaded)if(unit.object_path==path)return {unit.name,Support::available,"fixture Unit.Id"};
        assert(false);return {};
    };
    reader.slice=[](const std::string&)->Metric<std::string>{return {"system.slice",Support::available,"fixture"};};
    const auto rows = read_systemd_services(reader);
    assert(rows.size() == 9 && queried == 5);
    auto find = [&](const char* name) -> const ServiceInfo& {
        auto it = std::find_if(rows.begin(), rows.end(), [&](const auto& r) { return r.id == name; });
        assert(it != rows.end()); return *it;
    };
    assert(find("daemon.service").pid == 123 && find("daemon.service").status == "running");
    assert(find("daemon.service").unit_file_state == "enabled" && find("daemon.service").description == "Daemon description");
    assert(find("oneshot.service").status == "active/exited" && !find("oneshot.service").pid);
    assert(find("broken.service").status == "failed/failed");
    assert(find("stopped.service").status == "inactive/dead");
    assert(find("starting.service").status == "activating/start" && !find("starting.service").pid);
    assert(find("unloaded.service").status == "unknown" && find("unloaded.service").load_state == "not_loaded");
    assert(find("masked.service").status == "unknown" && find("masked.service").unit_file_state == "masked");
    assert(find("alias.service").status == "unknown");
    assert(find("daemon.service").group=="system.slice");
    assert(find("daemon.service").linux_details.value->group_kind=="systemd-slice");
    assert(find("daemon.service").linux_details.value->description.value=="Daemon description");
    assert(find("oneshot.service").linux_details.value->main_pid.value==0); // measured native sentinel, not inaccessible
    assert(find("starting.service").linux_details.value->main_pid.support==Support::permission_required);
    assert(!find("starting.service").linux_details.value->main_pid.value);
    assert(find("unloaded.service").group.empty()&&!find("unloaded.service").linux_details.value->status.value);
    assert(!find("unloaded.service").linux_details.value->description.value);
    auto must_fail = [&](const SystemdServiceReader& source) {
        try { (void)read_systemd_services(source); assert(false); } catch (const std::exception&) {}
    };
    auto failed = reader;
    failed.installed = []() -> std::vector<SystemdUnitFile> { throw std::runtime_error("bus denied"); };
    must_fail(failed);
    loaded.push_back(loaded.front()); must_fail(reader); loaded.pop_back();
    files.push_back({"daemon.service", "disabled"}); must_fail(reader); files.pop_back();
    const auto original = loaded[0];
    loaded[0].object_path = "/invalid"; must_fail(reader);
    loaded[0] = original; loaded[0].description = std::string("bad\0data", 8); must_fail(reader);
    loaded[0] = original;
    failed.loaded = [] { return std::vector<SystemdUnit>(10001); }; must_fail(failed);
    failed = reader; failed.main_pid = {};
    for (const auto& row : read_systemd_services(failed)) assert(!row.pid);
    // Identity must be verified on both sides of PID/Slice reads; no property
    // request at all if the first identity check is unavailable or mismatched.
    for(unsigned mode=0;mode<5;++mode){
        auto source=reader;unsigned ids=0,pids=0,slices=0;
        source.unit_id=[&](const std::string& path)->Metric<std::string>{
            ++ids;if(mode==0)return {{},Support::permission_required,"denied"};
            if(mode==1||(mode==2&&ids%2==0))return {"replaced.service",Support::available,"changed"};
            if(mode==3&&ids%2==0)return {{},Support::unsupported,"vanished"};
            if(mode==4)return {"daemon.service",Support::unsupported,"inconsistent value must not be used"};
            return reader.unit_id(path);
        };
        source.main_pid=[&](const std::string&)->Metric<std::uint32_t>{++pids;return {123,Support::available,"fixture"};};
        source.slice=[&](const std::string&)->Metric<std::string>{++slices;return {"system.slice",Support::available,"fixture"};};
        for(const auto& row:read_systemd_services(source)){
            assert(!row.pid&&row.group.empty()&&!row.linux_details.value->slice.value);
            if(row.load_state!="not_loaded")assert(row.linux_details.value->main_pid.support==(mode==0?Support::permission_required:Support::unsupported));
        }
        if(mode==0||mode==1||mode==4)assert(!pids&&!slices);
        else assert(pids==5&&slices==5);
    }
    for(const auto& slice:std::vector<std::string>{"systemd","../bad.slice","bad@x.slice","bad\\escape.slice",std::string("system.slice\0",13),std::string(256,'s')+".slice"}){
        auto source=reader;source.slice=[&](const auto&)->Metric<std::string>{return {slice,Support::available,"fixture"};};
        for(const auto& row:read_systemd_services(source))assert(row.group.empty()&&!row.linux_details.value->slice.value);
    }
    for(const auto& slice:{"","system.slice","-.slice","work\\x20load.slice"}){
        auto source=reader;source.slice=[&](const auto&)->Metric<std::string>{return {slice,Support::available,"fixture"};};
        for(const auto& row:read_systemd_services(source))if(row.load_state!="not_loaded")assert(row.group==slice&&row.linux_details.value->slice.value==slice);
    }
    {auto source=reader;source.slice=[](const auto&)->Metric<std::string>{return {"system.slice",Support::permission_required,"ignore value"};};
        for(const auto& row:read_systemd_services(source))if(row.load_state!="not_loaded")assert(row.group.empty()&&row.linux_details.value->slice.support==Support::permission_required);}
    {auto source=reader;source.main_pid=[](const auto&)->Metric<std::uint32_t>{return {std::numeric_limits<std::uint32_t>::max(),Support::available,"invalid PID"};};
        for(const auto& row:read_systemd_services(source))assert(!row.pid&&!row.linux_details.value->main_pid.value);}
    loaded[0].description.clear(); // empty description is a known value
    {const auto empty_description=read_systemd_services(reader);const auto i=std::find_if(empty_description.begin(),empty_description.end(),[](const auto& row){return row.id=="daemon.service";});assert(i!=empty_description.end()&&i->linux_details.value->description.value=="");}
    loaded[0]=original;
    loaded[0].object_path+="/child";must_fail(reader);loaded[0]=original;
    loaded[0].name="bad name.service";must_fail(reader);loaded[0]=original;
    {auto excessive=reader;excessive.loaded=[] {std::vector<SystemdUnit> data(257);for(auto& u:data){u.name="ignored.mount";u.description=std::string(65536,'x');}return data;};must_fail(excessive);}
    ServiceInfo fallback=rows.front();fallback.description="unverified file parser";fallback.group="systemd";
    apply_linux_service_file_metadata(fallback,false);assert(fallback.group.empty()&&fallback.description.empty()&&!fallback.pid&&fallback.status=="unknown");
    assert(fallback.linux_details.value->provider=="systemd-file-scan"&&!fallback.linux_details.value->slice.value);
#ifdef __linux__
    const auto native = read_native_systemd_services();
    if (!native) { std::cout << "Native systemd unavailable; fixtures only\n"; return 0; }
    assert(!native->empty());
    unsigned running = 0, exited = 0, unloaded = 0,slices=0,empty_slices=0,pid_reports=0;
    for (const auto& service : *native) {
        assert(service.id.ends_with(".service")&&service.linux_details.value);
        const auto& d=*service.linux_details.value;
        assert(d.provider=="systemd-system-manager"&&d.group_kind=="systemd-slice");
        assert(service.group==d.slice.value.value_or(""));
        assert(service.description==d.description.value.value_or(""));
        if(d.slice.value){++slices;if(d.slice.value->empty())++empty_slices;}if(d.main_pid.value)++pid_reports;
        if(service.pid)assert(d.main_pid.value==service.pid&&*service.pid>0);
        if (service.active_state == "active" && service.sub_state == "running") {
            assert(service.status == "running"); ++running;
        }
        if (service.active_state == "active" && service.sub_state == "exited") {
            assert(service.status == "active/exited"); ++exited;
        }
        if (service.load_state == "not_loaded") { assert(service.status == "unknown" && !service.pid); ++unloaded; }
    }
    std::cout << "Native systemd: services=" << native->size() << ", running=" << running
        << ", active/exited=" << exited << ", not_loaded=" << unloaded<<", Slice reports="<<slices<<" (empty="<<empty_slices<<"), MainPID reports including0="<<pid_reports << '\n';
    Monitor monitor;
    SampleOptions options;
    options.include_processes = options.include_disks = options.include_networks = options.include_gpus = false;
    assert(monitor.sample(options).services.empty());
    options.include_services = true;
    const auto snapshot = monitor.sample(options);
    assert(!snapshot.services.empty());
    assert(snapshot.linux_service_inventory_source.value=="systemd-system-manager");
    assert(std::any_of(snapshot.services.begin(), snapshot.services.end(), [](const auto& row) { return !row.active_state.empty(); }));
    options.include_services = false;
    const auto disabled=monitor.sample(options);assert(disabled.services.empty()&&!disabled.linux_service_inventory_source.value);
#endif
}
