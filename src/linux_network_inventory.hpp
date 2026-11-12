#pragma once
#include "pcinsight/linux_network_inventory.hpp"
#include "linux_network_selection.hpp"
namespace pcinsight::detail {
struct NetworkInventoryObservation {
    LinuxNetworkInventoryState state=LinuxNetworkInventoryState::unavailable;
    Metric<LinuxNetworkSelfIdentity> identity;
};
struct NetworkInventoryApi {
    void* context=nullptr;
    Metric<NetworkSelectionContext> (*observe)(void*)=nullptr;
    Metric<std::uint64_t> (*now)(void*)=nullptr;
    Metric<std::vector<std::uint32_t>> (*enumerate)(void*,const NetworkSelectionContext&)=nullptr;
    NetworkInventoryObservation (*inspect)(void*,std::uint32_t)=nullptr;
    explicit operator bool()const{return observe&&now&&enumerate&&inspect;}
};
NetworkInventoryApi native_network_inventory_api();
Metric<LinuxNetworkProcessInventory> collect_linux_network_inventory(bool,const NetworkInventoryApi&);
}
