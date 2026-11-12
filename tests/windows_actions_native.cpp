// Only launches/changes/terminates THIS executable's private sleeping child.
#include "pcinsight/actions.hpp"
#include "../src/windows_efficiency.hpp"
#include <iostream>
#include <stdexcept>
using namespace pcinsight;
namespace {
void check(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error(message + " Win32=" + std::to_string(GetLastError()));
}
std::string utf8(const std::wstring& value) {
    const auto size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    check(size > 0, "convert path");
    std::string out(size, '\0');
    check(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), out.data(), size, nullptr, nullptr) != 0, "convert path");
    return out;
}
struct Handle {
    HANDLE value{};
    ~Handle() { if (value) CloseHandle(value); }
};
struct Child : Handle {
    ~Child() {
        if (value && WaitForSingleObject(value, 0) == WAIT_TIMEOUT) {
            // Exception cleanup of our pinned private child only, never PID lookup.
            TerminateProcess(value, 3);
            WaitForSingleObject(value, 5000);
        }
    }
};
}
int main(int argc, char** argv) {
    try {
        if (argc > 1 && std::string(argv[1]) == "--native-action-child") {
            if (argc != 7 || std::string(argv[3]) != "space value" || std::string(argv[4]) != "" ||
                std::string(argv[5]) != "quote\"and\\" || std::string(argv[6]) != "ascii-end") return 2;
            const std::string name = argv[2];
            if (!name.starts_with("Local\\PCInsight_Private_Action_")) return 2;
            Handle ready{OpenEventA(EVENT_MODIFY_STATE, FALSE, name.c_str())};
            if (!ready.value || !SetEvent(ready.value)) return 2;
            Sleep(15000); // Bounded fallback if controller crashes.
            return 3;
        }
        check(argc == 1, "unexpected harness arguments");
        wchar_t path[32768]{};
        const auto length = GetModuleFileNameW(nullptr, path, 32768);
        check(length && length < 32768, "own executable path");
        const auto event_name = "Local\\PCInsight_Private_Action_" + std::to_string(GetCurrentProcessId()) + "_" + std::to_string(GetTickCount64());
        Handle ready{CreateEventA(nullptr, TRUE, FALSE, event_name.c_str())};
        const auto event_error = GetLastError();
        check(ready.value && event_error != ERROR_ALREADY_EXISTS, "exclusive private event");
        auto actions = make_system_actions();
        LaunchRequest request;
        request.executable = utf8(std::wstring(path, length));
        request.arguments = {"--native-action-child", event_name, "space value", "", "quote\"and\\", "ascii-end"};
        const auto launched = actions->launch_process(request);
        check(static_cast<bool>(launched) && launched.process_id > 4, "launch private child: " + launched.message);
        Child child;
        child.value = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE, FALSE, launched.process_id);
        check(child.value != nullptr, "open own private child");
        check(WaitForSingleObject(ready.value, 5000) == WAIT_OBJECT_0, "child acknowledged exact arguments");
        FILETIME created{}, exited{}, kernel{}, user{};
        check(GetProcessTimes(child.value, &created, &exited, &kernel, &user) != FALSE, "child creation token");
        const auto token = (std::uint64_t(created.dwHighDateTime) << 32) | created.dwLowDateTime;
        check(token != 0, "nonzero child token");
        check(actions->set_efficiency_mode_checked(launched.process_id, token ^ 1, true).error == ActionError::stale_target,
            "stale EcoQoS rejected");
        check(actions->terminate_process_checked(launched.process_id, token ^ 1, true).error == ActionError::stale_target,
            "stale termination rejected");
        check(WaitForSingleObject(child.value, 0) == WAIT_TIMEOUT, "stale actions kept child alive");
        const auto enabled = actions->set_efficiency_mode_checked(launched.process_id, token, true);
        check(static_cast<bool>(enabled), "enable native EcoQoS: " + enabled.message);
        auto policy = detail::read_windows_efficiency(child.value);
        check(policy.requested.value == true, "native EcoQoS bit on");
        const auto disabled = actions->set_efficiency_mode_checked(launched.process_id, token, false);
        check(static_cast<bool>(disabled), "disable native EcoQoS: " + disabled.message);
        policy = detail::read_windows_efficiency(child.value);
        check(policy.requested.value == false, "native EcoQoS bit off");
        const auto terminated = actions->terminate_process_checked(launched.process_id, token, true);
        check(static_cast<bool>(terminated), "end own child: " + terminated.message);
        check(WaitForSingleObject(child.value, 0) == WAIT_OBJECT_0, "same child handle signaled");
        DWORD exit_code{};
        check(GetExitCodeProcess(child.value, &exit_code) && exit_code == 1, "checked end-task exit code");
        std::cout << "Native actions PASS: exact argv acknowledged; stale tokens rejected; EcoQoS on/off verified; checked end-task exit=1; child cleaned\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Native private actions FAIL: " << error.what() << '\n';
        return 1;
    }
}
