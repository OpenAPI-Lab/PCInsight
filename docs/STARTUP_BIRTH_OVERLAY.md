# Explicit historical birth-impact view

Checkpoint119,2026-10-29. Public header startup_birth_overlay.hpp provides
apply_startup_birth_impact(snapshot, birth_archive, accepted_context). This PURE,
explicit presentation helper does not create a GUI, read files/system state,
start collection, update Monitor automatically or perform any action on processes.
Archive validation is shared with PCISTART v3, not a second permissive decoder.

## Acceptance is not authentication

StartupBirthOverlayContext must explicitly accept caller-declared associations
(default false, permission_denied) and specify the accepted capture ID, boot and
PID namespace device/inode. Mismatch is stale_target, with no snapshot mutation.
These are caller-selected source-context guards, NOT verification of the CURRENT
snapshot boot/catalog or file authenticity. Do not simply copy an untrusted file's
context and call that a current-system identity check. Checksums are not signatures.

Associations use ONLY the explicit startup_entry_id in retained rows and exact
snapshot entry IDs. No executable/name/PID/substring/parent matching. No proof that
the current command still names the historical app: ID/path reuse and edited
desktop entries remain possible. Even a disabled current entry can show a declared
historical observation; current status does not establish old enablement/cause.

The GUI must display the confidence and impact_scope alongside the shorthand.
Scope prominently says HISTORICAL, capture ID/boot/PID namespace, source state/
quality flags, caller declaration, per-child window and no automatic-startup/app
identity proof. None of these values is labelled current usage or complete global
recorder/login/boot impact. No action-authorization token is produced.

## Projection and separation

Uses the archive's saved window/policy with measure_startup_birth_impact. For ONE
bound group per entry, release-observed CPU/disk lower bounds remain estimated,
including known zero; impact is high or unknown, never low/medium. Late/pending
resources remain unknown, no clipping/proration. An unknown new observation
explicitly replaces an older overlay value for that matched entry.

For MULTIPLE groups bound to the same entry, CPU/disk/interval remain unknown:
overlapping group accounting is NOT summed. A high lower bound in ANY group may
survive; otherwise impact remains unknown (permission_required if appropriate).
The original rows stay available for drill-down outside the snapshot shorthand.

observed_startup_interval stays unknown even for ONE group: this kernel
birth-relative window is not the legacy steady-clock sampled interval. No fake
procfs start_ticks/PID or epoch conversion is manufactured. Other startup fields,
current name/command/status/publisher and unrelated entries are untouched.

Unbound rows are counted/skipped, missing target IDs counted/skipped. They do not
erase unrelated values. ALL source rows are validated first, including skipped
ones. Duplicate entry IDs/keys, malformed availability/window/policy, unknown flags
or inconsistent metadata reject the whole batch. Bounded64 source rows,100000
catalog IDs/8MiB ID payload, existing v3 string limits. All allocations precede
noexcept field moves; validation failures leave the snapshot unchanged. Repeated
successful application is idempotent, not an accumulation. Caller serializes data.

```cpp
#include <pcinsight/startup_birth_overlay.hpp>
// loaded.archive->births must exist. expected is a context the caller has accepted,
// NOT a fresh-system identity check derived merely by reading an untrusted file.
expected.accept_declared_associations = user_accepts_historical_declarations;
auto result = pcinsight::apply_startup_birth_impact(
    snapshot, *loaded.archive->births, expected);
// Display observed_startup_impact support + impact_scope; not just entry.impact.
```

## Verification and remaining scope

Pure fixture covers default refusal, exact context/declared IDs, historical scope,
disabled/current-command preservation, unique/multiple groups, high/unknown/zero,
permission, late resources, idempotence, no aggregation/clock coercion, skipped
counts, malformed skipped rows and whole-batch rollback. No OS/file calls occur.
Installed SDK consumer checks the symbol/default refusal without enabling capture.

Private native harness loads real v3 after reaping and explicitly applies this
helper to a private startup view. It verifies scope/unknown interval, exact single
group resources, suppressed multi-group totals, and high-only with a CUSTOM1ms
CPU threshold (default policy unchanged). Existing capture/oracle/file0600/FD/
cleanup checks remain. Execution evidence: SEGUIMIENTO.txt and
work/startup-birth-overlay-*.

I07 still requires reliable launch receipt/app/automatic-startup cause and other
reapers/descendants. A declared historical overlay does not close that criterion.
N03 remains open; no host/login/GUI installation or Windows feature added.
Final verification: Linux147/147 without skips+20 final focal repetitions,
MSVC10 pure fixture runs, installed SDK/consumer exit0, native13 scenarios23
retained rows through v3 and overlay (18 releases/5 absent finals), with previous
source/oracle/replay/0600/FD/cleanup checks preserved. C++ builds clean; assert-
enabling MSVC D9025 intentional. Render all untrusted labels as plain text in GUI.
