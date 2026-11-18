# L03: accepted limited scope for the advanced option

Revision 41 update, 2026-10-19: the user replied “continue” to the concrete
proposal to keep only an advanced, default-off option with acknowledgment
of numeric ID risk, without checked identity, EcoQoS, coverage of new threads,
or guaranteed restoration. This acceptance applies only to L03; it does
not authorize modifying unrelated processes in tests or installing helpers,
tracing, or cgroups. The revision 18 analysis below is historical evidence.

Revision 18, 2026-10-17. Design diagnosis, not implementation or new native
validation. The Linux 85/85, Windows 105/105, and 20 repeat runs are revision
17 evidence and were not rerun for this documentation revision.

## Finding

The per-thread preset and restoration receipt already exist and have been
tested within the [preset's limits](LINUX_EFFICIENCY.en.md). The documented
interfaces reviewed did not offer a direct replacement that makes those
priority writes through a pidfd instead of a numeric PID/TID. This does not
rule out future kernel extensions.

`setpriority(PRIO_PROCESS, who, prio)` and `ioprio_set(which, who, ioprio)`
take numeric IDs. Obtaining or verifying a pidfd in a separate call does not
change the target of the subsequent write. A post-check can detect a change
but cannot undo an effect on the wrong identity. That is why the current
variant is not promoted to checked.
Sources: [setpriority](https://man7.org/linux/man-pages/man2/setpriority.2.html),
[ioprio_set](https://www.man7.org/linux/man-pages/man2/ioprio_set.2.html),
[pidfd_open](https://man7.org/linux/man-pages/man2/pidfd_open.2.html).

Repeating enumeration does not guarantee observation of every thread born
and terminated between reads. Normal priority inheritance can help, but it
does not replace a participation or control mechanism or a restoration policy
for threads absent from the receipt. Do not invent an original value for
them or extend the receipt with zeros or already-modified values presented
as originals.

## Alternatives

| Alternative | What it permits | Limit or decision needed |
| --- | --- | --- |
| Keep the current provider optional and limited | Preset for captured threads, observed checks, and explicit receipt-based restoration | Explicitly accept this scope for L03; checked remains unavailable and numeric ID risk is shown before opt-in |
| Design a cooperative provider | Explore having each participating thread change its own priorities without selecting another process by numeric PID | Requires code in the target application and a protocol, authorization, thread registration, and restoration design; does not cover arbitrary apps and is not implemented |
| Explore invasive external control or dedicated groups | Study thread-creation tracing or policies for managed groups | Not a transparent replacement: permissions, pauses, effects, and semantics differ; needs design approval and additional tests, not an already demonstrated guarantee |

For the original request (“efficiency mode if available,” without installing
new applications), the first alternative is recommended ONLY if the user
accepts the limit. Consumers should not advertise it as Windows EcoQoS or
automatically select unchecked when checked fails. Without acceptance, the
criterion is not reduced and L03 is not closed.

For the third alternative, ptrace offers stop and clone tracking operations,
with access checks and effects on tracees. No debugger was attached or process
suspended in this revision. [ptrace](https://man7.org/linux/man-pages/man2/ptrace.2.html).
In cgroup v2, migrating an existing process by writing a PID does not turn
that selection into a pidfd. `CLONE_INTO_CGROUP` allows creation with a group
FD but changes the scenario to created or managed tasks and retains access
restrictions. It does not restore nice/ioprio for arbitrary existing processes.
[cgroup v2](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html),
[clone](https://man7.org/linux/man-pages/man2/clone.2.html).

## Historical tracking state (revision 18)

L03 moved E → B because the architecture and scope decision was unavailable;
all prior code and evidence were retained. No helper, protocol, tracing,
cgroup, or cooperative provider was added without the user's decision. No new
IDs, fractional credit, or acceptance inferred from “continue.” The count
remained 42/75 = 56.0%; C42/E17/P13/B3. The reply had to be recorded before
changing the criterion or starting a design requiring additional integration
or control.

## Implementation of the accepted scope (revision 41)

The PID/TID race was not fixed, and no checked workflow was added. On Linux,
generic SystemActions::set_efficiency_mode now returns not_supported without
reading or changing the target, including through AuditedActions. The checked
variant remains not_supported and never falls back to unchecked. ProcessInfo
reports efficiency_mode_available=false with available support for that generic
path, not an owner/root heuristic that invites automatic GUI activation.

The only public Linux API that modifies the preset is the advanced
apply_linux_efficiency_unchecked/restore_linux_efficiency_unchecked workflow.
acknowledge_pid_reuse_risk defaults to false; both steps require explicit
opt-in. The GUI must first explain that PID/TID reuse can affect a different
identity, partial effects and shared CLONE_IO effects are possible, and
restoring the original nice value may require permission. A bool does not
authenticate human consent or replace per-operation authorization.

This is a 0.x **behavioral breaking change**: consumers that used the generic
method to change priorities no longer do so. Do not catch not_supported and
automatically retry with ack=true. Migrate only an explicitly accepted advanced
option to the token/receipt workflow, retaining the receipt even on failure.
Signatures, vtables, layouts, and formats are unchanged; no stable ABI 1.0 is
announced. Windows EcoQoS is unchanged. The legacy leader algorithm remains
internal and uninstalled for regression tests, not as a GUI bypass.

Native tests use private children only. They check rejection of generic
on/off and checked operations without nice/I/O changes, auditing of rejection,
apply and restore without acknowledgment rejected without mutation, advanced
opt-in apply on two threads and exact restoration where possible, and a mixed
case with denied CPU restoration and an outstanding receipt. The caller is
unchanged and children are reaped. Fixtures still cover errors, observed
identity, changing threads, partial receipts, CLONE_IO, and conflicts. They
do NOT show elimination of the race. L03 closes on the accepted limited
alternative, not as a complete checked or all-thread mode. L05 authorization,
N02 hardware, and historical N03 risks stay separate. No new IDs or partial
credit.

Final verification: Linux 108/108 and Windows 122/122 Release; 20
linux-efficiency repetitions; both installed consumers rebuilt, relinked, and
exiting 0. Final builds had no warnings. The initial unused uid_known warning
after removing the heuristic was fixed and its log retained. Evidence:
work/efficiency-optin-*.log. These tests verify limited scope, not absence of
PID/TID races or coverage of future threads.
