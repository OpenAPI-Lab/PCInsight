# Contributing to PCInsight

**Language:** [Español](CONTRIBUTING.md) · English

This repository contains the SDK, examples, tests, and documentation. You do
not need to clone or change the GUI or the larger project that uses the API.
Start with the [support matrix](docs/SUPPORT_STATUS.en.md) and the
[C++ getting-started guide](docs/GETTING_STARTED_CPP.en.md).

## Prepare and test a change

From the repository root, use a fresh build directory:

```sh
cmake -S . -B build/contributor -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DPCINSIGHT_BUILD_TESTS=ON -DPCINSIGHT_BUILD_EXAMPLES=ON
cmake --build build/contributor --parallel 2
ctest --test-dir build/contributor --output-on-failure --no-tests=error --timeout 60
```

For Clang, add `-DCMAKE_CXX_COMPILER=clang++` to the initial configuration
in a separate build directory. Test assertions must remain active in Release
builds. Tests use fixtures; some create private files, sockets, or child
processes. Running privileged probes or service scripts, changing real
startup entries, and logging out are not required to contribute. A test
skipped for lack of permissions or capabilities is not a positive result.

To test installation and linking from a separate project:

```sh
bash scripts/verify-linux-sdk.sh build/contributor work/verification-new
```

The evidence directory must be new, and its parent must exist (`mkdir -p work`
if needed). The verifier does not require `sudo`. Review and redact private
information before sharing logs. The verifier tests the installed SDK; it
does not require your GUI to use CMake.

## Project guidelines

- Prioritize Linux. Collect data through kernel, firmware, or driver interfaces. Do not add external commands for collection. Explicit actions are separate and need their own permission model.
- Keep unknown values distinct from zero, `false`, or an empty list. Preserve `Metric` status, units, and scope. Do not present a proxy as a direct measurement.
- A C++ snapshot is not necessarily atomic. Handle PID and index reuse, discontinuities, permission changes, overflows, and disappearing resources.
- Test success, missing data, and relevant failures. Prefer private fixtures for cases you cannot reproduce on available hardware. Fixtures do not validate physical hardware.
- Document public type and semantic changes. Adding a member can break ABI. Rebuild the library and its consumers; see the [SDK version policy](docs/SDK_VERSION_POLICY.md) (Spanish).
- Do not update a baseline just to make a test pass. Measure the affected configuration, review the difference, and document it. A local test does not establish universal compatibility.

## What to include in an issue or pull request

Describe the problem, environment, expected and observed behavior, affected
API, tests you added, and remaining limitations. Include the version or commit,
compiler, architecture, and whether you ran on native Linux, in a VM, or in WSL.
For hardware, use the [validation guide](docs/LINUX_HARDWARE_EVIDENCE.en.md).
Checking one device or field is useful; separate the observation from
conclusions about the whole system.

Do not publish serial numbers, credentials, usernames, private paths,
unnecessary IP or MAC addresses, application command lines, firmware dumps,
or full unreviewed logs. For a potential security issue, see the
[security policy](SECURITY.en.md) before posting a public report.

PCInsight uses the [MIT license](LICENSE). Retain copyright and license
notices, and disclose any third-party material you propose to include.
The PCInsight license does not set the license for projects that use the SDK.
