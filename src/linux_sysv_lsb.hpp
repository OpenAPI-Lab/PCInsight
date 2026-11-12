#pragma once
#include "pcinsight/linux_sysv.hpp"
#include <functional>
namespace pcinsight::detail {
struct SysVLsbFileIdentity {
    std::uint64_t dev{},ino{},mode{},size{};
    std::int64_t ms{},mn{},cs{},cn{};
    bool operator==(const SysVLsbFileIdentity&)const=default;
};
// Borrowed pinned regular fd; checks expected identity before/after body I/O.
// Caller owns fd, context/path guards and lifetime/close coordination.
Metric<LinuxSysVLsbMetadata> read_sysv_lsb_pinned(int,const SysVLsbFileIdentity&);
// Per-call private fixture hooks, never process-global or installed/public API.
// Reader must obey the 2x64KiB prefix limit and not close/reuse borrowed fd.
struct SysVDescribedSource {
    std::function<void()> after_first_scan;
    std::function<Metric<LinuxSysVLsbMetadata>(int,const SysVLsbFileIdentity&)> read;
};
Metric<LinuxSysVDescribedCatalog> query_sysv_described_with_source(const std::string&,const std::string&,std::uint32_t,std::uint64_t,SysVDescribedSource);
}
