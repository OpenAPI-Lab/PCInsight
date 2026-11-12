#pragma once
#include "pcinsight/linux_s6.hpp"
#include <functional>
namespace pcinsight::detail {
// Per-instance private fixture seam, never a process-global override. No reentry.
// Request returns delivery counts, with no post-delivery throwing operations.
struct S6ObservationApi {
    std::function<Metric<LinuxS6StatusRecord>()> read;
    std::function<LinuxS6RequestResult(ServiceAction,bool)> request;
    std::function<std::chrono::steady_clock::time_point()> now;
};
struct S6ObservationAccess {static LinuxS6Observation make(S6ObservationApi);};
}
