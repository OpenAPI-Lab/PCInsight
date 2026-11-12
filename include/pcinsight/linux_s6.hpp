#pragma once
#include "metric.hpp"
#include "actions.hpp"
#include <array>
#include <cstdint>
#include <span>
#include <memory>
#include <vector>
#include <chrono>
namespace pcinsight {
namespace detail {struct S6ObservationAccess;}
// Raw 35-byte s6 supervisor RECORD, not proof of current process/supervisor life.
struct LinuxS6StatusRecord {
    std::array<std::uint8_t,12> changed_at_tai64n{},ready_changed_at_tai64n{};
    std::uint64_t reported_pid{}; // Big-endian wire u64, not a local identity token.
    std::uint16_t reported_wait_status{}; // Raw wait word, NOT an exit code.
    bool paused{},finishing{},want_up{},ready_reported{};
    bool operator==(const LinuxS6StatusRecord&)const=default;
};
Metric<LinuxS6StatusRecord> decode_linux_s6_status_record(std::span<const std::uint8_t>);
// Explicit trusted service directory. Read-only, no CLI/lib/lock probe/exec.
// Historical record only, even if available; absent/non-regular/malformed UNKNOWN.
// Caller serializes fork/setns/credentials/FD context. No atomic/ABA guarantee.
Metric<LinuxS6StatusRecord> read_linux_s6_status_record(const std::string& absolute_service_directory);
struct LinuxS6RequestResult {
    ActionError error{ActionError::none};
    std::uint32_t requested_bytes{},written_bytes{};
    Metric<bool> completed; // ALWAYS UNKNOWN, even after all bytes were written.
    std::string message;
    explicit operator bool()const{return error==ActionError::none;}
};
// Explicit caller-trusted control FIFO, NOT authenticated/version-checked peer.
// start=u, stop=d, restart=cru (resume/configured restart/ensure wanted-up).
// No CLI/exec/down-file changes, retries, waiting, workers or implicit elevation.
class LinuxS6Control {
public:
    static Metric<std::shared_ptr<LinuxS6Control>> open(const std::string& absolute_service_directory);
    ~LinuxS6Control();
    LinuxS6Control(const LinuxS6Control&)=delete;
    LinuxS6Control& operator=(const LinuxS6Control&)=delete;
    LinuxS6RequestResult request(ServiceAction,bool explicitly_authorized);
    // Raw record from the SAME pinned control target; not current path or life.
    Metric<LinuxS6StatusRecord> read_record()const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit LinuxS6Control(std::unique_ptr<Impl>);
};
struct LinuxS6InventoryEntry {
    std::string name,directory; // name relative to scan root, e.g. example/log.
    bool logger{};
    std::string parent_name; // Only for a directly associated log directory.
    Metric<LinuxS6StatusRecord> status_record; // Historical raw record, NOT alive.
    Metric<bool> real_control_fifo; // Type/presence ONLY, not writable/reader/peer.
};
struct LinuxS6Inventory {
    std::string root;
    bool complete{true};
    std::vector<LinuxS6InventoryEntry> entries;
    std::vector<std::string> diagnostics;
};
// Explicit scan tree, not global installed/active services or s6-rc dependency DB.
// Direct visible directories/aliases plus ONE log level, no recursion/actions.
Metric<LinuxS6Inventory> query_linux_s6_inventory(const std::string& absolute_scan_root,std::uint32_t max_entries=1024);
// Raw field condition ONLY. Not service life/readiness/request causality.
Metric<bool> observe_linux_s6_condition(ServiceAction,const LinuxS6StatusRecord& before,const LinuxS6StatusRecord& after);
enum class LinuxS6ObservationState { idle,pending,condition_observed,timed_out,failed,abandoned };
struct LinuxS6ObservationReport {
    LinuxS6ObservationState state{LinuxS6ObservationState::idle};
    ServiceAction action{ServiceAction::start};
    bool request_attempted{};
    LinuxS6RequestResult delivery;
    Metric<LinuxS6StatusRecord> before,latest;
    Metric<bool> baseline_satisfied,target_condition_observed;
    std::chrono::steady_clock::time_point started_at{},deadline{};
    std::optional<std::chrono::steady_clock::time_point> sampled_at;
    ActionError error{ActionError::none};
};
// One request with consent and caller-driven poll, no worker/timer/retry/undo.
// condition_observed is frozen historical record evidence, NOT job completion.
class LinuxS6Observation {
public:
    explicit LinuxS6Observation(std::shared_ptr<LinuxS6Control>);
    ~LinuxS6Observation();
    LinuxS6Observation(const LinuxS6Observation&)=delete;
    LinuxS6Observation& operator=(const LinuxS6Observation&)=delete;
    ActionResult start(ServiceAction,bool explicitly_authorized,std::chrono::milliseconds timeout=std::chrono::seconds(7));
    ActionResult poll();
    LinuxS6ObservationReport report()const; // Copy only, no I/O.
    void close(); // Abandon observation only; no service action/final read.
private:
    friend struct detail::S6ObservationAccess;
    struct Impl;std::unique_ptr<Impl> impl_;
    LinuxS6Observation(std::unique_ptr<Impl>,int);
};
}
