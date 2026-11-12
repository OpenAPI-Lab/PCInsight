#include "windows_compression.hpp"
#ifdef _WIN32
#include <cstddef>
#include <cstring>

namespace pcinsight::detail {
Metric<std::uint64_t> query_windows_compression(QuerySystemInfo query) {
#if defined(_M_X64) || defined(__x86_64__)
    static_assert(sizeof(CompressionRequest)==40);
    static_assert(offsetof(CompressionRequest,original_bytes)==16);
    static_assert(sizeof(StoreRequest)==24);
    if (!query) return {std::nullopt,Support::unsupported,"NtQuerySystemInformation unavailable"};
    CompressionRequest data;
    StoreRequest request;
    request.data=&data;
    request.length=sizeof(data);
    // Only query compression information. Never enable privileges or trim stores.
    const auto status=query(109,&request,sizeof(request),nullptr);
    if (status!=0) {
        const auto code=static_cast<ULONG>(status);
        const bool denied=code==0xc0000022UL || code==0xc0000061UL;
        return {std::nullopt,denied?Support::permission_required:Support::unsupported,
            "Experimental compression query failed; NTSTATUS="+std::to_string(code)};
    }
    if (request.version!=1 || request.operation!=22 || request.data!=&data ||
        request.length!=sizeof(data) || data.version!=3)
        return {std::nullopt,Support::unsupported,"Unexpected compression ABI response"};
    return {static_cast<std::uint64_t>(data.compressed_bytes),Support::estimated,
        "Experimental Windows store compressed payload bytes; not total resident store/Task Manager footprint"};
#else
    (void)query;
    return {std::nullopt,Support::unsupported,"Experimental compression ABI supports native x64 only"};
#endif
}
Metric<std::uint64_t> read_windows_compression() {
#if defined(PCINSIGHT_EXPERIMENTAL_WINDOWS_COMPRESSION) && (defined(_M_X64) || defined(__x86_64__))
    // ntdll is already loaded by Windows. Do not search for or load a DLL on disk.
    static const auto query=[] {
        QuerySystemInfo fn=nullptr;
        const auto module=GetModuleHandleW(L"ntdll.dll");
        if (module) {
            const auto symbol=GetProcAddress(module,"NtQuerySystemInformation");
            static_assert(sizeof(fn)==sizeof(symbol));
            std::memcpy(&fn,&symbol,sizeof(fn));
        }
        return fn;
    }();
    return query_windows_compression(query);
#else
    return {std::nullopt,Support::unsupported,
        "Experimental Windows compression provider disabled (or unsupported architecture)"};
#endif
}
}
#endif
