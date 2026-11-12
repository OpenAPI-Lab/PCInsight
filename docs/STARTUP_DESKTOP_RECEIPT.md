# Controlled manual launch receipt (internal, revision123)

`linux_desktop_receipt.hpp/.cpp` composes the effective
[XDG desktop route](STARTUP_DESKTOP_LAUNCH.md), pre-exec gate and
[selected first-exec capture](STARTUP_SELECTED_EXEC.md) into one internal owner.
It does NOT replace public RunNewTask, change archive formats, or turn a
caller-declared application association into authenticated origin.

## One-shot session

`LinuxDesktopLaunchSession::start(id,launch_consent,capture_consent)` requires
BOTH explicit consents before any file access, child creation or probe load.
The attempted session is one-shot, including failed attempts; create another
object for another launch. A consent denial alone does not consume the session.
No constructor starts anything. There is no fallback to an unobserved launch.

Inside the desktop wrapper's still-gated callback it copies the resolved route,
arms the pidfd-authenticated selected-exec provider and retains the selected
kernel/local identity and pinned executable tuple. Allocation/preparation is
done BEFORE release. The desktop wrapper still revalidates parsed fields,
precedence and eligibility after arming; the gate performs its own final checks.
Capture failure cancels the blocked child through the existing gate contract.

After the launch returns, its error and caller-owned PID are retained without
new allocations or a hidden sample. Any nonzero returned PID remains the
CALLER's waitpid responsibility, even for an uncertain launch-channel error.
The session never launches, signals or reaps an additional process in sample or
stop. stop/destructor release capture resources, not the application. An old
session cannot be relaunched after stop, avoiding accidental identity reuse.

## Receipt semantics

`sample()` returns a historical `LinuxDesktopLaunchReceipt` with:

- origin fixed to manual_request (no automatic-origin input exists);
- resolved desktop snapshot ID and actual literal launch request;
- complete authenticated kernel/local/boot/namespace identity;
- pinned file tuple, launch error and caller-owned PID;
- first-exec metadata, quality/publication state and pidfd exit observation;
- a separate optional entry_first_exec_matches result.

The last field is available true/false ONLY for a known first executable and
consistent available tuple comparison, exact source quality and released PID.
False means an observed filesystem tuple mismatch; absence means UNKNOWN, not
false or zero. Exited/reaped, successful return, error-channel EOF and a manual
request alone cannot confirm exec. Known-zero inode/device components survive.
Estimated/in-flight/sticky-error data cannot become an available causal match.

The pure `project_linux_manual_receipt` checks full selected/source identity,
key/route/pin/outcome, inner metric shapes, quality/publication, event UID/time
and independent tuple consistency. Contradictory match values, changed identity
or exec evidence for a cancelled/no-released-PID launch fail closed. A genuinely
observed exec can remain confirmed despite an uncertain system-error channel
when the caller-owned PID is retained. The original launch error is preserved.

Pure projections and mutable returned structs are NOT authentication tokens
for externally fabricated inputs. Trust comes from the native session owning
the controlled launch and sensor. A receipt is not a current-file/content/hash/
publisher/app identity, automatic-startup cause, resource total or descendant
claim. A wrapper ELF is still opaque. Existing desktop snapshot non-atomicity,
host serialization, privilege/kernel/BTF requirements and blocking-I/O limits
remain. Historical copies remain unchanged after stop, exit or later re-exec.
Failed sampling closes the owner; no implicit final sample is invented.

## Verification

Normal Release CTest `linux-desktop-receipt` is pure and loads NO BPF probes.
It tests pending/exited/error UNKNOWN, true and false matches, known zeros,
identity/quality/time/UID/metric contradictions, uncertain error-channel
positive evidence, manual-only origin and independent consent denial.

Manual `--authorized-private-receipt` is only for authorized private fixtures.
It launches its own helper ELF from private XDG entries. Native WSL validation
checks a successful first-exec receipt AFTER reap plus two pre-exec failures
(missing cwd, invalid ELF) retaining UNKNOWN and no caller-owned PID/ECHILD.
Replay and one-shot behavior are checked. A fourth helper waits on a private
pipe: its receipt is sampled while alive, telemetry is stopped, WNOHANG confirms
it was NOT killed/reaped, then the caller releases and waits for normal exit.
All capture references and FDs are cleaned; copied historical evidence persists.

Evidence is in `work/desktop-receipt-*`. No real login/startup application,
service installation, global census, GUI or new dependency is activated.
Linux151/151 without skips+20 focal repetitions; installed SDK consumer exit0.
MSVC10 pure/denial/no-Linux repetitions, not Windows feature coverage. Initial
sub-second future-dependency timestamp make warnings are preserved; incremental
rebuild and full151 rerun pass without warnings, no clock/mtime adjustment.
I07 stays E: integrate receipts with recorder/resource attribution and a trusted
managed-autostart path, without promoting this manual action to automatic cause.
Other reapers/descendants and N03 remain open. No fractional credit/new task IDs.

Revision124 adds an internal [birth-resource session](STARTUP_DESKTOP_RESOURCES.md)
which arms the creator before launch and joins this receipt to the retained
single group's resource lower bounds. Its original raw rows remain unbound;
this still does not identify automatic-startup cause or change persistence.
