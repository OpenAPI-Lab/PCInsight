#pragma once
#include "linux_process_details.hpp"
namespace pcinsight::detail {
struct LinuxDesktopDescription {std::string program,comment;};
std::optional<LinuxDesktopDescription> parse_linux_desktop_description(std::string_view);
struct LinuxDescriptionCandidate {LinuxExecutableStamp executable,metadata;std::string comment,source;};
using LinuxDescriptionIndex=Metric<std::vector<LinuxDescriptionCandidate>>;
LinuxDescriptionIndex read_linux_description_index(const std::vector<std::filesystem::path>& roots,std::string_view search_path);
LinuxDescriptionIndex read_native_linux_description_index();
Metric<LinuxProcessDescription> lookup_linux_description(const LinuxDescriptionIndex&,const Metric<LinuxExecutableInfo>&);
}
