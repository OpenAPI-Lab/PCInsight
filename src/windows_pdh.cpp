#include "windows_pdh.hpp"
#ifdef _WIN32
#include <pdhmsg.h>
#include <cmath>

namespace pcinsight::detail {
namespace {
Metric<double> failure(const char* operation, PDH_STATUS status) {
    const bool denied=static_cast<DWORD>(status)==PDH_ACCESS_DENIED || status==ERROR_ACCESS_DENIED;
    return {std::nullopt, denied ? Support::permission_required : Support::unsupported,
        std::string(operation)+" failed; PDH status="+std::to_string(static_cast<unsigned long>(status))};
}
}
void WindowsCpuCounter::reset() {
    if (query_) api_.close(query_);
    query_=nullptr; counter_=nullptr; primed_=false;
}
Metric<double> WindowsCpuCounter::sample(unsigned processors, unsigned groups) {
    if (!processors || !groups) {
        reset();
        return {std::nullopt,Support::unsupported,"Processor topology unavailable"};
    }
    if (processors!=processors_ || groups!=groups_) {
        reset(); processors_=processors; groups_=groups;
    }
    if (!query_) {
        auto status=api_.open(nullptr,0,&query_);
        if (status!=ERROR_SUCCESS) { reset(); return failure("PdhOpenQuery",status); }
        status=api_.add(query_,L"\\Processor Information(_Total)\\% Processor Time",0,&counter_);
        if (status!=ERROR_SUCCESS) { reset(); return failure("PdhAddEnglishCounter",status); }
    }
    auto status=api_.collect(query_);
    if (status!=ERROR_SUCCESS) { reset(); return failure("PdhCollectQueryData",status); }
    if (!primed_) {
        primed_=true;
        return {std::nullopt,Support::unsupported,"PDH CPU requires two successful samples"};
    }
    PDH_FMT_COUNTERVALUE value{};
    status=api_.format(counter_,PDH_FMT_DOUBLE|PDH_FMT_NOCAP100,nullptr,&value);
    if (status!=ERROR_SUCCESS) { reset(); return failure("PdhGetFormattedCounterValue",status); }
    if (value.CStatus!=PDH_CSTATUS_VALID_DATA && value.CStatus!=PDH_CSTATUS_NEW_DATA) {
        reset(); return failure("PDH CPU counter data",static_cast<PDH_STATUS>(value.CStatus));
    }
    if (!std::isfinite(value.doubleValue) || value.doubleValue<0 || value.doubleValue>100) {
        reset();
        return {std::nullopt,Support::unsupported,"PDH CPU percentage outside valid range"};
    }
    return {value.doubleValue,Support::available,
        "PDH Processor Information(_Total): all-group busy time, not frequency-weighted utility"};
}
}
#endif
