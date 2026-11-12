#include "linux_startup_birth.hpp"
#include <bit>
namespace pcinsight::detail {
Metric<std::vector<NetworkProbeInstruction>> make_startup_birth_final_probe(int control,int records,const LinuxStartupResourceBtfLayout& l,PrivateNetworkIdentity root,std::uint32_t capacity){
    auto absent=[](const char* n){return Metric<std::vector<NetworkProbeInstruction>>{{},Support::unsupported,n};};
    if(std::endian::native!=std::endian::little||control<0||records<0||control==records||!root.kernel_tgid||root.kernel_tgid>2147483647U||!root.leader_start_boottime_ns||!capacity||capacity>64)return absent("Invalid child final creator/maps/capacity");
    for(auto n:{l.tgid,l.leader,l.start,l.cred,l.uid,l.signal,l.live,l.nr_threads,l.task_runtime,l.signal_runtime,l.task_read,l.signal_read,l.task_write,l.signal_write})if(n>1048576)return absent("Child final offset exceeds bound");
    std::vector<NetworkProbeInstruction> c;std::vector<std::size_t> exits,errors,duplicates,found;
    auto op=[&](unsigned code,unsigned d,unsigned s,int off,int imm){c.push_back({static_cast<std::uint8_t>(code),static_cast<std::uint8_t>(d|(s<<4)),static_cast<std::int16_t>(off),imm});};
    auto jump=[&](auto& list,unsigned code,unsigned d,int imm){list.push_back(c.size());op(code,d,0,0,imm);};
    auto compare=[&](auto& list,unsigned code,unsigned d,unsigned s){list.push_back(c.size());op(code,d,s,0,0);};
    auto constant=[&](unsigned d,std::uint64_t v){op(0x18,d,0,0,static_cast<std::int32_t>(v));op(0,0,0,0,static_cast<std::int32_t>(v>>32));};
    auto read=[&](unsigned base,unsigned offset,unsigned bytes){
        op(0x7a,10,0,-16,0);op(0xbf,3,base,0,0);op(0x07,3,0,0,static_cast<int>(offset));op(0xbf,1,10,0,0);op(0x07,1,0,0,-16);
        op(0xb7,2,0,0,static_cast<int>(bytes));op(0x85,0,0,0,113);jump(errors,0x55,0,0);op(bytes==8?0x79:0x61,0,10,-16,0);
    };
    auto lookup=[&](int fd,unsigned key){op(0x62,10,0,-4,static_cast<int>(key));op(0xbf,2,10,0,0);op(0x07,2,0,0,-4);op(0x18,1,1,0,fd);op(0,0,0,0,0);op(0x85,0,0,0,1);};
    // This deliberately covers ONLY release_task calls made by the selected
    // creator/reaper. Unrelated reapers exit before task/map reads. No global scan.
    op(0xbf,6,1,0,0);op(0x85,0,0,0,14);op(0x77,0,0,0,32);jump(exits,0x55,0,static_cast<int>(root.kernel_tgid));
    op(0x85,0,0,0,35);op(0xbf,7,0,0,0);read(7,l.leader,8);op(0xbf,7,0,0,0);read(7,l.start,8);constant(9,root.leader_start_boottime_ns);compare(exits,0x5d,0,9);
    op(0x85,0,0,0,15);op(0xbc,0,0,0,0);constant(9,root.kernel_real_uid);compare(exits,0x5d,0,9);
    op(0x79,6,6,0,0);read(6,l.leader,8);compare(exits,0x5d,0,6);read(6,l.tgid,4);op(0x7b,10,0,-24,0);read(6,l.start,8);op(0x7b,10,0,-32,0);
    // Statically bounded ARRAY lookup, only in selected creator's release calls.
    // Entries are never recycled: PID reuse must also match the birth start.
    for(unsigned n=0;n<capacity;++n){std::vector<std::size_t> next;
        lookup(records,n);jump(errors,0x15,0,0);op(0xbf,8,0,0,0);op(0xb7,9,0,0,0);op(0xdb,8,9,0,0x41);jump(next,0x55,9,1); // Atomic acquire of birth state (OR0|FETCH).
        op(0x79,9,8,8,0);op(0x79,0,10,-24,0);compare(next,0x5d,9,0);
        op(0x79,9,8,24,0);op(0x79,0,10,-32,0);compare(next,0x5d,9,0);found.push_back(c.size());op(0x05,0,0,0,0);
        for(auto p:next)c[p].offset=static_cast<std::int16_t>(c.size()-p-1);
    }
    const auto done=c.size();op(0xb7,0,0,0,0);op(0x95,0,0,0,0);
    const auto matched=c.size();for(auto n:found)c[n].offset=static_cast<std::int16_t>(matched-n-1);
    read(6,l.start,8);op(0x79,9,8,24,0);compare(exits,0x5d,0,9);
    read(6,l.cred,8);jump(errors,0x15,0,0);op(0xbf,9,0,0,0);read(9,l.uid,4);op(0x79,9,8,16,0);compare(errors,0x5d,0,9);
    read(6,l.signal,8);jump(errors,0x15,0,0);op(0xbf,7,0,0,0);
    read(7,l.live,4);jump(exits,0x55,0,0);read(7,l.nr_threads,4);jump(exits,0x55,0,1);
    op(0x79,9,8,64,0);jump(duplicates,0x55,9,0);
    const std::array<std::array<unsigned,3>,3> pairs{{{l.task_runtime,l.signal_runtime,80},{l.task_read,l.signal_read,88},{l.task_write,l.signal_write,96}}};
    for(const auto& p:pairs){read(6,p[0],8);op(0xbf,9,0,0,0);read(7,p[1],8);op(0x0f,9,0,0,0);compare(errors,0xad,9,0);op(0x7b,8,9,static_cast<int>(p[2]),0);}
    read(7,l.live,4);jump(errors,0x55,0,0);read(7,l.nr_threads,4);jump(errors,0x55,0,1);
    op(0x85,0,0,0,5);op(0x7b,8,0,72,0);op(0xbf,0,8,0,0);op(0x07,0,0,0,64);op(0xb7,9,0,0,1);op(0xdb,0,9,0,0xe1);
    op(0xb7,0,0,0,0);op(0x95,0,0,0,0);
    auto failure=[&](auto& list,int flag){const auto at=c.size();for(auto n:list)c[n].offset=static_cast<std::int16_t>(at-n-1);lookup(control,1);const auto missing=c.size();op(0x15,0,0,0,0);op(0xb7,9,0,0,flag);op(0xdb,0,9,0,0x40);const auto end=c.size();op(0xb7,0,0,0,0);op(0x95,0,0,0,0);c[missing].offset=static_cast<std::int16_t>(end-missing-1);};
    failure(errors,8);failure(duplicates,16);for(auto n:exits)c[n].offset=static_cast<std::int16_t>(done-n-1);
    return {std::move(c),Support::available,"Creator-reaped direct leader final accounting; no per-child userspace selection"};
}
}
