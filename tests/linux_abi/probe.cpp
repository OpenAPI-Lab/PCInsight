#include <pcinsight/account_management.hpp>
#include <pcinsight/app_history.hpp>
#include <pcinsight/linux_notifications.hpp>
#include <pcinsight/linux_network_addresses.hpp>
#include <pcinsight/linux_openrc_control.hpp>
#include <pcinsight/linux_services.hpp>
#include <pcinsight/sampler.hpp>
#include <pcinsight/sdk_headers.hpp>
#include <pcinsight/startup.hpp>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <type_traits>

using namespace pcinsight;

// Source contracts independent of the observed layout baseline. Never invoke
// an action or start a provider to check a signature.
static_assert(std::is_same_v<decltype(&Monitor::sample),SystemSnapshot(Monitor::*)(const SampleOptions&)>);
static_assert(std::is_same_v<decltype(&Sampler::timed_history),std::vector<SamplerSample>(Sampler::*)() const>);
static_assert(std::is_same_v<decltype(&SystemActions::launch_process),ActionResult(SystemActions::*)(const LaunchRequest&)>);
static_assert(std::is_same_v<decltype(&SystemActions::terminate_process_checked),ActionResult(SystemActions::*)(std::uint32_t,std::uint64_t,bool)>);
static_assert(std::is_same_v<decltype(&SystemActions::terminate_user_session),ActionResult(SystemActions::*)(const std::string&)>);
static_assert(std::is_same_v<decltype(&control_linux_service),ActionResult(*)(const LinuxServiceTarget&,ServiceAction)>);
static_assert(std::is_same_v<decltype(&get_startup_properties),StartupPropertiesResult(*)(const std::string&)>);
static_assert(std::is_same_v<decltype(&LinuxOpenrcService::request),LinuxOpenrcControlResult(LinuxOpenrcService::*)(ServiceAction,bool)>);
static_assert(std::is_same_v<decltype(&AccountManagementIntegration::request_open),AccountManagementOpenResult(AccountManagementIntegration::*)() const>);
static_assert(std::is_same_v<decltype(CpuInfo::uptime),Metric<std::chrono::seconds>>);
static_assert(std::is_same_v<decltype(AppHistorySample::cpu_time),Metric<std::chrono::nanoseconds>>);
static_assert(std::is_same_v<decltype(ProcessInfo::linux_start_ticks),Metric<std::uint64_t>>);
static_assert(std::is_same_v<decltype(DiskInfo::read_bytes_per_second),Metric<std::uint64_t>>);
static_assert(std::is_same_v<decltype(LinuxServiceDetails::main_pid),Metric<std::uint32_t>>);
static_assert(std::is_same_v<decltype(UserSession::logout_token),std::string>);
static_assert(std::is_same_v<decltype(&query_linux_network_addresses),Metric<std::vector<LinuxInterfaceAddressList>>(*)()>);

namespace {
std::string profile() {
#if defined(__linux__) && defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__)) && defined(__GLIBCXX__) && defined(_GLIBCXX_USE_CXX11_ABI)
    std::ostringstream out;
#ifdef __clang__
    out << "profile linux-x86_64 clang=" << __clang_major__ << '.' << __clang_minor__ << '.' << __clang_patchlevel__;
#else
    out << "profile linux-x86_64 gcc=" << __GNUC__ << '.' << __GNUC_MINOR__ << '.' << __GNUC_PATCHLEVEL__;
#endif
    out << " libstdc++=" << __GLIBCXX__ << " cxx11abi=" << _GLIBCXX_USE_CXX11_ABI
        << " cxx=" << __cplusplus << " ptr=" << sizeof(void*)
#ifdef _GLIBCXX_DEBUG
        << " debug=1"
#else
        << " debug=0"
#endif
#ifdef _GLIBCXX_ASSERTIONS
        << " assertions=1";
#else
        << " assertions=0";
#endif
    return out.str();
#else
    return {};
#endif
}

std::string layout() {
    std::ostringstream out;
#define ROW(type) out << #type << ' ' << sizeof(type) << ' ' << alignof(type) << '\n'
    ROW(std::string); ROW(Metric<bool>); ROW(Metric<std::uint64_t>);
    ROW(SampleOptions); ROW(SystemSnapshot); ROW(Monitor); ROW(Sampler); ROW(SamplerSample);
    ROW(CpuInfo); ROW(MemoryInfo); ROW(DiskInfo); ROW(NetworkAdapter); ROW(GpuInfo);
    ROW(ProcessInfo); ROW(ServiceInfo); ROW(StartupEntry); ROW(UserSession);
    ROW(LinuxServiceTarget); ROW(LinuxServiceView); ROW(LinuxOpenrcControlConfig); ROW(LinuxOpenrcControlResult);
    ROW(AppHistorySample); ROW(LinuxNotificationReport); ROW(StartupPropertiesResult);
    ROW(ActionError); ROW(ActionResult); ROW(LaunchRequest); ROW(AccountManagementOpenResult);
#undef ROW
#if defined(__linux__) && defined(__GLIBCXX__) && !defined(_GLIBCXX_DEBUG)
    static_assert(std::is_standard_layout_v<ActionResult>);
    static_assert(std::is_standard_layout_v<LaunchRequest>);
#define OFFSET(type, field) out << #type "." #field " offset " << offsetof(type, field) << '\n'
    OFFSET(ActionResult, error);
    OFFSET(ActionResult, message);
    OFFSET(ActionResult, process_id);
    OFFSET(ActionResult, startup_backup_path);
    OFFSET(LaunchRequest, executable);
    OFFSET(LaunchRequest, arguments);
    OFFSET(LaunchRequest, working_directory);
    OFFSET(LaunchRequest, elevated);
#undef OFFSET
#endif
    out << "LinuxInterfaceAddressList " << sizeof(LinuxInterfaceAddressList) << ' ' << alignof(LinuxInterfaceAddressList) << '\n';
    return out.str();
}

bool read_line(std::istream& in,std::string& line) {
    if(!std::getline(in,line))return false;
    if(line.ends_with('\r'))line.pop_back(); // Accept both LF and CRLF checkouts.
    return true;
}
}

int main(int argc, char** argv) {
    require_matching_sdk_headers();
    const auto current = profile();
    if(current.empty()) { std::cerr << "Unsupported baseline profile\n"; return 2; }
    if(argc==2 && std::string_view(argv[1])=="--dump") { std::cout<<current<<'\n'<<layout();return 0; }
    if(argc!=2) { std::cerr<<"Usage: linux-abi-probe BASELINE or --dump\n";return 2; }
    std::ifstream in(argv[1]);
    if(!in) { std::cerr<<"Cannot read baseline\n";return 2; }
    std::string expected;
    if(!read_line(in,expected)) { std::cerr<<"Empty baseline\n";return 2; }
    if(expected!=current) { std::cerr<<"Profile mismatch: "<<current<<'\n';return 2; }
    std::istringstream actual(layout());
    for(std::string row;std::getline(actual,row);) {
        if(!read_line(in,expected)||row!=expected) {
            std::cerr<<"Layout mismatch: "<<row<<" (expected "<<expected<<")\n";return 1;
        }
    }
    if(read_line(in,expected)||in.bad()||!in.eof()) { std::cerr<<"Unexpected trailing baseline data or read error\n";return 1; }
    std::cout<<"Selected size/alignment baseline matches\n";
    return 0;
}
