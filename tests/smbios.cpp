#include "../src/smbios.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>
using namespace pcinsight;
using namespace pcinsight::detail;
using Data=std::vector<unsigned char>;
namespace {
void word(Data& d, std::size_t p, std::uint16_t n) {
    d[p]=static_cast<unsigned char>(n); d[p+1]=static_cast<unsigned char>(n>>8);
}
void dword(Data& d, std::size_t p, std::uint32_t n) {
    word(d,p,static_cast<std::uint16_t>(n)); word(d,p+2,static_cast<std::uint16_t>(n>>16));
}
Data device(std::uint16_t size=8192, std::uint16_t speed=3200) {
    Data d(0x22,0); d[0]=17; d[1]=0x22;
    word(d,4,0x10); word(d,0x0c,size); d[0x0e]=0x0d; d[0x12]=0x1a;
    word(d,0x20,speed); return d;
}
void append(Data& table,const Data& record) {
    table.insert(table.end(),record.begin(),record.end()); table.insert(table.end(),{0,0});
}
Data array(unsigned char use=3, std::uint16_t handle=0x10) {
    Data d(0x0f,0); d[0]=16; d[1]=0x0f; word(d,2,handle); d[5]=use; return d;
}
Data table(std::initializer_list<Data> records) {
    Data d; for (const auto& r:records) append(d,r); append(d,{127,4,0,0}); return d;
}
void rejected(const Data& d) {
    auto r=parse_smbios(d);
    assert(r.support==Support::unsupported);
    assert(!r.installed_memory_bytes && !r.slots_total && !r.firmware_cpu_ghz);
}
}
int main() {
    const auto d=table({array(),device(),device(0)});
    auto r=parse_smbios(d);
    assert(r.support==Support::available);
    assert(r.installed_memory_bytes==8192ULL*1024*1024);
    assert(r.memory_speed_mts==3200 && r.memory_type=="DDR4" && r.memory_form_factor=="SODIMM");
    assert(r.slots_used==1 && r.slots_total==2);
    assert(smbios_metric(r,r.memory_type).support==Support::available);
    assert(smbios_metric<double>(r,std::nullopt).support==Support::unsupported);
    auto denied=r; denied.support=Support::permission_required;
    assert(smbios_metric<double>(denied,std::nullopt).support==Support::permission_required);
    // File and in-memory paths share the same parser.
    const auto dir=std::filesystem::temp_directory_path() /
        ("pcinsight-smbios-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(std::filesystem::create_directory(dir));
    const auto path=dir/"table.bin";
    { std::ofstream f(path,std::ios::binary); f.write(reinterpret_cast<const char*>(d.data()),d.size()); assert(f); }
    assert(read_smbios(path.string()).installed_memory_bytes==r.installed_memory_bytes);
    assert(std::filesystem::remove(path)); assert(std::filesystem::remove(dir));
    assert(read_smbios(path.string()).support==Support::unsupported);
    // Unknown size must not silently become an empty slot or a partial total.
    r=parse_smbios(table({array(),device(),device(0xffff)}));
    assert(!r.installed_memory_bytes && !r.slots_used && r.slots_total==2);
    auto extended=device(0x7fff); extended.resize(0x20); extended[1]=0x20;
    dword(extended,0x1c,65536);
    assert(parse_smbios(table({array(),extended})).installed_memory_bytes==65536ULL*1024*1024);
    extended.resize(0x1c); extended[1]=0x1c;
    assert(!parse_smbios(table({array(),extended})).installed_memory_bytes);
    assert(parse_smbios(table({array(),device(0x8400)})).installed_memory_bytes==1024ULL*1024);
    auto max_only=device(8192,0); word(max_only,0x15,6400);
    assert(!parse_smbios(table({array(),max_only})).memory_speed_mts);
    auto fast=device(8192,0xffff); fast.resize(0x5c); fast[1]=0x5c; dword(fast,0x58,70000);
    assert(parse_smbios(table({array(),fast})).memory_speed_mts==70000);
    fast.resize(0x22); fast[1]=0x22;
    assert(!parse_smbios(table({array(),fast})).memory_speed_mts);
    auto mixed=device(8192,4800); mixed[0x12]=0x22; mixed[0x0e]=9;
    r=parse_smbios(table({array(),device(),mixed}));
    assert(!r.memory_speed_mts && !r.memory_type && !r.memory_form_factor);
    // Skip memory arrays explicitly identified as video memory.
    auto video=device(); word(video,4,0x20);
    r=parse_smbios(table({array(),array(4,0x20),device(),video}));
    assert(r.slots_total==1 && r.installed_memory_bytes==8192ULL*1024*1024);
    assert(parse_smbios(table({device()})).support==Support::estimated);
    rejected(table({array(),video})); // dangling array ownership
    rejected(table({array(),array(),device()}));
    Data cpu(0x19,0); cpu[0]=4; cpu[1]=0x19; cpu[5]=3; cpu[0x18]=0x41; word(cpu,0x16,4500);
    assert(parse_smbios(table({cpu})).firmware_cpu_ghz==4.5);
    auto cpu2=cpu; word(cpu2,0x16,3500);
    assert(!parse_smbios(table({cpu,cpu2})).firmware_cpu_ghz);
    cpu[0x18]=0; assert(!parse_smbios(table({cpu})).firmware_cpu_ghz);
    // Reject every truncation inside a record; a full record boundary is legal
    // for older SMBIOS tables which need not carry a type-127 terminator.
    Data single; append(single,device());
    for (std::size_t n=0;n<single.size();++n) rejected(Data(single.begin(),single.begin()+n));
    auto bad=single; bad[1]=3; rejected(bad);
    bad=single; bad[1]=255; rejected(bad);
    bad=d; bad.pop_back(); rejected(bad); // truncated end-of-table strings
    // Windows wrapper is independently bounded; no temporary file conversion.
    Data wrapped(8,0); wrapped[1]=3; wrapped[2]=3; dword(wrapped,4,static_cast<std::uint32_t>(d.size()));
    wrapped.insert(wrapped.end(),d.begin(),d.end());
    assert(parse_windows_smbios(wrapped).slots_total==2);
    for (std::size_t n=0;n<wrapped.size();++n)
        assert(parse_windows_smbios(Data(wrapped.begin(),wrapped.begin()+n)).support==Support::unsupported);
    wrapped.push_back(0); assert(parse_windows_smbios(wrapped).support==Support::unsupported);
}
