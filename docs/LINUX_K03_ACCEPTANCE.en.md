# K03: private acceptance testing for Linux supervisors

This check is separate from the existing protocol tests. Do not install or
activate an init system, use personal services, invoke the host's manager, or
run SysV scripts to complete this list. If the runtime or a suitable environment
is unavailable, record “not run” and move to another subtask.

## Setup and limits

1. Use a test environment that already has the runtime. Record the distribution,
   supervisor version, and isolation method. For runit/s6, use a new private
   directory and a single supervisor that is a child of the test, not the
   machine's service tree. For OpenRC, use a disposable instance with its own
   catalog and state and a synthetic service; do not target the host's OpenRC.
   Do not reuse paths from previous tests.
2. The synthetic service must stay active until stopped, with no network or
   access to user data. Keep independent control of the child that was launched
   so cleanup can finish within a time limit even if the library fails. Do not
   signal PIDs obtained from the inventory. Do not run the test unless isolation
   and cleanup can be guaranteed.
3. Query inventory and state through the library and compare them with the test
   service's observed lifecycle. Record each request's delivery, the subsequent
   state, and, where applicable, the new process identity separately. Successful
   delivery without a transition does not count as acceptance.

| Provider | Required positive evidence |
| --- | --- |
| runit | Real `runsv`; inventory of the private directory and start, stop, and restart of the selected entry. Verify that restart replaces the old child, not just that it remains `running`. |
| s6 | Real `s6-supervise`; check inventory and transitions for the private entry. Record readiness only if the test service announces it and it is observed independently. |
| OpenRC | Real runtime and explicit selection of the private entry through the public API. Inventory without scripts, followed by a consented request for each of start/stop/restart. Do not infer liveness or PID from markers. |
| SysV | Read-only inventory separate from control. Control was not run: the script exception lacks authorization, and OpenRC authorization does not extend to SysV. |

`sysv-file:...` IDs are inventory references, not systemd names or authorization
to run scripts. The caller must not strip the prefix and forward the name to
another control path. The selection test checks that the systemd control API
rejects start/stop/restart when given a SysV ID.

For authorized control paths, also record a rejection without dispatch (for
example, without consent) and verify that no test processes or services remain.
The [existing fixtures](LINUX_K03_AUDIT.en.md) demonstrate guards and projection;
they do not replace these results.

Revision 173: Ubuntu WSL, 18/18 focused tests (runit, s6, OpenRC, SysV, and
selection). The first path lookup using variables was invalid because the
launching shell expanded them. The corrected lookup with literal arguments
(`command -v`, `whereis -b`, and checks of specific OpenRC paths) found no
executables. This does not establish absence outside the paths checked. Real
acceptance was not run, and no runtime was installed or started.
