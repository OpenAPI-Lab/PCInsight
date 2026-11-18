# D03: Linux process state and identity

D03 closed on 2026-10-16 with status, UID, NSS, name, and ELF support,
public per-field availability, and an estimated XDG Comment description
with provenance. See [process descriptions](LINUX_PROCESS_DESCRIPTION.en.md)
for sources and coverage. Not every app has metadata.

## Implementation

- The initial stat read is capped at 64 KiB. An unbounded read no longer
  precedes the bounded parser.
- The process directory is pinned with O_PATH. stat/status/stat are read
  through openat with O_NOFOLLOW/CLOEXEC/NONBLOCK. Reads use 8 KiB chunks.
  status is capped at 1 MiB, 10,000 lines, and 64 KiB per line. EINTR gets
  at most three retries.
- PID must be a positive uint32 matching the expected row. PID and
  start_ticks must agree before and after. A changed or missing identity
  does not receive those details.
- Uid requires exactly four complete uint32 values: real, effective,
  saved, and filesystem. The real UID is used as before; attribution
  does not automatically use the effective UID. UID 0 is valid and is
  not confused with a missing UID. The collector does not invent root.
- Duplicate selected fields, signs, overflow, trailing text, NUL, and
  invalid bounds are rejected. Individual missing fields remain absent
  without invalidating a correct PID or inventing data.
- Name and State are observed kernel data, not a commercial label or a
  guarantee that the process will keep running. Future alphabetic state
  codes are preserved. Unknown states do not become running.
- Control and non-ASCII bytes are represented as ASCII escapes for display.
  The parser trims surrounding whitespace from the value. It does not
  guarantee byte-exact names or unique identity. Display forms do not
  replace UIDs, PIDs, or tokens.
- getpwuid_r replaces shared getpwuid. Its buffer starts at 1,024 B and
  grows to at most 1 MiB. Missing data and errors do not fall back to root.
  NSS can block or query identity services; there is no overall deadline.
  No external utilities are run.
- The description=name fallback was removed. The description now reflects
  only an associated estimated XDG Comment, or remains empty/unknown.
  The third work unit added the public layout change; the first two did
  not. The Windows backend's sources are unchanged.

## Initial evidence (first work unit, before the public report)

The new linux-process-details test keeps assertions enabled in Release.
It covers UID 0 and missing UID, exactly four UID values, range and signs,
incorrect PID, duplicates, partial fields, escaped characters, byte and
line limits, missing or changing tokens, and tick zero. Permission
propagation is tested synthetically; real permissions are not changed.
Private Linux fixtures check missing and oversized sources and rejected
final symlinks. The WSL test observed its own process: PID 1399, UID 1000,
NSS name wiguelito, State R (running), and Name pcinsight-linux. Monitor
matched the account and did not invent a description. No PID reuse, exec
races, account changes, or changes to other processes were forced.
Release regression passed 75/75 on Linux and 95/95 on Windows. Windows
runs portable fixtures; native Linux reads were verified only in WSL.
Installation and consumer checks were not repeated because the public
layout did not change.

## Description and availability completion (third D03 work unit)

ProcessInfo.linux_details now exposes a Metric per field and the real UID.
Legacy strings mirror their values or are empty. An XDG Comment is an
estimated description with a source, not a claim that comm or the binary
provides a certified description. Absence and ambiguity remain explicit.
This resolves D03 availability, not A01 for other legacy models such as
AppHistoryEntry. The new public 0.x layout requires rebuilding the library
and all consumers. Final regression passed 76/76 on Linux and 96/96 on
Windows; rebuilt installed consumers exited 0. A full row is not an atomic
snapshot. PID/start_ticks is scoped to the boot and namespace and has tick
resolution. It does not detect exec within the process or credential
changes after status. Escaped names can prevent matching with utmp or NSS
without equivalent normalization; they do not provide persistent identity
for aggregation by user.

## Executable and architecture (second D03 work unit, 2026-10-16)

The collector no longer combines read_symlink(exe) with a later open by that
path. It pins the proc directory and verifies stat before and after. It
opens exe with O_PATH/CLOEXEC, deliberately following the magic link, uses
fstat to require a regular file before any read, and retains the descriptor.
It opens only its own /proc/self/fd/N and reads at most 64 B with pread.
It does not execute the file. If the descriptor cannot be reopened, it
does not fall back to the sampled pathname.

Before and after the read, the collector checks dev, inode, size, mode,
mtime, and ctime, including nanoseconds. It rereads the exe link and reopens
its target with O_PATH to detect observable file changes. A changed link,
metadata, or PID/start discards the observation; data from two detected
executables is not mixed. The link string is capped at 64 KiB, and the
(deleted) suffix is preserved. The string is not a reusable path and must
not be reopened on the assumption that it identifies the file.

Architecture comes from a complete 52 B ELF32 or 64 B ELF64 header,
including compatible magic, class and byte order, versions, ET_EXEC or
ET_DYN, e_ehsize, and machine. Tests cover little and big endian and
multiple architectures. Host architecture is not inherited. Truncated
or unsupported ELF leaves architecture unknown even when a verified path
can be retained. This does not validate the whole ELF, its segments, or
signatures. An interpreted process identifies the interpreter executable,
not the architecture or description of the script or JIT code.

Reads are sequential. They do not certify an exec generation or rule out
A-to-B-to-A changes or content modifications invisible to metadata. The
collector does not lock the process, use ptrace, hash the whole binary, or
provide an atomic snapshot with UID, State, or RSS. The limit is in bytes;
syscalls and filesystems can block without a universal deadline. Proc and
file permissions differ, so an available path does not guarantee an
accessible architecture. Internal Metrics preserve support and note;
empty legacy strings still mean unknown. The later public report keeps
those notes per field. This is not stabilization of the entire A01 API.

Expanded linux-process-details tests cover every truncated ELF64 length,
invalid fields, ELF32, byte order/class/machine, continuity of timestamps,
path, PID, and start token, and a fixture whose name literally ends in
(deleted). That fixture is not a test of a real executable deleted while
running. Private Linux fixtures reject symlinks to a directory, FIFO, or
/dev/null before opening them for I/O. A regular non-ELF file retains its
path without an invented architecture. Native WSL PID 1354 returned x64
and its own binary path; Monitor matched. No real exec or PID reuse was
forced, and no other program's file was modified.
Final Release suites passed 75/75 on Linux and 95/95 on Windows, using the
same expanded test target, with no new subtasks or public layout change.
Consumer checks were not repeated. The first Linux regression failed in
linux-integration because global CPU was absent on the second sample.
That sample's counters were not retained, so the cause was undetermined.
Integration then passed three consecutive runs and the full 75/75 suite
passed without changing the assertion or CPU code. The risk remains
recorded for N03; the ELF change does not claim to fix a CPU issue.

Primary references for the semantics used:
[proc_pid_exe](https://man7.org/linux/man-pages/man5/proc_pid_exe.5.html),
[O_PATH and reopening by fd](https://man7.org/linux/man-pages/man2/open.2.html),
and [readlinkat and truncation](https://man7.org/linux/man-pages/man2/readlinkat.2.html).
