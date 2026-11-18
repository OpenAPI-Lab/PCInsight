# Opening Windows service details

Implemented on 2026-10-19. `open_windows_service(internal_name)` in
`pcinsight/windows_services.hpp` queries SCM directly for the caller's GUI
panel. It does not start processes, editors, services.msc, a shell, or UAC.
It uses current permissions and does not need to enumerate all services first.

```cpp
const auto opened = pcinsight::open_windows_service(selected.id);
if (opened.result && opened.view) {
    const auto& details = *opened.view;
    if (pcinsight::metric_is_known(details.description)) {
        // Even "" is a known empty description.
        show_description(*details.description.value);
    }
}
```

The outer result contains ActionResult and an optional view. Success means
the view is present; an error means it is absent. Invalid input, not found,
permission denial, other Win32 errors, and unsupported platforms are distinct.
Empty input, NUL, slashes, invalid UTF-8, more than 1,024 bytes, or more than
256 UTF-16 code units are rejected before opening SCM. Outside Windows, the
function returns not_supported without querying the system.

The view includes the requested internal name, display_name, description,
load_order_group, status, native_service_type, and process_id. All but the
identifier use Metric. SCM is opened with CONNECT. The status handle is
pinned first with QUERY_STATUS, and QUERY_CONFIG is requested separately.
Denied configuration access therefore does not hide readable status or
trigger elevation. The status handle prevents a replacement object with
the same name from being opened during the query.

Empty or null configuration fields are known empty strings. Denial, failure,
invalid data, and exceeded bounds yield absence with a reason, not an empty
string. permission_required identifies permission issues. Other field errors
use unsupported with the operation and code. That enum alone does not
distinguish every error, so preserve note. A global status failure does not
return a partially successful view. No fallback to caches, the registry,
files, or external tools.

The group is explicitly a **load-order** group, not a svchost hosting group.
Description is raw SCM text. Indirect `@...` references are preserved without
loading DLLs or retrieving text from remote paths. The GUI should treat it as
text, never as a command or HTML. Universal friendly resolution is not claimed.
The API does not expose executable paths, accounts, or secrets.

process_id is a Metric containing an optional value. A known outer value
with an empty inner value means the observed state does not supply a publishable
PID (stopped, start_pending, stop_pending). It does NOT mean PID 0 or prove
that no process exists. Running, paused, pause_pending, and continue_pending
publish only nonzero PIDs; unknown state or unexpected PID 0 remains absent.
A PID does not authenticate a process and is not a checked termination token.
For service actions, use the internal name with control_service, never
terminate the host based on this PID.

Own-process and shared-process Win32 services are supported, not kernel
drivers. Two 8 KiB buffers are used; pointers, ranges, alignment, terminators,
and UTF-16 are checked without unbounded growth or retry. QueryServiceConfig's
required-size output is documented only for insufficient-buffer failures,
so it is not used as the valid data length on success. Initializing to 0xFF
prevents old buffer zeros from fabricating terminators. Handles use RAII even
on exceptions. Status and configuration are queried at different instants,
not in an atomic snapshot. Windows controls RPC timeouts; this is not a
real-time API.

## Verification and M07 completion

`tests/windows_service_view.cpp` fixtures cover minimal rights, invalid
input before SCM access, not found/denied/truncated status/rejected drivers,
partial configuration, insufficient buffers, null or empty strings,
out-of-range/misaligned/unterminated pointers, invalid UTF-16, indirect
references, states 1–7/unknown/PID 0, and handle cleanup. The read-only native
query succeeded: eight services opened, with eight known descriptions and
eight known groups.

`tests/windows_actions_native.cpp` launches only its own executable in a
private child mode that checks literal arguments (spaces, empty argument,
quotes, and trailing backslash) and confirms them through a unique event.
The child performs no user or network I/O and shuts down after 15 seconds
if it loses the parent. The test obtains FILETIME through its own handle,
rejects stale EcoQoS/termination tokens, checks that the child is alive,
toggles checked EcoQoS, and reads the requested bit back. It then uses
checked termination and confirms signaling on the same handle and exit code 1.
RAII attempts to clean up the private child on exceptions too. It touches no
unrelated processes, does not elevate, and leaves the parent's policy unchanged.
EcoQoS remains an energy policy request, not evidence of savings or a combined
Task Manager mode with priority changes.

Both native tests passed, with 20 repetitions each (40 runs). Revision 36
SCM start/stop/restart validation is retained in [Windows services](WINDOWS_SERVICES.en.md):
an authorized private service was used, then its registration, copy, and
directory were removed. This revision does NOT repeat the SCM mutation.
M07 closes the supported inventory, control, details, launch, forced end, and
EcoQoS scope and its isolated validation. The production UAC helper remains
L05. M02 process resources/network, M06 sessions, N01 CI/toolchains, and N03
final audit are not closed. Additional versions and hardware and features such
as localized descriptions, svchost grouping, app-specific cooperative close,
or combined efficiency mode are not claimed. A simulated success does not
replace a pending action.

The additive API changes no prior layouts, virtual ABI, SystemSnapshot, or
history. Installed Linux and Windows consumers were rebuilt and relinked to
verify exports and linking. Evidence: work/service-view-*.log. Final Release
regression: Windows 121/121 and Linux 108/108. Both installed consumers were
rebuilt, relinked, and exited 0. Final builds had no warnings.

Primary references:
[QueryServiceConfigW](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryserviceconfigw),
and [QueryServiceStatusEx](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryservicestatusex).
