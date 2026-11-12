#include "../src/windows_compression.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
LONG result=0;
int mutation=0, calls=0;
SIZE_T compressed=4096;
LONG NTAPI query(ULONG kind,void* pointer,ULONG length,ULONG* returned) {
    ++calls;
    assert(kind==109 && length==sizeof(StoreRequest) && !returned);
    auto& request=*static_cast<StoreRequest*>(pointer);
    assert(request.version==1 && request.operation==22 && request.length==sizeof(CompressionRequest));
    auto& data=*static_cast<CompressionRequest*>(request.data);
    assert(data.version==3 && data.pid==0 && data.compressed_bytes==0);
    data.compressed_bytes=compressed;
    if (mutation==1) data.version=4;
    if (mutation==2) request.length=1;
    if (mutation==3) request.operation=21;
    if (mutation==4) request.data=nullptr;
    if (mutation==5) request.version=2;
    return result;
}
}
int main() {
#if defined(_M_X64) || defined(__x86_64__)
    assert(!query_windows_compression(nullptr).value);
    auto r=query_windows_compression(query);
    assert(r.value==4096 && r.support==Support::estimated);
    compressed=0; assert(query_windows_compression(query).value==0);
    for (auto code : {0xc0000022UL,0xc0000061UL,0xc0000003UL,0xc0000004UL,1UL}) {
        result=static_cast<LONG>(code);
        r=query_windows_compression(query);
        assert(!r.value);
        assert(r.support==((code==0xc0000022UL || code==0xc0000061UL)?Support::permission_required:Support::unsupported));
    }
    result=0;
    for (mutation=1;mutation<=5;++mutation) assert(!query_windows_compression(query).value);
    mutation=0; compressed=8192;
    assert(query_windows_compression(query).value==8192); // recover without stale data
#else
    assert(!query_windows_compression(query).value && calls==0);
#endif
    const auto live=read_windows_compression();
    if (live.value) {
        assert(live.support==Support::estimated);
        std::cout << "Experimental compressed payload bytes: " << *live.value << '\n';
    } else {
        assert(live.support==Support::unsupported || live.support==Support::permission_required);
        assert(!live.note.empty()); std::cout << live.note << '\n';
    }
}
