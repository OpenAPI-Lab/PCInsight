#pragma once
#include "actions.hpp"
#include "types.hpp"
#include <functional>

namespace pcinsight {
struct AccountManagementOpenResult {
    ActionResult action;
    // True once the host opener was invoked, including errors/exceptions.
    // Such failures may already have opened UI: do not retry blindly.
    bool host_invoked{};
    explicit operator bool() const {return host_invoked&&static_cast<bool>(action);}
};
// Explicit GUI-host integration, not an account database/editor. No global
// registration or guessed desktop commands. Default has no provider.
// Trusted callbacks run synchronously on the calling thread. Host owns GUI
// marshalling, callback lifetimes/thread safety and any launched child reaping.
// Query must be read-only; open must ONLY navigate to account-management UI,
// not edit users, log out, disconnect or elevate privileges automatically.
class AccountManagementIntegration {
public:
    using Query=std::function<Metric<bool>()>;
    using Open=std::function<ActionResult()>;
    AccountManagementIntegration()=default;
    AccountManagementIntegration(Query,Open);
    Metric<bool> capability() const;
    // Requeries capability on every explicit call. Only available+true permits
    // invocation; estimated/absent/false/permission failures never call opener.
    // Success is host acceptance, NOT proof that a window became visible.
    AccountManagementOpenResult request_open() const;
private:
    Query query_;
    Open open_;
};
}
