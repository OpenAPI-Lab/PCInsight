# Linux GPU names from local PCI IDs

DRM inventory now retains numeric IDs in hardware_vendor_id and
hardware_device_id (16-bit, Support::available), even without a name
database. It accepts both DRM attributes only in 0xHHHH format and
with a valid available PCI location. It does not parse a driver name
as a number or match by index.

When pci.ids exists, pci_vendor_name and pci_device_name are added as
Metric<string> with Support::estimated and the source path in note.
vendor and model use these strings for display; a later NVML model
takes precedence. PCI labels remain separate even when NVML supplies a
more specific name. They do not certify a retail model, board vendor,
or memory capacity. One PCI entry can describe several cards, and the
database can be stale.

## Reading and limits

The reader tries /usr/share/hwdata/pci.ids, /usr/share/misc/pci.ids, and
/usr/share/pci.ids in that order. It keeps the first readable, nonempty
regular file bounded to 16 MiB with stable size and mtime during the
read. It opens with O_CLOEXEC/O_NONBLOCK and does not read FIFOs or
directories. It does not run pciutils/lspci, link libpci, download or
install anything, access the network, or require root. The database is
not redistributed, and consumers are not required to install it.

Monitor loads the database lazily when GPU or, since F02, network data
is requested, and retains the result for its lifetime, including
absence. Create another Monitor to pick up database updates. GPU
inventory and IDs are collected afresh each sample; labels from a
removed GPU are not reused.

The built-in parser accepts top-level vendors and devices indented by
one tab, four-digit hexadecimal IDs, printable ASCII names, LF or
CRLF, and EOF without LF. Subsystems indented by two tabs do not
replace the device. Sections C, S, and unknown future sections reset
the current vendor, including for their child entries; comments do
not. Duplicate vendors invalidate their labels, and duplicate devices
invalidate the model even when the text matches. A line over 4,096
bytes invalidates all labels rather than publishing truncated records.
The parser stores only queried IDs for up to 256 GPU rows. Additional
rows retain IDs without labels.

Without a file, unique entry, or valid name, labels remain absent and
raw DRM strings are retained. No model is invented. IDs are not
atomic with respect to hotplug. This provider does not query ROM/BIOS,
driver dates, or non-PCI GPUs. Failure to access the optional database
is not confused with hardware permission denial. On Windows the new
PCI labels remain unsupported; DXGI is unchanged. The 0.x GpuInfo
change requires rebuilding the library and consumers.

## Verification

The dedicated Linux test uses a synthetic database and covers multiple
GPUs, vendors, and domains; raw IDs; NVML precedence; propagation of
NVML permissions even with a friendly name; absence; invalid IDs;
empty sources; duplicates; classes, subsystems, and unknown extensions;
long lines; NUL and controls; total size; and row limits. The native
loader was tested with its own temporary files, regular files,
missing paths, a directory, a FIFO without blocking, an oversized
sparse file, and permission denial for non-root callers. The system
database is read without modification. These tests do not replace
identification of a physical card on Linux.

Release passed 42/42 on Linux and 55/55 on Windows. Packages were
installed and external consumers rebuilt and run. The native query
found /usr/share/misc/pci.ids (1,444,058 bytes). WSL did not expose a
physical DRM card for end-to-end hardware identification.

Sources: [pci.ids format](https://github.com/pciutils/pciutils/blob/master/pci.ids.man)
and [PCI sysfs attributes](https://docs.kernel.org/PCI/sysfs-pci.html).
