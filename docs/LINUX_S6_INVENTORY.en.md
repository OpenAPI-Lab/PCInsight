# s6: explicit scan tree inventory

Revision 135, part of K03. Public API in `pcinsight/linux_s6.hpp`:

```cpp
auto inventory = pcinsight::query_linux_s6_inventory("/service");
// Check value/support, complete/diagnostics, and each Metric in each row.
```

The caller selects a trusted tree. The API does not autodetect the manager
or scan every installed service, s6-rc repository, dependency, namespace,
or system runlevel. The [official scan-directory documentation](https://skarnet.org/software/s6/scandir.html)
distinguishes the scanned tree from the service repository. The API does
NOT reproduce s6-svscan's internal state or which services it has activated.

## Rows and availability

- Direct visible directories and ordinary directory aliases are included.
  Dot-prefixed names, including `.s6-svscan`, and non-directory nodes are
  skipped. Broken links and unresolvable or inaccessible candidates remain
  UNKNOWN rows and make the inventory incomplete; they are not reported
  as stopped.
- One level of logger, `service/log`, is included if it resolves to a
  directory. It has `logger=true` and `parent_name`. The inventory does not
  recurse into `log/log` or expand other children. Each visible path retains
  its own row even if several aliases point to the same object. Rows do not
  establish the number of distinct processes or instances.
- `name` is relative and `directory` is a displayable path. Neither is a
  persistent identity, a control token, or proof that the path still names
  the same object.
- `status_record` is the [raw 35-byte record](LINUX_S6.en.md), read from
  the pinned directory, NOT by reopening each row's absolute path.
  Available does NOT mean the supervisor or process is alive, running,
  or effectively ready. The record can be stale.
- `real_control_fifo` reports the presence and type of a real FIFO at the
  control path. True does NOT imply write permission, a reader, successful
  delivery, an authenticated s6 peer, a compatible version, or a live daemon.
  The query does not open it for writing, probe for a reader, or take locks.
  Missing/ENOTDIR gives known false for this structural predicate; access
  or resolution errors give UNKNOWN. A control symlink is not a real FIFO
  even if it points to one.

An unmanaged row can have an UNKNOWN record and false control; that does not
mean the service is stopped. A missing or invalid record does not make the
name inventory incomplete: field availability is independent. An empty
result means the observed tree is empty, not that zero services are installed
or that s6 is installed and active.

## Selection, consistency, and limits

The root must be absolute, contain no NUL, and be at most 32,768 bytes.
The total row limit is 1–4,096 (default 1,024), including loggers. Limits
are 16,384 visits including dot/hidden entries and logger probes, 8 MiB
of names and paths, and up to 64 diagnostics. Bounds and errors are reported,
never silently truncated. Results are sorted by name; reaching the limit
does not guarantee the first alphabetical subset. Enumeration opens an
independent file description.

openat2 with NO_MAGICLINKS pins the root and candidates. The service and its
logger resolve from the same pinned parent, and selection is revalidated by
device/inode. Record reading reuses the two-read, type/size/metadata, and
context guards. Control is inspected through O_PATH/NOFOLLOW/fstat without
opening a FIFO or device for I/O. Root binding/mtime/ctime and the boot/procfs/
PID/start/UID/namespaces/eUID context are checked at the end. A changed context
discards the query. Known binding changes invalidate related observations and
make the inventory partial. Other observed raw fields can remain even with
complete=false. Do not treat them as current state or authority over the
final path. There is no atomic snapshot, guarantee against all ABA changes,
stable inventory during external changes, or hard I/O deadline.

The host serializes fork/setns/credentials/FD-context changes. There is no
cache, worker, or implicit polling. The query does not run s6-svscan, svstat,
svc, external libraries, or scripts, and sends no signals or requests.
It does not create supervise/event/control/status/lock/down or modify the
tree. It does not infer a Windows description/group or kernel PID from
names or hierarchy.

## Evidence and remaining work

Private `/tmp` fixture: missing root not created, empty root, service and
root aliases, unmanaged entries, hidden entries and configuration directory,
root files/FIFOs ignored, a single logger with its parent and no recursion,
raw 35-byte record/absence/status FIFO without blocking, real control FIFO/
symlink/absence and a reader receiving no bytes, a broken link making the
inventory partial (including broken log distinguished from absent log),
reported bounds, repetitions, and no FD leaks. No real supervisors,
programs, or services are used. MSVC covers arguments and non-Linux fallback
only; the SDK consumer uses the new symbol. Race guards are implemented,
but no deterministic fixture is claimed for every possible interleaving.
This is not real s6 acceptance or a liveness check.

At this revision, K03 remains open for observation, integration, real
acceptance, and other init systems. [FIFO control](LINUX_S6_CONTROL.en.md)
remains separate: the inventory exposes no actionable handles and does not
confirm jobs. Revision 136 adds [observation pinned to the control target](LINUX_S6_OBSERVATION.en.md),
retaining UNKNOWN without evidence of liveness or causation. OpenRC scripts
remained deferred under revision 132 without explicit authorization; the I07
observer is unchanged.

Revision 142 adds an [explicit ServiceInfo query](LINUX_S6_SERVICES.en.md) with
record-* labels, unknown common PID, and original logger and record evidence
retained. It does not turn a FIFO type or persistent file into proof of
liveness or control capability.

Primary reference for directories and loggers:
[s6-svscan](https://skarnet.org/software/s6/s6-svscan.html). The API reports
its own filesystem inventory, not an equivalent of the scanner's internal list.
