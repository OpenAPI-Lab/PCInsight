#pragma once
#ifdef _WIN32
#include "pcinsight/types.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace pcinsight::detail {
// Internal ABI, not part of the public PCInsight API. Layout reference:
// https://github.com/winsiderss/systeminformer/blob/master/phnt/include/ntexapi.h
// SystemStoreInformation / MemCompressionInfoRequest, version 3.
struct CompressionRequest {
    ULONG version=3, pid=0, working_set_pages=0;
    SIZE_T original_bytes=0, compressed_bytes=0, unique_original_bytes=0;
};
struct StoreRequest {
    ULONG version=1, operation=22;
    void* data=nullptr;
    ULONG length=0;
};
using QuerySystemInfo = LONG (NTAPI*)(ULONG,void*,ULONG,ULONG*);
Metric<std::uint64_t> query_windows_compression(QuerySystemInfo query);
Metric<std::uint64_t> read_windows_compression();
}
#endif
