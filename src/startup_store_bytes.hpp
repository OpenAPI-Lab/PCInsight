#pragma once
#include "pcinsight/startup_store.hpp"
#include <string_view>
namespace pcinsight::detail {
// Internal shared 16MiB bounded atomic IO/PCISTART memory codec. No activation.
struct StartupBytesLoad {std::optional<std::string> bytes;ActionError error{ActionError::none};std::string message;};
StartupBytesLoad read_startup_archive_bytes(const std::filesystem::path&);
StartupArchiveIoResult write_startup_archive_bytes(const std::filesystem::path&,std::string_view,bool require_private=false);
Metric<std::string> encode_startup_archive_bytes(const StartupImpactArchive&);
StartupArchiveLoadResult decode_startup_archive_bytes(std::string_view);
}
