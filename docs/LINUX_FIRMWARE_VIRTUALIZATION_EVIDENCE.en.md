# B08: Evidence of the firmware virtualization setting

Revision 171, 2026-11-06. Primary-source review and local read-only checks.
No speculative provider was added, and the result of
`CpuInfo.virtualization_enabled` did not change. B08 remains open: it needs a
source tied to the correct setting and a positive test on compatible hardware.

| Source | What it can establish | What PCInsight must not report |
| --- | --- | --- |
| [SMBIOS Type 4, Processor Characteristics](https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.4.0.pdf) | Characteristics the firmware attributes to the processor, including virtualization capability. | The current UEFI/BIOS setting. |
| CPUID, `vmx`/`svm`, or a CPU register read from a guest | Capabilities visible to that environment. The provider does not read MSRs or open devices to obtain them. | The physical firmware state, or enabled/disabled based solely on the presence or absence of a flag. |
| [`KVM_GET_API_VERSION`](https://docs.kernel.org/virt/kvm/api.html) | The KVM interface version queried by that process; the Linux metric already reports it separately. | Firmware state or successful creation or execution of a VM. |
| [Linux `firmware-attributes` class](https://github.com/torvalds/linux/blob/master/Documentation/ABI/testing/sysfs-class-firmware-attributes) | `current_value` exposes the value of a setting provided by the driver; `type` and `possible_values` describe its values. | A universal attribute name or effective enablement inferred from the name, default value, or a similar string. |

## OEM candidate and limits

The [Dell reference](https://dl.dell.com/topicspdf/command-configure_reference-guide4_en-us.pdf)
distinguishes processor virtualization from the VT-d I/O setting.
The [Linux dell-wmi-sysman driver](https://github.com/torvalds/linux/blob/v6.12/drivers/platform/x86/dell/dell-wmi-sysman/sysman.c)
is one candidate. **Inference, not a validated mapping:** a vendor CLI option
name does not establish the basename the driver exports on a given model.
Nor should the `ProcVirtualization` name from the
[iDRAC interface](https://dl.dell.com/topicspdf/idrac9-lifecycle-controller-v32-series_cli-guide_en-us.pdf)
be carried over to sysfs without evidence.
[Lenovo ThinkLMI](https://github.com/lenovo/thinklmi) is another candidate of the
same kind, not a universal alias. Installing those CLIs or adding a mandatory
dependency is not proposed.

Where available, the pending-reboot indicator applies to the provider's set
of settings; it does not necessarily identify the virtualization setting.
An observed configuration value is not proof that KVM works. Do not read
`default_value` as the effective setting or enumerate authentication
attributes, keys, or passwords.

## Minimum evidence for implementing a mapping

1. Use native Linux with an existing driver. Record the model, firmware
   version, kernel/driver, and exact attribute path. Do not include serial
   numbers or attribute dumps. A VM or WSL does not confirm the host setting.
2. Cross-check the OEM identifier against documentation and the **CPU**
   virtualization setting, not IOMMU/VT-d/SR-IOV. Read only `type`,
   `possible_values`, `current_value`, and, if applicable, `pending_reboot`.
   Compare with the value already known from the firmware interface. No
   setting change is required to collect an observation.
3. Accept only an exact provider mapping. Use bounded fixtures for valid and
   unknown values, permissions, absence, and changes. If there is ambiguity,
   a pending change, or absence, keep the value unknown. Do not use KVM or
   SMBIOS as a fallback for this boolean.

## Local result

WSL returned `ENOENT` for the `firmware-attributes` class, the three OEM
paths checked, the KVM sysfs entry, and `/dev/kvm`. This applies only to the
paths visible in that environment; it does not prove that virtualization is
disabled in physical firmware. The focused `linux-virtualization` test passed
ten times: the API 12 case is a classification fixture, not a positive hardware
reading. No writes, elevation, module loading, or VM creation occurred. No
production code or ABI changed. Logs: `work/firmware-b08-*.log`.
