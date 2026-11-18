# SELF capture scoped to the socket namespace

Revision 65, F03 in progress. The private session now supports
`start(authorized, expected_socket_netns_cookie)`. A nonzero value declares
the only accepted socket namespace. It neither selects another PID nor
widens SELF scope. Zero retains the prior unrestricted-namespace mode and is
returned as an absent optional field, not a valid identity.

## Identity and BTF

The BTF resolver checks the actual kernel path:
`socket.sk -> sock.__sk_common.skc_net.net -> net.net_cookie`.
It validates types, unsigned 64-bit widths, the struct net pointer, wrappers
and typedefs, container bounds, and offsets bounded to 1 MiB. The layout is
optional. A missing field or CONFIG_NET_NS, duplicates, an incorrect pointer,
or an invalid wrapper leaves this capability absent. It does not invent a
zero offset or prevent older tests. Cookie mode requires an exact layout
before creating maps or loading programs.

Identity comes from the **socket used by the operation**, not current->nsproxy,
getpid, the /proc/.../ns/net inode, an interface name, or the observer's
network namespace. Sockets can survive namespace changes or be passed between
processes; the process's current namespace does not establish the socket's.

The code uses bounded BPF reads and offsets from live BTF, not fixed kernel
addresses or a namespace-cookie helper for a different program type.
It needs no utilities or telemetry SDK. The cookie is opaque, not a pointer.
The output field `expected_socket_network_namespace_cookie` represents the
restriction declared by the caller, **not** an observed namespace inventory.
A fresh map can contain zero before any matching operation occurs. This
does not prove a socket exists in the declared namespace or that coverage
is complete.

## Rejection and consistency

After the early TGID filter and start/UID token check, each positive return
checks socket/sk/net/cookie. A null, failed read, zero cookie, or mismatch
sets a quality failure under the same writer/sequence guard. It is sticky:
later reads remain absent until capture is removed and recreated. Bytes
from different namespaces are not summed, and failures are not treated as
zero activity. Even positive PEEK/TRUNC/ERRQUEUE receives first undergo the
namespace check. If they match, they count as exclusions, never as useful
payload. Failed operations and zero returns still contribute no bytes.
An unrelated TGID exits before reading context or socket data. No payload
contents or network addresses are captured.

This mode fails closed on mixed namespaces. It does **not** yet implement
dynamic buckets for each netns. At this checkpoint, it does not handle
multiple actors, intervening UID changes, joins with procfs/apps/users,
coverage of all I/O paths, or reducer/Monitor/rate/history integration.
These experimental totals do not populate total fields in
ProcessInfo/UserSession. H04/J04 remain open.

Revision 78 adds a SEPARATE [declared-bucket mode](LINUX_PROCESS_NETWORK_BUCKETS.en.md)
to the public provider: other nonzero cookies are excluded without writing
to the map, leaving this strict mode unchanged. It supports multiple rows
per process with disjoint cookies, not automatic discovery or proof that a
declared namespace exists.

## Verification

Existing fixtures were extended without a new task ID or dependency:

- BTF: the valid path and ten corruptions involving pointers, widths,
  alignment, duplicates, wrappers, cycles, bounds, or a missing field.
  The optional capability fails closed.
- Bytecode interpreter: IPv4/IPv6, TCP/UDP, send/receive, 64-bit cookies
  with high bits, null/zero/mismatch/read failure, excluded flags, and
  unrelated TGIDs.
- Session: rejection of absent or estimated layouts, declared metadata,
  sticky failure, and stop/restart without carrying over the old map's
  restriction.

Manual mode `--authorized-self-namespace-session` uses the prior temporary
private WSL authorization. It obtains SO_NETNS_COOKIE from its own UDP
socket, runs the loopback matrix, and then tests a deliberately different
value. It does not create another namespace, call setns/unshare, or configure
interfaces. The negative case tests a real cookie against a mismatched
declared value, **not** two real namespaces at once. Without the flag,
capture is not loaded and the test returns 77.

The authorized native test exited 0 on WSL x86_64: cookie 1 confirmed,
TCP 6,144/6,144 B, UDP 384/384 B, 12 peeks, child exclusion and own threads,
and detach/stop/restart with no descriptor leaks. A different cookie
persistently invalidated reads. The previous unrestricted mode also exited 0.
Logs: `work/network-netns-native-linux.log` and
`work/network-netns-legacy-native-linux.log`. This does not certify all
kernels, memory models, architectures, or containers with separate namespaces.

Linux Release regression: 116/116; eight network tests repeated 20 times
each; installed consumer rebuilt and exiting 0. No build warnings.
Additional logs: `work/network-netns-{build,focused,suite,repeat,install,consumer}-linux.log`.
No Windows builds or captures were run for this unit, and no public API changed.

Primary sources reviewed: [Linux 6.6 struct net/possible_net_t](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/net/net_namespace.h)
and [Linux 6.6 SO_NETNS_COOKIE](https://raw.githubusercontent.com/torvalds/linux/v6.6/net/core/sock.c).
The effective layout is validated against BTF from the running kernel, not
offsets from those sources. [Lifecycle](LINUX_NETWORK_SESSION.en.md) and
[guards](LINUX_NETWORK_QUALITY.en.md) retain the earlier limitations.
