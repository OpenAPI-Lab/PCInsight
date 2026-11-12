#include "linux_network_probe.hpp"
#include <bit>
#include <limits>
#include <set>
#include <utility>

namespace pcinsight::detail {
Metric<std::vector<NetworkProbeInstruction>> make_private_network_probe(std::uint32_t tgid,int map_fd,bool receive){
    if(std::endian::native!=std::endian::little||!tgid||tgid>static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())||map_fd<0)
        return {std::nullopt,Support::unsupported,"private probe requires little endian, explicit positive TGID and map FD"};
    // Linux eBPF UAPI opcodes. Six-argument __sys_sendto / three-argument
    // sock_recvmsg signatures MUST be verified against live BTF before load.
    std::vector<NetworkProbeInstruction> code;
    auto add=[&](unsigned op,unsigned dst,unsigned src,int offset,int imm){
        code.push_back({static_cast<std::uint8_t>(op),static_cast<std::uint8_t>(dst|(src<<4)),
            static_cast<std::int16_t>(offset),imm});
    };
    add(0xbf,6,1,0,0);                 // save fexit ctx
    add(0x85,0,0,0,14);                // get_current_pid_tgid
    add(0x77,0,0,0,32);                // TGID high32
    const auto pid_jump=code.size();
    add(0x55,0,0,0,static_cast<int>(tgid)); // mismatch -> exit BEFORE ctx/map access
    add(0x79,7,6,receive?24:48,0);     // fexit signed int return slot
    add(0x67,7,0,0,32);
    add(0xc7,7,0,0,32);                // sign-extend int32 return
    const auto result_jump=code.size();
    add(0xd5,7,0,0,0);                 // signed <=0 -> exit
    add(0x62,10,0,-4,receive?1:0);     // array key on initialized stack
    add(0xbf,2,10,0,0);
    add(0x07,2,0,0,-4);
    add(0x18,1,1,0,map_fd);            // ldimm64 pseudo map FD
    add(0,0,0,0,0);
    add(0x85,0,0,0,1);                 // map_lookup_elem
    const auto map_jump=code.size();
    add(0x15,0,0,0,0);                 // null -> exit
    add(0xdb,0,7,0,0);                 // atomic add return bytes to u64 value
    const auto finish=code.size();
    add(0xb7,0,0,0,0);add(0x95,0,0,0,0);
    for(const auto index:{pid_jump,result_jump,map_jump})
        code[index].offset=static_cast<std::int16_t>(finish-index-1);
    return {std::move(code),Support::available,"private TGID-filtered capability test only, not production accounting"};
}
static Metric<std::vector<NetworkProbeInstruction>> build_classified_network_probe(
    std::uint32_t tgid,int map_fd,bool receive,const LinuxNetworkBtfLayout& layout,const PrivateNetworkIdentity* expected,std::uint64_t netns_cookie,bool filter_other_namespaces,bool discover,int actors_map=-1,unsigned actors=0){
    const bool shared=actors_map>=0;
    if(shared&&(!discover||!actors||actors>64))return {{},Support::unsupported,"Shared registry requires1..64 selected actors"};
    if(discover&&(!expected||netns_cookie||filter_other_namespaces))return {std::nullopt,Support::unsupported,"Discovery requires fixed actor identity and no declared cookie/filter"};
    if(filter_other_namespaces&&(!expected||!netns_cookie))return {std::nullopt,Support::unsupported,"Bucket filtering requires explicit identity and nonzero socket cookie"};
    if(std::endian::native!=std::endian::little||!tgid||tgid>2147483647U||map_fd<0||
       (layout.sock_protocol_width!=1&&layout.sock_protocol_width!=2)||
       layout.socket_sk_offset>1048576||layout.sock_family_offset>1048576||layout.sock_protocol_offset>1048576||
       (expected&&(expected->kernel_tgid!=tgid||!expected->leader_start_boottime_ns||
        layout.task_group_leader_offset>1048576||layout.task_start_boottime_offset>1048576)))
        return {std::nullopt,Support::unsupported,"invalid private classification target/layout"};
    if((netns_cookie||discover)&&(!expected||!layout.socket_namespace.value||layout.socket_namespace.support!=Support::available||
        layout.socket_namespace.value->sock_net_pointer_offset>1048576||layout.socket_namespace.value->net_cookie_offset>1048576))
        return {std::nullopt,Support::unsupported,"Exact socket namespace layout and fixed identity required"};
    std::vector<NetworkProbeInstruction> code;
    auto add=[&](unsigned op,unsigned dst,unsigned src,int offset,int imm){
        code.push_back({static_cast<std::uint8_t>(op),static_cast<std::uint8_t>(dst|(src<<4)),
            static_cast<std::int16_t>(offset),imm});
    };
    std::vector<std::size_t> exits,failures,exclusions,table_failures;
    auto jump=[&](auto& list,unsigned op,unsigned reg,int imm){list.push_back(code.size());add(op,reg,0,0,imm);};
    auto constant=[&](unsigned reg,std::uint64_t value){
        add(0x18,reg,0,0,static_cast<std::int32_t>(value));add(0,0,0,0,static_cast<std::int32_t>(value>>32));
    };
    auto read=[&](unsigned bytes,std::uint32_t offset,auto& errors){
        add(0x7a,10,0,-16,0);
        add(0xbf,3,8,0,0);add(0x07,3,0,0,static_cast<int>(offset));
        add(0xbf,1,10,0,0);add(0x07,1,0,0,-16);add(0xb7,2,0,0,static_cast<int>(bytes));
        add(0x85,0,0,0,113);jump(errors,0x55,0,0);
    };
    // Direct key at -72 never aliases per-actor keys or spilled base/guard.
    auto direct_lookup=[&](int fd){add(0xbf,2,10,0,0);add(0x07,2,0,0,-72);add(0x18,1,1,0,fd);add(0,0,0,0,0);add(0x85,0,0,0,1);};
    auto global_lookup=[&](int key){add(0x62,10,0,-72,key);direct_lookup(map_fd);};
    add(0xbf,6,1,0,0);add(0x85,0,0,0,14);add(0x77,0,0,0,32);
    if(shared){
        add(0x63,10,0,-72,0);direct_lookup(actors_map);jump(exits,0x15,0,0); // Unselected: no context/counter access.
        add(0x79,9,0,0,0);jump(table_failures,0x15,9,0);jump(table_failures,0x25,9,static_cast<int>(actors));
        add(0x07,9,0,0,-1);add(0x27,9,0,0,private_network_shared_stride);add(0x07,9,0,0,11);
        add(0x7b,10,9,-40,0); // actor's ARRAY105 counter base
        global_lookup(0);jump(table_failures,0x15,0,0);add(0x79,9,0,0,0);jump(exits,0x15,9,0);jump(table_failures,0x55,9,1);
        for(const auto& pair:{std::pair{-2,-64},std::pair{-1,-56}}){
            add(0x79,9,10,-40,0);add(0x07,9,0,0,pair.first);add(0x63,10,9,-72,0);direct_lookup(map_fd);jump(table_failures,0x15,0,0);
            add(0x79,9,0,0,0);if(pair.second==-56)jump(table_failures,0x15,9,0);add(0x7b,10,9,pair.second,0);
        }
    }else jump(exits,0x55,0,static_cast<int>(tgid)); // before any selected-context read
    if(discover)add(0x7a,10,0,-32,0); // Initialize all paths for verifier, including early identity failures.
    if(expected){
        // Identity failure never reaches network context or byte counters;
        // count it as quality failure so it cannot masquerade as idle traffic.
        add(0x85,0,0,0,35);add(0xbf,8,0,0,0);jump(failures,0x15,8,0);
        read(8,layout.task_group_leader_offset,failures);add(0x79,8,10,-16,0);jump(failures,0x15,8,0);
        read(8,layout.task_start_boottime_offset,failures);add(0x79,8,10,-16,0);
        if(shared)add(0x79,9,10,-56,0);else constant(9,expected->leader_start_boottime_ns);failures.push_back(code.size());add(0x5d,8,9,0,0);
        add(0x85,0,0,0,15);add(0xbc,0,0,0,0); // helper UID in initial user namespace
        if(shared)add(0x79,9,10,-64,0);else constant(9,expected->kernel_real_uid);failures.push_back(code.size());add(0x5d,0,9,0,0);
    }
    add(0x79,7,6,24,0);add(0x67,7,0,0,32);add(0xc7,7,0,0,32);
    jump(exits,0xd5,7,0);
    if(receive&&!netns_cookie&&!discover){
        add(0x79,0,6,16,0);add(0x57,0,0,0,0x2022); // PEEK | TRUNC | ERRQUEUE
        jump(exclusions,0x55,0,0);
    }
    add(0x79,8,6,0,0); // socket* from selected context
    jump(failures,0x15,8,0);
    read(8,layout.socket_sk_offset,failures);add(0x79,8,10,-16,0);jump(failures,0x15,8,0);
    if(netns_cookie||discover){
        // Save sock* before following its own namespace, NOT the current task's.
        add(0x7b,10,8,-24,0);
        const auto& ns=*layout.socket_namespace.value;
        read(8,ns.sock_net_pointer_offset,failures);add(0x79,8,10,-16,0);jump(failures,0x15,8,0);
        read(8,ns.net_cookie_offset,failures);add(0x79,9,10,-16,0);
        if(filter_other_namespaces||discover)jump(failures,0x15,9,0);
        if(discover)add(0x7b,10,9,-32,0); // Observed cookie survives subsequent helpers.
        else{constant(8,netns_cookie);(filter_other_namespaces?exits:failures).push_back(code.size());add(0x5d,9,8,0,0);}
        add(0x79,8,10,-24,0);
        // Even excluded positive receives must pass the namespace check.
        if(receive){add(0x79,0,6,16,0);add(0x57,0,0,0,0x2022);jump(exclusions,0x55,0,0);}
    }
    read(2,layout.sock_family_offset,failures);add(0x69,9,10,-16,0);
    const auto family4=code.size();add(0x15,9,0,0,2);
    jump(exits,0x55,9,10); // Ignore non-IP families, not synthetic network bytes.
    code[family4].offset=static_cast<std::int16_t>(code.size()-family4-1);
    read(layout.sock_protocol_width,layout.sock_protocol_offset,failures);
    add(layout.sock_protocol_width==1?0x71:0x69,9,10,-16,0);
    const auto tcp=code.size();add(0x15,9,0,0,6);
    jump(exits,0x55,9,17);
    add(0xb7,9,0,0,receive?3:2); // UDP
    const auto udp_to_count=code.size();add(0x05,0,0,0,0);
    code[tcp].offset=static_cast<std::int16_t>(code.size()-tcp-1);
    add(0xb7,9,0,0,receive?1:0); // TCP
    const auto tcp_to_count=code.size();add(0x05,0,0,0,0);
    const auto failure=code.size();add(0xb7,7,0,0,1);add(0xb7,9,0,0,4);
    const auto failure_to_count=code.size();add(0x05,0,0,0,0);
    const auto exclusion=code.size();if(receive){add(0xb7,7,0,0,1);add(0xb7,9,0,0,5);}
    const auto count=code.size();
    for(const auto at:{udp_to_count,tcp_to_count,failure_to_count})code[at].offset=static_cast<std::int16_t>(count-at-1);
    add(0x63,10,9,-4,0);
    auto lookup=[&](int key_offset){
        if(shared){add(0x61,1,10,key_offset,0);add(0x79,2,10,-40,0);add(0x0f,1,2,0,0);add(0x63,10,1,key_offset,0);}
        add(0xbf,2,10,0,0);add(0x07,2,0,0,key_offset);
        add(0x18,1,1,0,map_fd);add(0,0,0,0,0);add(0x85,0,0,0,1);
    };
    if(!expected){lookup(-4);jump(exits,0x15,0,0);add(0xdb,0,7,0,0);}
    else{
        // Writer guard encloses all value/flag/sequence writes. Keep its pointer
        // in r6, release it last. A failed lookup after entering leaves active>0
        // (fail closed); the ARRAY9 schema makes those lookups non-null normally.
        if(shared){global_lookup(7);jump(exits,0x15,0,0);add(0x7b,10,0,-48,0);add(0xb7,8,0,0,1);add(0xdb,0,8,0,1);}
        add(0x62,10,0,-8,7);lookup(-8);jump(exits,0x15,0,0);add(0xbf,6,0,0,0);
        add(0xb7,8,0,0,1);add(0xdb,6,8,0,1);
        auto checked_add=[&](unsigned amount,bool global=false){
            add(0xbf,8,amount,0,0); // save delta; fetch-add replaces amount with old value
            add(0xdb,0,amount,0,1);add(0xbf,9,amount,0,0);add(0x0f,9,8,0,0);
            const auto no_overflow=code.size();add(0x3d,9,amount,0,0); // unsigned sum >= old
            if(global)global_lookup(6);else{add(0x62,10,0,-8,6);lookup(-8);}jump(exits,0x15,0,0);
            add(0xb7,8,0,0,1);add(0xdb,0,8,0,0x41); // sticky fetch-OR overflow, NEVER cleared
            code[no_overflow].offset=static_cast<std::int16_t>(code.size()-no_overflow-1);
        };
        if(discover){
            // Identity/read failures may precede cookie initialization. Route them
            // directly to global failure slot4 without reading the cookie stack.
            add(0x61,9,10,-4,0);const auto known_failure=code.size();add(0x15,9,0,0,4);
            std::vector<std::size_t> routed;
            for(unsigned slot=0;slot<private_network_discovery_capacity;++slot){
                const auto base=9+slot*private_network_discovery_stride;
                add(0x62,10,0,-8,static_cast<int>(base));lookup(-8);jump(exits,0x15,0,0);
                add(0xbf,9,0,0,0);add(0x79,8,10,-32,0);add(0xb7,0,0,0,0);
                add(0xdb,9,8,0,0xf1); // atomic cmpxchg(map cookie, R0=0, observed cookie); old -> R0
                const auto claimed=code.size();add(0x15,0,0,0,0);
                const auto different=code.size();add(0x5d,0,8,0,0);
                code[claimed].offset=static_cast<std::int16_t>(code.size()-claimed-1);
                add(0x61,9,10,-4,0);const auto payload=code.size();add(0x55,9,0,0,5);
                add(0xb7,9,0,0,4); // Excluded receive is slot offset5, not a payload lane.
                code[payload].offset=static_cast<std::int16_t>(code.size()-payload-1);
                add(0x07,9,0,0,static_cast<int>(base+1));add(0x63,10,9,-4,0);
                routed.push_back(code.size());add(0x05,0,0,0,0);
                code[different].offset=static_cast<std::int16_t>(code.size()-different-1);
            }
            // Registry full: sticky global quality failure, never eviction/zero.
            add(0xb7,7,0,0,1);add(0x62,10,0,-4,4);
            const auto update=code.size();
            code[known_failure].offset=static_cast<std::int16_t>(update-known_failure-1);
            for(const auto at:routed)code[at].offset=static_cast<std::int16_t>(update-at-1);
        }
        lookup(-4);jump(exits,0x15,0,0);checked_add(7);
        add(0x62,10,0,-8,8);lookup(-8);jump(exits,0x15,0,0);
        add(0xb7,7,0,0,1);checked_add(7);
        add(0xb7,8,0,0,-1);add(0xdb,6,8,0,1); // sequence completed before releasing writer
        if(shared){global_lookup(8);jump(exits,0x15,0,0);add(0xb7,7,0,0,1);checked_add(7,true);
            add(0x79,6,10,-48,0);add(0xb7,8,0,0,-1);add(0xdb,6,8,0,1);}
    }
    std::optional<std::size_t> normal_exit;
    if(shared){normal_exit=code.size();add(0x05,0,0,0,0);const auto bad_table=code.size();
        global_lookup(7);jump(exits,0x15,0,0);add(0xbf,6,0,0,0);add(0xb7,8,0,0,1);add(0xdb,6,8,0,1);
        global_lookup(4);jump(exits,0x15,0,0);add(0xb7,8,0,0,1);add(0xdb,0,8,0,0x41);
        global_lookup(8);jump(exits,0x15,0,0);add(0xb7,8,0,0,1);add(0xdb,0,8,0,0);
        add(0xb7,8,0,0,-1);add(0xdb,6,8,0,1);
        for(auto at:table_failures)code[at].offset=static_cast<std::int16_t>(bad_table-at-1);}
    const auto finish=code.size();add(0xb7,0,0,0,0);add(0x95,0,0,0,0);
    if(normal_exit)code[*normal_exit].offset=static_cast<std::int16_t>(finish-*normal_exit-1);
    for(const auto at:exits)code[at].offset=static_cast<std::int16_t>(finish-at-1);
    for(const auto at:failures)code[at].offset=static_cast<std::int16_t>(failure-at-1);
    for(const auto at:exclusions)code[at].offset=static_cast<std::int16_t>(exclusion-at-1);
    return {std::move(code),Support::available,expected?"private identity-filtered counters with writer/sequence/overflow guards":"private classified socket-return test only; no production actor key"};
}
Metric<std::vector<NetworkProbeInstruction>> make_private_classified_network_probe(
    std::uint32_t tgid,int map_fd,bool receive,const LinuxNetworkBtfLayout& layout,const PrivateNetworkIdentity* expected,std::uint64_t cookie,bool filter){
    return build_classified_network_probe(tgid,map_fd,receive,layout,expected,cookie,filter,false);
}
Metric<std::vector<NetworkProbeInstruction>> make_private_discovering_network_probe(
    std::uint32_t tgid,int map_fd,bool receive,const LinuxNetworkBtfLayout& layout,const PrivateNetworkIdentity& expected){
    return build_classified_network_probe(tgid,map_fd,receive,layout,&expected,0,false,true);
}
Metric<std::vector<NetworkProbeInstruction>> make_private_shared_network_probe(int actors_map,int counters_map,unsigned actors,bool receive,const LinuxNetworkBtfLayout& layout){
    if(actors_map<0||counters_map<0||!actors||actors>64)return {{},Support::unsupported,"Shared map FDs and1..64 actors required"};
    const PrivateNetworkIdentity marker{1,0,1};
    return build_classified_network_probe(1,counters_map,receive,layout,&marker,0,false,true,actors_map,actors);
}
Metric<std::vector<NetworkProbeInstruction>> make_private_network_identity_probe(int map_fd,const LinuxNetworkBtfLayout& layout){
    if(std::endian::native!=std::endian::little||map_fd<0||layout.task_group_leader_offset>1048576||layout.task_start_boottime_offset>1048576)
        return {std::nullopt,Support::unsupported,"invalid private identity map/layout"};
    std::vector<NetworkProbeInstruction> code;std::vector<std::size_t> failures;
    auto add=[&](unsigned op,unsigned d,unsigned s,int off,int imm){code.push_back({static_cast<std::uint8_t>(op),static_cast<std::uint8_t>(d|(s<<4)),static_cast<std::int16_t>(off),imm});};
    auto fail=[&](unsigned op,unsigned reg){failures.push_back(code.size());add(op,reg,0,0,0);};
    add(0x85,0,0,0,14);add(0x77,0,0,0,32);add(0xbf,6,0,0,0);
    add(0x85,0,0,0,15);add(0xbc,7,0,0,0);
    add(0x85,0,0,0,35);add(0xbf,8,0,0,0);fail(0x15,8);
    for(const auto offset:{layout.task_group_leader_offset,layout.task_start_boottime_offset}){
        add(0x7a,10,0,-16,0);add(0xbf,3,8,0,0);add(0x07,3,0,0,static_cast<int>(offset));
        add(0xbf,1,10,0,0);add(0x07,1,0,0,-16);add(0xb7,2,0,0,8);add(0x85,0,0,0,113);fail(0x55,0);
        add(0x79,8,10,-16,0);fail(0x15,8);
    }
    add(0xbf,9,8,0,0);
    const std::array<unsigned,3> sources{6,7,9};
    for(unsigned key=0;key<3;++key){
        add(0x62,10,0,-4,static_cast<int>(key));add(0xbf,2,10,0,0);add(0x07,2,0,0,-4);
        add(0x18,1,1,0,map_fd);add(0,0,0,0,0);add(0x85,0,0,0,1);fail(0x15,0);
        add(0x7b,0,sources[key],0,0);
    }
    add(0xb7,0,0,0,0);add(0x95,0,0,0,0);
    const auto failure=code.size();add(0xb7,0,0,0,1);add(0x95,0,0,0,0);
    for(const auto at:failures)code[at].offset=static_cast<std::int16_t>(failure-at-1);
    return {std::move(code),Support::available,"unattached caller-only identity test-run, not target PID translation"};
}
Metric<PrivateNetworkQualityCounters> validate_private_network_quality_read(const PrivateNetworkQualityRead& r){
    auto fail=[](const char* note){return Metric<PrivateNetworkQualityCounters>{std::nullopt,Support::unsupported,note};};
    if(r.active_before||r.active_after||r.sequence_before!=r.sequence_after)return fail("writer overlap or changed update sequence");
    if(r.values[6])return fail("sticky overflow/quality flag; recreate capture instance, never clear in place");
    if(r.values[4])return fail("identity rejection or instrumented read failures; counters incomplete");
    PrivateNetworkQualityCounters out{{r.values[0],r.values[1],r.values[2],r.values[3]},r.values[5],r.sequence_after};
    return {out,Support::available,"coherent private instrumented scope only; excludes flagged receive operations"};
}
Metric<std::vector<PrivateDiscoveredNetworkBucket>> validate_private_network_discovery_read(const PrivateNetworkDiscoveryRead& r){
    auto fail=[](const char* note){return Metric<std::vector<PrivateDiscoveredNetworkBucket>>{{},Support::unsupported,note};};
    if(r.active_before||r.active_after||r.sequence_before!=r.sequence_after||r.values[7]||r.values[8]!=r.sequence_before)
        return fail("Discovery writer overlap or changed update sequence");
    if(r.values[4]||r.values[6])return fail("Discovery registry full, identity/read failure or overflow; recreate capture");
    if(r.values[0]||r.values[1]||r.values[2]||r.values[3]||r.values[5])return fail("Discovery map schema inconsistent");
    std::vector<PrivateDiscoveredNetworkBucket> out;std::set<std::uint64_t> seen;bool empty=false;
    for(unsigned slot=0;slot<private_network_discovery_capacity;++slot){const auto base=9+slot*private_network_discovery_stride;const auto cookie=r.values[base];
        if(!cookie){empty=true;for(unsigned i=1;i<private_network_discovery_stride;++i)if(r.values[base+i])return fail("Unclaimed discovery slot contains counters");continue;}
        if(empty||!seen.insert(cookie).second)return fail("Sparse or duplicate discovery cookie registry");
        bool activity=false;for(unsigned i=1;i<private_network_discovery_stride;++i)activity|=r.values[base+i]!=0;
        if(!activity)return fail("Claimed discovery bucket without completed activity");
        out.push_back({cookie,{r.values[base+1],r.values[base+2],r.values[base+3],r.values[base+4]},r.values[base+5]});
    }
    if(out.size()>r.sequence_after)return fail("Discovery registry has more buckets than completed updates");
    return {std::move(out),Support::available,"Observed cookies in selected instrumented operations; no socket inventory, rates or global totals"};
}
}
