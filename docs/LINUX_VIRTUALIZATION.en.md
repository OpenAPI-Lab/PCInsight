# Linux virtualization: firmware state and KVM access are separate evidence

Revision 171: [OEM evidence and criteria](LINUX_FIRMWARE_VIRTUALIZATION_EVIDENCE.en.md).
The WSL check found no visible firmware attribute. The result remains
unknown until the mapping is verified on a compatible machine.

The former `exists("/dev/kvm") => virtualization_enabled=true` condition
was removed. An existing path does not establish enabled firmware, access
permissions, or a working API. A missing device does not establish that
virtualization is disabled in BIOS either.

## Public contract

`CpuInfo.virtualization_enabled` remains without a value and unsupported
in this Linux provider because there is no direct firmware-state source.
It is not reused to indicate KVM access, an active hypervisor, execution
inside a VM, CPU vmx/svm capability, or nested virtualization support.
The Windows provider still uses its firmware flag and is unchanged here.

The new `CpuInfo.linux_kvm_api_version` (`Metric<unsigned>`) publishes 12
with Support::available only after a successful KVM_GET_API_VERSION query
on a descriptor verified against the sysfs KVM registration. This confirms
that API query, not that VM creation or execution, every extension, or a
particular machine type will work. It does not enable hardware, firmware,
or modules.

Permission errors while reading initial registration, opening the device,
querying metadata, or performing the ioctl are classified as
permission_required when errno is EACCES or EPERM. Missing data, unexpected
ABI, inconsistencies, and other failures leave the value absent. They never
become a measured false or zero for KVM or firmware. Failure of the final
registration check means an unstable or unverifiable sample, not a positive
API result.

The new field expands CpuInfo and therefore SystemSnapshot. **Rebuild the
library and all consumers of this 0.x API**, including on Windows. Do not
mix objects or binaries built with different layouts. On Windows the
Linux-specific field is absent. History persistence formats are unchanged.

## Native query

1. Read `/sys/class/misc/kvm/dev`: at most 64 bytes, strict decimal uint32
   major:minor, with no signs, trailing text, NUL, overflow, or empty parts.
2. Use lstat on `/dev/kvm` and require a character device with the registered
   major and minor before opening. Open with O_RDONLY/CLOEXEC/NONBLOCK/NOFOLLOW.
   This conservative policy does not accept a final symlink.
3. Use fstat on the same fd. It must identify a character device whose major
   and minor match the KVM registration. Files, directories, FIFOs, and
   other devices are rejected before the ioctl.
4. Issue only KVM_GET_API_VERSION. Reread registration and require the same
   major and minor before accepting version 12. Other versions are an
   incompatible ABI.
5. Close descriptors with RAII, including on errors or resource exceptions.

The stable ioctl is obtained with `_IO(0xAE,0x00)` from sys/ioctl.h. No
linux/kvm.h header dependency or tools such as libvirt or QEMU are added.
The provider does not issue KVM_CREATE_VM or KVM_RUN, write, query MSRs, or
load modules. Interrupted registration reads are retried up to three times.
Other operations fail closed without unbounded retries.

The provider trusts the environment's sysfs and devfs; it does not
cryptographically authenticate an adversarial kernel, driver, or mount.
Before-and-after comparison does not rule out all changes and reversals,
though the ioctl uses the same fd that was inspected. Hidden sysfs can
prevent validation even when the device is accessible. Some implementations
may reject O_RDONLY; the provider does not reopen for writing to bypass
that failure. Limits are not syscall timeouts. A valid result applies to
that instant, not to another credential, namespace, application, or time.
Each sample clears previous values.

## Verification and limits

Portable fixtures cover major/minor parsing, device identity, API 12
versus unexpected ABI, permission errno versus unavailability, and clearing
an old true firmware value or prior KVM data. The positive portable path
tests classification, not a response from real hardware.

Private Linux fixtures cover a regular file named kvm, a directory, FIFO,
symlink, missing device, and invalid or oversized registration. `/dev/null`
is used only as a character device with a different identity; it is rejected
before the ioctl. No device nodes or VMs are created. Monitor and the
installed consumer check that the metrics stay separate. The Windows
consumer checks that the Linux field is absent. A negative native
observation does not validate the positive hardware path.

Final run on 2026-10-15: Linux Release passed 71/71 and Windows Release
passed 91/91; installation and rebuilt external consumers exited 0. The
focused Linux test was repeated with detailed output. WSL had no KVM
registration (errno 2/ENOENT), so the API value was absent and firmware
status unknown. A positive native API 12 query remains unverified here and
requires a Linux environment exposing KVM. The test does not load modules
or modify permissions to obtain a positive result.

This work does not implement direct firmware reads (MSR/BIOS), active
hypervisor detection, guest enumeration, per-CPU or per-architecture
capability, or a VM creation test. It does not establish universal
virtualization support.

Primary references: [KVM API](https://www.kernel.org/doc/html/latest/virt/kvm/api.html)
and [KVM UAPI](https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/linux/kvm.h).
The public separation, limits, and verification policy are PCInsight decisions.
