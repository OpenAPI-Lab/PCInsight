#include "../src/linux_cpu_identity.hpp"
#include "../src/cpu_identity_text.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs=std::filesystem;
using Data=std::vector<unsigned char>;
void append_cpu(Data& table,std::uint16_t handle,const std::string& vendor,const std::string& model,unsigned vi=1,unsigned mi=2,bool populated=true){
    Data record(0x1a,0);record[0]=4;record[1]=0x1a;record[2]=static_cast<unsigned char>(handle);record[3]=static_cast<unsigned char>(handle>>8);record[5]=3;record[7]=static_cast<unsigned char>(vi);record[0x10]=static_cast<unsigned char>(mi);record[0x18]=populated?0x41:0;
    table.insert(table.end(),record.begin(),record.end());
    table.insert(table.end(),vendor.begin(),vendor.end());table.push_back(0);table.insert(table.end(),model.begin(),model.end());table.insert(table.end(),{0,0});
}
void write(const fs::path& path,const std::string& text){std::ofstream out(path,std::ios::binary);assert(out);out<<text;out.close();assert(out);}
int main(){
    assert(cpu_identity_text("  Vendor\t CPU   Model  ")=="Vendor CPU Model");
    assert(cpu_identity_text("\xc3\x91 CPU")=="\xc3\x91 CPU");
    for(const auto& bad:std::vector<std::string>{""," \t",std::string(1025,'x'),std::string("a\0b",3),"a\nb","\x7f","\x80","\xc0\xaf","\xe0\x80\xaf","\xed\xa0\x80","\xf4\x90\x80\x80","\xc2\x80","\xe2\x82"})assert(!cpu_identity_text(bad));
    const std::string x86="processor : 2\nvendor_id : GenuineFixture\nmodel name : Fixture CPU 9\n\nprocessor : 900\nvendor_id : GenuineFixture\nmodel name : Fixture  CPU\t9\n";
    auto names=parse_linux_cpu_names(x86);assert(names.value&&names.value->cpu_ids==std::vector<std::uint32_t>({2,900}));
    CpuInfo cpu;SmbiosInventory no_firmware;apply_linux_cpu_names(cpu,names,no_firmware);
    assert(cpu.vendor=="GenuineFixture"&&cpu.model=="Fixture CPU 9"&&cpu.reported_model.support==Support::available);
    Data raw;append_cpu(raw,1,"Firmware Vendor","Firmware Model");append_cpu(raw,2,"Firmware Vendor","Firmware Model");
    const auto fw=parse_smbios(raw);assert(fw.cpu_vendor=="Firmware Vendor"&&fw.cpu_model=="Firmware Model");
    apply_linux_cpu_names(cpu,names,fw);assert(cpu.model=="Fixture CPU 9"); // kernel names preferred
    const auto arm=parse_linux_cpu_names("processor : 0\nCPU implementer : 0x41\nCPU part : 0xd0b\n\nprocessor : 1\nCPU implementer : 0x41\nCPU part : 0xd03\n\nHardware : Board Vendor\nModel : Board Model\n");
    assert(arm.value&&!arm.value->model.value&&arm.value->model_fallback);
    apply_linux_cpu_names(cpu,arm,fw);assert(cpu.vendor=="Firmware Vendor"&&cpu.model=="Firmware Model"&&cpu.reported_model.support==Support::estimated&&cpu.reported_model.note.find("SMBIOS")!=std::string::npos);
    apply_linux_cpu_names(cpu,arm,no_firmware);assert(cpu.vendor.empty()&&cpu.model.empty()&&!cpu.reported_model.value); // no board or part-ID marketing lookup
    for(const auto& [key,value]:std::vector<std::pair<std::string,std::string>>{{"model name","ARMv7 Processor rev 1"},{"cpu model","MIPS Fixture V1.0"},{"cpu","POWER Fixture, altivec supported"}}){
        const auto text="processor : 0\n"+key+" : "+value+"\n\nprocessor : 2\n"+key+" : "+value+"\n";
        apply_linux_cpu_names(cpu,parse_linux_cpu_names(text),no_firmware);assert(cpu.model==value&&cpu.reported_model.note.find(key)!=std::string::npos);
    }
    for(const auto& text:std::vector<std::string>{"processor:0\nmodel name:A\n\nprocessor:1\nmodel name:B\n","processor:0\nmodel name:A\n\nprocessor:1\nflags:x\n","processor:0\nmodel name:A\nmodel name:A\n","processor:0\nmodel name:A\ncpu:A\n","processor:0\nmodel name:"+std::string(1025,'x')+"\n"}){
        const auto parsed=parse_linux_cpu_names(text);assert(parsed.value&&!parsed.value->model.value&&!parsed.value->model_fallback);
        apply_linux_cpu_names(cpu,parsed,fw);assert(!cpu.reported_model.value&&cpu.model.empty()); // firmware mustn't hide observed partial/mixed identity
    }
    auto denied=no_firmware;denied.support=Support::permission_required;denied.note="fixture denied";
    apply_linux_cpu_names(cpu,arm,denied);assert(!cpu.reported_model.value&&cpu.reported_model.support==Support::permission_required);
    apply_linux_cpu_names(cpu,{},fw);assert(cpu.model=="Firmware Model"&&cpu.reported_model.support==Support::estimated); // independent firmware source if proc unusable
    apply_linux_cpu_names(cpu,{},no_firmware);assert(cpu.model.empty()&&cpu.vendor.empty()&&!cpu.reported_model.value&&!cpu.reported_vendor.value);
    for(const auto& invalid:std::vector<std::string>{"","processor:-1\n","processor:+1\n","processor:4294967296\n","processor:0\n\nprocessor:0\n",std::string("processor:0\0\n",13),"ignored:"+std::string(65537,'x'),std::string(16*1024*1024+1,'x')})assert(!parse_linux_cpu_names(invalid).value);
    std::string many;for(unsigned i=0;i<4097;++i)many+="processor:"+std::to_string(i)+"\n";assert(!parse_linux_cpu_names(many).value);
    std::string lines;for(unsigned i=0;i<250001;++i)lines+='\n';assert(!parse_linux_cpu_names(lines).value);
    Data different;append_cpu(different,1,"Firmware Vendor","A");append_cpu(different,2,"Firmware Vendor","B");auto inventory=parse_smbios(different);assert(inventory.cpu_vendor=="Firmware Vendor"&&!inventory.cpu_model);
    Data empty_index;append_cpu(empty_index,1,"Vendor","Model",1,0);assert(!parse_smbios(empty_index).cpu_model);
    Data bad_index;append_cpu(bad_index,1,"Vendor","Model",1,3);assert(!parse_smbios(bad_index).cpu_model);
    Data missing_one;append_cpu(missing_one,1,"Vendor","Model");append_cpu(missing_one,2,"Vendor","Model",1,0);assert(!parse_smbios(missing_one).cpu_model);
    Data unused;append_cpu(unused,1,"Vendor","Model");append_cpu(unused,2,"Other","Other",1,2,false);assert(parse_smbios(unused).cpu_model=="Model");
    for(const auto& text:std::vector<std::string>{"Unknown","Not Specified","To Be Filled By O.E.M.",std::string(1025,'x'),"bad\x1b"}){Data table;append_cpu(table,1,"Vendor",text);assert(!parse_smbios(table).cpu_model);}
    Data short_cpu=raw;short_cpu.insert(short_cpu.end(),{4,4,99,0,0,0});assert(!parse_smbios(short_cpu).cpu_model);
    Data one;append_cpu(one,1,"Vendor","Model");for(std::size_t n=0;n<one.size();++n){auto partial=parse_smbios(Data(one.begin(),one.begin()+n));assert(!partial.cpu_vendor&&!partial.cpu_model);} // string sets cannot leak partial names
#ifdef __linux__
    const auto root=fs::temp_directory_path()/("pcinsight-identity-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));assert(fs::create_directory(root));
    struct Cleanup{fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}} cleanup{root};const auto path=root/"cpuinfo";
    fill_linux_cpu_names(cpu,fw,path);assert(cpu.model=="Firmware Model");
    write(path,x86);fill_linux_cpu_names(cpu,fw,path);assert(cpu.model=="Fixture CPU 9");
    write(path,"processor:0\nmodel name:A\n\nprocessor:1\nmodel name:B\n");fill_linux_cpu_names(cpu,fw,path);assert(cpu.model.empty());
    write(path,std::string(16*1024*1024+1,'x'));assert(!read_linux_cpu_names(path).value);
    const auto before=read_linux_cpu_names();assert(before.value);const auto native_fw=read_smbios();
    SampleOptions options;options.include_processes=false;options.include_app_history=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto sample=monitor.sample(options);const auto after=read_linux_cpu_names();assert(after.value);
    if(before.value->cpu_ids==after.value->cpu_ids&&before.value->vendor.value==after.value->vendor.value&&before.value->model.value==after.value->model.value){
        apply_linux_cpu_names(cpu,before,native_fw);assert(sample.cpu.reported_vendor.value==cpu.reported_vendor.value&&sample.cpu.reported_model.value==cpu.reported_model.value);
    }
    assert(sample.cpu.vendor==sample.cpu.reported_vendor.value.value_or("")&&sample.cpu.model==sample.cpu.reported_model.value.value_or(""));
    std::cout<<"Native Linux CPU: vendor="<<sample.cpu.vendor<<", model="<<sample.cpu.model<<"; "<<before.value->cpu_ids.size()<<" visible records\n";
    std::cout<<"Linux file fixtures: absent proc firmware fallback, preferred kernel names, heterogeneous reset and input bounds passed\n";
#else
    assert(!read_linux_cpu_names().value);
#endif
    std::cout<<"CPU identity: proc schemas/coverage/heterogeneity, no board-name guessing, UTF-8, SMBIOS indexed strings/common populated CPU names, truncations and availability passed\n";
}
