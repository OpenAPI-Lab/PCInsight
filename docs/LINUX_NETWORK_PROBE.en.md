# Private eBPF capability test (F03 in progress)

The following preserves the original revision 28 mode. Revision 29 adds
[classified mode](LINUX_NETWORK_CLASSIFIED.en.md) with four TCP/UDP counters
for IPv4/IPv6, excluded flags, and sendmsg tests. The original mode remains
available and passed its native regression test.

`src/linux_network_probe.hpp/.cpp` generates 18 eBPF instructions for a
bounded test. `tests/linux_network_probe_native.cpp` loads, attaches, and
cleans up its own objects through BPF syscalls without clang, libbpf,
bpftool, or other utilities. It is not the production provider and is not
connected to Monitor, the accounting reducer, ProcessInfo/UserSession, or
history. There is no new public API.

## Authorization and execution

On 2026-10-18 the user explicitly authorized temporary WSL probes filtered
to private processes, with the necessary privileges and removal afterward,
without permanent configuration or tool installation. The test runs
manually in a temporary root process. It does not assign capabilities or
change sysctls, firewall, services, cgroups, or startup configuration.

The executable requires `--authorized-private`. Without it, it returns 77
without loading a program, map, or link. It is not registered in CTest:
**the ordinary suite never runs this privileged test.** Constructing or
using Monitor does not attach probes. This authorization does not extend to
capture of unrelated processes.

Before attaching, the test obtains its own kernel TGID with an unattached
RAW_TRACEPOINT program, executed through BPF_PROG_TEST_RUN without context
or data or remote-CPU flags. It returns only get_current_pid_tgid >>32 and
closes immediately. getpid() and the helper are not assumed to use the same
namespace. In the observed run, visible PID 508 was kernel TGID 4492.
If identity cannot be verified, the test exits without attaching.

The fexit probe filters TGID before reading the return value from context
or accessing the map. It reads only the return from the private operation;
it does not read payload, arguments, or sockets of unrelated processes.
It counts positive, sign-extended int32 returns in two owned uint64
counters, send and receive. It does not change returns or write kernel
memory outside its own map. No objects are pinned.

BTF verifies the signatures: __sys_sendto has six arguments and return
offset 48, and sock_recvmsg has three arguments and return offset 24.
Offsets are not guessed from a different signature. The program uses
BPF_PROG_TYPE_TRACING/BPF_TRACE_FEXIT and BPF_LINK_CREATE links. Map, program,
and link FDs use RAII ownership; the private child closes inherited copies
before using sockets. Links close before the detach check and all objects
close on exit, including exceptions. No perf FDs or persistent files retain
a probe.

## What the test actually verifies

1. The map starts at 0/0. Private IPv4 TCP loopback sends and receives
   1,024 bytes. Contents and counters are checked at the end of the phase.
2. Private IPv4 UDP loopback sends and receives 64 bytes; the increment
   is verified. Total: 1,088/1,088. These are operation returns, not ACKs or
   wire throughput.
3. A private child with another TGID sends the same traffic. The allowed
   TGID's counters do not change, explicitly testing attribution and filtering.
4. The test closes both links, repeats traffic in the allowed process, and
   verifies that counters do not change, explicitly testing removal.

The generator also has a portable CTest with a minimal interpreter for
its instruction subset: filtering before context/map access, success,
zero, negative errors and int32 signedness, maximum int32, null lookup,
slots/jumps, and invalid parameters. It does not replace the kernel verifier.

## Findings and limitations

The first TGID identification attempt used an unattached SOCKET_FILTER
program. The kernel rejected helper 14 as an unknown function; nothing
was attached in that attempt. It was corrected to an unattached
RAW_TRACEPOINT, whose local TEST_RUN executes in the caller's context.

The initial sock_sendmsg/sock_recvmsg fexit pair loaded but gave 0/1,088,
which was not counted as success. send/sendto bypasses sock_sendmsg on this
path, as shown in the Linux 6.6 source. The test tried resolving
__sock_sendmsg, but it was absent from the observed BTF (possibly inlined,
not proven), so it was rejected without a blind fallback. Finally,
__sys_sendto was validated with its own signature and return offset.
The 1,088/1,088 positive test then passed along with filtering and detach.
The original failure and absence logs remain under work/network-probe-*.

This does NOT cover sendmsg/sendmmsg/write/splice/io_uring, asynchronous
operations, or all receive paths. The probe does not distinguish families
or protocols in the map. The fixture selects IPv4 TCP/UDP and checks phases;
it does not claim general classification. It does not handle MSG_PEEK/
MSG_TRUNC, loss, saturation/overflow, persistence, PID reuse, UID at the
time of the operation, start_boottime, or per-socket namespace cookies.
The allowed process lives throughout the test, without PID reuse. This is
not a safe reusable token for capturing other processes.

It therefore does not automatically feed the [revision 27 reducer](LINUX_NETWORK_ACCOUNTING.en.md):
it does not meet the full four-lane, consistency, identity, and coverage
contract. A real producer, typed coverage and privileges, production
filters, procfs/app/user joins, windows and integration, and more kernel
and hardware testing remain pending. At this checkpoint F03 remains E and
J04/H04 are open. No completed ID is created to raise the percentage.

Final result: Linux 100/100 and Windows 114/114 Release. Both installed
consumers were relinked and exited 0.
`work/network-probe-verified-native-linux.log` records the positive run
with verified phases (PID 293/kernel TGID 7067), child exclusion, removal,
and exit 0. `network-probe-no-authorization-linux.log` records rejection
with exit 77 and no loading when the flag is absent. Normal suites do not
run probes.

`network-probe-final-build-linux.log` and `network-probe-final-suite-linux.log`
confirm a clean final incremental build and 100/100 tests. An earlier build
warned of 6–12 ms clock skew in generated compiler_depend.make files.
Those warnings are retained in `network-probe-verified-build-linux.log`;
the clock was not changed and the warnings were not hidden. Windows
`network-probe-final-build-windows.log` had no warnings, and
`network-probe-suite-windows.log` records 114/114. Exit 77 means unavailable
or no positive result, never success. Historical N03 risks are not declared fixed.

Primary references: [program and attach types](https://docs.kernel.org/bpf/libbpf/program_types.html),
[BPF_PROG_RUN](https://docs.kernel.org/bpf/bpf_prog_run.html),
[Linux 6.6 raw_tp TEST_RUN implementation](https://raw.githubusercontent.com/torvalds/linux/v6.6/net/bpf/test_run.c),
[Linux 6.6 socket paths](https://raw.githubusercontent.com/torvalds/linux/v6.6/net/socket.c),
and [BPF UAPI](https://github.com/torvalds/linux/blob/v6.6/include/uapi/linux/bpf.h).
