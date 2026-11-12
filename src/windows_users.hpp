#pragma once
#ifdef _WIN32
#include "pcinsight/types.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wtsapi32.h>
namespace pcinsight::detail {
struct WindowsUsersApi {
    decltype(&WTSEnumerateSessionsW) enumerate=&WTSEnumerateSessionsW;
    decltype(&WTSQuerySessionInformationW) query=&WTSQuerySessionInformationW;
    decltype(&WTSFreeMemory) free=&WTSFreeMemory;
};
std::string windows_session_state(WTS_CONNECTSTATE_CLASS);
std::vector<UserSession> read_windows_users(WindowsUsersApi api = {});
}
#endif
