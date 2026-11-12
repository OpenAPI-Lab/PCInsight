#pragma once
#include "actions.hpp"
#include "metric.hpp"
#include <array>
#include <chrono>
#include <compare>
#include <memory>
namespace pcinsight {
namespace detail {struct StartupBirthObserverAccess;}
enum class StartupBirthObserverState { idle,recording,closed,failed };
struct StartupBirthObserverOptions {
    std::string capture_id; //1..128 bytes, no embedded NUL.
    std::uint32_t capacity{16}; //1..64 retained groups, never recycled.
    bool recover_final_resources{false}; // Extra selected-reaper hook, default OFF.
    bool observe_first_exec{false}; // First successful exec file metadata, NOT current exe/app ID.
};
// A historical identity in this capture, NOT a signal/launch authorization token.
struct StartupBirthProcessKey {
    std::string capture_id;
    std::array<std::uint64_t,2> boot{};
    std::uint32_t kernel_tgid{},kernel_real_uid{};
    std::uint64_t kernel_start_boottime_ns{};
    auto operator<=>(const StartupBirthProcessKey&) const = default;
};
struct StartupBirthPid {
    std::uint64_t namespace_device{},namespace_inode{};
    std::uint32_t tgid{};
    auto operator<=>(const StartupBirthPid&) const = default;
};
struct StartupBirthObservation {
    StartupBirthProcessKey key;
    Metric<std::string> startup_entry_id; // Caller declaration, never inferred from parent/name/PID.
    Metric<StartupBirthPid> local_pid;
    std::uint64_t birth_monotonic_ns{};
    std::optional<std::uint64_t> final_monotonic_ns; // Release entry, NOT last-thread exit.
    Metric<std::chrono::nanoseconds> cpu_time; // Release-observed runtime, not guaranteed post-final-scheduler-switch CPU.
    Metric<std::uint64_t> read_bytes,write_bytes; // Group accounting INCLUDES waited-child IO.
    struct Executable {
        std::uint64_t observed_monotonic_ns{},inode{};
        std::uint32_t device_major{},device_minor{},kernel_real_uid{};
        auto operator<=>(const Executable&) const = default;
    };
    Metric<Executable> first_executable; // Historical filesystem identifiers; no path/hash/app/cause proof.
};
struct StartupBirthObserverReport {
    StartupBirthObserverState state{StartupBirthObserverState::idle};
    std::string capture_id;
    std::array<std::uint64_t,2> boot{};
    std::uint64_t pid_namespace_device{},pid_namespace_inode{},quality_flags{};
    std::uint32_t publishing{};
    bool creator_exited{},pid_mapping_enabled{},final_resources_enabled{};
    bool first_exec_enabled{};
    std::string pid_mapping_note;
    ActionError error{ActionError::none};
    std::string message;
    Metric<bool> automatic_startup_origin;
    std::vector<StartupBirthObservation> observations;
};
// Linux optional privileged capture. Construction does nothing. Explicit start
// duplicates borrowed creator pidfd; never elevates, launches, installs or reaps.
// Serialized/poll-driven, no hidden thread/timer. Coordinate forks outside calls;
// inherited references must be closed/exec'd. No atfork magic is promised.
class LinuxStartupBirthObserver {
public:
    LinuxStartupBirthObserver();
    ~LinuxStartupBirthObserver();
    LinuxStartupBirthObserver(const LinuxStartupBirthObserver&)=delete;
    LinuxStartupBirthObserver& operator=(const LinuxStartupBirthObserver&)=delete;
    ActionResult start(bool explicitly_authorized,int borrowed_creator_pidfd,
        std::uint32_t creator_pid,std::uint64_t creator_start_ticks,const StartupBirthObserverOptions&);
    ActionResult poll(); // Snapshot/replay is idempotent; malformed transitions fail closed.
    // Exact retained key only; works after close/reap. Same binding is idempotent;
    // conflicting binding is refused. This is a declaration, not executable/boot-cause proof.
    ActionResult associate(const StartupBirthProcessKey&,const std::string& startup_entry_id);
    void close(); // No hidden final read; retained observations/bindings survive.
    StartupBirthObserverReport report() const; // Copy only, never reads the system.
private:
    friend struct detail::StartupBirthObserverAccess;
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit LinuxStartupBirthObserver(std::unique_ptr<Impl>);
};
}
