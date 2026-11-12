# Verify PCInsight on Linux hardware

**Language:** [Español](LINUX_HARDWARE_EVIDENCE.md) · English

The verifier uses PCInsight 0.2.0 from an installed SDK. It is separate from
the library and the future GUI and adds no dependencies for SDK users. A sample
shows what the SDK could observe in that environment. It does not by itself
validate any field on physical hardware.

## Run the check

From the PCInsight root, build and install the SDK into a local prefix.
If you already have the SDK from the same revision, skip the first three
commands and point `CMAKE_PREFIX_PATH` at its directory. Do not use `sudo`.

```sh
cmake -S . -B build/hardware-sdk -DCMAKE_BUILD_TYPE=Release
cmake --build build/hardware-sdk
cmake --install build/hardware-sdk --prefix "$PWD/build/hardware-prefix"
cmake -S tests/linux_hardware_evidence -B build/hardware-verifier \
  -DCMAKE_PREFIX_PATH="$PWD/build/hardware-prefix"
cmake --build build/hardware-verifier
ctest --test-dir build/hardware-verifier --output-on-failure
./build/hardware-verifier/linux-hardware-evidence --sample > evidence.txt
```

Sampling does not change permissions or settings, start services, enable
eBPF capture, request OpenGL contexts, or probe DRM clients. The report
omits text fields, notes, interface names, IP addresses, and exact PCI
locations. Numeric identifiers and capacities can still identify a hardware
profile; review the file before sharing. `unknown` is not zero and
`estimated` is not a direct measurement. GPU, disk, and interface indices
identify rows only within that sample.

## Physical hardware acceptance criteria

- **N02 — System.** Record whether the run was native or virtualized, plus the distribution, kernel, and relevant driver. Cross-check positive results against system sources: CPU topology and frequencies; RAM type, speed, and slot occupancy when firmware reports them; SATA/NVMe disk capacity and transport; and physical or USB network adapter identity when available. Keep the KVM API distinct from the firmware virtualization setting. Spread checks across several machines if needed, and record unavailable or restricted fields without counting them as passes.
- **G07 — GPU.** Record positive results by vendor (AMD, Intel, NVIDIA), device, and driver. Cross-check inventory, memory, and telemetry that the corresponding source exposes. An observed API version does not prove rendering or coverage of every engine. The report retains all engine rows without assuming utilization percentages are available. Opt-in paths need separate tests; an empty WSL sample does not validate any physical GPU provider.

Synthetic cases verify the reporting contract, not the hardware. N02 and
G07 remain open until each has corresponding physical evidence.

## Share results

You do not need to verify the whole machine. One device or field helps. Open
an issue using the **Linux hardware validation (English)** template. Include
unexpectedly missing fields, and do not change permissions to force a pass.

```text
PCInsight version and commit:
Environment (native Linux / VM / WSL), distribution, kernel, and architecture:
Device and driver (model and version if known; no serial number):
Field checked (CPU/RAM/disk/network or GPU):
PCInsight status (available / estimated / unknown):
Observed value and unit, if safe to share:
Independent source consulted and expected result:
Result (matches / differs / could not verify):
Reviewed report excerpt, if useful:
```

Share only the rows needed. Do not attach full firmware dumps, DMI binary
files, system logs, or captures containing personal identifiers. If no
independent source is available, describe that limitation without marking
the observation as validated.

**Linux delivery (revision 177):** Publish the verifiable scope without
waiting for physical hardware. Distinguish “tested in WSL,” “tested with
simulated data,” and “not validated on hardware.” Simulated cases should
cover valid data, missing data, and relevant failures. They do not prove
that real firmware or a driver exposes a field. If the source is absent,
keep the value unknown. N02 and G07 remain tracked for follow-up; a fixture
passing does not establish that B08 has an OEM provider.
