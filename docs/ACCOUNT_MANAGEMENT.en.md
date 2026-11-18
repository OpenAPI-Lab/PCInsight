# Manage user accounts: GUI integration (J06)

`pcinsight/account_management.hpp` provides AccountManagementIntegration.
It lets the consuming GUI navigate to its own account manager.
**It does not include a native account manager or settings page.**
It does not create or delete users, change credentials, disconnect sessions,
or elevate privileges. It neither searches for nor installs apps and
does not guess GNOME, KDE, or Windows commands. An application can open
its own view without an external tool.

## Contract

Configure an instance explicitly with two trusted callbacks: a read-only
Metric<bool> capability query and an ActionResult opener. There is no
global registry, autodetection, or execution on construction or Monitor
sampling. Without both callbacks, the capability and action are unavailable.

`capability()` normalizes absent, estimated, or invalid support, notes
containing NUL or exceeding limits, and exceptions. Permission_required
retains the reason and removes a contradictory value. Available+false
is known unavailability. Only available+true allows request_open. Each
call queries again before invoking the opener, even if the GUI checked
earlier.

`AccountManagementOpenResult.host_invoked` is false if the opener was
not called and true even if it returns an error or throws. In the
latter case the UI may already have opened, so do not retry automatically.
The host's specific error is preserved. Success means host acceptance,
not proof of a visible window or later account changes.

Callbacks run on the calling thread. The host manages the event loop,
dispatch to the GUI thread, synchronization, and captured references.
There is no background processing, hard timeout, or callback cancellation.
Callbacks are trusted code, not sandboxed. The host must honor the
navigation-only contract without implicit CRUD or elevation. If it
uses launch_process, it inherits request checks and responsibility for
reaping Linux children; process_id is not durable identity. This API
does not accept user-supplied targets, paths, or commands and does not
change the SystemActions/AuditedActions vtable.

Example of connecting to a view already owned by the consuming GUI:

```cpp
pcinsight::AccountManagementIntegration accounts(
    [&gui] { return pcinsight::Metric<bool>{gui.has_account_view(),
        pcinsight::Support::available, "account view supplied by host"}; },
    [&gui] {
        gui.request_account_view(); // The host dispatches to its event loop.
        return pcinsight::ActionResult{pcinsight::ActionError::none,
            "navigation request accepted"};
    });
// Only when the user explicitly selects Manage user accounts:
const auto receipt = accounts.request_open();
```

`gui` is consumer pseudocode, not a class included in PCInsight.
The instance must not outlive the references it captures.

## Verification and completion scope

tests/account_management.cpp covers acceptance, absence/false/estimated,
denial, incomplete setup, changing capability, invalid notes, exceptions
in both phases, and opener errors without retries. It also checks the
public SystemActions::launch_process path on Linux and Windows: a
private child writes a marker to a new directory, the parent confirms
acceptance, and Linux calls waitpid. **That child is not a real account
manager.** No desktop settings were opened or real users changed.

The external CMake consumer includes and links the new installed header
and verifies that the default state executes nothing. This is an
additive API; no existing public layout, virtual ABI, or history format
changes.

J06 calls for a safe action or integration to access account management
and tests of capabilities and errors, not account CRUD. This work meets
that scope through explicit host integration, not simply by returning
unsupported. Effective support depends on the GUI supplying its own
manager. It does not provide an automatic or universal desktop panel
provider.

Verification on 2026-10-19: Linux Release passed 102/102 and Windows
Release passed 116/116; builds had no warnings. Both packages were
reinstalled, and rebuilt, relinked external CMake consumers exited 0.
Logs include work/account-management-suite-{linux,windows}.log,
account-management-native-*, and build/install/consumer logs with the
same prefix. Earlier N03 work remains open. This passing regression
does not establish a fix for historical intermittent failures. No
real accounts or account managers were tested.
