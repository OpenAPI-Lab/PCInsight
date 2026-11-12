#include "windows_disk_events.hpp"
#include <limits>
#include <cstring>
namespace pcinsight::detail {
DiskAccumulation accumulate_private_disk(PrivateDiskTotals& totals,const DiskCompletion& e,
    std::uint32_t thread,std::int64_t begin,std::int64_t end) {
    if(!thread || thread==0xffffffffU || begin<=0 || end<begin)return DiskAccumulation::invalid;
    if(e.issuing_thread!=thread || e.qpc<begin || e.qpc>end)return DiskAccumulation::ignored;
    if(e.disk==0xffffffffU)return DiskAccumulation::invalid;
    auto& bytes=e.write?totals.write_bytes:totals.read_bytes;
    auto& count=e.write?totals.writes:totals.reads;
    if(bytes>std::numeric_limits<std::uint64_t>::max()-e.bytes ||
        count==std::numeric_limits<std::uint64_t>::max())return DiskAccumulation::invalid;
    bytes+=e.bytes;++count;
    return DiskAccumulation::added;
}
}
#ifdef _WIN32
namespace pcinsight::detail {
namespace {
template<class T> ULONG scalar(EVENT_RECORD& event,const wchar_t* name,T& out,const DiskEventApi& api){
    PROPERTY_DATA_DESCRIPTOR descriptor{};
    descriptor.PropertyName=reinterpret_cast<ULONGLONG>(name);
    descriptor.ArrayIndex=0xffffffffU;
    ULONG size=0;
    auto error=api.size(&event,0,nullptr,1,&descriptor,&size);
    if(error)return error;
    if(size!=sizeof(out))return ERROR_INVALID_DATA;
    // Immutable event payload: both reads must fully overwrite and agree.
    // Distinct seeds reject no-write/partial output without reserving any uint32.
    T a{},b{};std::memset(&a,0xa5,sizeof(a));std::memset(&b,0x5a,sizeof(b));
    error=api.read(&event,0,nullptr,1,&descriptor,sizeof(a),reinterpret_cast<PBYTE>(&a));
    if(error)return error;
    error=api.read(&event,0,nullptr,1,&descriptor,sizeof(b),reinterpret_cast<PBYTE>(&b));
    if(error)return error;
    if(a!=b)return ERROR_INVALID_DATA;
    out=a;return ERROR_SUCCESS;
}
}
DiskDecodeResult decode_windows_disk_event(EVENT_RECORD& event,DiskEventApi api,std::uint32_t required_thread){
    const auto& h=event.EventHeader;
    if(std::memcmp(&h.ProviderId,&disk_event_guid,sizeof(GUID)) ||
       (h.EventDescriptor.Opcode!=10 && h.EventDescriptor.Opcode!=11))return {};
    auto invalid=[](ULONG error){DiskDecodeResult r;r.status=DiskDecodeStatus::invalid;r.error=error;return r;};
    if(h.EventDescriptor.Version!=3)return invalid(ERROR_NOT_SUPPORTED);
    if(required_thread==0xffffffffU || !(h.Flags&EVENT_HEADER_FLAG_CLASSIC_HEADER) ||
       (h.Flags&EVENT_HEADER_FLAG_TRACE_MESSAGE) ||
       bool(h.Flags&EVENT_HEADER_FLAG_32_BIT_HEADER)==bool(h.Flags&EVENT_HEADER_FLAG_64_BIT_HEADER) ||
       !event.UserData || !event.UserDataLength || h.TimeStamp.QuadPart<=0 || !api.size || !api.read)
        return invalid(ERROR_INVALID_DATA);
    DiskDecodeResult r;
    r.event.qpc=h.TimeStamp.QuadPart;r.event.write=h.EventDescriptor.Opcode==11;
    auto error=scalar(event,L"IssuingThreadId",r.event.issuing_thread,api);
    if(error)return invalid(error);
    // Private probes may discard non-target/unattributable issuers before
    // retrieving sizes or disk IDs. This is NOT evidence of complete coverage.
    if(required_thread && r.event.issuing_thread!=required_thread)return {};
    if(!error)error=scalar(event,L"TransferSize",r.event.bytes,api);
    if(!error)error=scalar(event,L"DiskNumber",r.event.disk,api);
    if(error)return invalid(error);
    if(!r.event.issuing_thread || r.event.issuing_thread==0xffffffffU ||
       r.event.disk==0xffffffffU)return invalid(ERROR_INVALID_DATA);
    r.status=DiskDecodeStatus::decoded;
    return r;
}
DiskOperationDecode decode_windows_disk_operation(EVENT_RECORD& event,DiskEventApi api,std::uint32_t required_thread){
    const auto& h=event.EventHeader;const auto opcode=h.EventDescriptor.Opcode;
    if(std::memcmp(&h.ProviderId,&disk_event_guid,sizeof(GUID)) ||
       (opcode!=10 && opcode!=11 && opcode!=12 && opcode!=13))return {};
    auto invalid=[](ULONG error){DiskOperationDecode r;r.status=DiskDecodeStatus::invalid;r.error=error;return r;};
    DiskOperationDecode r;
    r.event.initiation=opcode==12 || opcode==13;r.event.write=opcode==11 || opcode==13;
    if(!r.event.initiation){
        const auto completion=decode_windows_disk_event(event,api,required_thread);
        if(completion.status!=DiskDecodeStatus::decoded){r.status=completion.status;r.error=completion.error;return r;}
        r.event.issuing_thread=completion.event.issuing_thread;r.event.qpc=completion.event.qpc;
        r.event.disk=completion.event.disk;r.event.bytes=completion.event.bytes;
    }else{
        if(h.EventDescriptor.Version!=3)return invalid(ERROR_NOT_SUPPORTED);
        if(required_thread==0xffffffffU || !(h.Flags&EVENT_HEADER_FLAG_CLASSIC_HEADER) ||
           (h.Flags&EVENT_HEADER_FLAG_TRACE_MESSAGE) ||
           bool(h.Flags&EVENT_HEADER_FLAG_32_BIT_HEADER)==bool(h.Flags&EVENT_HEADER_FLAG_64_BIT_HEADER) ||
           !event.UserData || !event.UserDataLength || h.TimeStamp.QuadPart<=0 || !api.size || !api.read)return invalid(ERROR_INVALID_DATA);
        r.event.qpc=h.TimeStamp.QuadPart;
        const auto error=scalar(event,L"IssuingThreadId",r.event.issuing_thread,api);
        if(error)return invalid(error);
        if(required_thread && r.event.issuing_thread!=required_thread)return {};
        if(!r.event.issuing_thread || r.event.issuing_thread==0xffffffffU)return invalid(ERROR_INVALID_DATA);
    }
    ULONG error=0;
    if(h.Flags&EVENT_HEADER_FLAG_32_BIT_HEADER){
        std::uint32_t pointer=0;error=scalar(event,L"Irp",pointer,api);
        if(!error && pointer==0xffffffffU)error=ERROR_INVALID_DATA;
        r.event.irp=pointer;
    }else error=scalar(event,L"Irp",r.event.irp,api);
    if(error)return invalid(error);
    if(!r.event.irp || r.event.irp==std::numeric_limits<std::uint64_t>::max())return invalid(ERROR_INVALID_DATA);
    r.status=DiskDecodeStatus::decoded;return r;
}
}
#endif
