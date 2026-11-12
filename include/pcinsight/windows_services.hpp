#pragma once
#include "actions.hpp"
#include "types.hpp"

namespace pcinsight {
struct WindowsServiceView {
    std::string id; // Internal UTF-8 SCM name, not display name or executable.
    Metric<std::string> display_name, description, load_order_group, status;
    Metric<std::uint32_t> native_service_type;
    // Known empty = no publishable PID in the observed SCM state, not PID zero.
    // A PID is NOT a process identity token and cannot authorize checked actions.
    Metric<std::optional<std::uint32_t>> process_id;
};
struct WindowsServiceOpenResult {
    ActionResult result;
    std::optional<WindowsServiceView> view;
};
// Read-only details for the host GUI. No shell/editor, activation or elevation.
// Success has a view; individual metadata fields may remain unavailable.
// Description is SCM's raw text, including unresolved indirect resource strings.
// Fields are separate observations, not an atomic snapshot or an action token.
WindowsServiceOpenResult open_windows_service(const std::string& internal_name);
}
