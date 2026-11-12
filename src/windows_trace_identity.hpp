#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <vector>
namespace pcinsight::detail {
// QPC clock of ONE trace epoch, not FILETIME and not an action identity.
struct TraceProcessLifetime {
    std::uint32_t pid=0;
    std::int64_t start=0,end=0; // end0: live through caller-certified watermark.
};
struct TraceThreadLifetime {
    std::uint32_t tid=0,pid=0;
    std::int64_t process_start=0,start=0,end=0;
};
struct TraceOwner {
    std::uint64_t epoch=0;
    std::uint32_t pid=0,tid=0;
    std::int64_t process_start=0,thread_start=0;
    bool operator==(const TraceOwner&) const = default;
};
enum class TraceIdentityState { unavailable, invalid, ready };
class WindowsTraceIdentity {
public:
    // Complete replacement, no partial publication. complete means the caller
    // has established loss-free/reconciled lifecycle coverage through watermark;
    // this index does not establish ETW completeness by itself.
    TraceIdentityState replace(std::uint64_t epoch,std::int64_t watermark,
        std::span<const TraceProcessLifetime>,std::span<const TraceThreadLifetime>,
        bool complete,std::size_t limit=100000);
    std::optional<TraceOwner> owner_at_initiation(std::uint32_t tid,std::int64_t issued) const;
    // Conservative alternative when a caller can establish the whole interval.
    // Rejects exit/reuse/boundary crossings instead of guessing from completion.
    std::optional<TraceOwner> owner_for_interval(std::uint32_t tid,
        std::int64_t issued,std::int64_t completed) const;
    void reset();
    TraceIdentityState state() const { return state_; }
    std::size_t size() const { return size_; }
    std::uint64_t epoch() const { return epoch_; }
    std::int64_t watermark() const { return watermark_; }
private:
    TraceIdentityState state_=TraceIdentityState::unavailable;
    std::uint64_t epoch_=0;
    std::int64_t watermark_=0;
    std::size_t size_=0;
    std::map<std::uint32_t,std::vector<TraceThreadLifetime>> threads_;
    std::map<std::pair<std::uint32_t,std::int64_t>,TraceProcessLifetime> processes_;
};
}
