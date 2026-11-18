# Linux per-process RSS: units, availability, and identity

`ProcessInfo.memory_bytes` remains the existing public field. On Linux
it represents VmRSS from `/proc/PID/status` converted to bytes, now with
Support::estimated and a scope note. It is not private or unique memory,
PSS, allocated virtual memory, swap, or the exact equivalent of the Windows
counter. Shared pages can count toward more than one process, so summing a
user's RSS does not necessarily yield unique physical usage.

The old unchecked stoull(VmRSS)*1024 was removed. It could accept prefixes,
ignore units, overflow during multiplication, or lose the row when
conversion threw in the legacy collector.

## Parsing and availability

The built-in provider recognizes VmRSS and Pid by key and colon, validates
one decimal uint64 value with exactly the kB unit, and checks the product
by 1,024 before converting. It rejects signs, fractions, exponents, guessed
units, trailing text, and duplicates of these fields, including when there
is no space after the colon. Pid, when present, is a unique positive uint32.
It is mandatory in native reads and must match the expected identity.

VmRSS of 0 kB is a valid reported estimated zero. Missing VmRSS, possible
for kernel tasks or exiting processes, is not converted to zero. No
fallback derives RSS from other counters, executable size, global memory,
or missing permission. A malformed or duplicate field, a status without
the expected PID, or an inaccessible source leaves the Metric absent.
No partial value is published. Unknown status fields are ignored within
the text limits.

Accepted status limits are 1 MiB, 10,000 lines, and 64 KiB per line;
stat is limited to 64 KiB. NUL invalidates status. Reads use 8 KiB chunks
without reserving 1 MiB for each small process. A chunk that detects an
exceeded limit is discarded without publishing its prefix. EACCES/EPERM
from syscalls becomes permission_required; other errors and invalid
format or range become unsupported. EINTR read retries are limited to
three. These limits are not syscall timeouts or a global bad_alloc guarantee.

## Process attribution

The initial row already has PID and start_ticks from stat. To collect RSS:

1. Open and pin the process directory with O_PATH/O_DIRECTORY/CLOEXEC.
2. Read stat with openat on that directory and check PID and start_ticks
   against the expected identity before reading memory.
3. Read bounded status with openat and validate Pid and VmRSS.
4. Read stat again on the same directory and require the same identity.

Final symlinks are not followed. RAII closes descriptors on errors and
exceptions as well. A disappeared process, invalid stat, or mismatched
PID or start_ticks does not receive the memory reading. A zero start_ticks
is not an error sentinel; it can be valid early in boot.

This is a PID/tick lifetime token within the same boot and namespace,
not a global cryptographic identity or exec generation. Tick resolution
is finite, so a collision before the directory is pinned cannot be ruled
out. Pinning normally prevents subsequent accesses through that fd from
switching directories when a PID name is reused. It does not freeze the
process. RSS can change during or after the read, and exec preserves PID
and start_ticks. The provider trusts the environment's procfs; file fixtures
do not authenticate procfs.

Kernel RSS can update asynchronously. Independent reads need not agree,
and bytes are not presented as an exact physical snapshot. smaps or PSS
would require a separate provider, which is not implemented here.

## Integration scope

The collector assigns the new Metric to each row rather than parsing
VmRSS in its old State/Uid loop. The API, layout, and Windows backend
are unchanged. It does not remove or alter processes, force RAM load,
read /proc/PID/mem, run utilities, or install dependencies. A Monitor
without internal process collection does not run this path.

This work did not harden all of fill_processes. State/Uid, name, ELF,
CPU, I/O rates, and app history retain their own paths and limits.
Other row fields do not form an atomic snapshot with RSS. Overall
resource and error handling in the legacy collector remains open.
Validating this metric does not complete all Processes/Details work.

## Verification

Portable fixtures cover absence versus zero, exact kB units, uint64
and multiplication overflow, duplicates, invalid text or PID, bounds,
and identity before and after reads, including reuse, disappearance,
and zero start_ticks. Private Linux files test openat, bounded reads,
absent or malformed stat/status, wrong PID or start token, oversized
files, and final symlink rejection. Identity changes between reads are
tested in the pure guard, not by forcing reuse of a real PID.

The native test of its own PID reports positive estimated RSS, matching
expected identity, and rejection of a wrong token. Monitor integration
retains the test process's own row and labels published memory as
estimated. Dynamic values need not agree across samples. Permission
denial is tested through synthetic result propagation; no permissions
on other processes are changed.

On 2026-10-15, Linux Release passed 73/73 and Windows Release passed 93/93.
Installation and external consumers exited 0. The focused Linux test
was repeated with detailed output: own PID 1539, start_ticks 5173,
RSS 4,538,368 B, and Monitor 5,976,064 B estimated; an incorrect start
token was rejected. These are two samples of a process whose own RSS
changes while running the tests and Monitor, not two values that
must match.

Primary reference: [kernel status/VmRSS and accuracy limits](https://www.kernel.org/doc/html/latest/filesystems/proc.html).
Validation, limits, and attribution policy are PCInsight decisions.
