#include "pcinsight/actions.hpp"
#include "pcinsight/startup.hpp"
#include "pcinsight/windows_startup_file_control.hpp"
#include "windows_service_actions.hpp"
#include "windows_launch.hpp"
#include "windows_terminate.hpp"
#include "windows_efficiency.hpp"
#include "linux_service_actions.hpp"
#include "linux_startup.hpp"
#include "linux_launch.hpp"
#include "linux_terminate.hpp"
#include "linux_efficiency.hpp"
#include "linux_session_actions.hpp"
#include "linux_logout.hpp"
#include "linux_startup_override.hpp"

namespace pcinsight {
namespace {
#ifdef __linux__
class LinuxActions final : public SystemActions {
public:
    ActionResult launch_process(const LaunchRequest& r) override {
        return detail::launch_linux_process(r);
    }
    ActionResult terminate_process(std::uint32_t pid,bool force) override {
        return detail::terminate_linux_process(pid, std::nullopt, force);
    }
    ActionResult terminate_process_checked(std::uint32_t pid, std::uint64_t expected, bool force) override {
        return detail::terminate_linux_process(pid, expected, force);
    }
    ActionResult set_efficiency_mode(std::uint32_t pid,bool enabled) override {
        (void)pid; (void)enabled;
        return {ActionError::not_supported,
            "Linux generic efficiency action is disabled; advanced users must explicitly acknowledge "
            "numeric PID/TID reuse risk via apply_linux_efficiency_unchecked; no changes"};
    }
    ActionResult control_service(const std::string& service, ServiceAction action) override {
        return detail::control_systemd_service(service, action);
    }
    ActionResult set_startup_enabled(const std::string& entry, bool enabled) override {
        return detail::set_linux_startup_enabled(entry, enabled);
    }
    ActionResult set_startup_enabled_for_current_user(const std::string& entry, bool enabled) override {
        return detail::set_linux_startup_for_current_user(entry, enabled);
    }
    ActionResult reset_startup_for_current_user(const std::string& entry) override {
        return detail::reset_linux_startup_for_current_user(entry);
    }
    ActionResult disconnect_user(const std::string& session_id) override {
        return detail::disconnect_linux_session(session_id);
    }
    ActionResult terminate_user_session(const std::string& token) override {
        return detail::logout_linux_session(token);
    }
};
#else
class UnsupportedActions final : public SystemActions {
    ActionResult no() { return {ActionError::not_supported,"backend de acciones no implementado para este sistema"}; }
public:
    ActionResult launch_process(const LaunchRequest& request) override{
#ifdef _WIN32
        return detail::launch_windows_process(request);
#else
        (void)request; return no();
#endif
    }
    ActionResult terminate_process(std::uint32_t pid,bool force)override{
#ifdef _WIN32
        return detail::terminate_windows_process(pid, force);
#else
        (void)pid; (void)force; return no();
#endif
    }
    ActionResult terminate_process_checked(std::uint32_t pid,std::uint64_t expected,bool force)override{
#ifdef _WIN32
        return detail::terminate_windows_process_checked(pid, expected, force);
#else
        (void)pid; (void)expected; (void)force; return no();
#endif
    }
    ActionResult set_efficiency_mode(std::uint32_t pid,bool enabled)override{
#ifdef _WIN32
        return detail::set_windows_efficiency(pid,enabled);
#else
        (void)pid; (void)enabled; return no();
#endif
    }
    ActionResult set_efficiency_mode_checked(std::uint32_t pid,std::uint64_t expected,bool enabled)override{
#ifdef _WIN32
        return detail::set_windows_efficiency(pid,enabled,{},expected);
#else
        (void)pid; (void)expected; (void)enabled; return no();
#endif
    }
    ActionResult control_service(const std::string& name,ServiceAction action)override{
#ifdef _WIN32
        return detail::control_windows_service(name, action);
#else
        (void)name; (void)action; return no();
#endif
    }
    ActionResult set_windows_run_registration_enabled(const WindowsRunRegistration& expected,bool enabled) override {
#ifdef _WIN32
        return pcinsight::set_windows_run_registration_enabled(expected,enabled);
#else
        (void)expected;(void)enabled;return no();
#endif
    }
    ActionResult set_startup_enabled(const std::string&,bool)override{return no();} ActionResult disconnect_user(const std::string&)override{return no();}
    ActionResult set_windows_startup_file_registered(const WindowsStartupFileRegistration& expected,bool registered)override{
#ifdef _WIN32
        return pcinsight::set_windows_startup_file_registered(expected,registered);
#else
        (void)expected;(void)registered;return no();
#endif
    }
};
#endif
}
std::unique_ptr<SystemActions> make_system_actions(){
#ifdef __linux__
    return std::make_unique<LinuxActions>();
#else
    return std::make_unique<UnsupportedActions>();
#endif
}
}
