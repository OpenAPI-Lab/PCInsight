#pragma once
#include <cstdint>
namespace pcinsight::detail {
struct DiskCompletion {
    std::uint32_t disk=0, bytes=0, issuing_thread=0;
    std::int64_t qpc=0;
    bool write=false;
};
// Internal normalized init/completion. IRP is an opaque key, never dereferenced.
struct DiskOperation {
    std::uint64_t irp=0;
    std::uint32_t issuing_thread=0;
    std::int64_t qpc=0;
    bool initiation=false,write=false;
    std::uint32_t disk=0,bytes=0; // Meaningful only for completion.
};
struct PrivateDiskTotals {
    std::uint64_t read_bytes=0, write_bytes=0, reads=0, writes=0;
};
enum class DiskAccumulation { ignored, added, invalid };
// Only for a caller-pinned, continuously live thread and an explicit QPC window.
// Not a general PID resolver. No zero-activity/completeness guarantee.
DiskAccumulation accumulate_private_disk(PrivateDiskTotals&,const DiskCompletion&,
    std::uint32_t pinned_thread,std::int64_t begin,std::int64_t end);
}
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <evntcons.h>
#include <tdh.h>
namespace pcinsight::detail {
inline constexpr GUID disk_event_guid{0x3d6fa8d4,0xfe05,0x11d0,{0x9d,0xda,0x00,0xc0,0x4f,0xd7,0xba,0x7c}};
struct DiskEventApi {
    decltype(&TdhGetPropertySize) size=&TdhGetPropertySize;
    decltype(&TdhGetProperty) read=&TdhGetProperty;
};
enum class DiskDecodeStatus { ignored, invalid, decoded };
struct DiskDecodeResult {
    DiskDecodeStatus status=DiskDecodeStatus::ignored;
    DiskCompletion event;
    ULONG error=ERROR_SUCCESS;
};
// Classic DiskIo Read/Write version3 only, TDH named properties, no PID-header
// attribution and no manual pointer-width-dependent payload offsets.
DiskDecodeResult decode_windows_disk_event(EVENT_RECORD&,DiskEventApi = {},std::uint32_t required_thread=0);
struct DiskOperationDecode {
    DiskDecodeStatus status=DiskDecodeStatus::ignored;
    DiskOperation event;
    ULONG error=ERROR_SUCCESS;
};
DiskOperationDecode decode_windows_disk_operation(EVENT_RECORD&,DiskEventApi = {},std::uint32_t required_thread=0);
}
#endif
