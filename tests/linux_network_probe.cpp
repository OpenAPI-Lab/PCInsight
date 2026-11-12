#include "../src/linux_network_probe.hpp"
#include <array>
#include <bit>
#include <cassert>
#include <limits>
using namespace pcinsight;using namespace pcinsight::detail;
// Tiny test interpreter for ONLY this generator's instruction subset. It is
// not a kernel verifier substitute. Access counters prove early filtering here.
struct Execution {std::array<std::uint64_t,2> values{};unsigned contexts{},maps{},writes{};};
Execution run(const std::vector<NetworkProbeInstruction>& code,std::uint32_t actor,std::int32_t result,bool receive,bool null_map=false){
    Execution out;std::array<std::uint64_t,11> r{};r[1]=1000;r[10]=2000;
    std::uint32_t key=99;unsigned steps=0;
    for(std::size_t pc=0;pc<code.size();++pc){
        assert(++steps<=32);const auto i=code[pc];const auto dst=i.registers&15,src=i.registers>>4;
        assert(dst<11&&src<11);
        switch(i.code){
        case 0xbf:r[dst]=r[src];break;
        case 0x85:
            if(i.immediate==14)r[0]=(std::uint64_t{actor}<<32)|42;
            else{assert(i.immediate==1&&r[1]==55&&r[2]==1996&&key<2);++out.maps;r[0]=null_map?0:3000+8*key;}
            break;
        case 0x77:r[dst]>>=i.immediate;break;
        case 0x55:if(r[dst]!=static_cast<std::uint64_t>(i.immediate))pc+=i.offset;break;
        case 0x79:assert(src==6&&r[src]==1000&&i.offset==(receive?24:48));++out.contexts;r[dst]=static_cast<std::uint32_t>(result);break;
        case 0x67:r[dst]<<=i.immediate;break;
        case 0xc7:r[dst]=static_cast<std::uint64_t>(std::bit_cast<std::int64_t>(r[dst])>>i.immediate);break;
        case 0xd5:if(std::bit_cast<std::int64_t>(r[dst])<=i.immediate)pc+=i.offset;break;
        case 0x62:assert(dst==10&&r[dst]==2000&&i.offset==-4);key=static_cast<std::uint32_t>(i.immediate);break;
        case 0x07:r[dst]+=static_cast<std::uint64_t>(static_cast<std::int64_t>(i.immediate));break;
        case 0x18:assert(dst==1&&src==1&&i.immediate==55&&code.at(pc+1)==NetworkProbeInstruction{});r[dst]=55;++pc;break;
        case 0x15:if(r[dst]==static_cast<std::uint64_t>(i.immediate))pc+=i.offset;break;
        case 0xdb:assert(dst==0&&src==7&&r[0]==3000+8*key&&i.immediate==0);++out.writes;out.values[key]+=r[src];break;
        case 0xb7:r[dst]=static_cast<std::uint64_t>(i.immediate);break;
        case 0x95:assert(r[0]==0);return out;
        default:assert(false);
        }
    }
    assert(false);return out;
}
int main(){
    assert(!make_private_network_probe(0,55,false).value);
    assert(!make_private_network_probe(10,-1,false).value);
    assert(!make_private_network_probe(std::numeric_limits<std::uint32_t>::max(),55,false).value);
    if(std::endian::native!=std::endian::little)return 0;
    for(bool receive:{false,true}){
        auto code=make_private_network_probe(123,55,receive);assert(metric_is_known(code)&&code.value->size()==18);
        auto out=run(*code.value,999,1024,receive);assert(!out.contexts&&!out.maps&&!out.writes);
        for(std::int32_t value:{0,-1,-4095,std::numeric_limits<std::int32_t>::min()}){
            out=run(*code.value,123,value,receive);assert(out.contexts==1&&!out.maps&&!out.writes);
        }
        out=run(*code.value,123,1024,receive);assert(out.writes==1&&out.values[receive?1:0]==1024);
        out=run(*code.value,123,std::numeric_limits<std::int32_t>::max(),receive);
        assert(out.values[receive?1:0]==2147483647);
        out=run(*code.value,123,1024,receive,true);assert(out.maps==1&&!out.writes);
    }
}
