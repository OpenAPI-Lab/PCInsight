#pragma once
#include "pcinsight/startup_birth_observer.hpp"
#include "linux_startup_birth.hpp"
#include "linux_network_selection.hpp"
#include <functional>
namespace pcinsight::detail {
// Private deterministic seam. Callbacks/context must outlive observer and must
// not reenter it. Stop must be idempotent/nonthrowing. No native/global override.
struct StartupBirthObserverApi {
    std::function<Metric<NetworkSelectionContext>()> context;
    std::function<Metric<PrivatePidfdNetworkJoin>(bool,int,std::uint32_t,std::uint64_t,std::uint32_t,bool,bool)> start;
    std::function<Metric<StartupBirthRead>()> read;
    std::function<void()> stop;
};
struct StartupBirthObserverAccess {static LinuxStartupBirthObserver make(StartupBirthObserverApi);};
}
