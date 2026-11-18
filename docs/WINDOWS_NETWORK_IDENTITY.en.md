# Windows adapter PnP identity and M03 completion

Implemented on 2026-10-19. The additional API
`query_windows_network_identity(adapter.id)` is declared in
`include/pcinsight/windows_network_identity.hpp`. It is an explicit
read-only GUI query, separate from Monitor so that PnP is not scanned for
each adapter on every dynamic sample. It does not change NetworkAdapter,
SystemSnapshot, or history.

Returns a Metric of WindowsNetworkIdentity with adapter_id, interface_guid,
manufacturer, and device_description. The last two properties are also
Metric values: a known, valid empty value is not the same as an absent field.
Check metric_is_known before displaying values. Manufacturer means the
value declared by PnP/INF, not an authenticated physical brand, a vendor
independent of the driver, or the machine's OEM brand.

## Source, identity, and errors

Accepts Monitor's luid:<decimal> ID without signs, padding, NUL, overflow,
or zero. GetIfEntry2 retrieves the LUID's GUID and type. SetupAPI enumerates
present devices in the Net class, opens each device's own existing software
key (DIREG_DRV, KEY_QUERY_VALUE), and reads NetCfgInstanceId as a strict
GUID. It requires a unique GUID match, not a match by name, MAC, description,
or enumeration order. It reads SPDRP_MFG and SPDRP_DEVICEDESC, does not
construct registry paths from user input, and does not expose the MAC,
serial number, or PnP instance ID.

It reopens the same node's key and rereads both keys; after enumeration,
it revalidates LUID, GUID, and type with GetIfEntry2. Changes, duplicates,
or global enumeration or association failures invalidate the result. This
is not an atomic snapshot, authentication of an adversarial host, or an
action token. The ID is local to the host and boot, not permanent. Documented
absence of the key or NetCfgInstanceId is skipped as a device without an
association. A logical interface may have no PnP node. Without a match, the
IP Helper identity remains known and the properties are absent. A property
failure does not erase the other property; permission_required distinguishes
access denied. Other failures use unsupported with the operation and error
code. Preserve note.

Limits: 4,096 nodes, 80 bytes for the GUID, and 4,096 bytes per string.
Types, sizes, NUL termination, and UTF-16/UTF-8 are checked, without
unbounded growth or retries. REG_EXPAND_SZ and MULTI_SZ are rejected;
strings are not expanded or interpreted. Keys and the device information
set use RAII. No CLI, WMI, global COM, downloads, new SDKs, writes, network
changes, or elevation. Returns unsupported outside Windows. Synchronous
OS calls have no provider-imposed deadline: use a GUI worker. No caching
or implicit effects.

## Verification

tests/windows_network_identity.cpp covers invalid input before I/O, minimum
rights, permissions, partial scans, duplicates, missing matches, different
or malformed GUIDs or incorrect types, changes between the retained and
reopened key, interface GUID/type changes or final query failures, the
4,096-node limit, invalid registry data, partial, denied, or empty
properties, invalid UTF-16, termination, or type, and handle cleanup.
Positive read-only results: 55 interfaces, 9 manufacturers, and 8 with
Ethernet ifType. Realtek / Realtek PCIe 2.5GbE Family Controller was matched
by its real GUID. Microsoft/Bluetooth, Kernel Debug, and PdaNet were also
present; they are not all claimed to be physical NICs. Twenty identity
repetitions and twenty network repetitions passed (40 runs).

M03 also verifies WMI on 5/5 disks: 2 HDDs and 3 SSDs on SATA/NVMe, without
UAC or permission or configuration changes. The previous 0x80041003 is
historical, not the current state or a cause demonstrated or corrected
by this revision; WMI code was unchanged. Filesystem capacity was available
on 5/5 disks; one was the Windows disk and one had a page file. Read/write
rates and latency were available on 5/5; active_percent was valid on 1/5 in
the detailed sample. Other invalid or out-of-range %IdleTime values (outside
0..100) are rejected without clamping or inventing zero. Availability varies
by sample; a permanently positive result is not required. A later
diagnostic without provider changes showed active_percent on 5/5, recorded
in work/network-identity-active-diagnostic-windows.log. It neither proves
the specific cause of the earlier missing values nor hides them. The second
live network sample showed known send/receive rates on 55/55 interfaces
without generating traffic; a known 0 B/s is not treated as unavailable.

Existing fixtures cover deltas, regressions, identity, IP addresses, and
multi-disk volumes, quotas, duplicates, and errors. Hotplug, RAID, MPIO,
Storage Spaces, and every physical filesystem are not claimed to have
been tested. Logical layers are not allocated arbitrarily. M03 closes the
supported disk, network, and identity providers with explicit availability
and positive results. M02/M06 for processes and sessions, F03 for Linux,
N01 for CI/toolchains, and N03 for auditing remain open. No new third-party
dependencies or changes to storage or connections. This additive API does
not change existing layouts, virtual ABI, or history. Windows Release:
122/122; Linux Release: 108/108. Installed consumers on both platforms
were rebuilt, relinked, and run with exit code 0. Logs:
work/network-identity-*.log.

Primary references:
[identifiers](https://learn.microsoft.com/en-us/windows/win32/network-interfaces),
[software key](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/opening-a-device-s-software-key),
[PnP manufacturer](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/devpkey-device-manufacturer).
