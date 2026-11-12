#include "../src/windows_disk_events.hpp"
#include "../src/windows_disk_correlation.hpp"
#include <array>
#include <cassert>
#include <limits>
#include <iostream>
#ifdef _WIN32
#include <cwchar>
#include <cstring>
using namespace pcinsight::detail;
namespace {
unsigned sizes,reads;
int size_mode,read_mode;
std::uint32_t tid=123,bytes=4096,disk=0;
std::uint64_t irp=0xfedcba9876543000ULL;
unsigned pointer_size=0;
ULONG WINAPI size_fn(PEVENT_RECORD event,ULONG contexts,PTDH_CONTEXT context,ULONG count,PPROPERTY_DATA_DESCRIPTOR d,ULONG* n){
    assert(!contexts && !context && count==1 && d->ArrayIndex==0xffffffffU && !d->Reserved);++sizes;
    if(size_mode==1)return ERROR_NOT_FOUND;
    if(size_mode==2)return 0; // No size output.
    const auto name=reinterpret_cast<const wchar_t*>(static_cast<std::uintptr_t>(d->PropertyName));
    *n=size_mode==3?8:std::wcscmp(name,L"Irp")==0?(pointer_size?pointer_size:(event->EventHeader.Flags&EVENT_HEADER_FLAG_32_BIT_HEADER?4:8)):4;return 0;
}
ULONG WINAPI read_fn(PEVENT_RECORD,ULONG contexts,PTDH_CONTEXT context,ULONG count,PPROPERTY_DATA_DESCRIPTOR d,ULONG n,PBYTE out){
    assert(!contexts && !context && count==1 && (n==4||n==8));++reads;
    if(read_mode==1)return 0;
    if(read_mode==4 || (read_mode==5 && reads%2==0))return ERROR_INVALID_DATA;
    const auto name=reinterpret_cast<const wchar_t*>(static_cast<std::uintptr_t>(d->PropertyName));
    std::uint64_t value=std::wcscmp(name,L"IssuingThreadId")==0?tid:std::wcscmp(name,L"TransferSize")==0?bytes:std::wcscmp(name,L"Irp")==0?irp:disk;
    if(read_mode==3)value+=reads%2;
    std::memcpy(out,&value,read_mode==2?2:n);return 0;
}
DiskEventApi api{size_fn,read_fn};
EVENT_RECORD event(){static BYTE payload[64]{};EVENT_RECORD e{};e.UserData=payload;e.UserDataLength=sizeof(payload);
    auto& h=e.EventHeader;h.ProviderId=disk_event_guid;h.EventDescriptor.Version=3;h.EventDescriptor.Opcode=10;
    h.Flags=EVENT_HEADER_FLAG_CLASSIC_HEADER|EVENT_HEADER_FLAG_64_BIT_HEADER;h.TimeStamp.QuadPart=12345;
    h.ProcessId=999;h.ThreadId=888;return e;}
void reset(){sizes=reads=0;size_mode=read_mode=0;tid=123;bytes=4096;disk=0;irp=0xfedcba9876543000ULL;pointer_size=0;}
void decoder(){
    reset();auto e=event();auto r=decode_windows_disk_event(e,api);
    assert(r.status==DiskDecodeStatus::decoded && r.event.issuing_thread==123 && r.event.bytes==4096 && r.event.disk==0 && !r.event.write && r.event.qpc==12345);
    assert(sizes==3 && reads==6); // Header PID999/thread888 are NOT used as issuer.
}
}
#endif
int main(){
    using namespace pcinsight::detail;
    PrivateDiskTotals totals;
    DiskCompletion e{0,4096,123,100,false};
    assert(accumulate_private_disk(totals,e,123,100,200)==DiskAccumulation::added && totals.read_bytes==4096 && totals.reads==1);
    e.write=true;e.qpc=200;assert(accumulate_private_disk(totals,e,123,100,200)==DiskAccumulation::added && totals.write_bytes==4096 && totals.writes==1);
    for(auto time:{99LL,201LL}){e.qpc=time;assert(accumulate_private_disk(totals,e,123,100,200)==DiskAccumulation::ignored);}
    e.qpc=100;e.issuing_thread=124;assert(accumulate_private_disk(totals,e,123,100,200)==DiskAccumulation::ignored);
    e.issuing_thread=123;e.bytes=0;const auto prior=totals.writes;
    assert(accumulate_private_disk(totals,e,123,100,200)==DiskAccumulation::added && totals.writes==prior+1 && totals.write_bytes==4096);
    e.bytes=4096;e.disk=0xffffffffU;assert(accumulate_private_disk(totals,e,123,100,200)==DiskAccumulation::invalid);
    e.disk=0;assert(accumulate_private_disk(totals,e,0,100,200)==DiskAccumulation::invalid);
    assert(accumulate_private_disk(totals,e,0xffffffffU,100,200)==DiskAccumulation::invalid);
    assert(accumulate_private_disk(totals,e,123,0,200)==DiskAccumulation::invalid);
    assert(accumulate_private_disk(totals,e,123,200,100)==DiskAccumulation::invalid);
    const auto maximum=std::numeric_limits<std::uint64_t>::max();
    totals.write_bytes=maximum-4096;assert(accumulate_private_disk(totals,e,123,100,200)==DiskAccumulation::added && totals.write_bytes==maximum);
    const auto before=totals.writes;assert(accumulate_private_disk(totals,e,123,100,200)==DiskAccumulation::invalid && totals.writes==before && totals.write_bytes==maximum);
    totals.write_bytes=0;totals.writes=maximum;assert(accumulate_private_disk(totals,e,123,100,200)==DiskAccumulation::invalid && totals.write_bytes==0);
    // Deterministic independent arithmetic/window oracle, no live capture.
    PrivateDiskTotals actual,expected;std::uint32_t seed=7;
    for(unsigned i=0;i<10000;++i){seed=seed*1664525U+1013904223U;
        DiskCompletion x{seed%7,seed%65537,seed%3==0?123U:456U,static_cast<std::int64_t>(seed%400),bool(seed&1)};
        const bool accepted=x.issuing_thread==123 && x.qpc>=100 && x.qpc<=300;
        const auto result=accumulate_private_disk(actual,x,123,100,300);
        assert(result==(accepted?DiskAccumulation::added:DiskAccumulation::ignored));
        if(accepted){if(x.write){expected.write_bytes+=x.bytes;++expected.writes;}else{expected.read_bytes+=x.bytes;++expected.reads;}}
    }
    assert(actual.read_bytes==expected.read_bytes && actual.write_bytes==expected.write_bytes && actual.reads==expected.reads && actual.writes==expected.writes);
#ifdef _WIN32
    decoder();
    for(unsigned bit:{EVENT_HEADER_FLAG_32_BIT_HEADER,EVENT_HEADER_FLAG_64_BIT_HEADER}){
        reset();auto x=event();x.EventHeader.Flags=static_cast<USHORT>(EVENT_HEADER_FLAG_CLASSIC_HEADER|bit);x.EventHeader.EventDescriptor.Opcode=11;
        auto r=decode_windows_disk_event(x,api);assert(r.status==DiskDecodeStatus::decoded && r.event.write);
    }
    for(int mode=1;mode<=3;++mode){reset();size_mode=mode;auto x=event();assert(decode_windows_disk_event(x,api).status==DiskDecodeStatus::invalid && !reads);}
    for(int mode=1;mode<=5;++mode){reset();read_mode=mode;auto x=event();assert(decode_windows_disk_event(x,api).status==DiskDecodeStatus::invalid);}
    for(int mode=0;mode<10;++mode){reset();auto x=event();
        switch(mode){case 0:x.EventHeader.ProviderId.Data1++;break;case 1:x.EventHeader.EventDescriptor.Opcode=12;break;
            case 2:x.EventHeader.EventDescriptor.Version=2;break;case 3:x.EventHeader.Flags=0;break;
            case 4:x.EventHeader.Flags|=EVENT_HEADER_FLAG_32_BIT_HEADER;break;case 5:x.EventHeader.Flags|=EVENT_HEADER_FLAG_TRACE_MESSAGE;break;
            case 6:x.UserData=nullptr;break;case 7:x.UserDataLength=0;break;case 8:x.EventHeader.TimeStamp.QuadPart=0;break;
            case 9:x.EventHeader.TimeStamp.QuadPart=-1;break;}
        assert(decode_windows_disk_event(x,api).status==(mode<2?DiskDecodeStatus::ignored:DiskDecodeStatus::invalid));assert(!sizes && !reads);
    }
    reset();{auto x=event();assert(decode_windows_disk_event(x,{nullptr,read_fn}).status==DiskDecodeStatus::invalid);}
    reset();{auto x=event();assert(decode_windows_disk_event(x,{size_fn,nullptr}).status==DiskDecodeStatus::invalid);}
    for(auto value:{0xa5a5a5a5U,0x5a5a5a5aU,0xffffffffU}){reset();bytes=value;auto x=event();assert(decode_windows_disk_event(x,api).event.bytes==value);}
    reset();bytes=0;{auto x=event();auto r=decode_windows_disk_event(x,api);assert(r.status==DiskDecodeStatus::decoded && r.event.bytes==0);}
    reset();{auto x=event();assert(decode_windows_disk_event(x,api,456).status==DiskDecodeStatus::ignored && sizes==1 && reads==2);}
    reset();tid=0xffffffffU;{auto x=event();assert(decode_windows_disk_event(x,api,123).status==DiskDecodeStatus::ignored && sizes==1 && reads==2);}
    reset();{auto x=event();assert(decode_windows_disk_event(x,api,0xffffffffU).status==DiskDecodeStatus::invalid && !sizes);}
    for(int field=0;field<3;++field){reset();if(field==0)tid=0;if(field==1)tid=0xffffffffU;if(field==2)disk=0xffffffffU;
        auto x=event();assert(decode_windows_disk_event(x,api).status==DiskDecodeStatus::invalid);}
    for(auto opcode:{10U,11U,12U,13U})for(auto width:{32U,64U}){
        reset();auto x=event();x.EventHeader.EventDescriptor.Opcode=static_cast<UCHAR>(opcode);
        x.EventHeader.Flags=static_cast<USHORT>(EVENT_HEADER_FLAG_CLASSIC_HEADER|(width==32?EVENT_HEADER_FLAG_32_BIT_HEADER:EVENT_HEADER_FLAG_64_BIT_HEADER));
        auto r=decode_windows_disk_operation(x,api);assert(r.status==DiskDecodeStatus::decoded);
        assert(r.event.initiation==(opcode>=12) && r.event.write==(opcode==11||opcode==13));
        assert(r.event.irp==(width==32?0x76543000ULL:irp) && r.event.issuing_thread==123 && r.event.qpc==12345);
        assert(sizes==(opcode>=12?2U:4U));
    }
    for(auto value:{0xa5a5a5a5a5a5a5a5ULL,0x5a5a5a5a5a5a5a5aULL}){reset();irp=value;auto x=event();assert(decode_windows_disk_operation(x,api).event.irp==value);}
    for(auto value:{0ULL,0xffffffffffffffffULL}){reset();irp=value;auto x=event();assert(decode_windows_disk_operation(x,api).status==DiskDecodeStatus::invalid);}
    reset();pointer_size=4;{auto x=event();assert(decode_windows_disk_operation(x,api).status==DiskDecodeStatus::invalid);}
    reset();pointer_size=8;{auto x=event();x.EventHeader.Flags=EVENT_HEADER_FLAG_CLASSIC_HEADER|EVENT_HEADER_FLAG_32_BIT_HEADER;
        assert(decode_windows_disk_operation(x,api).status==DiskDecodeStatus::invalid);}
    for(int mode=0;mode<10;++mode){reset();auto x=event();x.EventHeader.EventDescriptor.Opcode=12;
        switch(mode){case 0:x.EventHeader.ProviderId.Data1++;break;case 1:x.EventHeader.EventDescriptor.Opcode=15;break;
            case 2:x.EventHeader.EventDescriptor.Version=2;break;case 3:x.EventHeader.Flags=0;break;
            case 4:x.EventHeader.Flags|=EVENT_HEADER_FLAG_32_BIT_HEADER;break;case 5:x.EventHeader.Flags|=EVENT_HEADER_FLAG_TRACE_MESSAGE;break;
            case 6:x.UserData=nullptr;break;case 7:x.UserDataLength=0;break;case 8:x.EventHeader.TimeStamp.QuadPart=0;break;case 9:tid=0;break;}
        assert(decode_windows_disk_operation(x,api).status==(mode<2?DiskDecodeStatus::ignored:DiskDecodeStatus::invalid));
    }
    reset();{auto x=event();x.EventHeader.EventDescriptor.Opcode=12;assert(decode_windows_disk_operation(x,api,456).status==DiskDecodeStatus::ignored && sizes==1);}
    for(int mode=1;mode<=5;++mode){reset();read_mode=mode;auto x=event();x.EventHeader.EventDescriptor.Opcode=12;
        assert(decode_windows_disk_operation(x,api).status==DiskDecodeStatus::invalid);}
    // Injected TDH -> normalized events -> temporal owner -> late completion.
    // Header PID999/TID888 deliberately disagree with the emitting thread123.
    reset();{
        WindowsTraceIdentity id;
        const TraceProcessLifetime p[]={{42,10,140},{42,145,0}};
        const TraceThreadLifetime t[]={{123,42,10,20,130},{123,42,145,146,0}};
        assert(id.replace(1,1000,p,t,true)==TraceIdentityState::ready);
        auto start=event();start.EventHeader.EventDescriptor.Opcode=12;start.EventHeader.TimeStamp.QuadPart=100;
        auto finish=event();finish.EventHeader.TimeStamp.QuadPart=150;
        const auto a=decode_windows_disk_operation(start,api),b=decode_windows_disk_operation(finish,api);
        assert(a.status==DiskDecodeStatus::decoded && b.status==DiskDecodeStatus::decoded);
        const std::array<DiskOperation,2> operations{a.event,b.event};WindowsDiskCorrelation correlation(1000);
        const auto result=correlation.sample(1,200,operations,id,true);
        assert(result.completions.size()==1 && result.completions[0].owner.pid==42 && result.completions[0].owner.process_start==10);
        assert(id.owner_at_initiation(123,150)->process_start==145);
    }
#endif
    std::cout<<"Disk event decoding and private pinned-thread window accounting passed\n";
}
