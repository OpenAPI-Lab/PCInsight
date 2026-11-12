#pragma once
#include "pcinsight/metric.hpp"
#include "linux_privileged_budget.hpp"
#include <chrono>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>
namespace pcinsight::detail {
bool valid_privileged_job_path(std::string_view);
enum class PrivilegedJobState { pending,completed,failed,unknown };
struct PrivilegedJobSnapshot {
    PrivilegedJobState state{PrivilegedJobState::pending};
    Metric<std::string> result;
    Metric<bool> succeeded;
    std::string note;
};
struct PrivilegedJobEvent {std::uint32_t id{};std::string path,unit,result;};
// Data only: snapshots/destruction safe from consumer threads; no native bus calls.
class PrivilegedJobObservation {
public:
    using Clock=std::chrono::steady_clock;
    PrivilegedJobObservation(std::string unit,Clock::time_point deadline);
    void select(std::string path);
    void accept(PrivilegedJobEvent);
    void invalidate(std::string note);
    void close_pending(std::string note);
    PrivilegedJobSnapshot snapshot(Clock::time_point now=Clock::now());
private:
    void unknown_locked(std::string);
    void apply_locked(const PrivilegedJobEvent&);
    std::mutex mutex_;
    std::string unit_,path_;
    Clock::time_point deadline_;
    PrivilegedJobSnapshot state_;
    std::vector<PrivilegedJobEvent> early_;
    std::size_t events_{};
};
// Native match lifetime belongs exclusively to the serialized connection owner.
class PrivilegedNativeJobWatch {
public:
    struct Impl;
    ~PrivilegedNativeJobWatch();
    std::shared_ptr<PrivilegedJobObservation> observation()const;
private:
    explicit PrivilegedNativeJobWatch(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
    friend Metric<std::shared_ptr<PrivilegedNativeJobWatch>> create_privileged_job_watch(void*,std::string,std::string,std::uint64_t,bool,std::shared_ptr<PrivilegedRequestBudget>);
};
Metric<std::shared_ptr<PrivilegedNativeJobWatch>> create_privileged_job_watch(void* borrowed_bus,std::string manager_owner,std::string unit,std::uint64_t timeout_usec,bool subscribe,std::shared_ptr<PrivilegedRequestBudget> budget={});
}
