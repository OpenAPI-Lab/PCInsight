#pragma once
#ifdef _WIN32
#include <string>
namespace pcinsight::detail {
// VERSIONINFO FileDescription, UTF-8. Missing or invalid resources return empty.
std::string windows_file_description(const std::wstring& absolute_path);
// Self-reported VERSIONINFO CompanyName, NOT Authenticode signer/trust.
std::string windows_file_company_name(const std::wstring& absolute_path);
}
#endif
