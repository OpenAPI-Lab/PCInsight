#pragma once
#include "types.hpp"
namespace pcinsight {
struct WindowsNetworkIdentity {
    std::string adapter_id, interface_guid;
    Metric<std::string> manufacturer, device_description;
};
// Read-only PnP/INF-reported manufacturer, not a verified hardware brand.
// Pass NetworkAdapter.id unchanged (luid:<decimal>), from this Windows host.
// Logical interfaces without a unique PnP match keep metadata unavailable.
Metric<WindowsNetworkIdentity> query_windows_network_identity(const std::string& adapter_id);
}
