#pragma once
#include "pcinsight/history_store.hpp"
namespace pcinsight::detail {
// Shared exclusive-temporary/flush/replace primitive; no format conversion.
HistoryIoResult replace_history_file(const std::filesystem::path&,const std::string&);
}
