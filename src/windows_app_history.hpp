#pragma once
#include "pcinsight/types.hpp"
#include <map>
namespace pcinsight::detail {
class WindowsAppHistory {
    struct Previous{std::uint64_t created,ticks;std::string path;};
    std::map<std::uint32_t,Previous> previous_;
    std::map<std::string,WindowsAppHistoryEntry> rows_;
    std::chrono::system_clock::time_point started_{},last_{};
public:
    void break_continuity(){previous_.clear();}
    Metric<std::vector<WindowsAppHistoryEntry>> sample(const std::vector<ProcessInfo>&,std::chrono::system_clock::time_point);
    void reset(){previous_.clear();rows_.clear();started_={};last_={};}
};
}
