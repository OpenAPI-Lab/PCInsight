#pragma once
#include "pcinsight/types.hpp"
#include <functional>
namespace pcinsight::detail {
using StartupCompanyRead=std::function<Metric<std::string>(const std::string&)>;
std::string windows_startup_executable_candidate(const StartupEntry&);
void fill_windows_startup_publishers(std::vector<StartupEntry>&,const StartupCompanyRead&);
void fill_windows_startup_publishers(std::vector<StartupEntry>&);
Metric<std::string> read_windows_startup_company(const std::string&);
}
