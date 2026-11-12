#pragma once
#include "pcinsight/startup_recorder.hpp"
#include "pcinsight/startup_candidates.hpp"
#include "linux_manual_store.hpp"
#include <functional>
namespace pcinsight::detail {
// Successful empty optional means pending/outside coverage, NEVER zero bytes.
// Reader lifetime owns selected probes; destruction removes them.
struct StartupExitResourceTotals {
    std::uint64_t disk_bytes{};
    Metric<std::chrono::nanoseconds> scheduler_cpu_time;
    StartupExitResourceTotals(std::uint64_t bytes=0):disk_bytes(bytes){}
};
using StartupExitDiskReader=std::function<Metric<std::optional<StartupExitResourceTotals>>()>;
Metric<StartupExitDiskReader> make_native_startup_exit_disk_reader(const StartupProcessSample&);
// Closures own the observer. close must be idempotent/nonthrowing and must not poll.
struct StartupRecorderBirthSource {
    std::function<ActionResult()> poll;
    std::function<StartupBirthObserverReport()> report;
    std::function<ActionResult(const StartupBirthProcessKey&,const std::string&)> associate;
    std::function<void()> close;
};
struct StartupRecorderBirthResult {std::optional<StartupRecorderBirthSource> source;ActionResult result;};
using StartupRecorderBirthFactory=std::function<StartupRecorderBirthResult(bool,int,std::uint32_t,std::uint64_t,const StartupBirthObserverOptions&)>;
struct StartupRecorderManualOptions {std::string capture_id;std::chrono::nanoseconds window{std::chrono::seconds(60)};StartupImpactPolicy policy{};};
// Factory does not capture. start returns any released PID without later throws.
// close must not read/signal/reap. Private callbacks must not reenter the recorder.
struct StartupRecorderManualSource {
    std::function<ActionResult(const std::string&,bool,bool,const std::string&)> start;
    std::function<Metric<LinuxDesktopResourceReport>(std::chrono::nanoseconds,const StartupImpactPolicy&)> finish;
    std::function<void()> close;
};
StartupRecorderManualSource make_native_startup_recorder_manual_source();
StartupRecorderBirthResult make_native_startup_recorder_birth_source(bool,int,std::uint32_t,std::uint64_t,const StartupBirthObserverOptions&);
// Private deterministic seam. Callbacks run under the recorder mutex, must not
// reenter it, and borrowed captures must outlive it. No global/native override.
struct StartupRecorderApi {
    std::function<StartupProcessSampleResult()> context;
    std::function<SystemSnapshot()> snapshot;
    std::function<StartupCandidateReport(const SystemSnapshot&)> candidates;
    StartupImpactSampleReader sample;
    std::function<std::chrono::steady_clock::time_point()> now;
    std::function<std::chrono::system_clock::time_point()> wall;
    std::function<Metric<StartupExitDiskReader>(const StartupProcessSample&)> exit_disk;
    StartupImpactSampleReader scheduler_sample;
    StartupRecorderBirthFactory birth;
    std::function<StartupRecorderManualSource()> manual;
};
struct StartupRecorderAccess {
    static LinuxStartupRecorder make(StartupRecorderApi);
    static ActionResult start_manual(LinuxStartupRecorder&,const std::string& entry_id,bool launch_consent,bool capture_consent,const StartupRecorderManualOptions&);
    static ActionResult finish_manual(LinuxStartupRecorder&);
    static Metric<LinuxManualLaunchArchive> manual_archive(const LinuxStartupRecorder&); // Historical copy, no live authority.
    static StartupArchiveIoResult save_manual(const LinuxStartupRecorder&,const std::filesystem::path&);
};
}
