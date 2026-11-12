# Selected successful exec (internal, revision121)

`src/linux_selected_exec.hpp/.cpp` completes the optional selected-process exec
sensor for the [controlled pre-exec gate](STARTUP_GATED_LAUNCH.md). It is NOT a
public SDK API, startup application binding, or an automatic-startup receipt.
The normal RunNewTask action and existing archive/public layouts are unchanged.

## Arming and scope

Call `LinuxSelectedExecCapture::start(true,target)` only inside the gate's arm
callback, while its direct child cannot exec. The provider duplicates the
borrowed pidfd and pinned image fd. It authenticates the selected kernel
TGID/real UID/start-boottime through the private pidfd join and validates the
full local process, caller, boot, procfs and namespace context. It resolves BTF
for task identity and executable metadata, without requiring CPU/I/O accounting
or a release_task sensor.

It creates one selected slot, attaches sched_process_exec while the allowlist
is EMPTY, reauthenticates child/caller, and enrolls that TGID LAST. The launch
gate must perform its own final revalidation before releasing the child. The
tracepoint hook is global, but unselected groups get only a TGID hash lookup;
no executable metadata is read or stored for them. Selected events also verify
start-boottime to reject PID reuse. No process census or startup activation.

No external collection utility is executed or installed. This opt-in provider
requires usable kernel BTF, BPF/raw-tracepoint facilities and permissions;
missing facilities return explicit unsupported/permission-required, not made-up
data or a claim of support on every kernel configuration.

## Evidence and limits

The reused exec-only probe publishes the FIRST successful exec's monotonic
time, inode, kernel device major/minor and real UID. The source event follows
successful binary-handler loading in [Linux exec_binprm](https://github.com/torvalds/linux/blob/v6.6/fs/exec.c#L1661-L1701).
CAS publication prevents later re-exec from replacing that first event. A failed
first metadata read is sticky; a later exec must not impersonate the first.
State in flight is returned as publishing, without partially read fields.

The internal shared raw ABI uses word0=armed, words1..3=selected kernel key,
words4..14=UNUSED ZERO, word15=publication state, words16..19=event metadata.
This is NOT a birth record: there is no invented birth time, CPU baseline or
I/O counter. It must never be passed to the birth decoder/archive validator.
Unknown flags, key changes, malformed values and nonzero unused words fail
closed. Kernel `dev_t` is split using 20 minor bits, not compared raw to st_dev.

`sample()` compares the historical first-exec filesystem tuple with the still
pinned launch object. Known mismatch is false; no event is UNKNOWN, even when
the pidfd reports exit. EOF on the launch error channel, exit status, PID or
absence of an event is NOT positive successful-exec evidence. Known zero
inode/device components are preserved. Real UID mismatch is rejected.

This proves neither executable contents/hash/publisher nor desktop-entry/app
identity or automatic-startup cause. It says nothing about the CURRENT image
after re-exec. CPU/disk impact, child trees and birth mapping are not added here.
The next unit must connect an explicit validated desktop route and managed
launch receipt, rather than promote user-declared association to proof.

## Lifetime and validation

Sampling remains possible after waitpid/reap: identity was authenticated before
release, and the provider retains its pidfd/image pin/maps. Caller boot/full
namespace/procfs identity, PID and effective UID are checked around reads.
Repeated successful samples must preserve the first published event. A failed
guard closes all owned references. `stop()`/destructor detach first and release
all descriptors; no hidden final read, signal or reaping. Returned copies are
historical, not live authority. Serialize operations/forks/credentials/FD
ownership in the host; this is not an adversarial-process sandbox.

Pure `linux-selected-exec` tests cover pending/publishing/sticky failure,
known match/mismatch/zero, invalid keys/flags/unused fields/time/device/UID and
authorization denial. Normal CTest still loads NO BPF probes.

The explicitly authorized private-only manual test is:

```
pcinsight-startup-gated-launch-tests --authorized-private-selected-exec
```

WSL positive verification: five still-gated selected children, each with an
unselected private helper exec ignored before release; three successful first
exec matches (including symlink retarget and re-exec to a DIFFERENT inode), two
pre-exec failures remaining unknown (missing cwd and invalid ELF). Samples and
immutable replay are checked AFTER reap. Final descriptor census, parent signal
mask/NNP and temporary probe cleanup pass. No real login/startup app is launched.
Logs: `work/selected-exec-native.log`, `work/selected-exec-fixture.log` and the
`work/selected-exec-*` build/regression/consumer logs.

I07 remains open for reliable desktop/app/cause binding and remaining coverage.
N03 is not fixed by these tests. No partial percentage credit or new task IDs.

Revision122 adds the [direct effective XDG launch route](STARTUP_DESKTOP_LAUNCH.md)
and privately verifies its connection to this provider. That explicit MANUAL
action still must not be promoted to automatic-startup cause.
