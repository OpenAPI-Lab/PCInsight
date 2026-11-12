#include "../src/linux_memory.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace pcinsight;
using namespace pcinsight::detail;
int main(){
    const auto text="MemTotal: 1000 kB\nMemAvailable: 400 kB\nCached: 200 kB\nSReclaimable: 50 kB\nCommitted_AS: 5000 kB\nCommitLimit: 2000 kB\nZswap: 10 kB\nHugePages_Total: 2\nUnknownFuture: arbitrary fields ignored\n";
    const auto data=parse_linux_meminfo(text);assert(data.value&&data.value->size()==7);
    MemoryInfo memory;memory.type={"firmware fixture",Support::available,"fixture"};
    apply_linux_meminfo(memory,data);
    assert(memory.reported_total_bytes.value==1024000&&memory.total_bytes==1024000);
    assert(memory.available_bytes.value==409600&&memory.in_use_bytes.value==614400);
    assert(memory.utilization_percent.value==60&&memory.cached_bytes.value==256000);
    assert(memory.available_bytes.support==Support::estimated&&memory.in_use_bytes.support==Support::estimated&&memory.cached_bytes.support==Support::estimated);
    assert(memory.committed_bytes.value==5120000&&memory.commit_limit_bytes.value==2048000); // overcommit may exceed limit, not an invalid subtraction
    assert(memory.compressed_bytes.value==10240&&memory.compressed_bytes.support==Support::estimated);
    assert(!memory.paged_pool_bytes.value&&!memory.nonpaged_pool_bytes.value);
    for(auto field:{"MemTotal","MemAvailable","Cached","SReclaimable","Committed_AS","CommitLimit","Zswap"}){
        auto partial=data;partial.value->erase(field);apply_linux_meminfo(memory,partial);
        if(std::string_view(field)=="MemTotal")assert(!memory.reported_total_bytes.value&&memory.total_bytes==0&&!memory.in_use_bytes.value&&!memory.utilization_percent.value);
        if(std::string_view(field)=="MemAvailable")assert(!memory.available_bytes.value&&!memory.in_use_bytes.value&&!memory.utilization_percent.value);
        if(std::string_view(field)=="Cached"||std::string_view(field)=="SReclaimable")assert(!memory.cached_bytes.value);
        if(std::string_view(field)=="Committed_AS")assert(!memory.committed_bytes.value);
        if(std::string_view(field)=="CommitLimit")assert(!memory.commit_limit_bytes.value);
        if(std::string_view(field)=="Zswap")assert(!memory.compressed_bytes.value);
    }
    auto edited=data;(*edited.value)["MemAvailable"]=1024001;apply_linux_meminfo(memory,edited);
    assert(!memory.available_bytes.value&&!memory.in_use_bytes.value&&!memory.utilization_percent.value&&memory.reported_total_bytes.value);
    edited=data;(*edited.value)["MemAvailable"]=0;apply_linux_meminfo(memory,edited);assert(memory.available_bytes.value==0&&memory.utilization_percent.value==100);
    (*edited.value)["MemAvailable"]=(*edited.value)["MemTotal"];apply_linux_meminfo(memory,edited);assert(memory.in_use_bytes.value==0&&memory.utilization_percent.value==0);
    edited=data;(*edited.value)["MemTotal"]=0;apply_linux_meminfo(memory,edited);assert(!memory.reported_total_bytes.value&&!memory.utilization_percent.value);
    edited=data;(*edited.value)["Cached"]=std::numeric_limits<std::uint64_t>::max();(*edited.value)["SReclaimable"]=1;apply_linux_meminfo(memory,edited);assert(!memory.cached_bytes.value);
    edited=data;(*edited.value)["Cached"]=1024000;apply_linux_meminfo(memory,edited);assert(!memory.cached_bytes.value);
    const auto max_kib=std::numeric_limits<std::uint64_t>::max()/1024;
    assert(parse_linux_meminfo("MemTotal: "+std::to_string(max_kib)+" kB\n").value->at("MemTotal")==max_kib*1024);
    for(const auto& invalid:std::vector<std::string>{"","MemTotal: -1 kB\n","MemTotal: +1 kB\n","MemTotal: 1\n","MemTotal: 1 B\n","MemTotal: 1 KB\n","MemTotal: 1 kB extra\n","MemTotal: 1x kB\n","MemTotal: 18446744073709551616 kB\n","MemTotal: "+std::to_string(max_kib+1)+" kB\n",std::string(text)+"Cached: 1 kB\n",std::string("MemTotal: 1\0 kB",16),std::string(1024*1024+1,'x')}){
        const auto invalid_data=parse_linux_meminfo(invalid);assert(!invalid_data.value);
        apply_linux_meminfo(memory,data);apply_linux_meminfo(memory,invalid_data);
        assert(!memory.reported_total_bytes.value&&memory.total_bytes==0&&!memory.available_bytes.value&&!memory.cached_bytes.value&&!memory.compressed_bytes.value);
        assert(memory.type.value=="firmware fixture");
    }
    assert(!parse_linux_meminfo("Unknown: "+std::string(65536,'x')).value);
    std::string many;for(unsigned i=0;i<10001;++i)many+="Unknown: 1\n";assert(!parse_linux_meminfo(many).value);
    apply_linux_meminfo(memory,parse_linux_meminfo("MemTotal: 10 kB\nMemAvailable: 0 kB\nCached: 0 kB\nSReclaimable: 0 kB\nZswap: 0 kB\n"));
    assert(memory.available_bytes.value==0&&memory.cached_bytes.value==0&&memory.compressed_bytes.value==0);
    apply_linux_meminfo(memory,{{},Support::permission_required,"denied fixture"});assert(memory.reported_total_bytes.support==Support::permission_required&&!memory.reported_total_bytes.value);
#ifdef __linux__
    const auto native=read_linux_meminfo();assert(native.value);
    apply_linux_meminfo(memory,native);assert(memory.reported_total_bytes.value&&memory.total_bytes>0);
    SampleOptions options;options.include_processes=false;options.include_app_history=false;
    options.include_disks=false;options.include_networks=false;options.include_gpus=false;
    options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto sample=monitor.sample(options);const auto& observed=sample.memory;
    assert(observed.reported_total_bytes.value==observed.total_bytes&&observed.total_bytes==memory.total_bytes);
    assert(observed.available_bytes.value&&observed.in_use_bytes.value&&observed.utilization_percent.value);
    assert(*observed.available_bytes.value<=observed.total_bytes&&*observed.in_use_bytes.value==observed.total_bytes-*observed.available_bytes.value);
    assert(std::isfinite(*observed.utilization_percent.value)&&*observed.utilization_percent.value>=0&&*observed.utilization_percent.value<=100);
    assert(observed.utilization_percent.support==Support::estimated);
    std::cout<<"Native Linux RAM: "<<observed.total_bytes<<" bytes, available="<<*observed.available_bytes.value<<", usage="<<*observed.utilization_percent.value<<"%; consistent same-sample subtraction\n";
#else
    assert(!read_linux_meminfo().value);
#endif
    std::cout<<"Meminfo: strict units/numbers/duplicates/bounds, missing vs zero, overflow, relationships and stale-value clearing passed\n";
}
