#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
namespace pcinsight::detail {
struct ProcessSnapshotApi {
    decltype(&CreateToolhelp32Snapshot) create = &CreateToolhelp32Snapshot;
    decltype(&Process32FirstW) first = &Process32FirstW;
    decltype(&Process32NextW) next = &Process32NextW;
    decltype(&CloseHandle) close = &CloseHandle;
};
// Complete, validated inventory or an exception; never publishes a partial list.
std::vector<PROCESSENTRY32W> read_windows_process_snapshot(ProcessSnapshotApi = {},
                                                         std::size_t record_limit = 100000);
}
#endif
