#pragma once
#include "pcinsight/startup_store.hpp"
namespace pcinsight::detail {
// Shared by archive IO and pure overlay. No file/system reads or authentication.
std::string validate_startup_birth_archive(const StartupBirthArchive&);
}
