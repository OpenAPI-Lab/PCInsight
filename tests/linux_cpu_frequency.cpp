#include "../src/linux_cpu_frequency.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs=std::filesystem;
Metric<LinuxFrequencySnapshot> fixture(){
    return {LinuxFrequencySnapshot{{2,10,900},{{2,2400000,2000000,1800000},{10,2400000,2000000,1800000},{900,4800000,3500000,1800000}}},Support::available,"fixture"};
}
bool near(double a,double b){return std::abs(a-b)<=1e-12*std::max(1.0,std::abs(b));}
void write(const fs::path& p,const std::string& text){std::ofstream f(p,std::ios::binary);assert(f);f<<text;f.close();assert(f);}
int main(){
    assert(parse_linux_frequency_khz(" 2400000\n")==2400000);
    assert(parse_linux_frequency_khz("1")==1);
    const auto max=std::numeric_limits<std::uint64_t>::max();assert(parse_linux_frequency_khz(std::to_string(max))==max);
    for(const auto& s:std::vector<std::string>{""," ","0","-1","-0","+1","nan","inf","1.5","1e6","2400MHz","1 2","18446744073709551616",std::string("1\0",2),std::string(65,'1')})assert(!parse_linux_frequency_khz(s));
    CpuInfo cpu;auto a=fixture();apply_linux_cpu_frequency(cpu,a,a);
    assert(near(*cpu.current_ghz.value,3.2)&&near(*cpu.base_ghz.value,1.8));
    assert(cpu.current_ghz.support==Support::estimated&&cpu.current_ghz.note.find("cpuinfo_cur_freq")!=std::string::npos);
    auto b=a;for(auto& row:b.value->rows)row.hardware_khz=6000000;
    apply_linux_cpu_frequency(cpu,a,b);assert(near(*cpu.current_ghz.value,6)&&near(*cpu.base_ghz.value,1.8)); // dynamic values needn't match
    b=a;b.value->rows[0].hardware_khz.reset();apply_linux_cpu_frequency(cpu,a,b);
    assert(near(*cpu.current_ghz.value,2.5)&&cpu.current_ghz.note.find("scaling_cur_freq")!=std::string::npos); // full fallback, not hardware/scaling mixture
    b.value->rows[1].scaling_khz.reset();apply_linux_cpu_frequency(cpu,a,b);assert(!cpu.current_ghz.value&&cpu.base_ghz.value);
    b=a;b.value->rows[1].base_khz=1900000;apply_linux_cpu_frequency(cpu,b,b);assert(cpu.current_ghz.value&&!cpu.base_ghz.value); // heterogeneous bases not averaged
    b=a;b.value->rows[1].base_khz.reset();apply_linux_cpu_frequency(cpu,a,b);assert(!cpu.base_ghz.value&&cpu.current_ghz.value);
    b=a;for(auto& row:b.value->rows)row.base_khz=2000000;
    apply_linux_cpu_frequency(cpu,a,b);assert(!cpu.base_ghz.value&&cpu.current_ghz.value); // configuration changed between reads
    apply_linux_cpu_frequency(cpu,b,b);assert(near(*cpu.base_ghz.value,2));
    b=a;std::reverse(b.value->rows.begin(),b.value->rows.end());apply_linux_cpu_frequency(cpu,a,b);assert(near(*cpu.current_ghz.value,3.2)&&cpu.base_ghz.value);
    for(int mode=0;mode<7;++mode){
        b=a;
        if(mode==0)b.value->online.clear();
        if(mode==1)b.value->online.push_back(900);
        if(mode==2)b.value->rows[0].cpu=900;
        if(mode==3)b.value->rows.pop_back();
        if(mode==4)b.value->rows[0].cpu=7;
        if(mode==5)b.value->online.resize(4097);
        if(mode==6)b.value->rows[0].hardware_khz=0;
        apply_linux_cpu_frequency(cpu,b,b);assert(!cpu.current_ghz.value&&!cpu.base_ghz.value);
    }
    b=a;b.value->online.back()=901;b.value->rows.back().cpu=901;apply_linux_cpu_frequency(cpu,a,b);assert(!cpu.current_ghz.value&&!cpu.base_ghz.value);
    apply_linux_cpu_frequency(cpu,a,a);apply_linux_cpu_frequency(cpu,{},a);assert(!cpu.current_ghz.value&&!cpu.base_ghz.value); // stale values cleared
    b=a;for(auto& row:b.value->rows){row.hardware_khz=max;row.base_khz=max;}
    apply_linux_cpu_frequency(cpu,b,b);assert(near(*cpu.current_ghz.value,static_cast<double>(max)/1000000.0)&&std::isfinite(*cpu.base_ghz.value)); // sum would overflow uint64
    b=a;for(auto& row:b.value->rows){row.hardware_khz=1;row.base_khz=1;}
    apply_linux_cpu_frequency(cpu,b,b);assert(near(*cpu.current_ghz.value,0.000001)&&near(*cpu.base_ghz.value,0.000001));
#ifdef __linux__
    const auto root=fs::temp_directory_path()/("pcinsight-frequency-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(fs::create_directory(root));struct Cleanup{fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}} cleanup{root};
    fill_linux_cpu_frequency(cpu,root);assert(!cpu.current_ghz.value&&!cpu.base_ghz.value);
    write(root/"online","2,10,900\n");
    // CPUs2/10 share one policy; the aggregate is per logical CPU, not per policy.
    const auto policy=root/"policy-shared";assert(fs::create_directory(policy));
    for(const auto& row:a.value->rows){
        const auto path=root/("cpu"+std::to_string(row.cpu));assert(fs::create_directory(path));
        if(row.cpu!=900)fs::create_directory_symlink(policy,path/"cpufreq");else assert(fs::create_directory(path/"cpufreq"));
        const auto dir=path/"cpufreq";
        write(dir/"cpuinfo_cur_freq",std::to_string(*row.hardware_khz));write(dir/"scaling_cur_freq",std::to_string(*row.scaling_khz));write(dir/"base_frequency",std::to_string(*row.base_khz));
        write(dir/"cpuinfo_max_freq","9000000"); // never substituted for base
    }
    fill_linux_cpu_frequency(cpu,root);assert(near(*cpu.current_ghz.value,3.2)&&near(*cpu.base_ghz.value,1.8));
    write(policy/"cpuinfo_cur_freq","nan\n");fill_linux_cpu_frequency(cpu,root);assert(near(*cpu.current_ghz.value,2.5)&&cpu.current_ghz.note.find("scaling_cur_freq")!=std::string::npos);
    write(root/"cpu900/cpufreq/scaling_cur_freq","1.5\n");fill_linux_cpu_frequency(cpu,root);assert(!cpu.current_ghz.value&&cpu.base_ghz.value);
    write(policy/"base_frequency","0\n");fill_linux_cpu_frequency(cpu,root);assert(!cpu.current_ghz.value&&!cpu.base_ghz.value);
    write(policy/"base_frequency",std::string(65,'1'));fill_linux_cpu_frequency(cpu,root);assert(!cpu.base_ghz.value);
    write(policy/"base_frequency","1800000");fs::remove(root/"cpu900/cpufreq/base_frequency");fill_linux_cpu_frequency(cpu,root);assert(!cpu.base_ghz.value); // max does not become base
    write(root/"online","2,10,900,901\n");fill_linux_cpu_frequency(cpu,root);assert(!cpu.current_ghz.value&&!cpu.base_ghz.value); // no partial average
    write(root/"online",std::string(65537,'1'));assert(!read_linux_cpu_frequency(root).value);
    const auto native=read_linux_cpu_frequency();assert(native.value);
    SampleOptions options;options.include_processes=false;options.include_app_history=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto snapshot=monitor.sample(options);
    auto show=[](const char* label,const Metric<double>& m){
        std::cout<<label<<": ";if(m.value){assert(std::isfinite(*m.value)&&*m.value>0&&m.support==Support::estimated);std::cout<<*m.value<<" GHz";}else std::cout<<"unavailable";
        std::cout<<"; "<<m.note<<'\n';
    };
    std::cout<<"Native Linux frequency inventory: "<<native.value->online.size()<<" online CPUs\n";
    show("Current",snapshot.cpu.current_ghz);show("Base",snapshot.cpu.base_ghz);
    // When this native host exposes no values, require honest absence end-to-end.
    const auto after=read_linux_cpu_frequency();
    if(after.value&&native.value->online==after.value->online){
        auto no_frequencies=[](const LinuxFrequencySnapshot& s){return std::all_of(s.rows.begin(),s.rows.end(),[](const auto& r){return !r.hardware_khz&&!r.scaling_khz&&!r.base_khz;});};
        if(no_frequencies(*native.value)&&no_frequencies(*after.value))assert(!snapshot.cpu.current_ghz.value&&!snapshot.cpu.base_ghz.value);
    }
    std::cout<<"Linux fixtures: shared policy weighting, CPU0 offline, malformed/oversized/absent sources and no partial/max-as-base fallback passed\n";
#else
    assert(!read_linux_cpu_frequency().value);
#endif
    std::cout<<"Frequency: strict positive uint64 kHz, source separation, complete coverage, dynamic values, heterogeneous/changed base, overflow and stale clearing passed\n";
}
