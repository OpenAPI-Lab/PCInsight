# PCInsight 0.2: local Linux delivery

An SDK for reading system data and performing explicit actions from a C++20
program. This local delivery did not include the GUI, declare a 1.0 ABI, or
constitute a GitHub release. The tested profile was Ubuntu WSL x86-64 with
GCC 13.3 and a Release static library.

**Consolidated reference delivery: revision 230.** The
`pcinsight-sdk-0.2.0-linux-x86_64-gcc13-r230.tar.gz` and
`pcinsight-sdk-0.2.0-linux-x86_64-clang18-r230.tar.gz` packages supersede the
earlier packages described below. Each has its own `SHA256SUMS`. In WSL,
GCC passed 201/201 tests with the test recorder enabled, and Clang passed
198/198 without it. Both passed all 13 delivery tests, the header marker
check, and the direct consumer checks. Extraction and manifests were
verified for both packages. Timestamp warnings and a GCC warning about an
ignored `write` return value in the noexec test were retained. These results
do not establish a warning-free build or remote CI.

**Clang update 236:** `pcinsight-sdk-0.2.0-linux-x86_64-clang18-r236.tar.gz`
includes later diagnostics for IP queries and permissions. Its manifest,
extraction, and SHA-256 were verified, and both the normal consumer and the
denial test ran against the extracted copy. This does not replace the
full-suite evidence from r230. GCC revision 237
(`pcinsight-sdk-0.2.0-linux-x86_64-gcc13-r237.tar.gz`) includes the same
diagnostics. Normal queries and denial were tested against its extracted copy.
The 50 installed headers match between GCC 237 and Clang 236.

**Local verification, revision 181:** An existing Release build was installed
to a new prefix. The Linux suite passed 200/200, installed examples passed
12/12, and the marker test passed 1/1. Mixed markers were rejected, the
manifest was verified, and the direct g++ consumer ran successfully.

## Integration and verification

1. To integrate the SDK without CMake in your application, use the [g++ example](LINUX_SDK_SURFACE.en.md#verified-minimal-read-only-consumer). Use the headers and library from the same delivery, and rebuild when changing SDKs.
2. To display metrics, follow the [availability contract](METRIC_AVAILABILITY.en.md) and the [units and identity guide](LINUX_SDK_UNITS_IDENTITY.en.md). Missing data is not zero, and denial of an action does not authorize a fallback.
3. To repeat verification from an already compiled Release tree, follow the [Linux delivery test guide](LINUX_CI.en.md). The script installs only to a new local prefix, checks the manifest, and tests SDK consumers.

### Using the compressed SDK

To make a local package from a prefix already verified by the delivery script,
use its `sdk-payload.txt` and a new output directory outside the SDK:

```sh
bash scripts/package-linux-sdk.sh /ruta/sdk /ruta/sdk-payload.txt /ruta/salida-nueva \
  pcinsight-sdk-0.2.0-linux-x86_64-gcc13-r216.tar.gz
```

The script packages only the files listed in the manifest, compares their
contents after extraction, and generates `SHA256SUMS`. It neither installs
nor publishes anything. It refuses to write inside the source SDK, even
when the parent is reached through a symlink. The output directory must be new.

If you receive the local revision 194 package, put the `.tar.gz` and
`SHA256SUMS` in the same directory. Verify the hash and extract to a new directory:

```sh
sha256sum -c SHA256SUMS
mkdir pcinsight-extracted
tar -xzf pcinsight-sdk-0.2.0-linux-x86_64-gcc13-r194.tar.gz -C pcinsight-extracted
```

Use `pcinsight-extracted/sdk` as the prefix. The archive contains only the
installed SDK, including its MIT notice. It contains no GUI, logs, or
hardware reports. The hash verifies integrity, not authenticity. Moving
the package to another directory does not imply binary compatibility
across all distributions.

The local `pcinsight-sdk-0.2.0-linux-x86_64-clang18-r206.tar.gz` package is
verified the same way with its own `SHA256SUMS`. It includes the IP identity
fix and was tested with the Performance example built with Clang.
Revision 210 (`pcinsight-sdk-0.2.0-linux-x86_64-clang18-r210.tar.gz`) adds direct
address reads by index and replaces revision 206 for that profile. Its own
SHA-256 and the Performance example against the installed SDK were checked.
Revision 214 (`pcinsight-sdk-0.2.0-linux-x86_64-clang18-r214.tar.gz`) added
network name and index checks. Its manifest and the Performance example
were verified after extraction to another prefix.

Revision 228 (`pcinsight-sdk-0.2.0-linux-x86_64-clang18-r228.tar.gz`) supersedes
214 as the newer Clang package and includes `linux_network_addresses.hpp`.
That API's consumer was compiled and run against the extracted copy.

For GCC 13, `pcinsight-sdk-0.2.0-linux-x86_64-gcc13-r216.tar.gz` replaces the
revision 194 package. It includes reads by index, identity checks, and
CAN/IP/GRE labels. Use its own `SHA256SUMS` and do not mix GCC and Clang prefixes.

## What the GUI can consume

| Panel | Contract and limits |
| --- | --- |
| Performance | [CPU, memory, disks, network, and GPU](LINUX_PERFORMANCE_REQUIREMENTS.en.md) |
| Processes and details | [Identity, metrics, and actions](LINUX_PROCESS_REQUIREMENTS.en.md) |
| History | [Observed counters and attribution limits](LINUX_APP_HISTORY_REQUIREMENTS.en.md) |
| Startup | [Entries, explicit changes, and impact](LINUX_STARTUP_REQUIREMENTS.en.md) |
| Users | [Sessions, resources, and controls](LINUX_USERS_REQUIREMENTS.en.md) |
| Services | [Inventory and operations by provider](LINUX_SERVICES_REQUIREMENTS.en.md) |

## What this delivery does not validate

- **Physical hardware:** WSL and simulated tests do not validate firmware, drivers, or every topology. The [hardware contribution guide](LINUX_HARDWARE_EVIDENCE.en.md) explains how to check and report a field or device without `sudo`. When there is no implemented source, the value remains unknown.
- **Alternative service managers:** Private tests do not establish [acceptance on real supervisors](LINUX_K03_ACCEPTANCE.en.md). SysV script-based control is not offered as an implemented, authorized feature.
- **Versions and publication:** Local testing does not establish remote CI, 1.0 binary compatibility, or Windows coverage. No services were installed and no privileged helper was enabled for this delivery.

## Release decision at this checkpoint

| Milestone | Status | Remaining condition |
| --- | --- | --- |
| Local 0.2 delivery for GUI integration | Verified | Use the verified SDK and profile; no additional physical hardware is required. |
| GitHub publication | Not yet done at this checkpoint; [MIT license](../LICENSE) chosen | Confirm destination and authorization. Review what will be shared; do not automatically include local prefixes, logs, or private reports. |
| Validation on other Linux machines | Follow-up | Collect evidence from contributors. This is not a prerequisite to publishing the verified local scope. |
| Overall N03 audit and version 1.0 | Open | Do not mistake the Linux 0.2 milestone for remote CI, overall completion, a stable ABI, or Windows coverage. |

For R01–R20 scope by requirement, see the
[Linux audit](LINUX_DELIVERY_AUDIT.en.md#verifiable-scope-by-requirement).

Before the first public commit, review the exact files to be shared.
`.gitignore` excludes build trees, local evidence, and private coordination
material. It does not remove tracked files or replace review of documents,
fixtures, and attachments. Do not upload the whole workspace. The repository
must start at the PCInsight root, which contains `CMakeLists.txt` and `README.md`.

The repository license covers the PCInsight SDK, not the GUI or other
separate projects. New installations include [LICENSE](../LICENSE) at
`share/licenses/pcinsight/LICENSE` under the default prefix. If you redistribute
only the library or part of the SDK, retain that notice as well.
