#include <pcinsight/actions.hpp>
#include <pcinsight/app_history.hpp>
#include <pcinsight/metric.hpp>
#include <pcinsight/sampler.hpp>
#include <pcinsight/sdk_headers.hpp>

#include <iostream>
#include <type_traits>
#include <vector>

using namespace pcinsight;

static_assert(std::is_same_v<decltype(AppHistorySample::cpu_time), Metric<std::chrono::nanoseconds>>);
static_assert(std::is_same_v<decltype(AppHistorySample::period_start), std::chrono::system_clock::time_point>);
static_assert(std::is_same_v<decltype(SamplerSample::finished_at), std::chrono::steady_clock::time_point>);
static_assert(std::is_same_v<decltype(DiskInfo::average_response_ms), Metric<double>>);
static_assert(std::is_same_v<decltype(NetworkAdapter::send_bytes_per_second), Metric<std::uint64_t>>);
static_assert(std::is_same_v<decltype(ProcessInfo::linux_start_ticks), Metric<std::uint64_t>>);

// Example for presentation continuity, only within the same known Linux boot,
// PID namespace and sampling stream. Never an authorization check.
static bool same_process_row(const ProcessInfo& a, const ProcessInfo& b) {
    return a.pid != 0 && a.pid == b.pid &&
           a.linux_start_ticks.support == Support::available &&
           b.linux_start_ticks.support == Support::available &&
           metric_is_known(a.linux_start_ticks) && metric_is_known(b.linux_start_ticks) &&
           *a.linux_start_ticks.value != 0 && a.linux_start_ticks.value == b.linux_start_ticks.value;
}

int main() {
    require_matching_sdk_headers();
    int failures = 0;
    const auto check = [&](bool ok, const char* label) {
        if (!ok) { std::cerr << label << '\n'; ++failures; }
    };
    for (const auto support : {Support::available, Support::estimated,
                               Support::permission_required, Support::unsupported}) {
        const bool known = support == Support::available || support == Support::estimated;
        check(metric_is_known(Metric<int>{0, support, {}}) == known, "present zero depends on support");
        check(metric_is_known(Metric<bool>{false, support, {}}) == known, "present false depends on support");
        check(!metric_is_known(Metric<int>{std::nullopt, support, {}}), "absence is never known");
    }
    check(metric_is_known(Metric<std::vector<int>>{std::vector<int>{}, Support::available, {}}),
          "a known empty inventory is not missing");

    ProcessInfo first;
    first.pid = 42;
    first.linux_start_ticks = {1234, Support::available, {}};
    auto next = first;
    next.name = "renamed";
    check(same_process_row(first, next), "a label change need not split a process row");
    next.linux_start_ticks.value = 1235;
    check(!same_process_row(first, next), "reused PID with changed token must split the row");
    next.linux_start_ticks = {1234, Support::estimated, {}};
    check(!same_process_row(first, next), "estimated token cannot establish continuity");
    next.linux_start_ticks = {std::nullopt, Support::available, {}};
    check(!same_process_row(first, next), "missing token cannot establish continuity");

    for (const auto error : {ActionError::none, ActionError::not_supported,
                             ActionError::permission_denied, ActionError::not_found,
                             ActionError::invalid_argument, ActionError::system_error,
                             ActionError::stale_target}) {
        const ActionResult result{error, "fixture", 123, "fixture-backup"};
        check(static_cast<bool>(result) == (error == ActionError::none),
              "diagnostics, PID and backup must not override action error");
    }
    auto actions = make_system_actions();
    const auto invalid = actions->terminate_process_checked(0, 1);
    check(!invalid && invalid.error == ActionError::invalid_argument && invalid.process_id == 0,
          "reserved PID must be rejected before signaling");
    const auto unsupported = actions->set_efficiency_mode(0, true);
    check(!unsupported && unsupported.error == ActionError::not_supported && unsupported.process_id == 0,
          "Linux generic efficiency must not dispatch");
    return failures == 0 ? 0 : 1;
}
