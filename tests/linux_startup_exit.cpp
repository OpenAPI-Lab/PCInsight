#include "../src/linux_startup_exit.hpp"
#include "../src/linux_startup_resources.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace pcinsight;using namespace pcinsight::detail;
#include "startup_resource_vm.hpp"
struct Header{std::uint16_t magic=0xeb9f;std::uint8_t version=1,flags=0;std::uint32_t length=24,type_offset=0,type_length=0,string_offset=0,string_length=0;};
struct Btf {
    std::string strings=std::string(1,'\0');std::vector<std::uint32_t> words;
    unsigned name(const char* n){const auto offset=static_cast<unsigned>(strings.size());strings+=n;strings+='\0';return offset;}
    void type(unsigned kind,const char* text,unsigned value,unsigned count=0,std::initializer_list<unsigned> body={}){words.push_back(name(text));words.push_back((kind<<24)|count);words.push_back(value);for(auto v:body)words.push_back(v);}
    std::vector<std::byte> blob(){Header h;h.type_length=static_cast<unsigned>(words.size()*4);h.string_offset=h.type_length;h.string_length=static_cast<unsigned>(strings.size());std::vector<std::byte> bytes(24+h.type_length+h.string_length);std::memcpy(bytes.data(),&h,24);std::memcpy(bytes.data()+24,words.data(),h.type_length);std::memcpy(bytes.data()+24+h.type_length,strings.data(),strings.size());return bytes;}
};
int main(){
    Btf b;b.type(1,"int",4,0,{(1U<<24)|32});b.type(4,"task_struct",16);b.type(2,"",2);
    b.type(13,"",0,2,{b.name("tsk"),3,b.name("group_dead"),1});b.type(12,"taskstats_exit",4,1);
    const auto bytes=b.blob();const auto hook=resolve_linux_taskstats_exit_hook(bytes);assert(hook.value==5&&hook.support==Support::available);
    Btf born;born.type(4,"task_struct",16);born.type(2,"",1);born.type(13,"",0,1,{born.name("p"),2});born.type(12,"wake_up_new_task",3,1);
    const auto birth_blob=born.blob();assert(resolve_linux_startup_birth_hook(birth_blob).value==4);
    for(std::size_t n=0;n<birth_blob.size();++n)assert(!resolve_linux_startup_birth_hook(std::span(birth_blob).first(n)).value);
    auto ambiguous_birth=born;ambiguous_birth.type(12,"wake_up_new_task",3,1);assert(!resolve_linux_startup_birth_hook(ambiguous_birth.blob()).value);
    auto bad_birth=born;bad_birth.words[8]=1;assert(!resolve_linux_startup_birth_hook(bad_birth.blob()).value); // non-void return
    Btf ns;ns.type(1,"int",4,0,{(1U<<24)|32});ns.type(1,"u32",4,0,{32}); //1,2
    ns.type(4,"ns_common",8,1,{ns.name("inum"),2,32}); //3
    ns.type(4,"pid_namespace",16,1,{ns.name("ns"),3,64});ns.type(2,"",4); //4,5
    ns.type(4,"upid",16,2,{ns.name("nr"),1,0,ns.name("ns"),5,64}); //6
    ns.type(3,"",0,0,{6,1,0}); //7
    ns.type(4,"pid",8,2,{ns.name("level"),2,0,ns.name("numbers"),7,64});ns.type(2,"",8); //8,9
    ns.type(4,"task_struct",16,1,{ns.name("thread_pid"),9,64}); //10
    const auto nsblob=ns.blob();const auto ns_layout=resolve_linux_birth_pid_btf(nsblob);
    assert(ns_layout.value&&ns_layout.value->thread_pid==8&&ns_layout.value->numbers==8&&ns_layout.value->upid_size==16&&ns_layout.value->ns_inum==12);
    for(std::size_t n=0;n<nsblob.size();++n)assert(!resolve_linux_birth_pid_btf(std::span(nsblob).first(n)).value);
    assert(!resolve_linux_birth_pid_btf(nsblob,4).value);
    for(const auto& name:{"thread_pid","level","numbers","nr","ns","inum","upid","pid_namespace","ns_common"}){auto broken=ns;const auto at=broken.strings.find(std::string(1,'\0')+name+'\0');assert(at!=std::string::npos);broken.strings[at+1]='!';assert(!resolve_linux_birth_pid_btf(broken.blob()).value);}
    auto duplicate_ns=ns;duplicate_ns.type(4,"pid_namespace",16);assert(!resolve_linux_birth_pid_btf(duplicate_ns.blob()).value);
    auto bad_array=ns;bad_array.words[37]=2;assert(!resolve_linux_birth_pid_btf(bad_array.blob()).value); // array length2, not trailing flex/one
    auto one=ns;one.words[37]=1;one.words[40]=24;assert(resolve_linux_birth_pid_btf(one.blob()).value);
    auto wrong_nr=ns;wrong_nr.words[27]=2;assert(!resolve_linux_birth_pid_btf(wrong_nr.blob()).value);
    auto wrong_pointer=ns;wrong_pointer.words[22]=3;assert(!resolve_linux_birth_pid_btf(wrong_pointer.blob()).value);
    Btf ex;ex.type(1,"u64",8,0,{64});ex.type(1,"u32",4,0,{32}); //1,2
    ex.type(4,"super_block",8,1,{ex.name("s_dev"),2,0});ex.type(2,"",3); //3,4
    ex.type(4,"inode",16,2,{ex.name("i_ino"),1,0,ex.name("i_sb"),4,64});ex.type(2,"",5); //5,6
    ex.type(4,"file",8,1,{ex.name("f_inode"),6,0});ex.type(2,"",7); //7,8
    ex.type(4,"mm_struct",8,1,{ex.name("exe_file"),8,0});ex.type(2,"",9); //9,10
    ex.type(4,"task_struct",8,1,{ex.name("mm"),10,0});
    const auto exblob=ex.blob();const auto exe=resolve_linux_birth_exec_btf(exblob);assert(exe.value&&exe.value->inode_sb==8);assert(!resolve_linux_birth_exec_btf(exblob,4).value);
    for(std::size_t n=0;n<exblob.size();++n)assert(!resolve_linux_birth_exec_btf(std::span(exblob).first(n)).value);
    for(const auto& name:{"mm","exe_file","f_inode","i_ino","i_sb","s_dev"}){auto bad=ex;const auto at=bad.strings.find(std::string(1,'\0')+name+'\0');assert(at!=std::string::npos);bad.strings[at+1]='!';assert(!resolve_linux_birth_exec_btf(bad.blob()).value);}
    auto bad_ex_sign=ex;bad_ex_sign.words[7]=(1U<<24)|32;assert(!resolve_linux_birth_exec_btf(bad_ex_sign.blob()).value);
    auto dup_ex=ex;dup_ex.type(4,"file",8);assert(!resolve_linux_birth_exec_btf(dup_ex.blob()).value);
    auto anon_ex=ex;anon_ex.words[38]=0;anon_ex.words[46]=12;anon_ex.type(4,"mm_struct",16,1,{0,9,64});const auto ae=resolve_linux_birth_exec_btf(anon_ex.blob());assert(ae.value&&ae.value->exe_file==8);
    auto cycle_ex=anon_ex;cycle_ex.words[cycle_ex.words.size()-2]=12;cycle_ex.words.back()=0;assert(!resolve_linux_birth_exec_btf(cycle_ex.blob()).value);
    auto signed_inode=ns;signed_inode.words[7]=(1U<<24)|32;assert(!resolve_linux_birth_pid_btf(signed_inode.blob()).value);
    for(std::size_t n=0;n<bytes.size();++n)assert(!resolve_linux_taskstats_exit_hook(std::span(bytes).first(n)).value);
    for(unsigned mode=0;mode<5;++mode){auto changed=b;
        if(mode==0)changed.words[12]=1; // Return int, not void.
        if(mode==1)changed.words[14]=1; // First arg int, not task*.
        if(mode==2)changed.words[16]=3; // Second arg pointer, not int.
        if(mode==3)changed.words[11]=(13U<<24)|1; // Wrong arg count breaks BTF payload/shape.
        if(mode==4)changed.type(12,"taskstats_exit",4,1); // Ambiguous function.
        assert(!resolve_linux_taskstats_exit_hook(changed.blob()).value);}
    PrivateNetworkIdentity identity{77,0,123456};std::array<std::uint64_t,7> values{1,0,1,77,0,123456,99999};
    auto event=decode_startup_selected_exit(values,identity);assert(event.value&&event.value->has_value()&&(**event.value).monotonic_ns==99999);
    values[0]=0;assert(decode_startup_selected_exit(values,identity).value&&!decode_startup_selected_exit(values,identity).value->has_value());values[0]=1;
    for(unsigned index:{0U,1U,2U,3U,4U,5U,6U}){auto bad=values;if(index==0)bad[index]=2;else if(index==1)bad[index]=1;else if(index==6)bad[index]=0;else ++bad[index];assert(!decode_startup_selected_exit(bad,identity).value);}
    LinuxNetworkBtfLayout layout;layout.task_group_leader_offset=8;layout.task_start_boottime_offset=16;
    const auto code=make_startup_selected_exit_probe(3,layout,identity);assert(code.value&&code.value->size()<4096);
    const auto& c=*code.value;assert(c[1].code==0x85&&c[1].immediate==14&&c[3].code==0x55&&c[3].immediate==77); // TGID filter before ctx/map.
    for(std::size_t n=0;n<c.size();++n)if((c[n].code&7)==5&&c[n].code!=0x85&&c[n].code!=0x95)assert(n+1+static_cast<std::size_t>(c[n].offset)<c.size());
    assert(!make_startup_selected_exit_probe(-1,layout,identity).value);identity.leader_start_boottime_ns=0;assert(!make_startup_selected_exit_probe(3,layout,identity).value);
    LinuxStartupSelectedExitCapture capture;assert(capture.start(false,-1,0,0).support==Support::permission_required&&!capture.sample().value);capture.stop();
    // Independent release schema, with every nested member/width validated.
    Btf s;s.type(1,"int",4,0,{(1U<<24)|32}); // 1
    s.type(1,"u32",4,0,{32});s.type(1,"u64",8,0,{64}); // 2,3
    s.type(4,"kuid_t",4,1,{s.name("val"),2,0}); // 4
    s.type(4,"cred",8,1,{s.name("uid"),4,0});s.type(2,"",5); // 5,6
    s.type(4,"atomic_t",4,1,{s.name("counter"),1,0}); // 7
    s.type(4,"sched_entity",8,1,{s.name("sum_exec_runtime"),3,0}); // 8
    s.type(4,"task_io_accounting",16,2,{s.name("read_bytes"),3,0,s.name("write_bytes"),3,64}); // 9
    s.type(4,"signal_struct",32,4,{s.name("nr_threads"),1,0,s.name("live"),7,32,s.name("sum_sched_runtime"),3,64,s.name("ioac"),9,128}); //10
    s.type(2,"",10); //11
    s.type(4,"task_struct",64,7,{s.name("tgid"),1,0,s.name("group_leader"),13,64,s.name("start_boottime"),3,128,s.name("cred"),6,192,s.name("signal"),11,256,s.name("se"),8,320,s.name("ioac"),9,384}); //12
    s.type(2,"",12);s.type(13,"",0,1,{s.name("p"),13});s.type(12,"release_task",14,1); //13,14,15
    const auto resource_blob=s.blob();const auto schema=resolve_linux_startup_resource_btf(resource_blob);
    assert(schema.value&&schema.value->function_id==15&&schema.value->task_write==56&&schema.value->signal_read==16);
    for(std::size_t n=0;n<resource_blob.size();++n)assert(!resolve_linux_startup_resource_btf(std::span(resource_blob).first(n)).value);
    assert(!resolve_linux_startup_resource_btf(resource_blob,4).value);
    for(const auto& field:{"nr_threads","live","counter","sum_exec_runtime","sum_sched_runtime","ioac","read_bytes","write_bytes","tgid","cred","uid","val","group_leader","start_boottime","signal","release_task"}){
        auto broken=s;const auto at=broken.strings.find(std::string(1,'\0')+field+'\0');assert(at!=std::string::npos);broken.strings[at+1]='!';assert(!resolve_linux_startup_resource_btf(broken.blob()).value);
    }
    auto ambiguous=s;ambiguous.type(12,"release_task",14,1);assert(!resolve_linux_startup_resource_btf(ambiguous.blob()).value);
    auto wrong_width=s;wrong_width.words[11]=32;assert(!resolve_linux_startup_resource_btf(wrong_width.blob()).value);
    auto wrong_sign=s;wrong_sign.words[7]=(1U<<24)|32;assert(!resolve_linux_startup_resource_btf(wrong_sign.blob()).value);
    for(unsigned offset:{6U,4U}){auto wrong_proto=s;wrong_proto.words[wrong_proto.words.size()-offset]=1;assert(!resolve_linux_startup_resource_btf(wrong_proto.blob()).value);}
    identity={77,0,123456};std::array<std::uint64_t,9> resource_values{1,0,77,0,123456,99999,0,0,0};
    auto resources=decode_startup_resources(resource_values,identity);assert(resources.value&&resources.value->has_value()&&(**resources.value).scheduler_runtime_ns==0); // measured zero, not absence
    resource_values[0]=0;assert(decode_startup_resources(resource_values,identity).value&&!decode_startup_resources(resource_values,identity).value->has_value());resource_values[0]=1;
    for(unsigned n=0;n<6;++n){auto bad=resource_values;if(n==5)bad[n]=0;else ++bad[n];assert(!decode_startup_resources(bad,identity).value);}
    const auto resource_code=make_startup_resource_probe(3,*schema.value,identity);assert(resource_code.value&&resource_code.value->size()<4096);
    check_resource_vm(*schema.value,identity,*resource_code.value);
    for(std::size_t n=0;n<resource_code.value->size();++n){const auto& ins=(*resource_code.value)[n];
        assert(!(ins.code==0x85&&(ins.immediate==14||ins.immediate==15))); // REAPER current identity is forbidden.
        if((ins.code&7)==5&&ins.code!=0x85&&ins.code!=0x95)assert(n+1+static_cast<std::size_t>(ins.offset)<resource_code.value->size());
    }
    assert(!make_startup_resource_probe(-1,*schema.value,identity).value);
    auto bad_offset=*schema.value;bad_offset.live=1048577;assert(!make_startup_resource_probe(3,bad_offset,identity).value);
    LinuxStartupResourceCapture resource_capture;assert(resource_capture.start(false,-1,0,0).support==Support::permission_required&&!resource_capture.sample().value);
    std::cout<<"selected exit/resources: BTF schema/truncations/shape, decoder/UID0/zero/absence/identity, filtered bytecode VM/sums/overflow/failure/duplicate passed\n";
}
