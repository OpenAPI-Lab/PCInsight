#pragma once
#include "pcinsight/types.hpp"
#include <functional>
namespace pcinsight::detail {
using SelectedServiceQuery=std::function<Metric<std::vector<ServiceInfo>>(const LinuxServiceSelection&)>;
struct LinuxServiceSelectionApi {
    SelectedServiceQuery systemd_system,systemd_user,sysv,runit,s6,openrc_configured,openrc_marked;
};
LinuxServiceSelectionApi native_service_selection_api();
// Explicit route only; legacy handled by existing collector branch, never here.
// Clears stale rows/source on every call. Fixture injection is per-call/private.
void fill_linux_selected_services(SystemSnapshot&,const LinuxServiceSelection&,const LinuxServiceSelectionApi&);
void fill_linux_selected_services(SystemSnapshot&,const LinuxServiceSelection&);
}
