# A05: Version policy and criteria for freezing the ABI

**Language:** [Español](SDK_VERSION_POLICY.md) · English

Revision 169, November 6, 2026. The package is still **0.2.0**, a C++20
static library. Version 1.0 has not been announced or published. This policy
applies to future tagged releases; a development checkpoint is not a release.

## What to change for each release

| Change | Examples | Rule for a 0.x release |
| --- | --- | --- |
| Incompatible source API or contract | Renaming a method or changing units, the meaning of zero or absence, identity, permissions, or an action's scope | Bump the minor version. Document the old and new contracts, migration, and a test. A signature alone does not define compatibility. |
| Incompatible ABI | Adding or reordering public struct members or changing types, alignment, linked signatures, vtables, or conventions | Bump the minor version and require all consumers to rebuild. Adding even a trailing member can change size and calling conventions. |
| Compatible addition | A separate opt-in API that leaves existing types, signatures, and behavior unchanged | Bump the minor version for new public functionality. Verify the existing contract; successful compilation does not establish compatibility. |
| Compatible fix | Restoring documented behavior without changing the contract or ABI | Bump the patch version and record the evidence and limits. If the fix requires data or code migration, do not classify it as compatible just because it is a fix. |
| Persisted format | Changing the structure or semantics of a history or startup file | Review and version the format separately. Document reads, writes, migration, data loss, and rollback as applicable. The SDK version does not replace the file format version. |

Do not replace a published package with different bytes under the same
distribution identity. For unpublished checkpoints that still report
0.2.0, identify artifacts by revision, build profile, and manifest.
The CMake package uses `ExactVersion`; that does not make all 0.2.0
builds interchangeable. Use headers and the library from the same SDK,
use compatible tools and flags, and rebuild when changing SDKs. The GUI
application does not have to use CMake.

## Three checks with different scopes

1. [Header marker](SDK_HEADER_COMPATIBILITY.en.md): verifies the marked header
   set at link time. It does not identify implementation-only changes.
2. [Linux baseline](LINUX_ABI_BASELINE.en.md): compares selected contracts and
   size and alignment under the recorded profile. A match does not establish
   complete ABI compatibility.
3. [Package manifest](../tests/linux_delivery/sdk_payload_manifest.cmake):
   records each installed file's SHA-256 digest by sorted relative path.
   It identifies the bytes of a local release even if version and headers
   are unchanged. It is not a signature, authentication, evidence of a
   reproducible build, or an ABI guarantee.

To verify a manifest against the same package, use a dedicated Linux SDK
prefix and an existing working directory. From this standalone repository:

```sh
cmake -DSDK_PREFIX="$PWD/work/sdk-headers-167-linux" \
  -DAUDIT_OUTPUT="$PWD/work/linux-release-169" \
  -P tests/linux_delivery/check_payload_manifest.cmake
```

The original audit ran the same script from the parent workspace, where
its path was `outputs/pcinsight/tests/linux_delivery/check_payload_manifest.cmake`.

The [test](../tests/linux_delivery/check_payload_manifest.cmake) records
the contents, compares them again, and changes the `.a` digest **in a copy
of the manifest**, not in the library. The comparison must reject that row.
The read-only script does not install or run SDK components. An earlier
manifest is useful as a reference only if retained through a trusted channel.

## Known changes that motivate the policy

- [Linux network history, revision 100](LINUX_NETWORK_APP_HISTORY.en.md):
  expanded `SystemSnapshot`. Rebuild consumers. Do not treat the separate
  report as part of cumulative CPU history or change the legacy format
  on its account.
- [Service selection, revision 144](LINUX_SERVICE_SELECTION.en.md): added a
  field to `SampleOptions`. Rebuild even if the caller does not use it.
- [Optional marker, revision 167](SDK_HEADER_COMPATIBILITY.en.md): added a
  header and symbol without changing existing layouts. Use the new archive
  when opting in to the check.
- [Persistence](HISTORY.en.md): distinguish the legacy format from observed
  history stores, and distinguish missing data from a measured zero counter.

## Criteria before declaring 1.0

| Gate | Required evidence | Status at this revision |
| --- | --- | --- |
| Public contract | Inventory stable headers, symbols, units, identity, availability, and effects. Document migrations for incompatible changes and separate experimental APIs without promising stability. | N03 has Linux traceability, but the global audit remains open. |
| ABI matrix | Define supported architecture, compiler, STL, runtime, and option profiles. Record and compare relevant layouts, symbols, and vtables, and test consumers compiled against the previous baseline. | One Linux profile has a partial probe, and the marker was checked on Linux and MSVC. There is no complete matrix. |
| Release and upgrades | Install in a clean prefix, link consumers, verify version, fingerprint, and manifest, and test advertised format compatibility and migrations. | The local Linux audit was verified; it is not equivalent to publishing a release. |
| Scope and acceptance | Validate advertised behavior in real environments, run CI for supported profiles, and explicitly address open requirements and historical risks. | N01, B08/G07/N02, K03/I07/L05, and remaining Windows work are not closed here. |

Do not freeze the ABI or relax `ExactVersion` just because the local suite
passes. Future binary compatibility will be announced only for verified
profiles and surfaces. A05 remains in progress. This does not change the
Linux-first scope or imply new authorizations.

Revisions 174–176 verified [errors and availability](METRIC_AVAILABILITY.md),
[Linux units and identity](LINUX_SDK_UNITS_IDENTITY.en.md), and rejection of
mismatched markers when two objects in one executable call the check.
Package version checks reject a different version but do not distinguish
two artifacts reporting 0.2.0. The marker does not cover objects that omit
the call, and a single-profile baseline is not a comparison of historical
consumers across profiles. These checks do not replace an inventory of
the stable surface or validation with the tools and options to be supported.

Revision 179 recorded the [Linux SDK inventory](LINUX_SDK_SURFACE.en.md):
49 headers reconciled against the source and installed package. Header
classification guides GUI integration; it does not freeze every symbol
in the candidate baseline.

Local verification: 54 installed SDK files, successful manifest comparison,
rejection of the changed digest, 1/1 marker check, 3/3 ABI probes, and 200/200
Linux tests. Evidence: `work/linux-release-169/`. The official count at
this checkpoint was 60/75 (80%).
