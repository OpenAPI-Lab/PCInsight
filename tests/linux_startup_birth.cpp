#include "../src/linux_startup_birth.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
using namespace pcinsight;using namespace pcinsight::detail;
// Minimal VM for this generator only, not a replacement for kernel verification.
struct Vm {
    std::array<std::byte,65536> m{};unsigned reads{},maps{},fail_read{};std::uint32_t actor=77;std::uint64_t uid=0;
    std::uint64_t task=root;bool fail_update{};std::map<std::uint32_t,std::uint64_t> allowlist;
    static constexpr unsigned root=4096,child=8192,sig=12288,cred=16384,control=32768,records=40000;
    void put(std::uint64_t a,std::uint64_t v,unsigned n=8){assert(a+n<=m.size());std::memcpy(m.data()+a,&v,n);}
    std::uint64_t get(std::uint64_t a,unsigned n=8){assert(a+n<=m.size());std::uint64_t v{};std::memcpy(&v,m.data()+a,n);return v;}
    Vm(const LinuxStartupResourceBtfLayout& l){put(1024,child);put(root+l.leader,root);put(root+l.start,123456);put(child+l.leader,child);put(child+l.start,789000);put(child+l.tgid,88,4);put(child+l.cred,cred);put(cred+l.uid,0,4);put(child+l.signal,sig);put(sig+l.live,1,4);put(sig+l.nr_threads,1,4);}
    void run(const std::vector<NetworkProbeInstruction>& c){std::array<std::uint64_t,11> r{};r[1]=1024;r[10]=2048;unsigned steps=0;
        for(std::size_t pc=0;pc<c.size();++pc){assert(++steps<2048);auto i=c[pc];const unsigned d=i.registers&15,s=i.registers>>4;assert(d<11&&s<11);const auto imm=static_cast<std::uint64_t>(static_cast<std::int64_t>(i.immediate));
            switch(i.code){case 0xbf:r[d]=r[s];break;case 0xbc:r[d]=static_cast<std::uint32_t>(r[s]);break;case 0xb7:r[d]=imm;break;case 0x07:r[d]+=imm;break;case 0x0f:r[d]+=r[s];break;case 0x77:r[d]>>=i.immediate;break;
            case 0x18:r[d]=static_cast<std::uint32_t>(i.immediate)|(std::uint64_t{static_cast<std::uint32_t>(c.at(++pc).immediate)}<<32);break;
            case 0x79:case 0x61:r[d]=get(r[s]+i.offset,i.code==0x79?8:4);break;case 0x7b:case 0x63:put(r[d]+i.offset,r[s],i.code==0x7b?8:4);break;case 0x7a:case 0x62:put(r[d]+i.offset,imm,i.code==0x7a?8:4);break;
            case 0x05:pc+=i.offset;break;case 0x55:if(r[d]!=imm)pc+=i.offset;break;case 0x15:if(r[d]==imm)pc+=i.offset;break;case 0x35:if(r[d]>=imm)pc+=i.offset;break;case 0x25:if(r[d]>imm)pc+=i.offset;break;case 0xa5:if(r[d]<imm)pc+=i.offset;break;case 0x5d:if(r[d]!=r[s])pc+=i.offset;break;case 0xad:if(r[d]<r[s])pc+=i.offset;break;
            case 0xdb:{const auto at=r[d]+i.offset,old=get(at);if(i.immediate==1){put(at,old+r[s]);r[s]=old;}else if(i.immediate==0xf1){if(old==r[0])put(at,r[s]);r[0]=old;}else if(i.immediate==0x40||i.immediate==0x41){put(at,old|r[s]);if(i.immediate==0x41)r[s]=old;}else{assert(i.immediate==0xe1);put(at,r[s]);r[s]=old;}break;}
            case 0x85:if(i.immediate==14)r[0]=(std::uint64_t{actor}<<32)|3;else if(i.immediate==15)r[0]=uid;else if(i.immediate==35)r[0]=task;else if(i.immediate==5)r[0]=999999;else if(i.immediate==113){++reads;if(reads==fail_read)r[0]=static_cast<std::uint64_t>(-14);else{auto v=get(r[3],static_cast<unsigned>(r[2]));put(r[1],v,static_cast<unsigned>(r[2]));r[0]=0;}}
            else if(i.immediate==2){assert(r[1]==5);if(fail_update)r[0]=static_cast<std::uint64_t>(-12);else{allowlist[static_cast<std::uint32_t>(get(r[2],4))]=get(r[3]);r[0]=0;}}
            else{assert(i.immediate==1);++maps;auto k=get(r[2],4);if(r[1]==3){assert(k<2);r[0]=control+k*8;}else if(r[1]==5){const auto it=allowlist.find(static_cast<std::uint32_t>(k));r[0]=it==allowlist.end()?0:28000;if(r[0])put(r[0],it->second);}else{assert(r[1]==4&&k<2);r[0]=records+k*sizeof(StartupBirthRaw);}}break;
            case 0x95:assert(!r[0]);return;default:assert(false);
            }
        }assert(false);
    }
    StartupBirthRaw row(unsigned n=0){StartupBirthRaw v{};for(unsigned k=0;k<v.size();++k)v[k]=get(records+n*sizeof(StartupBirthRaw)+k*8);return v;}
};
int main(){
    LinuxStartupResourceBtfLayout l;l.leader=8;l.start=16;l.tgid=24;l.cred=32;l.uid=0;l.signal=40;l.live=0;l.nr_threads=4;l.task_runtime=48;l.signal_runtime=8;l.task_read=56;l.signal_read=16;l.task_write=64;l.signal_write=24;
    PrivateNetworkIdentity root{77,0,123456};const auto code=make_startup_birth_probe(3,4,l,root,2);assert(code.value&&code.value->size()<2048);
    assert(!make_startup_birth_probe(3,3,l,root,2).value&&!make_startup_birth_probe(3,4,l,root,0).value&&!make_startup_birth_probe(3,4,l,root,65).value);
    assert(!resolve_linux_startup_birth_hook({}).value);
    Vm v(l);v.run(*code.value);auto row=decode_startup_birth(v.row());assert(row.value&&row.value->has_value()&&(**row.value).kernel.kernel_tgid==88&&!(**row.value).scheduler_runtime_at_birth_ns);
    v.put(Vm::child+l.tgid,89,4);v.run(*code.value);assert(v.get(Vm::control)==2&&decode_startup_birth(v.row(1)).value->has_value());v.run(*code.value);assert(v.get(Vm::control+8)==2&&v.get(Vm::control)==2);
    for(unsigned mode=0;mode<5;++mode){Vm ignored(l);if(mode==0)ignored.actor=99;if(mode==1)ignored.put(Vm::root+l.start,1);if(mode==2)ignored.uid=2;if(mode==3)ignored.put(Vm::child+l.leader,Vm::root);if(mode==4)ignored.fail_read=1;ignored.run(*code.value);assert(!ignored.get(Vm::records));if(mode==4)assert(ignored.get(Vm::control+8)==1);else assert(!ignored.maps);}
    Vm duplicate(l);duplicate.put(Vm::records,1);duplicate.run(*code.value);assert(duplicate.get(Vm::control+8)==4);
    Vm overflow(l);overflow.put(Vm::child+l.task_runtime,UINT64_MAX);overflow.put(Vm::sig+l.signal_runtime,1);overflow.run(*code.value);assert(overflow.get(Vm::control+8)==1&&!overflow.row()[0]);
    for(unsigned n=0;n<5;++n){auto bad=v.row();if(n==0)bad[n]=2;else if(n==2)bad[n]=UINT64_MAX;else bad[n]=0;assert(!decode_startup_birth(bad).value);}
    StartupBirthRaw empty{};auto waiting=decode_startup_birth(empty);assert(waiting.value&&!waiting.value->has_value());
    const auto final_code=make_startup_birth_final_probe(3,4,l,root,2);assert(final_code.value&&final_code.value->size()<2048);
    assert(make_startup_birth_final_probe(3,4,l,root,64).value->size()<4096);
    assert(!make_startup_birth_final_probe(3,3,l,root,1).value&&!make_startup_birth_final_probe(3,4,l,root,65).value);
    Vm completed(l);completed.run(*code.value);completed.put(Vm::sig+l.live,0,4);completed.put(Vm::child+l.task_runtime,1000);completed.put(Vm::sig+l.signal_runtime,2000);completed.put(Vm::child+l.task_read,4000);completed.put(Vm::child+l.task_write,8000);
    completed.run(*final_code.value);auto finish=decode_startup_birth(completed.row());assert(finish.value&&finish.value->has_value()&&(**finish.value).final_monotonic_ns&&(**finish.value).cpu_time.value==std::chrono::nanoseconds(3000)&&(**finish.value).read_bytes.value==4000&&(**finish.value).write_bytes.value==8000);
    completed.run(*final_code.value);assert(completed.get(Vm::control+8)==16&&completed.row()[10]==3000);
    for(unsigned mode=0;mode<7;++mode){Vm skipped(l);skipped.run(*code.value);skipped.put(Vm::sig+l.live,0,4);
        if(mode==0)skipped.actor=99;if(mode==1)skipped.put(Vm::child+l.start,1);if(mode==2)skipped.put(Vm::child+l.tgid,99,4);if(mode==3)skipped.put(Vm::sig+l.live,1,4);if(mode==4)skipped.put(Vm::sig+l.nr_threads,2,4);
        if(mode==5)skipped.put(Vm::cred+l.uid,2,4);if(mode==6){skipped.put(Vm::child+l.task_runtime,UINT64_MAX);skipped.put(Vm::sig+l.signal_runtime,1);}
        skipped.run(*final_code.value);assert(!skipped.row()[8]);assert(skipped.get(Vm::control+8)==(mode>=5?8U:0U));auto pending=decode_startup_birth(skipped.row());assert(pending.value&&pending.value->has_value()&&!(**pending.value).cpu_time.value);
    }
    auto final_raw=completed.row();for(unsigned field:{8U,9U,10U,11U,12U}){auto bad=final_raw;if(field==8)bad[field]=2;else if(field==9)bad[field]=1;else{bad[field-5]=10;bad[field]=9;}assert(!decode_startup_birth(bad).value);}
    auto too_large=final_raw;too_large[10]=UINT64_MAX;auto over=decode_startup_birth(too_large);assert(over.value&&over.value->has_value()&&!(**over.value).cpu_time.value&&(**over.value).read_bytes.value);
    auto zero_final=final_raw;zero_final[10]=zero_final[5];zero_final[11]=zero_final[6];zero_final[12]=zero_final[7];auto zero=decode_startup_birth(zero_final);assert((**zero.value).cpu_time.value==std::chrono::nanoseconds(0)&&(**zero.value).write_bytes.value==0);
    Vm reuse(l);reuse.put(Vm::child+l.start,111);reuse.run(*code.value);reuse.put(Vm::child+l.start,222);reuse.run(*code.value);reuse.put(Vm::sig+l.live,0,4);reuse.run(*final_code.value);assert(!reuse.row(0)[8]&&reuse.row(1)[8]==1); // Must search past reused TGID's OLD start.
    StartupBirthPidProfile p{{72,0,8,16,0,8,0},101};
    const auto local_code=make_startup_birth_probe(3,4,l,root,2,&p);assert(local_code.value&&local_code.value->size()<4096);
    auto populate=[&](Vm& m,unsigned level){m.put(Vm::child+72,20000);m.put(20000,level,4);
        for(unsigned n=0;n<=level&&n<=32;++n){m.put(20008+n*16,n?11+n:88,4);m.put(20016+n*16,24000+n*32);m.put(24000+n*32,100+n,4);}};
    Vm local(l);populate(local,2);local.run(*local_code.value);auto mapped=decode_startup_birth(local.row());
    assert(mapped.value&&(**mapped.value).kernel.kernel_tgid==88&&(**mapped.value).local_pid.value==StartupBirthLocalPid({101,12}));
    local.put(Vm::sig+l.live,0,4);local.run(*final_code.value);assert((**decode_startup_birth(local.row()).value).local_pid.value==(**mapped.value).local_pid.value);
    for(unsigned mode=0;mode<6;++mode){Vm bad(l);populate(bad,2);
        if(mode==0)bad.put(20000,33,4);if(mode==1)bad.put(Vm::child+72,0);if(mode==2)bad.put(20024,0,4);if(mode==3)bad.put(20024,UINT32_MAX,4);if(mode==4)bad.put(20032,0);if(mode==5)bad.put(24032,999,4);
        bad.run(*local_code.value);const auto result=decode_startup_birth(bad.row());assert(result.value&&result.value->has_value()&&!(**result.value).local_pid.value&&bad.get(Vm::control+8)==32);
        bad.put(Vm::sig+l.live,0,4);bad.run(*final_code.value);assert((**decode_startup_birth(bad.row()).value).cpu_time.value==std::chrono::nanoseconds(0));
    }
    Vm baseline(l);baseline.run(*code.value);Vm miss(l);populate(miss,0);miss.run(*local_code.value);assert(miss.reads==baseline.reads+4&&miss.get(Vm::control+8)==32); // No index > level read.
    Vm fault(l);populate(fault,2);fault.fail_read=baseline.reads+3;fault.run(*local_code.value);assert(fault.row()[0]==1&&!fault.row()[13]&&!fault.row()[14]&&fault.get(Vm::control+8)==32);
    for(unsigned n:{0U,32U}){auto max=p;max.namespace_inode=100+n;const auto max_code=make_startup_birth_probe(3,4,l,root,64,&max);Vm m(l);populate(m,n);m.run(*max_code.value);assert((**decode_startup_birth(m.row()).value).local_pid.value->tgid==(n?11+n:88));}
    auto high_inode=p;high_inode.namespace_inode=4026531836ULL;Vm high(l);populate(high,0);high.put(24000,high_inode.namespace_inode,4);high.run(*make_startup_birth_probe(3,4,l,root,1,&high_inode).value);assert(high.row()[13]==high_inode.namespace_inode);
    for(unsigned n=0;n<5;++n){auto bad=local.row();if(n==0)bad[13]=0;if(n==1)bad[14]=0;if(n==2)bad[13]=UINT64_MAX;if(n==3)bad[14]=2147483648ULL;if(n==4)bad[14]=UINT64_MAX;assert(!decode_startup_birth(bad).value);}
    auto invalid=p;invalid.layout.upid_size=0;assert(!make_startup_birth_probe(3,4,l,root,1,&invalid).value);invalid=p;invalid.namespace_inode=UINT64_MAX;assert(!make_startup_birth_probe(3,4,l,root,1,&invalid).value);
    const auto enrolled=make_startup_birth_probe(3,4,l,root,2,nullptr,5);LinuxBirthExecBtfLayout el{80,0,0,0,8,0};const auto exec_code=make_startup_birth_exec_probe(3,4,5,l,el,2);assert(enrolled.value&&exec_code.value&&exec_code.value->size()<2048);
    auto image=[&](Vm& m){m.actor=88;m.task=Vm::child;m.put(Vm::child+80,20000);m.put(20000,21000);m.put(21000,22000);m.put(22000,34567);m.put(22008,23000);m.put(23000,(8U<<20)|15U,4);};
    Vm ex(l);ex.run(*enrolled.value);assert(ex.allowlist.at(88)==0);image(ex);ex.run(*exec_code.value);const auto first=decode_startup_birth(ex.row(),true);assert(first.value&&(**first.value).first_executable.value&&(**first.value).first_executable.value->inode==34567&&(**first.value).first_executable.value->device_major==8&&(**first.value).first_executable.value->device_minor==15);
    ex.put(22000,999);ex.run(*exec_code.value);assert(ex.row()[17]==34567&&ex.row()[15]==1); // Re-exec cannot rewrite FIRST.
    ex.fail_read=ex.reads+1;ex.run(*exec_code.value);assert(ex.row()[15]==1&&ex.row()[17]==34567&&ex.get(Vm::control+8)==64); // Later auth failure cannot erase known first.
    assert(!decode_startup_birth(ex.row()).value);auto pending=ex.row();pending[15]=2;assert(!decode_startup_birth(pending,true).value);
    for(unsigned n=0;n<6;++n){Vm ignored(l);ignored.run(*enrolled.value);image(ignored);if(n==0)ignored.actor=99;if(n==1)ignored.put(Vm::child+l.start,1);if(n==2)ignored.put(Vm::child+80,0);if(n==3)ignored.fail_read=ignored.reads+3;if(n==4)ignored.allowlist[88]=2;if(n==5)ignored.put(Vm::records,0);
        const auto before=ignored.reads;ignored.run(*exec_code.value);assert(ignored.row()[15]==(n==2||n==3?3U:0U));if(n==0||n==4||n==5)assert(ignored.reads==before);if(n==2||n==3||n==4)assert(ignored.get(Vm::control+8)==64);
    }
    Vm enrollment_fail(l);enrollment_fail.fail_update=true;enrollment_fail.run(*enrolled.value);assert(enrollment_fail.row()[0]==1&&enrollment_fail.row()[15]==3&&enrollment_fail.get(Vm::control+8)==64&&enrollment_fail.allowlist.empty());
    auto bad_ex=ex.row();bad_ex[16]=1;assert(!decode_startup_birth(bad_ex,true).value);bad_ex=ex.row();bad_ex[18]=UINT64_MAX;assert(!decode_startup_birth(bad_ex,true).value);bad_ex=ex.row();bad_ex[19]=UINT64_MAX;assert(!decode_startup_birth(bad_ex,true).value);
    auto zeros=ex.row();zeros[17]=zeros[18]=zeros[19]=0;assert((**decode_startup_birth(zeros,true).value).first_executable.value->inode==0); // Measured zero is not absence.
    LinuxStartupBirthCapture denied;assert(denied.start(false,-1,0,0).support==Support::permission_required&&!denied.sample().value);
    std::cout<<"birth VM: creator/birth/final/filter/arithmetic/publication/PID-namespace translation/level0..32/failure/retention passed\n";
}
