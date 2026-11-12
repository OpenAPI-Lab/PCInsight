# Manual receipt joined to birth resources (internal, revision124)

`linux_desktop_resources.hpp/.cpp` connects a confirmed
[manual launch receipt](STARTUP_DESKTOP_RECEIPT.md) to the existing selected-
creator birth/final-resource observer and impact measurement. It is NOT generic
automatic-startup attribution, process-tree accounting or a public SDK API.

## Native one-shot owner

`LinuxDesktopResourceSession::start(entry,launch_consent,capture_consent,capture_id)`
requires both consents before I/O, pidfds, children or probes. Capture consent
here covers the selected CREATOR birth/final/first-exec hooks as well as the
selected CHILD exec hook. It pins the current caller as creator and arms birth
capture BEFORE launching the desktop child. Capacity is one; final-resource
recovery and first-exec observation are enabled. Missing facilities/permission
do not cause an unobserved fallback launch. No constructor activates capture.

Serialize calls and unrelated forks/child reapers: every direct child of this
selected creator is in its authorized scope, so another child could consume
the sole slot. No global census is performed. Existing BTF/permission/kernel,
borrowed-FD, host privilege/environment and callback contracts still apply.

The desktop receipt owner then performs its existing authenticated gated launch.
Any nonzero returned PID remains caller-owned and must be reaped by that caller.
No allocating preparation or hidden sampling follows child release in start.

`finish(window,policy)` explicitly polls births ONCE, closes that observer,
samples the selected receipt and detaches its provider. It returns original raw
birth rows, the receipt and a separate resource-join result. This is one-shot
finalization, not a wait loop or hidden timer. Reap first to obtain final values.
Finishing while the child is alive stops telemetry without killing/reaping it;
missing final counters remain unknown. stop/destructor do not perform a final
read. Returned copies are historical. A failed join does not erase its raw
evidence. Report availability alone does NOT mean resources were obtained.

## Read-only exact join

`measure_linux_manual_launch_resources` is a pure function of the receipt and
closed birth report. It rechecks receipt consistency and uses the existing
birth-archive validator READ-ONLY for full report/window/policy validation;
no archive is saved, upgraded or changed.

It requires exact successful manual first-exec evidence and loss-free enabled
birth/final/exec/PID-mapping modes. Join keys include boot, kernel TGID, kernel
real UID and kernel start-boottime, plus the actual retained capture ID. It
checks observer PID namespace, mapped child PID/namespace and independently
observed executable inode/device/UID. Both exec times must lie within the
retained lifetime; timestamps from different probe invocations need NOT be
bitwise equal. Namespace PIDs or filenames alone are never identity proof.

An existing conflicting startup binding is refused. An unbound row is bound
only in a temporary COPY for the existing birth-impact measurement. Original
observer/recorder rows and declaration APIs are untouched. The result includes
a separate confirmed controlled-entry link and origin fixed to manual_request;
automatic_startup_origin stays unknown. Externally fabricated structs/pure
inputs are not authentication tokens.

CPU/disk remain estimated birth-to-release observed lower bounds of ONE group,
including pre-exec overhead. CPU is not guaranteed post-final-scheduler-switch
runtime; group I/O includes waited-child I/O. No overlapping tree/session sums,
tick conversions or cross-sensor exact CPU oracle. The configurable window is
birth-relative (default60s): late/missing release cannot be clipped/prorated.
The existing strict high-only lower-bound policy applies; no inferred low/medium.
No current-app/content/automatic-cause claim is added.

## Verification

Normal CTest is pure: complete/partial/late results, different first-exec sensor
timestamps, no input mutation, binding conflicts, key/ns/file/quality/mode
corruption, manual-only origin and double-consent denial. It loads no probes.

Authorized `--authorized-private-resources` uses four private helper children
and private XDG entries only. WSL positive result after caller reap: observed
CPU22418570ns and read+write1155072B for a helper writing/fsyncing1MiB and doing
20ms CPU work. Values exactly match the SAME retained source counters, not an
independent CPU sensor. Two pre-exec failures (missing cwd/invalid ELF) yield
no attributed resources. An early finish on a pipe-blocked live helper yields
unknown final counters; caller verifies WNOHANG0, releases it and waits normally.
All raw rows remain unbound, all owned probes/FDs are released, no hidden kill.

Evidence: `work/desktop-resources-*`. No real startup app, login activation,
service installation, GUI, external collector or new dependency. No format or
public API change. I07 remains E for recorder/durable integration, trusted
managed-autostart context and other reapers/descendants; N03 is not fixed.

Verification: Linux152/152 without skips+20 focal repetitions; installed SDK
consumer exit0; MSVC10 pure/denial/no-Linux repetitions, not Windows support.
Initial sub-second future dependency-timestamp make warnings are preserved.
Incremental rebuild and full152 rerun pass without warnings; no clocks/mtimes
were adjusted. No new task IDs or fractional progress credit.

Revision125 adds explicit [manual evidence persistence](STARTUP_MANUAL_STORE.md),
preserving raw unbound rows and receipt in one historical envelope. Loading it
does not reactivate this session, own a live PID or prove automatic startup.
