#include "linux_startup_birth.hpp"
#include <bit>
namespace pcinsight::detail {
Metric<std::vector<NetworkProbeInstruction>> make_startup_birth_exec_probe(int control,int records,int allowlist,const LinuxStartupResourceBtfLayout& l,const LinuxBirthExecBtfLayout& e,std::uint32_t capacity){
    auto absent=[](const char* n){return Metric<std::vector<NetworkProbeInstruction>>{{},Support::unsupported,n};};
    if(std::endian::native!=std::endian::little||control<0||records<0||allowlist<0||control==records||control==allowlist||records==allowlist||!capacity||capacity>64)return absent("Invalid exec maps/capacity");
    for(auto n:{l.leader,l.start,e.task_mm,e.exe_file,e.file_inode,e.inode_number,e.inode_sb,e.sb_device})if(n>1048576)return absent("Exec offset exceeds bound");
    std::vector<NetworkProbeInstruction> c;std::vector<std::size_t> exits,errors,global;
    auto op=[&](unsigned code,unsigned d,unsigned s,int off,int imm){c.push_back({static_cast<std::uint8_t>(code),static_cast<std::uint8_t>(d|(s<<4)),static_cast<std::int16_t>(off),imm});};
    auto jump=[&](auto& list,unsigned code,unsigned d,int imm){list.push_back(c.size());op(code,d,0,0,imm);};
    auto compare=[&](auto& list,unsigned code,unsigned d,unsigned s){list.push_back(c.size());op(code,d,s,0,0);};
    auto lookup=[&](int fd){op(0xbf,2,10,0,0);op(0x07,2,0,0,-4);op(0x18,1,1,0,fd);op(0,0,0,0,0);op(0x85,0,0,0,1);};
    auto read=[&](unsigned base,unsigned offset,unsigned bytes){op(0x7a,10,0,-16,0);op(0xbf,3,base,0,0);op(0x07,3,0,0,static_cast<int>(offset));op(0xbf,1,10,0,0);op(0x07,1,0,0,-16);op(0xb7,2,0,0,static_cast<int>(bytes));op(0x85,0,0,0,113);jump(errors,0x55,0,0);op(bytes==8?0x79:0x61,0,10,-16,0);};
    // Unselected TGIDs return after ONE hash lookup, before task/file reads.
    op(0x85,0,0,0,14);op(0x77,0,0,0,32);op(0xbf,6,0,0,0);op(0x63,10,0,-4,0);lookup(allowlist);jump(exits,0x15,0,0);
    op(0x79,0,0,0,0);jump(global,0x35,0,static_cast<int>(capacity));op(0x63,10,0,-4,0);lookup(records);jump(global,0x15,0,0);op(0xbf,8,0,0,0);
    op(0xb7,9,0,0,0);op(0xdb,8,9,0,0x41);jump(exits,0x55,9,1);op(0x79,9,8,8,0);compare(exits,0x5d,9,6);
    op(0x85,0,0,0,35);op(0xbf,6,0,0,0);read(6,l.leader,8);op(0xbf,6,0,0,0);read(6,l.start,8);op(0x79,9,8,24,0);compare(exits,0x5d,0,9); // PID reuse is not enrollment.
    // Claim first event only. Re-exec NEVER rewrites the historical first image.
    op(0xbf,7,8,0,0);op(0x07,7,0,0,120);op(0xb7,0,0,0,0);op(0xb7,9,0,0,2);op(0xdb,7,9,0,0xf1);jump(exits,0x55,0,0);
    read(6,e.task_mm,8);jump(errors,0x15,0,0);op(0xbf,7,0,0,0);
    read(7,e.exe_file,8);jump(errors,0x15,0,0);op(0xbf,7,0,0,0);
    read(7,e.file_inode,8);jump(errors,0x15,0,0);op(0xbf,7,0,0,0);
    read(7,e.inode_number,8);op(0x7b,8,0,136,0);read(7,e.inode_sb,8);jump(errors,0x15,0,0);op(0xbf,7,0,0,0);
    read(7,e.sb_device,4);op(0x7b,8,0,144,0);
    op(0x85,0,0,0,15);op(0xbc,0,0,0,0);op(0x7b,8,0,152,0);
    op(0x85,0,0,0,5);op(0x7b,8,0,128,0);op(0xbf,7,8,0,0);op(0x07,7,0,0,120);op(0xb7,9,0,0,1);op(0xdb,7,9,0,0xe1);
    const auto done=c.size();op(0xb7,0,0,0,0);op(0x95,0,0,0,0);
    const auto failure=c.size();for(auto n:errors)c[n].offset=static_cast<std::int16_t>(failure-n-1);
    op(0x79,9,8,120,0);const auto already_done=c.size();op(0x15,9,0,0,1);const auto already_failed=c.size();op(0x15,9,0,0,3);
    op(0xbf,7,8,0,0);op(0x07,7,0,0,120);op(0xb7,9,0,0,3);op(0xdb,7,9,0,0xe1);
    const auto flag=c.size();for(auto n:global)c[n].offset=static_cast<std::int16_t>(flag-n-1);
    c[already_done].offset=static_cast<std::int16_t>(flag-already_done-1);c[already_failed].offset=static_cast<std::int16_t>(flag-already_failed-1);
    op(0x62,10,0,-4,1);lookup(control);const auto missing=c.size();op(0x15,0,0,0,0);op(0xb7,9,0,0,64);op(0xdb,0,9,0,0x40);
    const auto end=c.size();op(0xb7,0,0,0,0);op(0x95,0,0,0,0);c[missing].offset=static_cast<std::int16_t>(end-missing-1);
    for(auto n:exits)c[n].offset=static_cast<std::int16_t>(done-n-1);
    return {std::move(c),Support::available,"Allowlisted first successful exec filesystem metadata; no current-image/app/automatic-startup claim"};
}
}
