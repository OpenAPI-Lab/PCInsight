#include "../src/windows_launch.hpp"
#include <shellapi.h>
#include <cassert>
#include <iostream>
#include <random>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
unsigned creates, closes;
DWORD native_error;
std::wstring application, command, directory;
bool inherited_directory;
BOOL WINAPI create(LPCWSTR app, LPWSTR args, LPSECURITY_ATTRIBUTES process_attributes,
                   LPSECURITY_ATTRIBUTES thread_attributes, BOOL inherit, DWORD flags,
                   LPVOID environment, LPCWSTR cwd, LPSTARTUPINFOW startup, LPPROCESS_INFORMATION output) {
    ++creates;
    assert(app && args && !process_attributes && !thread_attributes && !inherit);
    assert(flags == 0 && !environment && startup->cb == sizeof(STARTUPINFOW));
    assert(startup->dwFlags == 0 && !startup->hStdInput && !startup->hStdOutput && !startup->hStdError);
    application = app; command = args; inherited_directory = !cwd; directory = cwd ? cwd : L"";
    // The native API requires a writable command line.
    args[0] = L'X';
    if (native_error) { SetLastError(native_error); return FALSE; }
    output->hProcess = reinterpret_cast<HANDLE>(1);
    output->hThread = reinterpret_cast<HANDLE>(2);
    output->dwProcessId = 1234;
    return TRUE;
}
BOOL WINAPI close(HANDLE handle) {
    assert(handle == reinterpret_cast<HANDLE>(1) || handle == reinterpret_cast<HANDLE>(2));
    ++closes; return TRUE;
}
LaunchApi api{create, close};
LaunchRequest request() { return {"C:\\Program Files\\PCInsight\\test.exe", {}, "", false}; }
void reset() { creates = closes = native_error = 0; }
ActionResult run(const LaunchRequest& r) {
    auto result = launch_windows_process(r, api);
    assert(closes == (creates && !native_error ? 2u : 0u));
    return result;
}
void roundtrip(const std::vector<std::wstring>& expected) {
    int count = 0;
    auto parsed = CommandLineToArgvW(command.c_str(), &count); // Parser only; executes nothing.
    assert(parsed && count == static_cast<int>(expected.size()) + 1);
    assert(std::wstring(parsed[0]) == application);
    for (std::size_t i = 0; i < expected.size(); ++i) assert(parsed[i + 1] == expected[i]);
    LocalFree(parsed);
}
}
int main() {
    reset(); auto r = request(); auto result = run(r);
    assert(result && result.process_id == 1234 && result.message.find("1234") != std::string::npos);
    assert(creates == 1 && inherited_directory);
    assert(command == L"\"C:\\Program Files\\PCInsight\\test.exe\"");
    reset(); r.arguments = {"", "a b", "a\tb", "a\"b", "trailing\\", "\\\"", "&& %PATH% | >", "caf\xc3\xa9", "\xf0\x9f\x98\x80"};
    r.working_directory = "C:\\work dir";
    assert(run(r)); assert(directory == L"C:\\work dir" && !inherited_directory);
    roundtrip({L"", L"a b", L"a\tb", L"a\"b", L"trailing\\", L"\\\"", L"&& %PATH% | >", L"caf\u00e9", L"\U0001f600"});
    reset(); r = request(); r.arguments = {"", "a\"b", "end\\"}; assert(run(r));
    assert(command == L"\"C:\\Program Files\\PCInsight\\test.exe\" \"\" \"a\\\"b\" \"end\\\\\"");
    // Deterministic mixtures of whitespace, quotes and backslashes.
    std::mt19937 random(1741);
    const std::string alphabet = "ab \\\"\t";
    for (int i = 0; i < 300; ++i) {
        reset(); r = request(); std::vector<std::wstring> expected;
        for (int j = 0; j < 5; ++j) {
            std::string arg;
            const auto length = random() % 70;
            for (unsigned k = 0; k < length; ++k) arg += alphabet[random() % alphabet.size()];
            r.arguments.push_back(arg); expected.emplace_back(arg.begin(), arg.end());
        }
        assert(run(r)); roundtrip(expected);
    }
    for (const auto& path : {std::string{}, std::string("test.exe"), std::string("C:test.exe"),
                            std::string("\\test.exe"), std::string("C:\\bad\"name.exe"),
                            std::string("\\\\server"), std::string("\\\\?\\C:\\test.exe"),
                            std::string("C:\\dir\\"), std::string("C:\\a\0.exe",10), std::string("C:\\\xff")}) {
        reset(); r = request(); r.executable = path;
        assert(run(r).error == ActionError::invalid_argument && !creates);
    }
    reset(); r = request(); r.executable = "\\\\server\\share\\test.exe"; assert(run(r));
    reset(); r = request(); r.executable = "C:/Program Files/test.exe"; assert(run(r));
    for (const auto& arg : {std::string("a\0b",3), std::string("\xc0\xaf"), std::string(32767,'a')}) {
        reset(); r = request(); r.arguments = {arg};
        assert(run(r).error == ActionError::invalid_argument && !creates);
    }
    reset(); r = request(); r.working_directory = "relative";
    assert(run(r).error == ActionError::invalid_argument && !creates);
    reset(); r = request(); r.working_directory = std::string("C:\\a\0b",6);
    assert(run(r).error == ActionError::invalid_argument && !creates);
    reset(); r = request(); r.elevated = true;
    assert(run(r).error == ActionError::permission_denied && !creates);
    reset(); r = request(); r.arguments.resize(11000);
    assert(run(r).error == ActionError::invalid_argument && !creates);
    // Exactly 32766 UTF-16 units is accepted; another unit exceeds the native limit.
    reset(); r = request(); const auto base = r.executable.size() + 2;
    r.arguments = {std::string(32766 - base - 3, 'a')}; assert(run(r)); assert(command.size() == 32766);
    reset(); r.arguments[0] += 'a'; assert(run(r).error == ActionError::invalid_argument && !creates);
    for (auto code : {ERROR_ACCESS_DENIED, ERROR_PRIVILEGE_NOT_HELD, ERROR_ELEVATION_REQUIRED,
                      ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_BAD_EXE_FORMAT,
                      ERROR_INVALID_NAME, ERROR_DIRECTORY, ERROR_NOT_ENOUGH_MEMORY}) {
        reset(); r = request(); native_error = code; result = run(r);
        auto expected = ActionError::system_error;
        if (code == ERROR_ACCESS_DENIED || code == ERROR_PRIVILEGE_NOT_HELD || code == ERROR_ELEVATION_REQUIRED)
            expected = ActionError::permission_denied;
        else if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND) expected = ActionError::not_found;
        else if (code != ERROR_NOT_ENOUGH_MEMORY) expected = ActionError::invalid_argument;
        assert(result.error == expected && result.process_id == 0 && result.message.find(std::to_string(code)) != std::string::npos);
    }
    assert(make_system_actions()->launch_process({}).error == ActionError::invalid_argument);
    std::cout << "Windows launch: injected tests passed; no application launched\n";
}
