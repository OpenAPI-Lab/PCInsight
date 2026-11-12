// Manual, explicitly authorized native SCM test. NOT a CTest test.
#include "pcinsight/actions.hpp"
#include "../src/windows_services.hpp"
#include <shlobj.h>
#include <sddl.h>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {
SERVICE_STATUS_HANDLE status_handle{};
SERVICE_STATUS service_status{};
SRWLOCK status_lock = SRWLOCK_INIT;
HANDLE stop_event{};
std::wstring service_name;

void report(DWORD state) {
    AcquireSRWLockExclusive(&status_lock);
    service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    service_status.dwCurrentState = state;
    service_status.dwControlsAccepted = state == SERVICE_RUNNING ? SERVICE_ACCEPT_STOP : 0;
    service_status.dwWin32ExitCode = NO_ERROR;
    service_status.dwCheckPoint = state == SERVICE_STOP_PENDING ? 1 : 0;
    service_status.dwWaitHint = state == SERVICE_STOP_PENDING ? 5000 : 0;
    SetServiceStatus(status_handle, &service_status);
    ReleaseSRWLockExclusive(&status_lock);
}
void WINAPI handler(DWORD control) {
    if (control == SERVICE_CONTROL_STOP) {
        report(SERVICE_STOP_PENDING);
        SetEvent(stop_event);
    } else if (control == SERVICE_CONTROL_INTERROGATE) {
        AcquireSRWLockShared(&status_lock);
        SetServiceStatus(status_handle, &service_status);
        ReleaseSRWLockShared(&status_lock);
    }
}
void WINAPI service_main(DWORD, LPWSTR*) {
    stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!stop_event) return;
    status_handle = RegisterServiceCtrlHandlerW(service_name.c_str(), handler);
    if (!status_handle) { CloseHandle(stop_event); return; }
    report(SERVICE_RUNNING);
    // Even if the controller crashes, this private process exits after 60s.
    WaitForSingleObject(stop_event, 60000);
    report(SERVICE_STOPPED);
    // SCM no longer calls the handler after the final stopped report.
    CloseHandle(stop_event);
}
struct ScHandle {
    SC_HANDLE value{};
    ~ScHandle() { if (value) CloseServiceHandle(value); }
};
struct FileHandle {
    HANDLE value{INVALID_HANDLE_VALUE};
    ~FileHandle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
void require(bool ok, const char* what) {
    if (!ok) throw std::runtime_error(std::string(what) + " Win32=" + std::to_string(GetLastError()));
}
void log(HANDLE out, const std::string& message) {
    const auto line = message + "\r\n";
    DWORD written{};
    WriteFile(out, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
    FlushFileBuffers(out);
}
SERVICE_STATUS_PROCESS query(SC_HANDLE service) {
    SERVICE_STATUS_PROCESS result{};
    DWORD size{};
    require(QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
        reinterpret_cast<BYTE*>(&result), sizeof result, &size) && size == sizeof result,
        "query private service");
    return result;
}
bool cleanup(SC_HANDLE service, HANDLE out) {
    // This handle exists ONLY after successful exclusive CreateServiceW.
    // Never reopen/stop/delete a service whose creation failed.
    bool stopped = false;
    const auto began = GetTickCount64();
    for (;;) {
        SERVICE_STATUS_PROCESS s{};
        DWORD size{};
        if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
            reinterpret_cast<BYTE*>(&s), sizeof s, &size)) break;
        if (s.dwCurrentState == SERVICE_STOPPED) { stopped = true; break; }
        if (s.dwCurrentState == SERVICE_RUNNING) {
            SERVICE_STATUS ignored{};
            ControlService(service, SERVICE_CONTROL_STOP, &ignored);
        }
        if (GetTickCount64() - began >= 70000) break;
        Sleep(100);
    }
    const bool deleted = DeleteService(service) != FALSE;
    log(out, std::string("cleanup stopped=") + (stopped ? "yes" : "NO") +
        " delete_requested=" + (deleted ? "yes" : "NO") +
        (deleted ? "" : " error=" + std::to_string(GetLastError())));
    return stopped && deleted;
}
int run(const std::wstring& exe) {
    // Unique, create-new evidence file: no overwrite of existing files.
    const auto suffix = std::to_wstring(GetCurrentProcessId()) + L"_" + std::to_wstring(GetTickCount64());
    const auto output = exe + L".result_" + suffix + L".txt";
    FileHandle evidence{CreateFileW(output.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (evidence.value == INVALID_HANDLE_VALUE) return 2;
    ScHandle manager, service;
    FileHandle pin;
    FileHandle service_pin;
    std::wstring private_directory, private_executable;
    bool directory_created = false, executable_copied = false;
    bool passed = false, cleaned = true;
    std::string id;
    try {
        // Keep the executable pinned against ordinary replacement while SCM runs it.
        pin.value = CreateFileW(exe.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(pin.value != INVALID_HANDLE_VALUE, "pin own executable");
        service_name = L"PCInsight_Private_SCM_" + suffix;
        // Fixed ASCII prefix and decimal PID/tick suffix, not arbitrary Unicode.
        for (const auto ch : service_name) id.push_back(static_cast<char>(ch));
        log(evidence.value, "private_service=" + id);
        manager.value = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
        require(manager.value != nullptr, "SCM create permission (administrator consent required)");
        // LocalService cannot execute from a protected user profile. Use an
        // exclusively created, administrator-owned directory, never relax the
        // user's folder ACLs or run a user-writable binary as LocalSystem.
        PWSTR program_data{};
        require(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &program_data)), "ProgramData");
        private_directory = std::wstring(program_data) + L"\\" + service_name;
        CoTaskMemFree(program_data);
        PSECURITY_DESCRIPTOR descriptor{};
        require(ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;GRGX;;;LS)",
            SDDL_REVISION_1, &descriptor, nullptr) != FALSE, "private ACL");
        SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), descriptor, FALSE};
        directory_created = CreateDirectoryW(private_directory.c_str(), &attributes) != FALSE;
        const auto directory_error = GetLastError();
        LocalFree(descriptor);
        if (!directory_created) { SetLastError(directory_error); require(false, "exclusive private directory"); }
        private_executable = private_directory + L"\\native-test.exe";
        executable_copied = CopyFileW(exe.c_str(), private_executable.c_str(), TRUE) != FALSE;
        require(executable_copied, "private executable copy");
        service_pin.value = CreateFileW(private_executable.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(service_pin.value != INVALID_HANDLE_VALUE, "pin service executable");
        log(evidence.value, "private executable: protected ProgramData directory, LocalService RX only");
        const auto command = L"\"" + private_executable + L"\" --private-service " + service_name;
        service.value = CreateServiceW(manager.value, service_name.c_str(), service_name.c_str(),
            SERVICE_QUERY_STATUS | SERVICE_STOP | SERVICE_CHANGE_CONFIG | DELETE,
            SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
            command.c_str(), nullptr, nullptr, nullptr, L"NT AUTHORITY\\LocalService", nullptr);
        require(service.value != nullptr, "exclusive CreateService LocalService");
        log(evidence.value, "created LocalService demand-start, no dependencies");
        wchar_t description_text[] = L"PCInsight disposable native SCM test";
        SERVICE_DESCRIPTIONW description{description_text};
        require(ChangeServiceConfig2W(service.value, SERVICE_CONFIG_DESCRIPTION, &description) != FALSE,
            "private description");
        const auto inventory = [&](DWORD expected) {
            bool found = false;
            for (const auto& item : pcinsight::detail::read_windows_services()) {
                if (item.id != id) continue;
                found = true;
                require(item.description == "PCInsight disposable native SCM test", "inventory description");
                require(item.group.empty(), "inventory no load-order group");
                require(item.status == (expected == SERVICE_RUNNING ? "running" : "stopped"), "inventory state");
                const auto native = query(service.value);
                require(expected == SERVICE_RUNNING ? item.pid == native.dwProcessId && item.pid.value_or(0) != 0 : !item.pid,
                    "inventory PID");
            }
            require(found, "private inventory row");
        };
        auto actions = pcinsight::make_system_actions();
        const auto action = [&](pcinsight::ServiceAction operation, DWORD expected, const char* label) {
            const auto result = actions->control_service(id, operation);
            if (!result) throw std::runtime_error(std::string(label) + ": " + result.message);
            require(query(service.value).dwCurrentState == expected, label);
            inventory(expected);
            log(evidence.value, std::string("PASS ") + label);
        };
        inventory(SERVICE_STOPPED);
        action(pcinsight::ServiceAction::stop, SERVICE_STOPPED, "stop already stopped");
        action(pcinsight::ServiceAction::start, SERVICE_RUNNING, "start native");
        const auto first_pid = query(service.value).dwProcessId;
        action(pcinsight::ServiceAction::start, SERVICE_RUNNING, "start already running");
        require(query(service.value).dwProcessId == first_pid, "idempotent start kept process");
        action(pcinsight::ServiceAction::restart, SERVICE_RUNNING, "restart running native");
        action(pcinsight::ServiceAction::stop, SERVICE_STOPPED, "stop native");
        action(pcinsight::ServiceAction::restart, SERVICE_RUNNING, "restart stopped native");
        action(pcinsight::ServiceAction::stop, SERVICE_STOPPED, "final stop native");
        passed = true;
        // Keep pin live through cleanup, not merely through test operations.
        cleaned = cleanup(service.value, evidence.value);
    } catch (const std::exception& error) {
        log(evidence.value, std::string("FAIL ") + error.what());
        if (service.value) cleaned = cleanup(service.value, evidence.value);
    }
    if (service.value) {
        CloseServiceHandle(service.value);
        service.value = nullptr;
        ScHandle remaining{OpenServiceW(manager.value, service_name.c_str(), SERVICE_QUERY_STATUS)};
        const auto error = GetLastError();
        const bool removed = !remaining.value && error == ERROR_SERVICE_DOES_NOT_EXIST;
        log(evidence.value, removed ? "PASS registration removed" : "FAIL registration still present/unknown");
        cleaned = cleaned && removed;
    }
    if (service_pin.value != INVALID_HANDLE_VALUE) {
        CloseHandle(service_pin.value);
        service_pin.value = INVALID_HANDLE_VALUE;
    }
    if (executable_copied) {
        bool removed = false;
        const auto began = GetTickCount64();
        do {
            removed = DeleteFileW(private_executable.c_str()) != FALSE;
            if (removed || GetTickCount64() - began >= 5000) break;
            Sleep(100);
        } while (true);
        log(evidence.value, removed ? "PASS private executable removed" : "FAIL private executable remains");
        cleaned = cleaned && removed;
    }
    if (directory_created) {
        const bool removed = RemoveDirectoryW(private_directory.c_str()) != FALSE;
        log(evidence.value, removed ? "PASS private directory removed" : "FAIL private directory remains");
        cleaned = cleaned && removed;
    }
    log(evidence.value, passed && cleaned ? "RESULT PASS" : "RESULT FAIL");
    return passed && cleaned ? 0 : 1;
}
}
int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--private-service") {
        const std::string name = argv[2];
        if (!name.starts_with("PCInsight_Private_SCM_") || name.size() > 100 ||
            name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") != std::string::npos)
            return 2;
        service_name.assign(name.begin(), name.end());
        SERVICE_TABLE_ENTRYW table[]{{service_name.data(), service_main}, {nullptr, nullptr}};
        return StartServiceCtrlDispatcherW(table) ? 0 : 2;
    }
    if (argc != 2 || std::string(argv[1]) != "--run-private-service-test") {
        std::puts("Manual only: --run-private-service-test requires explicit consent and admin rights.");
        return 2;
    }
    wchar_t path[32768]{};
    const auto length = GetModuleFileNameW(nullptr, path, 32768);
    if (!length || length >= 32768) return 2;
    return run(std::wstring(path, length));
}
