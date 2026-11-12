#include "../src/linux_users.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
template<class T> Metric<T> known(T n){return {n,Support::available,"fixture"};}
LinuxLoginRecord login(std::string id,std::string name,std::uint32_t uid){
    LinuxLoginRecord r{std::move(id),std::move(name),"online"};
    LinuxSessionDetails d;d.provider="fixture";d.unix_uid=known(uid);
    r.details={d,Support::available,"fixture reported UID"};return r;
}
ProcessInfo proc(std::uint32_t pid,std::string name,std::uint32_t uid,std::uint64_t rate){
    ProcessInfo p;p.pid=pid;p.username=std::move(name);p.linux_start_ticks=known<std::uint64_t>(10);
    p.linux_details.real_uid=known(uid);p.linux_details.disk_rate_real_uid=known(uid);
    p.disk_bytes_per_second=known(rate);p.network_bytes_per_second=known<std::uint64_t>(999);
    return p;
}
const LinuxUserIoDetails& io(const UserSession& r){assert(r.linux_io_details.value);return *r.linux_io_details.value;}
int main(){
    std::vector<LinuxLoginRecord> logins{login("1","alice",1000),login("2","alias",1000),login("3","alice",2000),login("4","root",0),{"5","alice","reported_login"}};
    std::vector<ProcessInfo> ps{proc(1,"unrelated-label",1000,10),proc(2,"alice",1000,999),proc(3,"alice",2000,20),proc(4,"alice",55,999),proc(5,"zero",0,0),proc(6,"alice",1000,888)};
    ps[1].linux_details.disk_rate_real_uid=known<std::uint32_t>(2000);
    ps[3].linux_details.real_uid={{},Support::unsupported,"no UID"};
    ps[5].disk_bytes_per_second={{},Support::permission_required,"denied"};
    ps[5].linux_details.disk_rate_real_uid={{},Support::permission_required,"denied"};
    const auto rows=aggregate_linux_users(logins,ps);assert(rows.size()==5);
    for(unsigned i=0;i<2;++i){
        const auto& d=io(rows[i]);assert(d.real_uid.value==1000&&d.matching_processes.value==3);
        assert(d.input_processes==6&&d.processes_without_uid==1);
        assert(d.disk_bytes_per_second.value==10&&rows[i].disk_bytes_per_second.value==10);
        assert(d.disk_bytes_per_second.support==Support::estimated);
        assert(d.disk_coverage.value->known==1&&d.disk_coverage.value->missing==2);
        assert(d.disk_coverage.value->permission_denied==1&&d.disk_coverage.value->identity_unverified==2);
        assert(!d.network_bytes_per_second.value&&!rows[i].network_bytes_per_second.value);
    }
    assert(rows[2].disk_bytes_per_second.value==20); // same username, distinct UID.
    assert(rows[3].disk_bytes_per_second.value==0&&io(rows[3]).real_uid.value==0);
    assert(!rows[4].disk_bytes_per_second.value&&!io(rows[4]).real_uid.value);
    assert(!io(rows[4]).matching_processes.value&&!io(rows[4]).disk_coverage.value);
    auto empty=aggregate_linux_users(logins,{});
    assert(io(empty[0]).matching_processes.value==0&&io(empty[0]).disk_coverage.value->known==0);
    assert(!empty[0].disk_bytes_per_second.value); // no known processes != measured zero.
    auto changed=ps;changed[0].linux_start_ticks.value.reset();
    auto r=aggregate_linux_users(logins,changed);
    assert(!r[0].disk_bytes_per_second.value&&r[0].disk_bytes_per_second.support==Support::permission_required);
    changed={proc(10,"a",1000,std::numeric_limits<std::uint64_t>::max()),proc(11,"b",1000,1)};
    r=aggregate_linux_users(logins,changed);
    assert(!r[0].disk_bytes_per_second.value&&io(r[0]).disk_coverage.value->overflow);
    assert(io(r[0]).disk_coverage.value->known==2&&io(r[0]).disk_coverage.value->missing==0);
    auto hidden=logins;hidden[0].details.value->unix_uid.support=Support::permission_required;
    r=aggregate_linux_users(hidden,ps);
    assert(!r[0].disk_bytes_per_second.value&&!io(r[0]).real_uid.value&&r[0].disk_bytes_per_second.support==Support::permission_required);
    changed=ps;changed[0].linux_details.real_uid.support=Support::unsupported;
    r=aggregate_linux_users(logins,changed);assert(io(r[0]).processes_without_uid==2&&!r[0].disk_bytes_per_second.value);
    changed=ps;changed.push_back(changed[0]);bool threw=false;
    try{(void)aggregate_linux_users(logins,changed);}catch(const std::invalid_argument&){threw=true;}assert(threw);
#ifdef __linux__
    SampleOptions opt;opt.include_app_history=opt.include_user_sessions=opt.include_disks=opt.include_networks=opt.include_gpus=opt.include_services=opt.include_startup_entries=false;
    opt.include_linux_process_descriptions=false;
    Monitor monitor;const auto first=monitor.sample(opt);
    auto own=[](const SystemSnapshot& s)->const ProcessInfo&{
        const auto it=std::find_if(s.processes.begin(),s.processes.end(),[](const auto& p){return p.pid==static_cast<std::uint32_t>(getpid());});
        assert(it!=s.processes.end());return *it;
    };
    assert(!own(first).linux_details.disk_rate_real_uid.value);
    char path[]="/tmp/pcinsight-user-io-XXXXXX";const int fd=mkstemp(path);assert(fd>=0);
    struct Cleanup{const char* path;~Cleanup(){unlink(path);}} cleanup{path};
    const std::string data(131072,'x');std::size_t done=0;
    while(done<data.size()){const auto n=write(fd,data.data()+done,data.size()-done);assert(n>0);done+=static_cast<std::size_t>(n);}
    assert(fsync(fd)==0&&close(fd)==0);
    const auto second=monitor.sample(opt);const auto& p=own(second);
    assert(p.linux_details.disk_rate_real_uid.value==static_cast<std::uint32_t>(getuid()));
    assert(p.disk_bytes_per_second.value);
    const std::vector<LinuxLoginRecord> selected{login("private-observed-user","label-not-used",getuid()),login("private-observed-user-2","another-label",getuid())};
    const auto native=aggregate_linux_users(selected,second.processes);
    assert(native[0].disk_bytes_per_second.value&&native[0].disk_bytes_per_second.value==native[1].disk_bytes_per_second.value);
    assert(io(native[0]).disk_coverage.value->known>0&&!native[0].network_bytes_per_second.value);
    std::cout<<"Native own process UID="<<getuid()<<" rate="<<*p.disk_bytes_per_second.value<<" B/s; visible UID sum="<<*native[0].disk_bytes_per_second.value
        <<" B/s known="<<io(native[0]).disk_coverage.value->known<<" missing="<<io(native[0]).disk_coverage.value->missing<<"; private file only, synthetic session labels\n";
    // Public integration still collects resources when it hides process rows.
    opt.include_processes=false;opt.include_user_sessions=true;
    const auto hidden_processes=monitor.sample(opt);assert(hidden_processes.processes.empty());
    for(const auto& s:hidden_processes.user_sessions){assert(s.linux_io_details.value&&!s.network_bytes_per_second.value);}
    std::cout<<"Native session I/O reports="<<hidden_processes.user_sessions.size()<<" with hidden process list\n";
#endif
}
