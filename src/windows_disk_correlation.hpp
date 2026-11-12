#pragma once
#include "windows_disk_events.hpp"
#include "windows_trace_identity.hpp"
namespace pcinsight::detail {
struct CorrelatedDiskCompletion {
    TraceOwner owner;
    std::uint64_t irp=0;
    std::int64_t issued=0,completed=0;
    std::uint32_t disk=0,bytes=0;
    bool write=false;
};
enum class DiskCorrelationState { unavailable, invalid, processed };
struct DiskCorrelationBatch {
    DiskCorrelationState state=DiskCorrelationState::unavailable;
    std::vector<CorrelatedDiskCompletion> completions;
    std::uint64_t unmatched=0,discarded=0;
};
class WindowsDiskCorrelation {
public:
    explicit WindowsDiskCorrelation(std::int64_t max_age_ticks,std::size_t limit=100000)
        :max_age_(max_age_ticks),limit_(limit){}
    // Caller supplies reconciled, ordered, loss-free events in [previousW,W).
    // processed means positive pairs processed, NOT a complete usage metric.
    DiskCorrelationBatch sample(std::uint64_t epoch,std::int64_t watermark,
        std::span<const DiskOperation>,const WindowsTraceIdentity&,bool complete);
    void reset();
    std::size_t pending() const { return pending_.size(); }
private:
    struct Pending {DiskOperation init;std::optional<TraceOwner> owner;};
    std::map<std::uint64_t,Pending> pending_;
    std::uint64_t epoch_=0;
    std::int64_t watermark_=0,max_age_;
    std::size_t limit_;
};
}
