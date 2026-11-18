# Linux adapter identification (F02)

## Explicit address availability query

```cpp
#include <pcinsight/linux_network_addresses.hpp>
auto report = pcinsight::query_linux_network_addresses();
if (pcinsight::metric_is_known(report)) {
    // An empty list here DOES mean a completed dump with no addresses.
    for (const auto& row : *report.value) {
        // row.interface_index, row.ipv4, row.ipv6
    }
} else {
    // Display unknown and report.note; do not invent a known empty list.
}
```

This is a separate opt-in query of the process's network namespace.
It includes only indices with reported addresses, not adapters without
IP addresses. It does not fall back to `getifaddrs`: an incomplete dump
remains unknown. Observed `EACCES` or `EPERM` from syscalls yields
`permission_required`. The note also retains valid error codes from
[kernel Netlink](https://docs.kernel.org/userspace-api/netlink/intro.html)
error replies, with the same permission classification. An ACK is not
accepted as a complete dump. Notes distinguish a timeout, a budget limit,
and an invalid reply. This does not grant permission to elevate. Other
unrecognized denials may remain unsupported. The query does not change
`NetworkAdapter` or make its result atomic with Monitor. Do not join by
index across namespaces or keep an index as permanent identity. Other
systems return unsupported. The addition requires new headers and
library; existing types are unchanged.

The `linux-network-addresses-denied` test forces socket `EPERM` with a
private seccomp filter in the test process. It calibrates the denial and
checks that the public query returns `permission_required` with no value.
Host permissions are unchanged. If the environment does not allow the
filter, CTest records skip 77.

## Adapter inventory

The primary reader uses
[`RTM_GETADDR`](https://docs.kernel.org/netlink/specs/rt-addr.html#getaddr)
and associates IPv4/IPv6 by `ifa-index` with the sysfs index of the counter
row. For the join, [`if_nametoindex`](https://man7.org/linux/man-pages/man3/if_nametoindex.3.html)
must also resolve that name to the same index from the process. Failure or
disagreement leaves addresses empty without discarding counters. A match
does not establish identity across network namespaces that reuse a name
and index. The join does not depend on the address label. If `IFA_LOCAL`
exists, it takes precedence over `IFA_ADDRESS` so a point-to-point peer
is not shown as the local address. Only an error-free completed dump is
used. Partial, interrupted, invalid, over-4-MiB, or over-100-ms responses
are discarded.

In that case the previous fallback remains: the `AF_PACKET` record in
the same [getifaddrs](https://man7.org/linux/man-pages/man3/getifaddrs.3.html)
list must match the sysfs row's name and index. This fallback can still omit
addresses whose `IFA_LABEL` appears as a different name. Neither path
makes the sample atomic or detects all index reuse. Synthetic tests cover
labels and point-to-point addresses; the native test checks loopback.

`NetworkAdapter::linux_details` is a Metric of LinuxNetworkDetails. It
replaces the heuristic that displayed PCI_ID, driver, or modalias as the
model and classified any non-lo, nonwireless interface as Ethernet.
Interface name and ID retain their meanings; they are never interpreted
as a vendor or medium.

## Data and sources

| Data | Source / meaning |
|---|---|
| interface_index | ifindex, checked against the counter row |
| link_layer_type | sysfs type (ARPHRD), not the bus or physical medium |
| connection_type | ARPHRD plus wireless/phy80211 and virtual/net markers |
| bus / driver | Basename of the device's subsystem / driver symlinks |
| hardware_vendor_id / hardware_device_id | 16-bit PCI or USB IDs according to the bus |
| vendor / model | Optional local PCI labels or USB strings |
| virtual_device | Parent compared with devices/virtual/net |
| wireless | Valid wireless/phy80211 directory |

An absent outer report means coherent identity was not established.
When the report is present, individual Metrics can still be absent with
reasons. Legacy `model` and `connection_type` reflect values or empty
strings; they do not show modalias as a model. Windows leaves linux_details
unsupported and retains its network provider. Virtual and loopback
interfaces do not inherit host vendor, model, or driver. Without a
supported PCI/USB source, vendor and model remain unknown even when the
bus or driver is known. This is not universal commercial identification
or BIOS data.

## Type without name heuristics

ARPHRD 772 yields Loopback even after renaming. PPP 512, InfiniBand 32,
and IEEE802.11 (801–803) have their own labels. 802.11 monitor mode does
not imply an associated connection. CAN and IP/GRE tunnel types recognized
by the [kernel](https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/linux/if_arp.h)
have link-layer labels without identifying underlying physical transport.
Other types keep their number and have no label; they do not default to
Ethernet. ARPHRD_ETHER 1 with a wireless marker yields Wi-Fi; with a known
virtual parent, Virtual Ethernet link-layer; otherwise Ethernet link-layer.
Labels do not certify cable, fiber, speed, link state, or Internet access.
An inaccessible marker is not treated as false. virtual_device=false
does not certify physical hardware. Exact veth, bridge, VLAN, or tunnel
classes are not fabricated without evidence.

## Vendor and model

PCI vendor/device use 0xHHHH. IDs are available even without a dictionary.
Names require a unique entry in local pci.ids: root-level vendors and
devices indented by one tab. Subsystems and classes do not substitute for
devices. Duplicates, NUL, empty input, input over 16 MiB, or lines over
4,096 B invalidate dependent labels, not IDs. Names are printable ASCII,
estimated with a path, not authenticated retail names. This reuses the
bounded regular-file reader described in [PCI names](LINUX_GPU_NAMES.en.md) without libpci,
lspci, downloads, or installation. Monitor loads the first available
database when network or GPU data is requested and caches the result even
if no database is available. Create another Monitor to pick up changes.
Mapping is limited to the first 256 network rows per sample; later rows
keep IDs and metadata without PCI labels.

USB requires subsystem=usb and a unique DEVTYPE of usb_device. If the
device is a usb_interface, the reader moves up exactly one parent,
which must also be on the USB bus and have DEVTYPE=usb_device. It reads
idVendor/idProduct (HHHH) and manufacturer/product with independent
availability. Device strings are estimated and unauthenticated. Empty
strings, controls, non-ASCII text, and values over 4,096 B remain unknown.
The USB interface name, driver, or an arbitrary ancestor is not used as a
substitute. No usb.ids or lsusb is used. This conservative provider does
not accept non-ASCII multilingual strings or promise coverage of all
USB text.

## Consistency and limits

The reader pins root, interface, and device by fd. ifindex must match
F01, and type must be a valid 16-bit decimal value. It verifies ifindex,
type, and the same interface directory before and after. An existing
device must keep its directory and bus at the end. This is not an atomic
snapshot of IDs, strings, driver, addresses, and traffic or a generation
token for hotplug/reset. ABA changes can go undetected. Recreate Monitor
when changing the network namespace. Metadata failure does not clear
valid counters: the row remains with an absent report and empty text.
F01 baselines and rates are unchanged.

Attributes are opened read-only with CLOEXEC/NONBLOCK/NOFOLLOW; only
regular files up to 4,096 B with no NUL are accepted. Attribute FIFOs and
symlinks are rejected without blocking on open; sysfs directory symlinks
are followed. Names are 1–15 B, with no slash, NUL, dot or dot-dot, and
internal roots are bounded. EACCES or EPERM preserves permission_required
when supported by evidence. No elevation, commands, interface changes,
or external traffic are involved. Limits are not syscall deadlines or
a global memory cap. include_networks=false skips the provider. Caching
the local database does not retain hardware metadata: sysfs identity
is queried each sample without keeping data from a removed NIC.

## Usage and compatibility

```cpp
for (const auto& adapter : snapshot.networks) {
    if (!adapter.linux_details.value) continue;
    const auto& d = *adapter.linux_details.value;
    if (d.vendor.value) { /* display with support and note */ }
    if (d.model.value) { /* do not confuse with driver or interface name */ }
    // Interpret numeric IDs with the bus.
}
```

NetworkAdapter has a new public 0.x layout. Rebuild the library and all
consumers, including on Windows, with the new headers. F02 adds no direct
fields to SystemSnapshot, but its NetworkAdapter elements must be
compatible. No stable ABI is promised.

## Tests

tests/linux_network_identity.cpp covers PCI labels, duplicates, missing
values, classes, subsystems, NUL, and bounds; name-independent types,
virtual/loopback/Wi-Fi/PPP/unknown; simulated PCI and USB parents,
DEVTYPE, strings, and IDs; invalid ifindex/type; FIFO or symlink attributes;
and absence. Private sysfs fixtures do not create adapters. Positive
PCI/USB/Wi-Fi results are fixtures, not physical NIC evidence. Monitor
integration checks native interfaces and opt-out. Physical validation
remains under N02.

Verification on 2026-10-16: Linux Release passed 81/81 and Windows Release
passed 101/101. Packages were reinstalled and rebuilt external consumers
exited 0. The final Linux build had no warnings. Native WSL showed lo,
ifindex 1, ARPHRD 772 Loopback; and eth0, ifindex 2, ARPHRD 1 Ethernet
link-layer with bus vmbus and unknown model, not a fabricated commercial
NIC. Positive PCI/USB/Wi-Fi cases are still fixtures.

An initial Linux regression failed in linux-integration because global CPU
was absent (assert second.cpu.utilization_percent.value, line 15).
The network identity and rate test passed. Logs were retained in
work/network-identity-failed-linux-tests.log and
work/network-identity-failed-linux-list.log under the workspace. Then
integration passed 3/3 and the suite passed 81/81 without code or
assertion changes. The cause remains unknown. This recurred under the
existing N03 risk; F02 does not fix it.

Primary sources: [net sysfs ABI](https://github.com/torvalds/linux/blob/master/Documentation/ABI/testing/sysfs-class-net),
[ARPHRD](https://github.com/torvalds/linux/blob/master/include/uapi/linux/if_arp.h),
[PCI sysfs](https://docs.kernel.org/PCI/sysfs-pci.html), and
[USB sysfs](https://github.com/torvalds/linux/blob/master/drivers/usb/core/sysfs.c).
Presentation labels and limits are PCInsight decisions, not kernel
guarantees of retail identity.
