#pragma once
#include "pcinsight/metric.hpp"
#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <string_view>
namespace pcinsight::detail {
struct PrivilegedAdmissionConfig {
    std::uint32_t global_burst{64},global_per_second{16},sender_burst{16},sender_per_second{4},max_senders{64};
    std::uint64_t idle_timeout_usec{300000000},max_read_queue{64},max_write_queue{64};
};
bool valid_privileged_admission_config(const PrivilegedAdmissionConfig&);
// Per RECEIVER connection, not per UID/process. Keys are broker-assigned senders.
class PrivilegedAdmission {
public:
    using Clock=std::chrono::steady_clock;using Now=std::function<Clock::time_point()>;
    explicit PrivilegedAdmission(PrivilegedAdmissionConfig={},Now={});
    Metric<bool> admit(std::string_view sender,std::uint64_t queued_read=0,std::uint64_t queued_write=0);
    std::size_t tracked_senders()const;
private:
    struct Bucket{std::uint64_t credit{};Clock::time_point updated{},seen{};};
    void refill(Bucket&,std::uint32_t capacity,std::uint32_t rate,Clock::time_point);
    PrivilegedAdmissionConfig config_;Now now_;mutable std::mutex mutex_;
    Bucket global_;std::map<std::string,Bucket,std::less<>> senders_;bool valid_{};
};
}
