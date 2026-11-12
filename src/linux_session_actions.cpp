#include "linux_session_actions.hpp"
#include <algorithm>
namespace pcinsight {
Metric<bool> query_linux_disconnect_capability(LinuxDisconnectProvider provider) {
    switch (provider) {
    case LinuxDisconnectProvider::utmp:
#ifdef __linux__
        return {false, Support::available,
            "PCInsight utmp adapter: login records are not a session control protocol; "
            "no detach preserving applications. TTY/PID does not identify a remote transport"};
#else
        break;
#endif
    case LinuxDisconnectProvider::login1:
#ifdef __linux__
        return {false, Support::available,
            "PCInsight login1 adapter: documented session API has no detach operation; "
            "Lock/Activate/ReleaseControl/Terminate are not disconnect preserving applications. "
            "Remote does not identify a transport-specific controller"};
#else
        break;
#endif
    default:
        return {{}, Support::unsupported, "Unknown Linux disconnect provider; no capability inferred"};
    }
    return {{}, Support::unsupported, "Built-in Linux disconnect capabilities require a Linux build"};
}
}
namespace pcinsight::detail {
Metric<bool> linux_session_disconnect_capability(const Metric<LinuxSessionDetails>& details) {
    if (!metric_is_known(details))
        return {{}, Support::unsupported, "Linux inventory provider unknown; no disconnect capability inferred"};
    if (details.value->provider == "utmp")
        return query_linux_disconnect_capability(LinuxDisconnectProvider::utmp);
    if (details.value->provider == "login1")
        return query_linux_disconnect_capability(LinuxDisconnectProvider::login1);
    return {{}, Support::unsupported, "Unrecognized Linux inventory provider; no disconnect capability inferred"};
}
ActionResult disconnect_linux_session(const std::string& id) {
    if (id.empty() || id.size() > 4096 || std::any_of(id.begin(), id.end(), [](unsigned char c) {
        return c < 0x20 || c == 0x7f;
    })) return {ActionError::invalid_argument, "session_id vacío, excesivo o con bytes de control"};
    // ut_line, a POSIX session ID and a process group ID are different identities.
    // SIGHUP to -getsid(ut_pid) is neither a verified session action nor a detach
    // preserving applications. Do not guess a target or map to destructive logout.
    return {ActionError::not_supported,
        "los adaptadores Linux utmp/login1 de PCInsight no ofrecen detach que conserva aplicaciones; "
        "un ID o token de logout no identifica un controlador de transporte remoto; "
        "no se enviaron señales, no se bloqueo ni se cerro sesion"};
}
}
