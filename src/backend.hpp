#pragma once
#include "pcinsight/types.hpp"
#include <memory>

namespace pcinsight::detail {
class Backend { public: virtual ~Backend() = default; virtual SystemSnapshot sample(const SampleOptions&) = 0; };
std::unique_ptr<Backend> make_backend();
}
