#pragma once
#include "pcinsight/types.hpp"
namespace pcinsight::detail {
// service override is internal, for an owned native test service only.
Metric<std::string> read_native_gnome_session_name(const std::string& service="org.gnome.SessionManager");
}
