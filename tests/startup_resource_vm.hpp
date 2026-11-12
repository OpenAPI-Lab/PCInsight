#pragma once
#include <cstring>
#include <functional>
// Test-only interpreter for this bounded generator, NOT a kernel verifier.
struct ResourceVm {
    std::array<std::byte,65536> memory{};
    unsigned reads{},maps{},fault_read{};
    static constexpr std::uint64_t task=4096,signal=8192,cred=12288,map=32768;
    void put(std::uint64_t address,std::uint64_t value,unsigned bytes=8){assert(address+bytes<=memory.size());std::memcpy(memory.data()+address,&value,bytes);}
    std::uint64_t get(std::uint64_t address,unsigned bytes=8){assert(address+bytes<=memory.size());std::uint64_t v{};std::memcpy(&v,memory.data()+address,bytes);return v;}
    ResourceVm(const LinuxStartupResourceBtfLayout& l,PrivateNetworkIdentity k){
        put(1024,task);put(task+l.tgid,k.kernel_tgid,4);put(task+l.leader,task);put(task+l.start,k.leader_start_boottime_ns);
        put(task+l.cred,cred);put(cred+l.uid,k.kernel_real_uid,4);put(task+l.signal,signal);
        put(signal+l.nr_threads,1,4);put(signal+l.live,0,4);
        put(task+l.task_runtime,100);put(signal+l.signal_runtime,200);
        put(task+l.task_read,300);put(signal+l.signal_read,400);put(task+l.task_write,500);put(signal+l.signal_write,600);
    }
    std::array<std::uint64_t,9> run(const std::vector<NetworkProbeInstruction>& code){
        std::array<std::uint64_t,11> r{};r[1]=1024;r[10]=2048;unsigned steps=0;
        for(std::size_t pc=0;pc<code.size();++pc){assert(++steps<4096);const auto i=code[pc];const unsigned d=i.registers&15,s=i.registers>>4;assert(d<11&&s<11);
            const auto immediate=static_cast<std::uint64_t>(static_cast<std::int64_t>(i.immediate));
            switch(i.code){
            case 0xbf:r[d]=r[s];break;
            case 0xb7:r[d]=immediate;break;
            case 0x07:r[d]+=immediate;break;
            case 0x0f:r[d]+=r[s];break;
            case 0x18:r[d]=static_cast<std::uint32_t>(i.immediate)|(std::uint64_t{static_cast<std::uint32_t>(code.at(++pc).immediate)}<<32);break;
            case 0x79:case 0x61:r[d]=get(r[s]+i.offset,i.code==0x79?8:4);break;
            case 0x7b:put(r[d]+i.offset,r[s]);break;
            case 0x7a:case 0x62:put(r[d]+i.offset,immediate,i.code==0x7a?8:4);break;
            case 0x55:if(r[d]!=immediate)pc+=i.offset;break;
            case 0x15:if(r[d]==immediate)pc+=i.offset;break;
            case 0x5d:if(r[d]!=r[s])pc+=i.offset;break;
            case 0xad:if(r[d]<r[s])pc+=i.offset;break;
            case 0xdb:{const auto old=get(r[d]+i.offset);assert(i.immediate==0xe1||i.immediate==0x40);put(r[d]+i.offset,i.immediate==0xe1?r[s]:old|r[s]);if(i.immediate==0xe1)r[s]=old;break;}
            case 0x85:
                if(i.immediate==113){++reads;if(reads==fault_read)r[0]=static_cast<std::uint64_t>(-14);else{const auto v=get(r[3],static_cast<unsigned>(r[2]));put(r[1],v,static_cast<unsigned>(r[2]));r[0]=0;}}
                else if(i.immediate==1){++maps;assert(r[1]==3);const auto key=get(r[2],4);assert(key<9);r[0]=map+8*key;}
                else {assert(i.immediate==5);r[0]=99999;}
                break;
            case 0x95:{assert(r[0]==0);std::array<std::uint64_t,9> result{};for(unsigned n=0;n<9;++n)result[n]=get(map+8*n);return result;}
            default:assert(false);
            }
        }
        assert(false);return {};
    }
};
inline void check_resource_vm(const LinuxStartupResourceBtfLayout& l,PrivateNetworkIdentity k,const std::vector<NetworkProbeInstruction>& code){
    ResourceVm good(l,k);auto v=good.run(code);assert(v[0]==1&&!v[1]&&v[6]==300&&v[7]==700&&v[8]==1100);
    auto duplicate=good.run(code);assert(duplicate[1]==1&&!decode_startup_resources(duplicate,k).value);
    ResourceVm other(l,k);other.put(ResourceVm::task+l.tgid,k.kernel_tgid+1,4);v=other.run(code);assert(!v[0]&&!v[1]&&other.reads==1&&!other.maps);
    ResourceVm nonleader(l,k);nonleader.put(ResourceVm::task+l.leader,ResourceVm::task+256);v=nonleader.run(code);assert(!v[0]&&!v[1]&&nonleader.reads==2&&!nonleader.maps);
    for(unsigned offset:{l.live,l.nr_threads}){ResourceVm alive(l,k);alive.put(ResourceVm::signal+offset,2,4);v=alive.run(code);assert(!v[0]&&!v[1]&&!alive.maps);}
    for(const auto& fields:std::array<std::array<unsigned,2>,3>{{{l.task_runtime,l.signal_runtime},{l.task_read,l.signal_read},{l.task_write,l.signal_write}}}){
        ResourceVm overflow(l,k);overflow.put(ResourceVm::task+fields[0],UINT64_MAX);overflow.put(ResourceVm::signal+fields[1],1);v=overflow.run(code);assert(!v[0]&&v[1]==1&&!decode_startup_resources(v,k).value);
    }
    ResourceVm broken(l,k);broken.fault_read=9;v=broken.run(code);assert(!v[0]&&v[1]==1);
    ResourceVm ignored(l,k);ignored.fault_read=1;v=ignored.run(code);assert(!v[0]&&!v[1]&&!ignored.maps);
    ResourceVm zero(l,k);for(auto offset:{l.task_runtime,l.task_read,l.task_write})zero.put(ResourceVm::task+offset,0);for(auto offset:{l.signal_runtime,l.signal_read,l.signal_write})zero.put(ResourceVm::signal+offset,0);
    v=zero.run(code);assert(v[0]==1&&!v[6]&&!v[7]&&!v[8]&&decode_startup_resources(v,k).value->has_value());
}
