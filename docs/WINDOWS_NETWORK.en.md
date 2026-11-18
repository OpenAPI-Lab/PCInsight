# Windows: interfaces, addresses, and traffic

Implemented on 2026-10-13 using GetIfTable2 and GetUnicastIpAddressTable,
joined by InterfaceLuid. `include_networks=true` enables collection; false
skips the queries and clears the rate baselines. No packets are generated
and no network configuration is changed.

## Fields

- id: local LUID as text (`luid:...`). It is not a persistent identity
  portable across machines or reboots. Deltas also require matching GUIDs
  and interface types.
- name: interface alias; model: adapter description supplied by Windows.
  The hardware model is not verified. The additional
  [query_windows_network_identity](WINDOWS_NETWORK_IDENTITY.en.md) query
  retrieves the PnP/INF manufacturer by GUID without inferring it from this text.
- connection_type: Ethernet, Wi-Fi, Loopback, Tunnel, PPP, or a numeric IANA
  type. An Ethernet type can also represent a virtual adapter.
- ipv4/ipv6: configured unicast addresses from system tables, deduplicated.
  IPv6 retains the numeric scope when supplied by Windows. Addresses are
  not filtered by DAD state, and their connectivity is not guaranteed.
- total_sent_bytes/total_received_bytes: the interface's OutOctets/InOctets.
  These are not necessarily totals since boot or traffic for a single application.
- send_bytes_per_second/receive_bytes_per_second: the octet delta between
  samples divided by elapsed monotonic seconds. Rates are integer bytes/s
  rounded down, not bits/s or nominal link speed.

The first sample has no rate. Counter regression, a change in identity or
operational state, a nonpositive interval, or overflow leaves the rate
unavailable. The next valid sample restores calculation; a valid zero delta
yields 0 B/s. Interfaces that disappear are removed from the cache. Rates
are not summed across interfaces: virtual layers, tunnels, and loopback
can count the same traffic multiple times.

The tables include logical and physical interfaces, active and inactive
interfaces, and filters. The GUI can select which ones to display. The
two queries do not form an atomic snapshot: addresses without a matching
interface are omitted, and an interface may change after it is read.
Intervals reflect polling rather than continuous packet measurement.

## Errors and resources

A global failure in either table throws system_error and resets the rate
baselines, instead of returning a partial list as a success. Validation
requires non-null tables, at most 100,000 records per table, nonzero and
unique LUIDs, and valid, terminated Unicode strings. Memory allocated by
Windows is freed with FreeMibTable on every path. The APIs do not report
allocation size: a valid native table is trusted to contain the declared
rows, and the numeric limit does not replace that system-provider guarantee.

The iphlpapi/ws2_32 dependencies are Windows system libraries. No external
installation or collection subprocess is required. Normal reads do not
require elevation; access errors propagate.

## Verification and remaining work

Fixtures cover LUID matching, IPv4, IPv6 scopes, deduplication, Unicode,
deltas, zero, counter regression, GUID or state changes, zero intervals,
resets, failure and recovery, null or oversized tables, duplicate LUIDs,
invalid strings, and memory cleanup. Read-only integration covered 61
interfaces on the local machine and disabling and re-enabling the option.
Windows Release: 15/15; Linux WSL: 6/6. The Linux suite does not validate
these Windows APIs.

2026-10-19 update, revision 38: M03 completes the supported providers.
Disks, volumes, performance, and WMI had positive native results; the
additional PnP manufacturer query reports availability. The second sample
had known send/receive rates for 55 interfaces without generating traffic.
Network and identity checks each passed 20 repetitions. The [network
identity guide](WINDOWS_NETWORK_IDENTITY.en.md) documents scope and limits.
Physical hardware, hotplug, and toolchain coverage are not universal.
Per-process and per-session network metrics remain under M02/M06; CI N01
and audit N03 are separate.

References: [GetIfTable2](https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getiftable2),
[MIB_IF_ROW2](https://learn.microsoft.com/en-us/windows/win32/api/netioapi/ns-netioapi-mib_if_row2),
[unicast addresses](https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getunicastipaddresstable).
