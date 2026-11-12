#include "backend.hpp"
#include "smbios.hpp"
#include "linux_process_stat.hpp"
#include "linux_process_memory.hpp"
#include "linux_process_io.hpp"
#include "linux_process_details.hpp"
#include "linux_process_metadata.hpp"
#include "linux_process_cpu.hpp"
#include "linux_app_history.hpp"
#include "linux_counts.hpp"
#include "linux_cpu_usage.hpp"
#include "linux_memory.hpp"
#include "linux_zram.hpp"
#include "linux_cpu_cache.hpp"
#include "linux_cpu_topology.hpp"
#include "linux_cpu_frequency.hpp"
#include "linux_uptime.hpp"
#include "linux_virtualization.hpp"
#include "linux_cpu_identity.hpp"
#include "linux_network_rates.hpp"
#include "linux_network_identity.hpp"
#include "linux_disk_rates.hpp"
#include "linux_disk_properties.hpp"
#include "linux_disk_identity.hpp"
#include "linux_formatted_capacity.hpp"
#include "linux_system_disk.hpp"
#include "linux_swap_disks.hpp"
#include "linux_nvml.hpp"
#include "linux_gpu_vulkan.hpp"
#include "linux_gpu_opencl.hpp"
#include "linux_current_opengl.hpp"
#include "linux_gpu_names.hpp"
#include "linux_gpu_driver_date.hpp"
#include "linux_gpu_sysfs.hpp"
#include "linux_systemd.hpp"
#include "linux_service_selection.hpp"
#include "linux_startup.hpp"
#include "linux_startup_inventory.hpp"
#include "pcinsight/linux_startup_publisher.hpp"
#include "linux_users.hpp"
#include "linux_logout.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <unordered_map>

#ifdef __linux__
#include <arpa/inet.h>
#include <dlfcn.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <utmpx.h>
#include <unistd.h>
#endif

namespace pcinsight::detail {

#ifdef __linux__
namespace {
using Clock = std::chrono::steady_clock;

std::string read_text(const std::filesystem::path& p) { std::ifstream f(p); std::ostringstream s; s << f.rdbuf(); return s.str(); }
std::string trim(std::string s) { while (!s.empty() && (s.back()=='\n'||s.back()=='\r'||s.back()==' '||s.back()=='\t')) s.pop_back(); auto n=s.find_first_not_of(" \t"); return n==std::string::npos?"":s.substr(n); }
template<class T> Metric<T> avail(T v, std::string note={}) { return {std::move(v), Support::available, std::move(note)}; }
template<class T> Metric<T> unsup(std::string note) { return {std::nullopt, Support::unsupported, std::move(note)}; }

using CpuCounters=std::optional<LinuxCpuCounters>;
CpuCounters cpu_counters(){return read_linux_cpu_counters();}


struct MountInventory { Metric<LinuxFormattedMap> formatted; Metric<std::string> root_disk; Metric<LinuxSwapDiskReport> swap_disks; };
MountInventory mount_inventory() {
    MountInventory out;out.formatted=read_native_linux_formatted_capacity();
    out.root_disk=read_native_linux_system_disk();
    out.swap_disks=read_native_linux_swap_disks();
    return out;
}

class LinuxBackend final : public Backend {
    CpuCounters prev_cpu_{};
    std::unordered_map<std::uint32_t,LinuxProcessStat> prev_proc_ticks_;
    LinuxProcessIoRateTracker process_io_;
    LinuxProcessCpuTracker process_cpu_;
    LinuxNetworkRateTracker network_rates_;
    LinuxDiskRateTracker disk_rates_;
    LinuxAppHistory history_;
    SmbiosInventory smbios_{read_smbios()};
    NvmlApi nvml_{};
    bool nvml_loaded_{};
    std::optional<PciNamesDatabase> pci_names_;
public:
    SystemSnapshot sample(const SampleOptions& options) override {
        SystemSnapshot s; s.captured_at=std::chrono::system_clock::now(); auto now=Clock::now(); auto cc=cpu_counters();
        if(!cc)prev_cpu_.reset();
        fill_cpu(s.cpu,cc); fill_memory(s.memory);
        std::vector<LinuxHistoryObservation> history_rows;
        if(!options.include_app_history){history_.reset();s.linux_app_history.note="Linux CPU history disabled; capture resets when disabled";}
        if(options.include_processes||options.include_app_history||options.include_user_sessions) {
            try{fill_processes(s.processes,cc,options.include_linux_process_descriptions,options.include_app_history?&history_rows:nullptr);}
            catch(...){history_.break_continuity();throw;}
            for (auto& process : s.processes) {
                const auto identity = prev_proc_ticks_.find(process.pid);
                if (identity != prev_proc_ticks_.end())
                    process.linux_start_ticks = avail(identity->second.start_ticks,
                        "/proc/PID/stat field 22; same host/boot/PID namespace only");
            }
        }
        else { prev_proc_ticks_.clear(); process_io_.reset(); process_cpu_.reset(); }
        if(options.include_networks) fill_networks(s.networks,now);
        else network_rates_.reset();
        if(options.include_disks) fill_disks(s.disks);
        else disk_rates_.reset();
        s.linux_gpu_inventory_source={{},Support::unsupported,"GPU inventory disabled"};
        if(options.include_gpus) {
            const auto drm_source=fill_linux_gpu_sysfs(s.gpus);
            for(auto& gpu:s.gpus)gpu.driver_date=linux_gpu_driver_date_unavailable(LinuxGpuDriverDateProvider::drm_sysfs);
            if (!nvml_loaded_) { nvml_ = load_linux_nvml(); nvml_loaded_ = true; }
            if(!pci_names_)pci_names_=load_linux_pci_names();
            fill_linux_gpu_names(s.gpus,*pci_names_);
            const auto nvml_snapshot=read_linux_nvml(nvml_);
            merge_linux_nvml(s.gpus,nvml_snapshot);
            s.linux_gpu_inventory_source=linux_gpu_inventory_source(drm_source,nvml_snapshot.support,nvml_snapshot.note);
            fill_linux_gpu_vulkan(s.gpus);
            fill_linux_gpu_opencl(s.gpus);
            if(options.include_current_opengl_context)fill_linux_current_opengl(s.gpus);
        }
        if(options.include_user_sessions) {
            auto logins = options.prefer_logind_sessions ? read_logind_logins() : std::nullopt;
            if(logins){
                s.user_sessions=aggregate_linux_users(*logins,s.processes);
                s.linux_session_inventory_source={"login1",Support::available,"Selected login1 ListSessions enumeration completed; not all host users, not atomic; CPU/RAM by username, I/O has separate reported-real-UID scope/coverage"};
            }else{
                const auto records=read_linux_login_records();
                s.linux_session_inventory_source={records.value?std::optional<std::string>{"utmp"}:std::nullopt,records.support,records.note};
                if(records.value)s.user_sessions=aggregate_linux_users(*records.value,s.processes);
            }
        }else s.linux_session_inventory_source.note="Linux user-session inventory disabled";
        if(options.include_startup_entries) {
            fill_startup(s.startup_entries);
            if(options.include_linux_startup_publisher)
                apply_linux_startup_publishers(s.startup_entries, read_linux_startup_publishers());
        }
        if(options.include_services) {
            if(options.linux_service_selection.provider!=LinuxServiceProvider::legacy_default){
                fill_linux_selected_services(s,options.linux_service_selection);
            }else if (auto services = read_native_systemd_services()){
                s.services = std::move(*services);
                s.linux_service_inventory_source={"systemd-system-manager",Support::available,"ListUnits/ListUnitFiles completed on one manager owner; per-field metadata/availability, not an atomic snapshot"};
            }else{
                fill_services(s.services);
                s.linux_service_inventory_source={"partial-file-scan",Support::estimated,"Init/unit filenames only; coverage/errors may be partial, no execution state or init-manager control"};
            }
        }else s.linux_service_inventory_source.note="Linux service inventory disabled";
        if(options.include_app_history)s.linux_app_history=history_.sample(cc,sysconf(_SC_CLK_TCK),history_rows,s.captured_at);
        if(!options.include_processes) s.processes.clear();
        prev_cpu_=cc; return s;
    }
private:
    void fill_cpu(CpuInfo& c, const CpuCounters& cc) {
        fill_linux_cpu_names(c,smbios_);
        fill_linux_cpu_topology(c);
        c.utilization_percent=linux_cpu_usage(prev_cpu_,cc);
        fill_linux_cpu_frequency(c);
        fill_linux_uptime(c);
        fill_linux_virtualization(c);
        fill_cache(c); fill_counts(c);
    }
    void fill_cache(CpuInfo& c){detail::fill_linux_cpu_cache(c);}
    void fill_counts(CpuInfo& c) { read_linux_counts(c); }
    void fill_memory(MemoryInfo& m){
        apply_linux_meminfo(m,read_linux_meminfo());
        m.type=smbios_metric(smbios_,smbios_.memory_type);m.form_factor=smbios_metric(smbios_,smbios_.memory_form_factor);
        m.speed_mts=smbios_metric(smbios_,smbios_.memory_speed_mts);m.slots_used=smbios_metric(smbios_,smbios_.slots_used);m.slots_total=smbios_metric(smbios_,smbios_.slots_total);
        apply_linux_memory_reservation(m,smbios_metric(smbios_,smbios_.installed_memory_bytes));
        m.linux_details.zram_devices=read_linux_zram();
    }
void fill_processes(std::vector<ProcessInfo>& out,const CpuCounters& cc,bool include_descriptions,std::vector<LinuxHistoryObservation>* history_rows){
    const auto descriptions=include_descriptions?read_native_linux_description_index():LinuxDescriptionIndex{{},Support::unsupported,"Desktop description lookup disabled by caller"};
    std::unordered_map<std::uint32_t,LinuxProcessStat> current;
    std::vector<LinuxProcessIoObservation> io_rows;
    std::vector<LinuxProcessCpuObservation> cpu_rows;
    std::size_t process_candidates=0;
    const auto ticks_per_second=sysconf(_SC_CLK_TCK);
    for(auto& e:std::filesystem::directory_iterator("/proc",std::filesystem::directory_options::skip_permission_denied)){
        auto n=e.path().filename().string();
        if(n.empty()||!std::all_of(n.begin(),n.end(),::isdigit))continue;
        if(++process_candidates>100000)throw std::length_error("Process enumeration exceeds 100000 candidates");
        try{
            ProcessInfo p;p.pid=static_cast<std::uint32_t>(std::stoul(n));
            const auto observed=read_linux_process_identity(e.path());const auto observed_at=Clock::now();
            auto stat=observed.value;if(!stat||stat->pid!=p.pid)continue;
            current[p.pid]=*stat;p.parent_pid=stat->parent_pid;p.thread_count=stat->threads;
            p.memory_bytes=read_linux_process_memory(e.path(),*stat);
            const auto details=read_linux_process_status(e.path(),*stat);
            Metric<std::string> account{{},Support::unsupported,"Real UID not available"};
            if(details.value&&details.value->real_uid)account=linux_username_for_uid(*details.value->real_uid);
            p.efficiency_mode_available={false,Support::available,
                "Generic/checked efficiency action disabled on Linux; advanced unchecked thread preset requires explicit PID/TID reuse-risk acknowledgement"};
            const auto executable=read_linux_process_executable(e.path(),*stat);
            const auto description=lookup_linux_description(descriptions,executable);
            apply_linux_process_details(p,details,account,executable,description);
            LinuxProcessIoObservation io{p.pid,stat->start_ticks,read_linux_process_io_counters(e.path(),*stat),Clock::now(),{}};
            // Each status read already verifies the same PID/start_ticks. Compare
            // real UID around this read, then the tracker compares BOTH endpoints.
            const auto after_io=read_linux_process_status(e.path(),*stat);
            io.real_uid=linux_process_io_uid(details,after_io);
            p.network_bytes_per_second=unsup<std::uint64_t>("requiere eBPF/cgroup o contabilidad del firewall");
            out.push_back(std::move(p));io_rows.push_back(std::move(io));
            cpu_rows.push_back({out.back().pid,observed,observed_at});
            if(history_rows)history_rows->push_back({out.back().pid,observed,executable,out.back().name,observed_at});
        }catch(...){}
    }
    const auto rates=process_io_.sample(io_rows);
    for(auto& p:out)if(const auto it=rates.find(p.pid);it!=rates.end()){
        p.disk_bytes_per_second=it->second;p.linux_details.disk_rate_real_uid=process_io_.attributed_uid(p.pid);
    }
    const auto cpu_rates=process_cpu_.sample(cc,ticks_per_second,cpu_rows);
    for(auto& p:out)if(const auto it=cpu_rates.find(p.pid);it!=cpu_rates.end())p.cpu_percent=it->second;
    prev_proc_ticks_=std::move(current);
}
    void fill_networks(std::vector<NetworkAdapter>& out,Clock::time_point now) {
        try {
            struct Addresses {
                LinuxNetworkAddressIdentity identity;
                std::vector<std::string> ipv4, ipv6;
            };
            std::map<std::string,Addresses> ips;
            auto indexed_ips=read_linux_interface_addresses();
            ifaddrs* addresses=nullptr;
            if (!indexed_ips&&getifaddrs(&addresses)==0) {
                // Own the list even if vector/string allocation throws.
                const std::unique_ptr<ifaddrs,decltype(&freeifaddrs)> owner(addresses,freeifaddrs);
                for (auto* p=addresses;p;p=p->ifa_next) {
                    if (!p->ifa_addr || !p->ifa_name) continue;
                    char text[INET6_ADDRSTRLEN]{};
                    const auto family=p->ifa_addr->sa_family;
                    if (family==AF_PACKET) {
                        const auto index=reinterpret_cast<const sockaddr_ll*>(p->ifa_addr)->sll_ifindex;
                        ips[p->ifa_name].identity.observe(index>0?static_cast<std::uint64_t>(index):0);
                    } else if (family==AF_INET && inet_ntop(family,&reinterpret_cast<sockaddr_in*>(p->ifa_addr)->sin_addr,text,sizeof text))
                        ips[p->ifa_name].ipv4.push_back(text);
                    else if (family==AF_INET6 && inet_ntop(family,&reinterpret_cast<sockaddr_in6*>(p->ifa_addr)->sin6_addr,text,sizeof text))
                        ips[p->ifa_name].ipv6.push_back(text);
                }
            }
            std::vector<LinuxNetworkCounters> counters;
            std::vector<NetworkAdapter> adapters;
            const PciNamesDatabase no_network_labels;
            if(!pci_names_)pci_names_=load_linux_pci_names();
            for (const auto& entry:std::filesystem::directory_iterator("/sys/class/net")) {
                const auto index=parse_linux_network_counter(read_text(entry.path()/"ifindex"));
                const auto received=parse_linux_network_counter(read_text(entry.path()/"statistics/rx_bytes"));
                const auto sent=parse_linux_network_counter(read_text(entry.path()/"statistics/tx_bytes"));
                const auto after=parse_linux_network_counter(read_text(entry.path()/"ifindex"));
                // Legacy totals aren't optional: omit unreadable/replaced rows, never fake zero.
                if (!index || !*index || index!=after || !received || !sent) continue;
                NetworkAdapter n;
                n.id=n.name=entry.path().filename().string();
                n.linux_details=read_linux_network_identity(n.name,*index,adapters.size()<256?*pci_names_:no_network_labels);
                if(n.linux_details.value){n.model=n.linux_details.value->model.value.value_or("");n.connection_type=n.linux_details.value->connection_type.value.value_or("");}
                if(indexed_ips) {
                    if(auto ip=take_linux_interface_addresses(*indexed_ips,*index,if_nametoindex(n.name.c_str()))) {
                        n.ipv4=std::move(ip->ipv4);n.ipv6=std::move(ip->ipv6);
                    }
                } else if (const auto ip=ips.find(n.name);ip!=ips.end()&&ip->second.identity.matches(*index)) {
                    n.ipv4=std::move(ip->second.ipv4);n.ipv6=std::move(ip->second.ipv6);
                }
                n.total_received_bytes=*received;n.total_sent_bytes=*sent;
                if (counters.size()>=100000) throw std::runtime_error("Too many network interfaces");
                counters.push_back({n.name,*index,*received,*sent});
                adapters.push_back(std::move(n));
            }
            const auto rates=network_rates_.sample(counters,now);
            for (auto& adapter:adapters) {
                const auto& value=rates.at(adapter.name);
                adapter.receive_bytes_per_second=value.received;
                adapter.send_bytes_per_second=value.sent;
            }
            out=std::move(adapters);
        } catch (...) { network_rates_.reset(); throw; }
    }
    void fill_disks(std::vector<DiskInfo>& out) {
        try {
            const auto now=Clock::now();
            const auto mounts=mount_inventory();
            std::vector<DiskInfo> disks;std::vector<LinuxDiskCounters> counters;
            for(const auto& entry:std::filesystem::directory_iterator("/sys/block",std::filesystem::directory_options::skip_permission_denied)) {
                if(disks.size()>=100000)throw std::runtime_error("Too many block devices");
                DiskInfo disk;disk.id=entry.path().filename().string();
                disk.model=trim(read_text(entry.path()/"device/model"));
                const auto rotational=read_text(entry.path()/"queue/rotational");
                std::error_code error;
                const auto subsystem=std::filesystem::read_symlink(entry.path()/"device/subsystem",error).filename().string();
                apply_linux_disk_properties(disk,read_text(entry.path()/"size"),rotational,subsystem);
                apply_linux_disk_identity(disk,read_linux_disk_identity(disk.id));
                disk.formatted_capacity_bytes=unsup<std::uint64_t>(mounts.formatted.value?
                    "No mapped mounted filesystem capacity; not zero or total formatted disk capacity":mounts.formatted.note);
                if(mounts.formatted.value)if(const auto found=mounts.formatted.value->find(disk.id);found!=mounts.formatted.value->end())
                    disk.formatted_capacity_bytes=found->second;
                disk.read_bytes_per_second.note=disk.write_bytes_per_second.note=disk.transfer_bytes_per_second.note=
                    disk.active_percent.note=disk.average_response_ms.note="Block counters or identity unavailable/changed during read";
                const auto device=trim(read_text(entry.path()/"dev"));
                const auto sequence_text=trim(read_text(entry.path()/"diskseq"));
                const auto stat=parse_linux_disk_stat(read_text(entry.path()/"stat"));
                const auto device_after=trim(read_text(entry.path()/"dev"));
                const auto sequence_after=trim(read_text(entry.path()/"diskseq"));
                const auto sequence=sequence_text.empty()?std::optional<std::uint64_t>{}:parse_linux_network_counter(sequence_text);
                if(stat&&valid_linux_disk_device(device)&&device==device_after&&sequence_text==sequence_after&&(sequence_text.empty()||sequence))
                    counters.push_back({disk.id,device,sequence,*stat});
                disks.push_back(std::move(disk));
            }
            apply_linux_system_disk_flags(disks,mounts.root_disk);
            apply_linux_swap_disk_flags(disks,mounts.swap_disks);
            const auto rates=disk_rates_.sample(counters,now);
            for(auto& disk:disks)if(const auto found=rates.find(disk.id);found!=rates.end()) {
                const auto& value=found->second;
                disk.read_bytes_per_second=value.read;disk.write_bytes_per_second=value.write;
                disk.transfer_bytes_per_second=value.transfer;disk.active_percent=value.active;
                disk.average_response_ms=value.response;
            }
            out=std::move(disks);
        }catch(...){disk_rates_.reset();throw;}
    }
    void fill_startup(std::vector<StartupEntry>& out) {
        out = read_native_xdg_startup_inventory().entries;
    }
    void fill_services(std::vector<ServiceInfo>& out) {
        std::set<std::string> seen;
        const std::filesystem::path dirs[]{"/etc/systemd/system", "/run/systemd/system",
            "/usr/lib/systemd/system", "/lib/systemd/system", "/etc/init.d"};
        for (const auto& dir : dirs) {
            std::error_code ec;
            std::filesystem::recursive_directory_iterator it(dir, std::filesystem::directory_options::skip_permission_denied, ec), end;
            for (; !ec && it != end; it.increment(ec)) {
                const bool sysv = dir == "/etc/init.d";
                if (!sysv && it->path().extension() != ".service") continue;
                std::error_code type_error;
                if (sysv && !it->is_regular_file(type_error)) continue;
                auto id = it->path().filename().string();
                if (!seen.insert(id).second) continue;
                ServiceInfo service;
                service.id = service.name = id;
                apply_linux_service_file_metadata(service,sysv);
                out.push_back(std::move(service));
            }
        }
    }
};
}
std::unique_ptr<Backend> make_backend(){return std::make_unique<LinuxBackend>();}
#elif !defined(_WIN32)
namespace { class UnsupportedBackend final : public Backend { public: SystemSnapshot sample(const SampleOptions&) override { SystemSnapshot s; s.captured_at=std::chrono::system_clock::now(); s.cpu.model="Backend aún no implementado para este sistema"; return s; } }; }
std::unique_ptr<Backend> make_backend(){return std::make_unique<UnsupportedBackend>();}
#endif
}
