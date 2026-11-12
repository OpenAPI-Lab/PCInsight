#pragma once
#include "pcinsight/actions.hpp"
#include <optional>

namespace pcinsight::detail {
struct ServiceJobCompletion { std::string path, unit, result; };
struct ServiceJobTracker {
    std::string expected_path;
    std::vector<ServiceJobCompletion> early;
    std::optional<std::string> result;
    bool invalid{};
    void accept(ServiceJobCompletion completion);
    void select(std::string path);
};
ActionResult systemd_job_result(const std::string& result);
bool valid_systemd_service_name(const std::string& name);
ActionResult control_systemd_service(const std::string& name, ServiceAction action);

// Internal dynamic ABI shared with isolated native integration fixtures.
struct ActionBus; struct ActionMessage; struct ActionSlot;
struct ActionBusError { const char* name{}; const char* message{}; int need_free{}; };
struct SystemdActionApi {
    std::shared_ptr<void> library;
    int (*open)(ActionBus**){};
    ActionBus* (*unref)(ActionBus*){};
    ActionMessage* (*message_unref)(ActionMessage*){};
    ActionSlot* (*slot_unref)(ActionSlot*){};
    void (*error_free)(ActionBusError*){};
    int (*timeout)(ActionBus*, std::uint64_t){};
    int (*call)(ActionBus*, const char*, const char*, const char*, const char*, ActionBusError*, ActionMessage**, const char*, ...){};
    int (*read)(ActionMessage*, const char*, ...){};
    int (*match)(ActionBus*, ActionSlot**, const char*, const char*, const char*, const char*,
        int(*)(ActionMessage*, void*, ActionBusError*), void*){};
    int (*process)(ActionBus*, ActionMessage**){};
    int (*wait)(ActionBus*, std::uint64_t){};
    explicit operator bool() const;
};
SystemdActionApi load_systemd_action_api();
// Borrowed bus, pinned unique destination for scoped callers. Does not close bus.
ActionResult run_systemd_service_job(const SystemdActionApi& api, ActionBus* bus,
    const std::string& destination, const std::string& name, ServiceAction action);
}
