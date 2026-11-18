# Shared firmware inventory

Linux reads the binary table from sysfs. Windows uses
`GetSystemFirmwareTable` with the RSMB provider. Both use the same memory
parser without external tools. The table limit is 16 MiB. The inventory
is retained for the lifetime of the monitor; it is not a source of live
utilization, temperature, or frequency.

## Reading rules

- Tables with invalid lengths, truncated records, or unterminated strings
  are rejected without partial results. The Windows wrapper also validates
  its length. Padding after the end marker is allowed.
- Devices are filtered to system memory arrays when firmware publishes that
  relationship. Without arrays, the data is marked estimated because it
  can include memory other than system RAM.
- Slots are positions described by firmware device records, not a physical
  motherboard inspection. They can also represent soldered memory. Positions
  omitted by firmware are not reconstructed.
- Unknown size does not count as an empty slot or allow a partial sum to be
  presented as total capacity. Extended sizes and KiB/MiB units are accepted.
  An absent extended size is not interpreted as 32,767 MiB.
- Type, form factor, and speed are summarized only when all installed
  devices have the same known values. The current API does not expose a
  per-DIMM list. A heterogeneous combination leaves the summary absent.
- Speed is the configured speed, including extended fields. Maximum module
  speed is not used as a substitute.
- CPU SMBIOS Current Speed is an estimate of the value at boot and does
  not guarantee nominal base frequency. Empty sockets are ignored, and
  the summary is omitted when processors report different values.
- An accessible table does not imply every field is accessible. A missing
  field returns `unsupported`, never `available` without a value.
  `permission_required` is retained if permissions prevented access.

Firmware can contain incorrect information even in a structurally valid
table. There is no promise of obtaining missing data on every machine.
Persistent memory and advanced logical memory cases require a more detailed
per-device model before exhaustive inventory can be offered.

## Tests

In-memory and file-read fixtures cover DDR4, empty slots, unknown and extended
sizes, extended configured speeds, mixed devices, exclusion of video arrays,
missing ownership, populated and unpopulated CPUs, and truncation of records
and the Windows wrapper. Assertions remain enabled in Release.

References: the [Windows firmware API](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemfirmwaretable)
and the [DMTF SMBIOS specification](https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.9.0.pdf).
