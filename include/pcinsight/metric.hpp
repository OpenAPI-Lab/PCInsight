#pragma once
#include <optional>
#include <string>
namespace pcinsight {
enum class Support { available, permission_required, unsupported, estimated };
template <class T> struct Metric {
    std::optional<T> value;
    Support support{Support::unsupported};
    std::string note;
};
// Present zero is known; absence/non-measurement support is not. Never value_or(0).
template<class T> constexpr bool metric_is_known(const Metric<T>& metric) noexcept {
    return metric.value.has_value()&&(metric.support==Support::available||metric.support==Support::estimated);
}
}
