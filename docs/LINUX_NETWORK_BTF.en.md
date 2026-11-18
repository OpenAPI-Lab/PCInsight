# BTF preparation for network accounting (F03 in progress)

The internal `src/linux_network_btf.hpp/.cpp` module reads and validates kernel
metadata for a future optional provider of per-process socket activity. **It is
not a traffic collector or eBPF loader.** This unit adds no public capture API,
counters, maps, programs, or attached links. It does not use clang, libbpf,
bpftool, or any other external utility to read BTF.

## What it resolves and what it does not prove

`read_linux_network_btf` opens `/sys/kernel/btf/vmlinux` with a pinned
descriptor, O_NOFOLLOW/O_CLOEXEC, bounded reads, and identity and metadata
checks before and after. The portable parser resolves an internal
LinuxNetworkBtfLayout:

- Exact, unambiguous FUNC IDs for sock_sendmsg and sock_recvmsg, with non-extern
  definitions, signed int32 returns, and the expected arguments (socket* and
  msghdr*, plus int32 flags for receive). It does not accept a same-named symbol
  with a different signature or guess fexit context indices.
- From revision 28, it also requires __sys_sendto: a signed int32 return and six
  arguments of types int32, void*, uint64, unsigned32, sockaddr*, and int32.
  This function has a different fexit return slot. The real test showed that
  sock_sendmsg does NOT observe send/sendto on this kernel: the path uses
  __sock_sendmsg, which was absent from its BTF (possibly inlined, but not
  proven). sock_sendmsg remains a reference layout, not a complete send
  provider. The private test uses __sys_sendto exclusively for sends and does
  not cover sendmsg/sendmmsg.
- Revision 29 adds optional inet_sendmsg/inet6_sendmsg signatures: int32 return
  and socket*, msghdr*, and 64-bit size_t arguments. Missing symbols have internal
  ID 0 (unavailable); incorrect or ambiguous signatures are rejected. The
  classified test uses these hooks rather than __sys_sendto and requires both
  for a positive IPv4/IPv6 result. This is not universal capture availability.
  See [classified counters](LINUX_NETWORK_CLASSIFIED.en.md).
- socket.sk must point to struct sock. The parser resolves __sk_common.skc_family
  as uint16 and sk_protocol as uint8 or uint16. It stores the offsets and width
  it finds, **not fixed offsets copied from another kernel version**.
- task_struct.group_leader must point to task_struct. The uint64 start_boottime
  and uint32 flags fields must be aligned and within their structures.

These are BTF-verified types, signatures, and offsets, not kernel memory values.
They do not prove fexit is enabled, that a usable trampoline exists, that the
verifier will accept bytecode, or that the user can load and attach probes.
Support::available means only that this compatible layout was resolved. It
does not justify turning absent capture into a zero rate.

These IDs and offsets are not reused across kernels or boots or exposed as
process identity tokens. task_struct.start_boottime is not presented as procfs
start_ticks. Relating the two to clocks and namespaces and identifying event
actors requires later work.

## Validation and limits

The parser accepts BTF v1 in native byte order with a bounded extensible header
and known flags. Type and string regions must be inside the blob and must not
overlap. The string table must start and end with NUL, and offsets and name
termination are checked. Each type's payload length must match its kind and vlen;
unknown new kinds are not blindly skipped. The parser handles the 19 standard
kinds to advance and check references and names. It does not replace the kernel's
BTF verifier for all types.

Referenced IDs must exist. typedef/const/volatile/restrict/type_tag are unwrapped
to a maximum depth of 32, preventing infinite loops on cycles. Selected fields
cannot be bitfields, misaligned, outside their structure, or of unexpected width
or signedness. Ambiguous selected type or function names are rejected instead of
choosing the first. The initial layout requires 64-bit kernel pointers; native
32-bit builds are not declared compatible or given 64-bit offsets.

Limits: 64 MiB of source data, 1,000,000 types, 4,096-byte names, and four EINTR
retries in the reader. These bounds limit memory use, with no global cache or
background work. There is no hard I/O or parsing deadline. Missing sources,
denials, and incompatibilities yield absence with a reason; reader EPERM/EACCES
preserves permission_required. Non-Linux backends do not read the source and
return absence. The system source is trusted; this is not authentication against
root replacing mounts or a cryptographic kernel signature.

## Authorization and next step

Read-only WSL inspection found UID 1000, CapEff=0, unprivileged_bpf_disabled=2,
kernel 6.6.114.1-microsoft-standard-WSL2, and readable BTF. Read-only revision 26
did not modify that sysctl, assign capabilities, or run as root. Revision 28
uses an authorized temporary root process for its private test without changing
sysctls or permanent capabilities. Reading BTF successfully does not imply BPF
permission.

Revision 26 requested explicit authorization for temporary probes filtered to
private processes and removed afterward, without firewall changes or tool
installation. The user explicitly granted it on 2026-10-18. Revision 26 does not
call bpf/perf_event_open, load or attach programs or maps, or capture contents or
process events. The production loader, filters, TCP/UDP accounting, losses,
overflow and windows, identity/UID/namespaces, and cleanup remained pending.
See the [revision 28 manual probe test](LINUX_NETWORK_PROBE.en.md). There is
no automatic elevation.

At this checkpoint F03 remains E, J04/H04 remain open, and legacy network data
is absent. No completed subtask is added to raise progress for the parser.
This verifies layout preparation, not positive per-app TCP/UDP capture.

## Tests

Revision 28 extends fixtures for the __sys_sendto signature and void pointers,
with rejection cases for argument count, width, pointee, and ambiguity. Native
BTF passed without a skip, and Linux passed 100/100 and Windows 114/114. The
[manual eBPF test](LINUX_NETWORK_PROBE.en.md) is separate and limited. BTF
compatibility is not universal capture availability.

Portable fixtures cover valid layouts and offsets, typedefs and cycles, other
kinds, malformed headers/byte order/flags/regions/strings/truncation/IDs, types,
signatures/linkage, out-of-structure or misaligned fields and bitfields,
ambiguous hook names, signedness/width, and incompatible 32-bit pointers.
The native test only reads real BTF and prints the layout or reports skip 77
with a reason. A skip does not count as a pass. Final results: Linux 98/98 and
Windows 112/112 in Release; both installed consumers were relinked and exited 0.
Native BTF passed without a skip on WSL 6.6.114.1: send FUNC 126968, recv FUNC
126963, socket.sk 24, sock.family 16, sock.protocol 516/2, task.group_leader
2480, task.start_boottime 2728, and task.flags 44. These are observations from
that kernel, not reusable constants or evidence of capture.

The valid fixture initially failed on both systems: `std::string{1,'\0'}`
selected initializer_list and produced bytes 1 and NUL rather than a single
NUL. It was corrected to `std::string(1,'\0')` without relaxing the parser;
an explicit initial-NUL check was added. Final focused tests passed 2/2 on
Linux and 1/1 on Windows. Initial failures are retained in
`work/network-btf-focused-{linux,windows}.log`. Later verification and
layout output are in `network-btf-verified-focused-*`,
`network-btf-verified-build-*`, `network-btf-suite-*`, and
`network-btf-native-linux.log`. Final builds had no warnings. N03 remains
open; passing suites do not identify or fix its historical intermittent failures.

Primary references: [BTF format](https://www.kernel.org/doc/html/latest/bpf/btf.html),
[socket signatures](https://github.com/torvalds/linux/blob/master/include/linux/net.h),
[send/receive implementation](https://github.com/torvalds/linux/blob/master/net/socket.c),
and [BPF program types](https://www.kernel.org/doc/html/v6.16/bpf/libbpf/program_types.html).
