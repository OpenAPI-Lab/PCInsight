#pragma once
#include "pcinsight/actions.hpp"
#include "pcinsight/linux_session_control.hpp"
namespace pcinsight::detail {
// Uses only a recognized, known inventory provider; never infer from id/TTY/Remote.
Metric<bool> linux_session_disconnect_capability(const Metric<LinuxSessionDetails>&);
// Fail closed: no portable Linux detach provider is currently implemented.
ActionResult disconnect_linux_session(const std::string& session_id);
}
