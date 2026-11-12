#pragma once
#include "pcinsight/types.hpp"
#include <filesystem>
#include <string_view>
namespace pcinsight::detail {
struct LinuxKvmDevice {std::uint32_t major{},minor{};bool operator==(const LinuxKvmDevice&)const=default;};
enum class LinuxKvmFailure {registration,open,descriptor,identity,query,version,changed};
std::optional<LinuxKvmDevice> parse_linux_kvm_device(std::string_view);
bool linux_kvm_device_matches(bool character_device,LinuxKvmDevice actual,LinuxKvmDevice registered);
Metric<unsigned> linux_kvm_failure(LinuxKvmFailure,int code=0);
Metric<unsigned> linux_kvm_query_result(int version);
Metric<unsigned> read_linux_kvm_api(const std::filesystem::path& device="/dev/kvm",const std::filesystem::path& registration="/sys/class/misc/kvm/dev");
void apply_linux_virtualization(CpuInfo&,Metric<unsigned> kvm_api);
void fill_linux_virtualization(CpuInfo&);
}
