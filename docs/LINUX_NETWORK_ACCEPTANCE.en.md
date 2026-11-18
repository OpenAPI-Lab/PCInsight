# F03: native validation and acceptance criteria

Revision 98, 2026-10-24. The official tracker retains the original criterion:
an optional native accounting provider with documented support and privileges,
tested attribution and windows, and no external utilities. Completing F03
does not complete H04 per-app history integration or J04 session aggregation,
or guarantee universal or system-wide traffic coverage.

## Live process view and corrected failure

The user confirmed continuation after an explicit authorization question for
a temporary test of up to 30 seconds against WSL's /proc view. The scope was
PID/UID and TCP/UDP counters only, without content, arguments, environment,
installation, or configuration changes. This confirmation does NOT authorize
future permanent capture.

Tested kernel: 6.6.114.1-microsoft-standard-WSL2. The new manual
linux-network-visible-native executable is NOT a CTest test. It requires the
separate --authorized-visible-wsl-30s flag, which does NOT replace human consent.

In the first run, strict and verified_only rejected an identity with EINVAL.
The second diagnostic run confirmed the selected PID was the collector's own
PID. The library had assumed that querying its own pidfd would pass through
security_ptrace_access_check just as child queries did. The early return for
the same thread group in the [upstream Linux 6.6 code](https://github.com/torvalds/linux/blob/v6.6/kernel/ptrace.c)
is consistent with the missing event observed in WSL. This does not establish
that the tagged code is identical to the installed vendor kernel.

The correction selects the SELF path only after verifying pidfd, procfs, and
context, and only if the ENTIRE local identity matches the native observer
(/proc/self): PID, UID, start, procfs device/inode, process inode, and both
namespaces. PID equality alone is insufficient. It uses two current kernel
reads for the caller through BPF_TEST_RUN and retains post-validation,
liveness, and context and kernel identity equality checks. There is no local-
to-initial PID/UID conversion, cached identity, or fallback for external
processes. The external path still requires two pidfd queries and current
permissions. Failure notes now include the selected and caller PIDs for
diagnosis without publishing command lines or traffic content.

An intermediate run admitted 38/38 but found that one pidfd no longer
referred to a live process before sampling. That failure was retained:
selection is a dynamic inventory, not an atomic snapshot or liveness
guarantee. The actor was not hidden, and zeros were not published.

The final run, after concurrent builds finished, used a REAL inventory
without injection: 30 visible, 30 verified, and zero excluded under
strict_all. Monitor produced three snapshots with matching metadata,
network data, epoch, and cardinality. The collector's own UDP traffic
produced a 97-byte delta in both send and receive and an integer rate of
97e9 divided by the nominal interval. Stop restored the FD census; a later
sample did not rearm capture, and there were no permanent changes. The
final run took 1,326 ms; the first diagnostic runs took 498 and 490 ms;
the entire intermediate command took under 3 s. Together the runs took
under 10 s, within the 30-second authorization. The final watchdog was
reduced to 20 s to reserve cumulative margin. No probes remained.

## Evidence for the F03 criterion

| Criterion | Evidence | Retained limitation |
|---|---|---|
| Source without helper applications | Direct procfs/pidfd/BPF/BTF syscall access | Requires compatible kernel, configuration, and permissions |
| Attribution to a live actor | Joins at both endpoints, typed local/kernel domains, and attested SELF or external source | Socket operator, not a presumed exclusive owner or inferred app |
| Counters, rates, and windows | 17 native TCP/UDP IPv4/IPv6 scenarios, 65 native UDP actors, real 30-actor Monitor run, and fixtures | Payload from instrumented paths, not total wire, ACK, or asynchronous traffic |
| Scale and lifecycle | Fixtures with 1,024 processes and 16,384 buckets; 65 real private actors in two batches; update/gap/fork/worker checks | Explicit limits and costs: G, 1 s per sweep, 5 s for admission |
| Discovery and coverage | Real 30/30 inventory, strict/partial policies, typed exclusions | Dynamic visible view, not a host census or retroactive capture |
| Consumable API | Standalone/owner/Monitor/Sampler and installed consumer | H04 and J04 still have their own pending integrations at this checkpoint |

The successful live test satisfies the remaining F03 source and admission
validation. Certifying every hardware and distribution combination is not
required to declare this optional provider implemented. N02 retains the
cross-platform matrix, and N03 its own audit and risks. Unsupported fields
and their failures or absences do not become supported by closing F03.
The policy still fails closed when the process set changes: the caller
needs a new, explicitly authorized refresh. There is no silent rearming
or filtering, and the limitation remains visible in the API.

Logs: work/network-visible-*. SEGUIMIENTO.txt records the final decision,
regression, and secondary mirror. The new harness does not print names,
arguments, payload, or third-party PID lists; it prints only allowed counts
and failure notes.

Final decision, revision 98: F03 E → C. Linux 133/133 without skips plus 30
focused runs, ten MSVC join runs, and the installed consumer exiting 0.
Seventeen private native TCP/UDP IPv4/IPv6 scenarios and update/gap/fork/
cleanup regression passed. Fixtures rejected ten identity mismatches and
invalid SELF context. No new public layouts or APIs. Temporary WSL clock
warnings on generated files were retained; the recheck build exited 0
without warnings. The first target had no rule; regenerating CMake fixed it
and the log was retained. Official count: C58/E10/P5/B2, 58/75 = 77.3%,
17/75 = 22.7%. Next, H04: read [history](HISTORY.en.md). Revision 98 mirror
confirmed: comment 90140272937903; the TXT remains authoritative.
