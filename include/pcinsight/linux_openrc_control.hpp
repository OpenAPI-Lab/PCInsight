#pragma once
#include "actions.hpp"
#include "metric.hpp"
#include <memory>
namespace pcinsight {
// Experimental explicit control only; NEVER used by Monitor/collection.
// Trusted application configuration, not an untrusted request payload.
struct LinuxOpenrcControlConfig {
    bool enabled{};
    std::string runtime_path,init_directory; // Absolute, no symlink/PATH resolution.
    std::uint32_t owner_uid{0};
    bool allow_sticky_ancestors{}; // Default OFF; explicit private fixture exception.
};
struct LinuxOpenrcRunnerStatus {
    bool exited{},timed_out{};
    std::optional<int> exit_code,signal; // Runner exit, NOT service liveness/readiness.
};
class LinuxOpenrcService;
class LinuxOpenrcRunner {
public:
    ~LinuxOpenrcRunner();
    LinuxOpenrcRunner(const LinuxOpenrcRunner&)=delete;
    LinuxOpenrcRunner& operator=(const LinuxOpenrcRunner&)=delete;
    // 0..30000ms; timeout does NOT cancel. Retain runner until it is reaped.
    Metric<LinuxOpenrcRunnerStatus> wait(std::uint32_t timeout_ms=1000);
    // Signals ONLY the runner pidfd, NOT the service, group or dependencies.
    ActionResult terminate_runner(bool force,bool explicitly_requested);
    std::uint32_t process_id()const; // Diagnostic only; never signal by this PID.
private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit LinuxOpenrcRunner(std::unique_ptr<Impl>);
    friend class LinuxOpenrcService;
};
struct LinuxOpenrcControlResult {
    ActionResult result{ActionError::not_supported,"No OpenRC request dispatched"};
    bool runtime_may_have_executed{}; // Conservative; no automatic retry/rollback.
    std::shared_ptr<LinuxOpenrcRunner> runner; // Must reap even after uncertain error.
};
// Selection reads/pins metadata only, never starts the runtime. It is not a
// catalog index, root-helper authorization ticket or proof of installed OpenRC.
// One dispatch per selection once a child is returned; reopen for a new request.
class LinuxOpenrcService {
public:
    static Metric<std::shared_ptr<LinuxOpenrcService>> open(const LinuxOpenrcControlConfig&,const std::string& service_name);
    ~LinuxOpenrcService();
    LinuxOpenrcService(const LinuxOpenrcService&)=delete;
    LinuxOpenrcService& operator=(const LinuxOpenrcService&)=delete;
    std::string name()const; // Copy, no IO.
    void close(); // Revoke future requests; never cancel an already returned runner.
    LinuxOpenrcControlResult request(ServiceAction,bool explicitly_requested);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit LinuxOpenrcService(std::unique_ptr<Impl>);
};
// No elevation/install/CLI discovery/shell fallback. Runtime executes with the
// caller's existing credentials, fixed minimal environment and /dev/null stdio.
// Trusted owners serialize script/config/runtime changes; no atomic snapshot or
// sandbox. Coordinate forks/namespaces/credentials/FDs/environment/reapers and
// trusted pthread_atfork callbacks. Destructors close FDs, NEVER kill/reap live
// runners; abandoning one can leave a zombie. Real OpenRC acceptance is pending.
}
