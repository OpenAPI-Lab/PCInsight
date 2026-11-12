#include "../src/linux_network_probe.hpp"
#include <array>
#include <bit>
#include <cassert>
#include <limits>
using namespace pcinsight;using namespace pcinsight::detail;
struct Scenario {
    std::uint32_t actor{123},flags{};std::int32_t result{1024};
    unsigned family{2},protocol{6};int fail_read{-1};bool null_socket{},null_sk{},null_map{};
    std::uint32_t uid{1000};std::uint64_t start{0x1122334455667788ULL};
    bool null_task{},null_leader{},enforce_guard{};
    bool null_net{};std::uint64_t netns_cookie{0xaabbccddeeff0011ULL};
    std::array<std::uint64_t,private_network_discovery_entries> initial{};
    std::uint64_t race_cookie{}; // One peer claim just before our first cmpxchg.
};
struct Execution{std::array<std::uint64_t,private_network_discovery_entries> values{};unsigned contexts{},reads{},maps{};std::uint64_t result{};};
Execution run(const std::vector<NetworkProbeInstruction>& code,const LinuxNetworkBtfLayout& layout,const Scenario& s,bool identity_program=false){
    Execution out;std::array<std::uint64_t,11> r{};r[1]=1000;r[10]=2000;
    out.values=s.initial;
    std::uint64_t scratch=0,saved_sock=0,saved_cookie=0;std::array<std::uint32_t,2> keys{99,99};unsigned steps=0;bool raced=false;
    for(std::size_t pc=0;pc<code.size();++pc){
        assert(++steps<=4096);const auto i=code[pc];const auto d=i.registers&15,v=i.registers>>4;
        assert(d<11&&v<11);
        switch(i.code){
        case 0xbf:r[d]=r[v];break;
        case 0x85:
            if(i.immediate==14)r[0]=(std::uint64_t{s.actor}<<32)|42;
            else if(i.immediate==15)r[0]=(1234ULL<<32)|s.uid;
            else if(i.immediate==35)r[0]=s.null_task?0:7000;
            else if(i.immediate==113){
                assert(r[1]==1984);unsigned field;
                if(r[3]==5000+layout.socket_sk_offset){field=0;assert(r[2]==8);scratch=s.null_sk?0:6000;}
                else if(r[3]==6000+layout.sock_family_offset){field=1;assert(r[2]==2);scratch=s.family;}
                else if(r[3]==6000+layout.sock_protocol_offset){field=2;assert(r[2]==layout.sock_protocol_width);scratch=s.protocol;}
                else if(r[3]==7000+layout.task_group_leader_offset){field=3;assert(r[2]==8);scratch=s.null_leader?0:8000;}
                else if(layout.socket_namespace.value&&r[3]==6000+layout.socket_namespace.value->sock_net_pointer_offset){field=5;assert(r[2]==8);scratch=s.null_net?0:9000;}
                else if(layout.socket_namespace.value&&r[3]==9000+layout.socket_namespace.value->net_cookie_offset){field=6;assert(r[2]==8);scratch=s.netns_cookie;}
                else{field=4;assert(r[3]==8000+layout.task_start_boottime_offset&&r[2]==8);scratch=s.start;}
                ++out.reads;r[0]=s.fail_read==static_cast<int>(field)?static_cast<std::uint64_t>(-14LL):0;
            }else{assert(i.immediate==1&&r[1]==55&&(r[2]==1996||r[2]==1992));
                const auto key=keys[r[2]==1996?0:1];assert(key<out.values.size());++out.maps;r[0]=s.null_map?0:3000+8*key;}
            break;
        case 0x77:r[d]>>=i.immediate;break;
        case 0x55:if(r[d]!=static_cast<std::uint64_t>(i.immediate))pc+=i.offset;break;
        case 0x15:if(r[d]==static_cast<std::uint64_t>(i.immediate))pc+=i.offset;break;
        case 0x5d:if(r[d]!=r[v])pc+=i.offset;break;
        case 0x3d:if(r[d]>=r[v])pc+=i.offset;break;
        case 0x79:
            if(v==6){assert(r[v]==1000);++out.contexts;
                if(i.offset==24)r[d]=static_cast<std::uint32_t>(s.result);
                else if(i.offset==16)r[d]=s.flags;
                else{assert(i.offset==0);r[d]=s.null_socket?0:5000;}}
            else{assert(v==10&&(i.offset==-16||i.offset==-24||i.offset==-32));r[d]=i.offset==-24?saved_sock:i.offset==-32?saved_cookie:scratch;}break;
        case 0x61:assert(v==10&&i.offset==-4);r[d]=keys[0];break;
        case 0x69:assert(v==10&&i.offset==-16);r[d]=scratch&65535;break;
        case 0x71:assert(v==10&&i.offset==-16);r[d]=scratch&255;break;
        case 0x67:r[d]<<=i.immediate;break;
        case 0xc7:r[d]=static_cast<std::uint64_t>(std::bit_cast<std::int64_t>(r[d])>>i.immediate);break;
        case 0xd5:if(std::bit_cast<std::int64_t>(r[d])<=i.immediate)pc+=i.offset;break;
        case 0x57:r[d]&=i.immediate;break;
        case 0x7a:assert(d==10&&(i.offset==-16||i.offset==-32)&&i.immediate==0);if(i.offset==-32)saved_cookie=0;else scratch=0;break;
        case 0x63:assert(d==10&&i.offset==-4);keys[0]=static_cast<std::uint32_t>(r[v]);break;
        case 0x62:assert(d==10&&(i.offset==-4||i.offset==-8));keys[i.offset==-4?0:1]=static_cast<std::uint32_t>(i.immediate);break;
        case 0x07:r[d]+=static_cast<std::uint64_t>(static_cast<std::int64_t>(i.immediate));break;
        case 0x18:
            if(v==1){assert(d==1&&i.immediate==55&&code.at(pc+1)==NetworkProbeInstruction{});r[d]=55;}
            else{assert(v==0&&code.at(pc+1).code==0);r[d]=static_cast<std::uint32_t>(i.immediate)|(std::uint64_t{static_cast<std::uint32_t>(code.at(pc+1).immediate)}<<32);}
            ++pc;break;
        case 0x05:pc+=i.offset;break;
        case 0xdb:{assert(r[d]>=3000&&(r[d]-3000)%8==0);const auto key=(r[d]-3000)/8;assert(key<out.values.size());
            if(s.enforce_guard&&key!=7)assert(out.values[7]>0);
            if(i.immediate==0xf1){if(s.race_cookie&&!raced&&key==9){raced=true;out.values[key]=s.race_cookie;out.values[key+5]=1;++out.values[8];}
                const auto old=out.values[key];if(old==r[0])out.values[key]=r[v];r[0]=old;break;}
            const auto old=out.values[key];
            if(i.immediate==0x40||i.immediate==0x41){out.values[key]|=r[v];if(i.immediate==0x41)r[v]=old;}
            else{assert(i.immediate==0||i.immediate==1);out.values[key]+=r[v];if(i.immediate==1)r[v]=old;}
            break;}
        case 0x7b:{if(d==10){assert(i.offset==-24||i.offset==-32);if(i.offset==-24)saved_sock=r[v];else saved_cookie=r[v];break;}assert(d==0&&r[d]>=3000);const auto key=(r[d]-3000)/8;assert(key<3);out.values[key]=r[v];break;}
        case 0xbc:r[d]=static_cast<std::uint32_t>(r[v]);break;
        case 0x0f:r[d]+=r[v];break;
        case 0xb7:r[d]=static_cast<std::uint64_t>(i.immediate);break;
        case 0x95:out.result=r[0];if(!identity_program)assert(r[0]==0);return out;
        default:assert(false);
        }
    }
    assert(false);return out;
}
PrivateNetworkDiscoveryRead discovery_read(const Execution& out){
    PrivateNetworkDiscoveryRead raw;raw.values=out.values;raw.sequence_before=raw.sequence_after=out.values[8];return raw;
}
void test_discovery(const LinuxNetworkBtfLayout& layout,const PrivateNetworkIdentity& identity){
    auto no_namespace=layout;no_namespace.socket_namespace.value.reset();
    assert(!make_private_discovering_network_probe(123,55,false,no_namespace,identity).value);
    auto estimated=layout;estimated.socket_namespace.support=Support::estimated;
    assert(!make_private_discovering_network_probe(123,55,false,estimated,identity).value);
    for(bool receive:{false,true})for(unsigned family:{2u,10u})for(unsigned protocol:{6u,17u}){
        const auto code=make_private_discovering_network_probe(123,55,receive,layout,identity);assert(code.value&&code.value->size()<4096);
        Scenario s;s.family=family;s.protocol=protocol;s.enforce_guard=true;const auto lane=(protocol==17?2u:0u)+(receive?1u:0u);
        auto out=run(*code.value,layout,s);auto parsed=validate_private_network_discovery_read(discovery_read(out));
        assert(parsed.value&&parsed.value->size()==1&&parsed.value->at(0).socket_namespace_cookie==s.netns_cookie&&parsed.value->at(0).bytes[lane]==1024);
        s.initial=out.values;++s.netns_cookie;out=run(*code.value,layout,s);
        parsed=validate_private_network_discovery_read(discovery_read(out));assert(parsed.value&&parsed.value->size()==2);
        s.initial=out.values;--s.netns_cookie;out=run(*code.value,layout,s);
        parsed=validate_private_network_discovery_read(discovery_read(out));assert(parsed.value&&parsed.value->size()==2&&parsed.value->at(0).bytes[lane]==2048&&parsed.value->at(1).bytes[lane]==1024);
        for(bool same:{false,true}){s.initial={};s.race_cookie=s.netns_cookie+(same?0:1);out=run(*code.value,layout,s);
            parsed=validate_private_network_discovery_read(discovery_read(out));assert(parsed.value&&parsed.value->size()==(same?1u:2u));
            std::uint64_t sum=0;for(const auto& bucket:*parsed.value)sum+=bucket.bytes[lane];assert(sum==1024&&out.values[7]==0&&out.values[8]==2);}
        s.race_cookie=0;s.initial={};
        for(unsigned i=0;i<private_network_discovery_capacity;++i){s.netns_cookie=0x100000000ULL+i;out=run(*code.value,layout,s);s.initial=out.values;}
        parsed=validate_private_network_discovery_read(discovery_read(out));assert(parsed.value&&parsed.value->size()==private_network_discovery_capacity);
        // A known cookie in a full registry remains one bucket, without insertion.
        s.netns_cookie=0x100000000ULL;out=run(*code.value,layout,s);assert(out.values[4]==0);s.initial=out.values;
        s.netns_cookie=0xffffffffffffffffULL;out=run(*code.value,layout,s);assert(out.values[4]==1&&out.values[7]==0);
        assert(!validate_private_network_discovery_read(discovery_read(out)).value);
        s.initial=out.values;s.netns_cookie=0x100000000ULL;out=run(*code.value,layout,s);assert(out.values[4]==1); // Sticky, never eviction.
        for(unsigned fault=0;fault<7;++fault){s={};s.enforce_guard=true;
            if(fault==0)s.netns_cookie=0;if(fault==1)s.null_net=true;if(fault==2)s.fail_read=5;
            if(fault==3)s.fail_read=6;if(fault==4)++s.uid;if(fault==5)++s.start;if(fault==6)s.null_sk=true;
            out=run(*code.value,layout,s);assert(out.values[4]==1&&out.values[7]==0&&out.values[9]==0);}
        s={};s.actor=999;out=run(*code.value,layout,s);assert(!out.contexts&&!out.reads&&!out.maps);
        s={};s.result=0;out=run(*code.value,layout,s);assert(!out.maps);
        s={};s.family=1;out=run(*code.value,layout,s);assert(!out.maps);
        s={};s.protocol=99;out=run(*code.value,layout,s);assert(!out.maps);
        if(receive){s={};s.flags=2;out=run(*code.value,layout,s);parsed=validate_private_network_discovery_read(discovery_read(out));
            assert((parsed.value&&parsed.value->size()==1&&parsed.value->at(0).excluded_positive_receives==1&&parsed.value->at(0).bytes==std::array<std::uint64_t,4>{}));}
        s={};s.family=family;s.protocol=protocol;s.initial[9]=s.netns_cookie;s.initial[10+lane]=UINT64_MAX;s.initial[8]=1;
        out=run(*code.value,layout,s);assert(out.values[6]==1&&out.values[7]==0&&!validate_private_network_discovery_read(discovery_read(out)).value);
        s={};s.initial[8]=UINT64_MAX;out=run(*code.value,layout,s);assert(out.values[6]==1&&out.values[7]==0);
    }
    PrivateNetworkDiscoveryRead empty;assert(validate_private_network_discovery_read(empty).value->empty());
    PrivateNetworkDiscoveryRead valid;valid.values[9]=1;valid.values[10]=10;valid.values[15]=2;valid.values[16]=20;
    valid.sequence_before=valid.sequence_after=valid.values[8]=2;assert(validate_private_network_discovery_read(valid).value->size()==2);
    for(unsigned fault=0;fault<12;++fault){auto bad=valid;
        if(fault==0)bad.active_before=1;if(fault==1)bad.active_after=1;if(fault==2)++bad.sequence_after;
        if(fault==3)bad.values[7]=1;if(fault==4)++bad.values[8];if(fault==5)bad.values[4]=1;
        if(fault==6)bad.values[6]=1;if(fault==7)bad.values[0]=1;if(fault==8)bad.values[15]=1;
        if(fault==9)bad.values[9]=0;if(fault==10)bad.values[16]=0;if(fault==11)bad.values[8]=bad.sequence_before=bad.sequence_after=1;
        assert(!validate_private_network_discovery_read(bad).value);}
}
int main(){
    LinuxNetworkBtfLayout layout{};layout.socket_sk_offset=24;layout.sock_family_offset=16;layout.sock_protocol_offset=516;
    assert(!make_private_classified_network_probe(0,55,false,layout).value);
    for(unsigned width:{1u,2u})for(bool receive:{false,true}){
        layout.sock_protocol_width=width;
        auto code=make_private_classified_network_probe(123,55,receive,layout);
        if(std::endian::native!=std::endian::little){assert(!code.value);continue;}
        assert(metric_is_known(code)&&code.value->size()<128);
        for(unsigned family:{2u,10u})for(unsigned protocol:{6u,17u}){
            Scenario s;s.family=family;s.protocol=protocol;
            auto out=run(*code.value,layout,s);const unsigned lane=(protocol==17?2:0)+(receive?1:0);
            for(unsigned i=0;i<6;++i)assert(out.values[i]==(i==lane?1024u:0u));
            assert(out.reads==3&&out.maps==1);
        }
        Scenario s;s.actor=999;auto out=run(*code.value,layout,s);
        assert(!out.contexts&&!out.reads&&!out.maps);
        for(int value:{0,-1,-4095,std::numeric_limits<int>::min()}){
            s={};s.result=value;out=run(*code.value,layout,s);assert(!out.reads&&!out.maps);
        }
        for(int failure=0;failure<3;++failure){s={};s.fail_read=failure;out=run(*code.value,layout,s);assert(out.values[4]==1);}
        s={};s.null_socket=true;out=run(*code.value,layout,s);assert(!out.reads&&out.values[4]==1);
        s={};s.null_sk=true;out=run(*code.value,layout,s);assert(out.reads==1&&out.values[4]==1);
        s={};s.family=1;out=run(*code.value,layout,s);assert(out.reads==2&&!out.maps);
        s={};s.protocol=132;out=run(*code.value,layout,s);assert(out.reads==3&&!out.maps);
        s={};s.null_map=true;out=run(*code.value,layout,s);assert(out.maps==1&&out.values==Execution{}.values);
        if(receive)for(unsigned flags:{2u,32u,8192u,8194u}){
            s={};s.flags=flags;out=run(*code.value,layout,s);assert(!out.reads&&out.values[5]==1);
        }
        s={};s.flags=64;out=run(*code.value,layout,s);assert(out.values[receive?1:0]==1024);
    }
    layout.sock_protocol_width=4;assert(!make_private_classified_network_probe(1,0,false,layout).value);
    layout.sock_protocol_width=2;layout.socket_sk_offset=1048577;
    assert(!make_private_classified_network_probe(1,0,false,layout).value);
    layout.socket_sk_offset=24;layout.task_group_leader_offset=2480;layout.task_start_boottime_offset=2728;
    if(std::endian::native!=std::endian::little)return 0;
    const PrivateNetworkIdentity identity{123,1000,0x1122334455667788ULL};
    constexpr std::uint64_t cookie=0xaabbccddeeff0011ULL;
    assert(!make_private_classified_network_probe(123,55,false,layout,&identity,cookie).value);
    layout.socket_namespace={LinuxSocketNamespaceBtfLayout{64,128},Support::available,{}};
    assert(!make_private_classified_network_probe(123,55,false,layout,nullptr,cookie).value);
    for(auto support:{Support::estimated,Support::permission_required,Support::unsupported}){
        layout.socket_namespace.support=support;assert(!make_private_classified_network_probe(123,55,false,layout,&identity,cookie).value);}
    layout.socket_namespace.support=Support::available;
    test_discovery(layout,identity);
    assert(!make_private_classified_network_probe(123,55,false,layout,&identity,0,true).value);
    assert(!make_private_classified_network_probe(123,55,false,layout,nullptr,cookie,true).value);
    // Two explicit namespace buckets for ONE process. The same operation must
    // update exactly one bucket; an undeclared nonzero cookie updates neither.
    for(bool receive:{false,true})for(unsigned family:{2u,10u})for(unsigned protocol:{6u,17u}){
        const auto a=make_private_classified_network_probe(123,55,receive,layout,&identity,cookie,true);
        const auto b=make_private_classified_network_probe(123,55,receive,layout,&identity,cookie+1,true);
        assert(a.value&&b.value);
        Scenario s;s.family=family;s.protocol=protocol;s.enforce_guard=true;
        const auto lane=(protocol==17?2u:0u)+(receive?1u:0u);
        for(unsigned chosen=0;chosen<3;++chosen){s.netns_cookie=cookie+chosen;
            const auto ra=run(*a.value,layout,s),rb=run(*b.value,layout,s);
            for(unsigned i=0;i<4;++i){assert(ra.values[i]==((chosen==0&&i==lane)?1024u:0u));assert(rb.values[i]==((chosen==1&&i==lane)?1024u:0u));}
            assert(ra.values[4]==0&&rb.values[4]==0);
            assert(ra.values[8]==(chosen==0?1u:0u)&&rb.values[8]==(chosen==1?1u:0u));
        }
        for(unsigned fault=0;fault<7;++fault){s={};s.enforce_guard=true;
            if(fault==0)s.netns_cookie=0;if(fault==1)s.null_net=true;if(fault==2)s.fail_read=5;
            if(fault==3)s.fail_read=6;if(fault==4)++s.uid;if(fault==5)++s.start;if(fault==6)s.null_sk=true;
            const auto bad=run(*a.value,layout,s);assert(bad.values[4]==1&&bad.values[7]==0);
            for(unsigned i=0;i<4;++i)assert(bad.values[i]==0);
        }
        s={};s.actor=999;auto ignored=run(*a.value,layout,s);assert(!ignored.contexts&&!ignored.reads&&!ignored.maps);
        s={};s.flags=0x2022;s.netns_cookie=cookie+1;
        ignored=run(*a.value,layout,s);assert(!ignored.maps&&ignored.values[5]==0);
        if(receive){s.netns_cookie=cookie;auto excluded=run(*a.value,layout,s);assert(excluded.values[5]==1&&excluded.values[4]==0);}
        s={};s.family=family;s.protocol=protocol;s.enforce_guard=true;s.initial[lane]=UINT64_MAX;
        const auto overflow=run(*a.value,layout,s);assert(overflow.values[6]==1&&overflow.values[7]==0);
    }
    for(bool receive:{false,true})for(unsigned family:{2u,10u})for(unsigned protocol:{6u,17u}){
        auto generated=make_private_classified_network_probe(123,55,receive,layout,&identity,cookie);assert(generated.value);
        Scenario ns;ns.family=family;ns.protocol=protocol;ns.enforce_guard=true;
        auto result=run(*generated.value,layout,ns);assert(result.values[(protocol==17?2:0)+(receive?1:0)]==1024&&result.values[4]==0);
        for(int fault=0;fault<6;++fault){
            auto bad=ns;if(fault==0)bad.netns_cookie^=1;if(fault==1)bad.netns_cookie=0;
            if(fault==2)bad.null_net=true;if(fault==3)bad.fail_read=5;if(fault==4)bad.fail_read=6;
            if(fault==5){bad.netns_cookie^=1;bad.flags=0x2022;}
            result=run(*generated.value,layout,bad);assert(result.values[4]==1&&result.values[5]==0&&result.values[7]==0);
            for(unsigned i=0;i<4;++i)assert(result.values[i]==0);
        }
        if(receive){ns.flags=2;result=run(*generated.value,layout,ns);assert(result.values[5]==1&&result.values[4]==0);}
        ns.actor=999;result=run(*generated.value,layout,ns);assert(result.reads==0&&result.maps==0);
    }
    for(bool cookie_offset:{false,true}){
        auto invalid=layout;if(cookie_offset)invalid.socket_namespace.value->net_cookie_offset=1048577;
        else invalid.socket_namespace.value->sock_net_pointer_offset=1048577;
        assert(!make_private_classified_network_probe(123,55,false,invalid,&identity,cookie).value);
    }
    auto discovery=make_private_network_identity_probe(55,layout);assert(discovery.value);
    Scenario s;auto out=run(*discovery.value,layout,s,true);
    assert(out.result==0&&out.values[0]==123&&out.values[1]==1000&&out.values[2]==identity.leader_start_boottime_ns);
    s.fail_read=3;out=run(*discovery.value,layout,s,true);assert(out.result==1&&out.maps==0);
    s={};s.null_map=true;out=run(*discovery.value,layout,s,true);assert(out.result==1);
    s={};s.null_leader=true;out=run(*discovery.value,layout,s,true);assert(out.result==1);
    for(bool receive:{false,true}){
        auto generated=make_private_classified_network_probe(123,55,receive,layout,&identity);
        assert(generated.value&&generated.value->size()<256);
        s={};s.enforce_guard=true;out=run(*generated.value,layout,s);
        assert(out.values[receive?1:0]==1024&&out.values[6]==0&&out.values[7]==0&&out.values[8]==1);
        assert(out.reads==5);
        // A different generation or UID must not even read network context.
        for(int bad=0;bad<5;++bad){
            s={};s.enforce_guard=true;
            if(bad==0)++s.start;if(bad==1)++s.uid;if(bad==2)s.fail_read=3;
            if(bad==3)s.null_task=true;if(bad==4)s.null_leader=true;
            out=run(*generated.value,layout,s);assert(!out.contexts&&out.values[4]==1&&out.values[7]==0&&out.values[8]==1);
        }
        s={};s.actor=999;out=run(*generated.value,layout,s);assert(!out.reads&&!out.contexts&&!out.maps);
        // Overflow detection is sticky: wrapped raw values NEVER pass validator.
        s={};s.enforce_guard=true;s.initial[receive?1:0]=std::numeric_limits<std::uint64_t>::max()-500;
        out=run(*generated.value,layout,s);assert(out.values[6]==1&&out.values[7]==0&&out.values[8]==1);
        PrivateNetworkQualityRead read{};read.sequence_before=read.sequence_after=out.values[8];
        for(unsigned i=0;i<7;++i)read.values[i]=out.values[i];
        assert(!validate_private_network_quality_read(read).value);
        s.initial=out.values;out=run(*generated.value,layout,s);assert(out.values[6]==1);
        s={};s.enforce_guard=true;s.initial[8]=std::numeric_limits<std::uint64_t>::max();
        out=run(*generated.value,layout,s);assert(out.values[6]==1&&out.values[7]==0&&out.values[8]==0);
        auto large_uid=identity;large_uid.kernel_real_uid=std::numeric_limits<std::uint32_t>::max();
        auto large=make_private_classified_network_probe(123,55,receive,layout,&large_uid);assert(large.value);
        s={};s.uid=large_uid.kernel_real_uid;out=run(*large.value,layout,s);assert(out.values[receive?1:0]==1024);
    }
    PrivateNetworkQualityRead read{};auto valid=validate_private_network_quality_read(read);
    assert((valid.value&&valid.value->bytes==std::array<std::uint64_t,4>{}));
    for(int bad=0;bad<5;++bad){
        read={};if(bad==0)read.active_before=1;if(bad==1)read.active_after=1;
        if(bad==2)read.sequence_after=1;if(bad==3)read.values[6]=1;if(bad==4)read.values[4]=1;
        assert(!validate_private_network_quality_read(read).value);
    }
    read={};read.values[5]=8;read.values[0]=4096;read.sequence_before=read.sequence_after=20;
    valid=validate_private_network_quality_read(read);assert(valid.value&&valid.value->bytes[0]==4096&&valid.value->excluded_positive_receives==8);
    auto invalid_identity=identity;invalid_identity.leader_start_boottime_ns=0;
    assert(!make_private_classified_network_probe(123,55,false,layout,&invalid_identity).value);
}
