#include "../src/linux_network_btf.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>
using namespace pcinsight;using namespace pcinsight::detail;
struct Header{std::uint16_t magic{0xeb9f};std::uint8_t version{1},flags{};std::uint32_t length{24},type_offset{},type_length{},string_offset{},string_length{};};
template<class T> void put(std::vector<std::byte>& data,std::size_t at,T value){assert(at+sizeof(T)<=data.size());std::memcpy(data.data()+at,&value,sizeof(T));}
struct Builder {
    std::vector<std::byte> types;std::string strings=std::string(1,'\0');std::vector<std::size_t> positions{0};
    std::uint32_t name(std::string_view s){const auto off=static_cast<std::uint32_t>(strings.size());strings+=s;strings+='\0';return off;}
    void word(std::uint32_t value){const auto at=types.size();types.resize(at+4);put(types,at,value);}
    std::uint32_t type(unsigned kind,std::string_view text,std::uint32_t value,unsigned count=0,std::initializer_list<std::uint32_t> words={}){
        positions.push_back(types.size());word(text.empty()?0:name(text));word((kind<<24)|count);word(value);
        for(const auto w:words)word(w);return static_cast<std::uint32_t>(positions.size()-1);
    }
    std::vector<std::byte> blob()const{
        Header h;h.type_length=static_cast<std::uint32_t>(types.size());h.string_offset=h.type_length;h.string_length=static_cast<std::uint32_t>(strings.size());
        std::vector<std::byte> out(24+types.size()+strings.size());put(out,0,h);
        std::memcpy(out.data()+24,types.data(),types.size());std::memcpy(out.data()+24+types.size(),strings.data(),strings.size());return out;
    }
};
Builder fixture(bool inet_hooks=true,bool netns=false,bool pidfd=false){
    Builder b;
    assert(b.type(1,"int",4,0,{(1u<<24)|32})==1);
    assert(b.type(1,"unsigned short",2,0,{16})==2);
    assert(b.type(1,"unsigned long long",8,0,{64})==3);
    assert(b.type(1,"unsigned int",4,0,{32})==4);
    if(netns)assert(b.type(4,"sock_common",32,2,{b.name("skc_family"),2,16,b.name("skc_net"),29,64})==5);
    else assert(b.type(4,"sock_common",16,1,{b.name("skc_family"),2,16})==5);
    assert(b.type(4,"sock",netns?40:24,2,{b.name("__sk_common"),5,0,b.name("sk_protocol"),4,netns?256u:128u})==6);
    // Protocol in the real layout must be uint8/16, not uint32.
    put(b.types,b.positions[6]+12+12+4,std::uint32_t{2});
    assert(b.type(2,"",6)==7);
    assert(b.type(4,"socket",16,1,{b.name("sk"),7,64})==8);
    assert(b.type(2,"",8)==9);
    assert(b.type(4,"msghdr",32)==10);
    assert(b.type(2,"",10)==11);
    if(pidfd)assert(b.type(4,"task_struct",48,5,{b.name("group_leader"),13,64,b.name("start_boottime"),3,128,b.name("flags"),4,192,b.name("tgid"),1,256,b.name("cred"),33,320})==12);
    else assert(b.type(4,"task_struct",32,3,{b.name("group_leader"),13,64,b.name("start_boottime"),3,128,b.name("flags"),4,192})==12);
    assert(b.type(2,"",12)==13);
    assert(b.type(13,"",1,2,{b.name("sock"),9,b.name("msg"),11})==14);
    assert(b.type(13,"",1,3,{b.name("sock"),9,b.name("msg"),11,b.name("flags"),1})==15);
    assert(b.type(12,"sock_sendmsg",14,1)==16);
    assert(b.type(12,"sock_recvmsg",15,1)==17);
    assert(b.type(2,"",0)==18);
    assert(b.type(4,"sockaddr",16)==19);
    assert(b.type(2,"",19)==20);
    assert(b.type(13,"",1,6,{b.name("fd"),1,b.name("buffer"),18,b.name("len"),3,
        b.name("flags"),4,b.name("address"),20,b.name("address_len"),1})==21);
    assert(b.type(12,"__sys_sendto",21,1)==22);
    if(inet_hooks){
        assert(b.type(13,"",1,3,{b.name("sock"),9,b.name("msg"),11,b.name("size"),3})==23);
        assert(b.type(12,"inet_sendmsg",23,1)==24);
        assert(b.type(12,"inet6_sendmsg",23,1)==25);
    }
    if(netns){assert(inet_hooks);
        assert(b.type(4,"net",32,1,{b.name("net_cookie"),3,64})==26);
        assert(b.type(2,"",26)==27);
        assert(b.type(4,"",8,1,{b.name("net"),27,0})==28);
        assert(b.type(8,"possible_net_t",28)==29);
    }
    if(pidfd){assert(netns&&inet_hooks);
        assert(b.type(4,"file",8)==30);assert(b.type(2,"",30)==31);
        assert(b.type(4,"cred",16,1,{b.name("uid"),35,64})==32);assert(b.type(2,"",32)==33);
        assert(b.type(4,"",4,1,{b.name("val"),4,0})==34);assert(b.type(8,"kuid_t",34)==35);
        assert(b.type(13,"",1,2,{b.name("task"),13,b.name("mode"),4})==36);assert(b.type(12,"security_ptrace_access_check",36,1)==37);
    }
    return b;
}
int main(int argc,char** argv){
    if(argc==2&&std::string_view(argv[1])=="--native"){
        const auto r=read_linux_network_btf();
        if(!metric_is_known(r)){std::cout<<"Native BTF layout unavailable (not capture or positive layout validation): "<<r.note<<'\n';return 77;}
        const auto& l=*r.value;
        std::cout<<"Native BTF layout: send FUNC="<<l.send_function_id<<" sendto FUNC="<<l.sendto_function_id<<" recv FUNC="<<l.receive_function_id
            <<" socket.sk="<<l.socket_sk_offset<<" sock.family="<<l.sock_family_offset
            <<" sock.protocol="<<l.sock_protocol_offset<<"/"<<l.sock_protocol_width
            <<" task.group_leader="<<l.task_group_leader_offset<<" task.start_boottime="<<l.task_start_boottime_offset<<" task.flags="<<l.task_flags_offset
            <<" socket namespace layout="<<(l.socket_namespace.value?"validated":"absent")
            <<" pidfd identity="<<(l.pidfd_identity.value?"validated":l.pidfd_identity.note)
            <<". Layout only; NO BPF program/map/link loaded, NO capture/authorization tested\n";
        return 0;
    }
    auto b=fixture();assert(b.strings.front()=='\0');auto r=resolve_linux_network_btf(b.blob());
    auto pidfd=resolve_linux_network_btf(fixture(true,true,true).blob());assert(pidfd.value&&pidfd.value->pidfd_identity.value);
    assert(pidfd.value->pidfd_identity.value->function_id==37&&pidfd.value->pidfd_identity.value->task_tgid_offset==32&&
        pidfd.value->pidfd_identity.value->task_cred_offset==40&&pidfd.value->pidfd_identity.value->cred_uid_offset==8);
    for(unsigned mode=0;mode<8;++mode){auto n=fixture(true,true,true);
        if(mode==0)put(n.types,n.positions[36]+8,std::uint32_t{31}); // wrong return
        if(mode==1)put(n.types,n.positions[36]+12+8+4,std::uint32_t{1}); // signed mode
        if(mode==2)put(n.types,n.positions[12]+12+3*12+4,std::uint32_t{4}); // unsigned TGID
        if(mode==3)put(n.types,n.positions[33]+8,std::uint32_t{12}); // credential pointer target
        if(mode==4)put(n.types,n.positions[34]+12+4,std::uint32_t{1}); // signed kuid.val
        if(mode==5)n.type(12,"security_ptrace_access_check",36,1);
        if(mode==6)put(n.types,n.positions[34]+12+8,std::uint32_t{8}); // unaligned val
        if(mode==7)put(n.types,n.positions[37]+4,std::uint32_t{(12u<<24)|2}); // external linkage
        auto bad=resolve_linux_network_btf(n.blob());assert(bad.value&&!bad.value->pidfd_identity.value);
    }
    if(!metric_is_known(r))std::cerr<<r.note<<'\n';
    assert(metric_is_known(r));
    assert(r.value->send_function_id==16&&r.value->receive_function_id==17);
    assert(r.value->sendto_function_id==22);
    assert(r.value->inet_send_function_id==24&&r.value->inet6_send_function_id==25);
    const auto optional_missing=resolve_linux_network_btf(fixture(false).blob());
    assert(optional_missing.value&&optional_missing.value->inet_send_function_id==0&&optional_missing.value->inet6_send_function_id==0);
    assert(r.value->socket_sk_offset==8&&r.value->sock_family_offset==2&&r.value->sock_protocol_offset==16&&r.value->sock_protocol_width==2);
    assert(r.value->task_group_leader_offset==8&&r.value->task_start_boottime_offset==16&&r.value->task_flags_offset==24);
    assert(!r.value->socket_namespace.value); // optional, not a fabricated offset0
    auto ns=resolve_linux_network_btf(fixture(true,true).blob());assert(ns.value&&ns.value->socket_namespace.value);
    assert(ns.value->socket_namespace.value->sock_net_pointer_offset==8&&ns.value->socket_namespace.value->net_cookie_offset==8);
    for(unsigned mode=0;mode<10;++mode){auto n=fixture(true,true);
        if(mode==0)put(n.types,n.positions[27]+8,std::uint32_t{12}); // pointer to task, not net
        if(mode==1)put(n.types,n.positions[26]+12+4,std::uint32_t{4}); // uint32 cookie
        if(mode==2)put(n.types,n.positions[26]+12+8,std::uint32_t{17}); // unaligned cookie
        if(mode==3)n.type(4,"net",32); // ambiguous net struct
        if(mode==4)put(n.types,n.positions[28]+12+8,std::uint32_t{64}); // wrapper out of bounds
        if(mode==5)put(n.types,n.positions[29]+8,std::uint32_t{29}); // qualifier cycle
        if(mode==6)put(n.types,n.positions[5]+12+12+8,std::uint32_t{256}); // skc_net outside common
        if(mode==7)put(n.types,n.positions[26]+12+8,std::uint32_t{256}); // cookie outside net
        if(mode==8){put(n.types,n.positions[28]+8,std::uint32_t{64});put(n.types,n.positions[5]+12+12+8,std::uint32_t{64});} // wrapper exceeds containing struct
        if(mode==9)put(n.types,n.positions[26]+12,std::uint32_t{0}); // missing member
        ns=resolve_linux_network_btf(n.blob());assert(ns.value&&!ns.value->socket_namespace.value);
    }
    assert(!resolve_linux_network_btf(b.blob(),4).value);
    // All other standard kinds can coexist with the selected layout.
    b.type(3,"",0,0,{4,1,8});b.type(6,"E",4,1,{b.name("ENUM"),1});
    b.type(5,"U",4,1,{b.name("member"),4,0});b.type(7,"forward",0);
    const auto var=b.type(14,"var",4,0,{1});b.type(15,"section",4,1,{var,0,4});
    b.type(16,"float",4);b.type(17,"tag",4,0,{std::numeric_limits<std::uint32_t>::max()});
    b.type(18,"type_tag",4);b.type(19,"E64",8,1,{b.name("ENUM64"),0,1});
    assert(resolve_linux_network_btf(b.blob()).value);
    b=fixture();const auto qualifier=b.type(8,"family_alias",2);put(b.types,b.positions[5]+12+4,qualifier);
    assert(resolve_linux_network_btf(b.blob()).value);
    b=fixture();const auto qualifier_cycle=b.type(8,"cycle",0);put(b.types,b.positions[qualifier_cycle]+8,qualifier_cycle);put(b.types,b.positions[5]+12+4,qualifier_cycle);
    assert(!resolve_linux_network_btf(b.blob()).value);
    for(unsigned mode=0;mode<14;++mode){
        b=fixture();auto data=b.blob();Header h;std::memcpy(&h,data.data(),24);
        if(mode==0)h.magic=0x9feb;if(mode==1)h.version=2;if(mode==2)h.flags=1;
        if(mode==3)h.length=23;if(mode==4)h.length=static_cast<std::uint32_t>(data.size()+1);
        if(mode==5)h.type_length=std::numeric_limits<std::uint32_t>::max();
        if(mode==6)h.string_offset=std::numeric_limits<std::uint32_t>::max();
        if(mode==7)h.string_offset=0;
        put(data,0,h);
        if(mode==8)put(data,24+b.positions[1]+4,std::uint32_t{20u<<24});
        if(mode==9)put(data,24+b.positions[1],std::numeric_limits<std::uint32_t>::max());
        if(mode==10)put(data,24+b.positions[7]+8,std::uint32_t{9999});
        if(mode==11)data.resize(24);
        if(mode==12)data[24+b.types.size()]=std::byte{'x'};
        if(mode==13)data.back()=std::byte{'x'};
        assert(!resolve_linux_network_btf(data).value);
    }
    for(unsigned mode=0;mode<16;++mode){
        b=fixture();
        if(mode==0)put(b.types,b.positions[14]+8,std::uint32_t{3}); // 64-bit return, not expected int.
        if(mode==1)put(b.types,b.positions[7]+8,std::uint32_t{10}); // wrong pointer pointee.
        if(mode==2)put(b.types,b.positions[5]+12+8,std::uint32_t{17}); // non-byte aligned member.
        if(mode==3)put(b.types,b.positions[12]+12+12+8,std::uint32_t{512}); // member outside struct.
        if(mode==4)put(b.types,b.positions[3]+12,std::uint32_t{(1u<<24)|64}); // signed timestamp.
        if(mode==5)b.type(12,"sock_sendmsg",14,1); // ambiguous exact hook name.
        if(mode==6){const auto p=b.type(13,"",1);put(b.types,b.positions[16]+8,p);}
        if(mode==7)put(b.types,b.positions[16]+4,std::uint32_t{(12u<<24)|2}); // extern only.
        if(mode==8){put(b.types,b.positions[5]+4,std::uint32_t{(4u<<24)|0x80000000u|1});put(b.types,b.positions[5]+12+8,std::uint32_t{(2u<<24)|16});}
        if(mode==9)put(b.types,b.positions[21]+4,std::uint32_t{(13u<<24)|5});
        if(mode==10)put(b.types,b.positions[21]+12+2*8+4,std::uint32_t{4}); // size_t not uint64
        if(mode==11)put(b.types,b.positions[21]+12+1*8+4,std::uint32_t{20}); // buffer not void*
        if(mode==12)b.type(12,"__sys_sendto",21,1);
        if(mode==13)put(b.types,b.positions[23]+12+2*8+4,std::uint32_t{4});
        if(mode==14)b.type(12,"inet6_sendmsg",23,1);
        if(mode==15)put(b.types,b.positions[24]+4,std::uint32_t{(12u<<24)|2});
        assert(!resolve_linux_network_btf(b.blob()).value);
    }
    assert(!resolve_linux_network_btf({}).value);
#ifndef __linux__
    assert(!read_linux_network_btf().value);
#endif
}
