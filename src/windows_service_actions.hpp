#pragma once
#ifdef _WIN32
#include "pcinsight/actions.hpp"
#include <windows.h>
#include <winsvc.h>
namespace pcinsight::detail {
struct ServiceActionsApi {
    decltype(&OpenSCManagerW) manager = &OpenSCManagerW;
    decltype(&OpenServiceW) open = &OpenServiceW;
    decltype(&QueryServiceStatusEx) query = &QueryServiceStatusEx;
    decltype(&StartServiceW) start = &StartServiceW;
    decltype(&ControlService) control = &ControlService;
    decltype(&CloseServiceHandle) close = &CloseServiceHandle;
    decltype(&GetTickCount64) clock = &GetTickCount64;
    decltype(&Sleep) sleep = &Sleep;
};
ActionResult control_windows_service(const std::string&, ServiceAction, ServiceActionsApi = {});
}
#endif
