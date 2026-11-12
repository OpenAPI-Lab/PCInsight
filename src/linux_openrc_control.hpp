#pragma once
#include "pcinsight/linux_openrc_control.hpp"
#include <optional>
namespace pcinsight::detail {
struct OpenrcControlOptions {
    bool enabled{},explicitly_requested{},allow_sticky_ancestors{};
    std::string runtime_path,script_path; // Mechanism trusted, never catalog index/PATH.
    std::uint32_t owner_uid{0};
};
using OpenrcRunnerStatus=LinuxOpenrcRunnerStatus;
struct OpenrcControlSelection;
struct OpenrcControlDispatch;
class OpenrcControlChild {
public:
    struct Impl;
    ~OpenrcControlChild();
    Metric<OpenrcRunnerStatus> wait(std::uint32_t timeout_ms=1000);
    ActionResult terminate_runner(bool force,bool explicitly_requested);
    std::uint32_t process_id()const;
private:
    explicit OpenrcControlChild(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
    friend struct OpenrcControlDispatch;
    friend OpenrcControlDispatch dispatch_openrc_control(const OpenrcControlOptions&,ServiceAction);
    friend OpenrcControlDispatch dispatch_openrc_control_selected(const std::shared_ptr<OpenrcControlSelection>&,ServiceAction,bool);
};
struct OpenrcControlDispatch {
    ActionResult result{ActionError::not_supported,"OpenRC control disabled"};
    bool runtime_may_have_executed{};
    std::shared_ptr<OpenrcControlChild> child;
};
// INTERNAL, no collection integration. No privilege elevation or shell fallback.
// Retain any returned child and wait/reap it, even if result reports uncertainty.
// Serialize fork/credentials/environment/FD mutation and external child reapers.
OpenrcControlDispatch dispatch_openrc_control(const OpenrcControlOptions&,ServiceAction);
Metric<std::shared_ptr<OpenrcControlSelection>> select_openrc_control(const OpenrcControlOptions&);
void revoke_openrc_control(const std::shared_ptr<OpenrcControlSelection>&);
OpenrcControlDispatch dispatch_openrc_control_selected(const std::shared_ptr<OpenrcControlSelection>&,ServiceAction,bool);
}
