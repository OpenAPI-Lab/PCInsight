#include "../src/linux_selected_exec.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;using namespace pcinsight::detail;
int main(){
    const PrivateNetworkIdentity key{12,0,33};const SelectedExecFile file{55,8,1};SelectedExecRaw raw{};raw[0]=1;raw[1]=12;raw[3]=33;
    auto r=decode_selected_exec(raw,key,file,0);assert(r.value&&!r.value->first_exec.value&&!r.value->pinned_file_matches.value&&!r.value->publishing);
    raw[15]=2;raw[17]=99;r=decode_selected_exec(raw,key,file,0);assert(r.value&&r.value->publishing&&!r.value->first_exec.value); // Partial bytes never published.
    raw[15]=3;r=decode_selected_exec(raw,key,file,0);assert(r.value&&r.value->publishing&&!r.value->first_exec.value); // Failure flag may follow event state.
    r=decode_selected_exec(raw,key,file,64);assert(r.value&&!r.value->publishing&&!r.value->first_exec.value&&!r.value->pinned_file_matches.value);
    raw[15]=1;raw[16]=100;raw[17]=55;raw[18]=(8U<<20)|1U;raw[19]=0;
    r=decode_selected_exec(raw,key,file,0);assert(r.value&&r.value->first_exec.value&&r.value->first_exec.value->observed_monotonic_ns==100&&r.value->pinned_file_matches.value==true&&r.value->pinned_file_matches.support==Support::available);
    auto other=file;other.inode=56;r=decode_selected_exec(raw,key,other,0);assert(r.value&&r.value->first_exec.value&&r.value->pinned_file_matches.value==false); // Measured nonmatch, not unknown.
    r=decode_selected_exec(raw,key,file,64);assert(r.value&&r.support==Support::estimated&&r.value->first_exec.value&&r.value->pinned_file_matches.value==true); // Sticky later diagnostics do not erase published evidence.
    for(unsigned n=0;n<13;++n){auto bad=raw;
        if(n==0)bad[0]=0;if(n==1)bad[1]=13;if(n==2)bad[2]=1;if(n==3)bad[3]=34;if(n==4)bad[4]=100; // NO fabricated birth timestamp.
        if(n==5)bad[8]=1;if(n==6)bad[14]=1;if(n==7)bad[15]=4;if(n==8)bad[16]=0;if(n==9)bad[18]=UINT64_MAX;
        if(n==10)bad[19]=1;if(n==11)bad[19]=UINT64_MAX;if(n==12)bad[15]=0;
        assert(!decode_selected_exec(bad,key,file,0).value);
    }
    assert(!decode_selected_exec(raw,key,file,1).value);auto invalid=file;invalid.device_major=4096;assert(!decode_selected_exec(raw,key,invalid,0).value);
    raw[17]=raw[18]=0;auto zero=decode_selected_exec(raw,key,{0,0,0},0);assert(zero.value&&zero.value->first_exec.value->inode==0&&zero.value->pinned_file_matches.value==true); // Known zeros preserved.
    LinuxSelectedExecCapture capture;assert(capture.start(false,{}).support==Support::permission_required&&!capture.sample().value);capture.stop();
    std::cout<<"selected exec decoder: ARM not birth/key/first-event/publishing/failure/known-zero/match-mismatch/flags/UID/unused fields/denial passed\n";
}
