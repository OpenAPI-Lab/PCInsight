#pragma once
#include "pcinsight/types.hpp"
#include <functional>
namespace pcinsight::detail {
WindowsSignatureStatus windows_signature_status(std::int32_t);
using StartupSignatureRead=std::function<Metric<WindowsStartupSignature>(const std::string&)>;
void fill_windows_startup_signatures(std::vector<StartupEntry>&,const StartupSignatureRead&);
void fill_windows_startup_signatures(std::vector<StartupEntry>&);
Metric<WindowsStartupSignature> read_windows_startup_signature(const std::string&);
}
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
namespace pcinsight::detail {
struct WindowsSignatureApi {
    decltype(&WinVerifyTrust) verify=&WinVerifyTrust;
    decltype(&WTHelperProvDataFromStateData) provider=&WTHelperProvDataFromStateData;
    decltype(&WTHelperGetProvSignerFromChain) signer=&WTHelperGetProvSignerFromChain;
    decltype(&CertGetNameStringW) name=&CertGetNameStringW;
};
// Same already-opened file handle is passed to the trust provider; caller owns it.
Metric<WindowsStartupSignature> evaluate_windows_signature(const std::wstring&,HANDLE,const WindowsSignatureApi&);
}
#endif
