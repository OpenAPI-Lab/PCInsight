# N03: Linux SDK delivery audit

Revision 166, 2026-11-06. This audit verifies a local SDK 0.2.0 delivery and
indexes the Linux scope of R01–R20. It is not a 1.0 release, remote CI,
hardware certification, or closure of N03. `SEGUIMIENTO.txt` remains the
official tracker.

## Verifiable scope by requirement

| ID | Consumer contract and evidence | What is not certified |
| --- | --- | --- |
| R01 | C++20 library and installed SDK: static library, public headers, and link configuration; reproducible test below. | 1.0 ABI or universal compatibility. The SDK installs no GUI, recorder, service, autostart entry, or helper. |
| R02 | [Performance](LINUX_PERFORMANCE_REQUIREMENTS.en.md): dynamic sampling and `Sampler::timed_history`; [window test](../tests/sampler_window.cpp). | Exactly 61 points or filling missing intervals. |
| R03 | [CPU and counts](LINUX_PERFORMANCE_REQUIREMENTS.en.md): utilization, identity, processes, threads, descriptors, and uptime. | Windows handles or exact physical inventory. |
| R04 | [Frequencies, topology, caches, and KVM](LINUX_PERFORMANCE_REQUIREMENTS.en.md). | B08 firmware virtualization; KVM does not replace that check. |
| R05 | [Memory and composition](LINUX_PERFORMANCE_REQUIREMENTS.en.md). | Windows paged/nonpaged pools or global compression derived from zswap. |
| R06 | [SMBIOS and reserved memory](LINUX_PERFORMANCE_REQUIREMENTS.en.md). | Unpublished firmware data or exact hardware reservation inferred by subtraction. |
| R07 | [Disk rates](LINUX_DISK_RATES.en.md) and the Performance sample. | Application latency or physical saturation. |
| R08 | [Disk identity, capacity, and swap](LINUX_PERFORMANCE_REQUIREMENTS.en.md). | Total formatted capacity from mounts or medium type from rotation. |
| R09 | [Adapter identity, rates, and IP](LINUX_PERFORMANCE_REQUIREMENTS.en.md). | Completeness or individual failures in legacy IP lists; N02 physical coverage. |
| R10 | [GPU engines and client aggregates](LINUX_PERFORMANCE_REQUIREMENTS.en.md). | Overall or 3D utilization inferred by summing clients; G07 physical validation. |
| R11 | [GPU memory, utilization, and temperature](LINUX_GPU_TELEMETRY.en.md). | Windows shared memory inferred from GTT, globally allocatable memory, or physical coverage across AMD, Intel, and NVIDIA. |
| R12 | [API versions and GPU location](LINUX_PERFORMANCE_REQUIREMENTS.en.md). | A reliable driver date without a source, DirectX on Linux, or a fabricated OpenGL context. |
| R13 | [Processes and Details](LINUX_PROCESS_REQUIREMENTS.en.md), example and field tests. | Total network traffic per PID, global identity, an atomic snapshot, or private RSS. |
| R14 | [App history](LINUX_APP_HISTORY_REQUIREMENTS.en.md), with separate sources. | Automatic joins across CPU, network, and notifications; retroactive measurements or displayed notifications. |
| R15 | [Startup](LINUX_STARTUP_REQUIREMENTS.en.md), state, eligibility, Properties, and actions. | Automatic I07 launch causation, a signed publisher, or total login impact. |
| R16 | [Users and sessions](LINUX_USERS_REQUIREMENTS.en.md), with separate scopes. | Attribution of all resources by session, a global total, or joining UIDs without their domain. |
| R17 | [Services](LINUX_SERVICES_REQUIREMENTS.en.md), inventories by provider. | Live state inferred from files or markers, or real K03 acceptance on every init system. |
| R18 | [Run new task](LINUX_LAUNCH.en.md) is one explicit API shared by all panels. | Implicit shell use, readiness, or implicit elevation. |
| R19 | [End task and efficiency](LINUX_PROCESS_REQUIREMENTS.en.md); [control and details by provider](LINUX_SERVICES_REQUIREMENTS.en.md). | Checked Linux efficiency mode, unauthorized SysV control, or the L05 privileged helper. |
| R20 | [User actions](LINUX_USERS_REQUIREMENTS.en.md): host integration, Disconnect, and logout are separate. | A universal native manager or treating Disconnect as logout. |

Contract tests use fixtures and do not replace physical results. Delivery
samples read Linux; they do not enable observed network capture, notifications,
recording, launch, service control, or startup/session mutations. A successful
sample can report missing data or permission denial.

The `linux-collection-noexec` test applies
[seccomp](https://docs.kernel.org/userspace-api/seccomp_filter.html) only to
its own processes. It first checks that an `execve` attempt is rejected.
It then creates a Monitor and collects two samples with service, startup,
user, and history reads enabled. It does not enable capture or actions.
A Sampler created after the filter produces at least two samples and stops;
new threads retain the execution restriction. An attempt to run a program
terminates the process and fails the test. If the environment cannot apply
the filter, CTest marks it skipped, not passed. This covers paths exercised
in that environment, not all opt-in extensions or existing drivers.

Revision 225 adds a pipe notification from the `SIGSYS` handler to also
detect an attempt in a child that keeps the handler and descriptor, even
if the parent ignores its exit status. A calibration child verifies the
notification. This is not a universal syscall audit: a descendant that resets
the handler or closes the channel falls outside this notification path, but
the inherited filter still blocks program execution.

## Consumer using a dedicated prefix

The [external verifier](../tests/linux_delivery/CMakeLists.txt) checks that
configuration, the Release library, and exported include directories resolve
inside the prefix. It requires exact 0.2.0, rejects exact 0.2.1, and audits
installed contents. It builds the repository's six examples **against the
installed SDK** and runs one fixture and one sample per panel. The examples
are not installed as user dependencies. The verifier is for tests and does
not require CMake in the GUI.

```sh
# From the workspace root, with a Linux Release build already compiled.
stage=$(mktemp -d work/sdk-audit.XXXXXX)
cmake --install work/build-linux --config Release --prefix "$PWD/$stage"
cmake -S outputs/pcinsight/tests/linux_delivery -B work/build-linux-delivery-166 \
  -DCMAKE_BUILD_TYPE=Release -DPCINSIGHT_AUDIT_PREFIX="$PWD/$stage"
cmake --build work/build-linux-delivery-166 -j4
ctest --test-dir work/build-linux-delivery-166 --output-on-failure
```

These historical commands use the original workspace layout. In the
standalone repository, the verifier and example are under `tests/` and
`examples/`, respectively. Use a fresh verifier build directory when
changing prefixes. Do not reuse a `pcinsight_DIR` cache that points to another
installation.

An application without CMake can instead pass the include directory and
static archive to the compiler. This example uses the verified `lib` layout;
adjust the path if installation used another libdir:

```sh
g++ -std=c++20 -I"$stage/include" outputs/pcinsight/examples/linux_services.cpp \
  "$stage/lib/libpcinsight.a" -pthread -ldl -o work/linux-delivery-direct
work/linux-delivery-direct --self-test
```

Use the headers and static archive from the same build. Exact CMake package
version acceptance does not detect layout differences between 0.x revisions;
rebuild all consumers when changing SDKs. A05 remains open.

## Limits on closing this work

- B08, G07, and N02 require evidence from real firmware, GPUs, distributions,
  and hardware. WSL and fixtures do not fulfill those criteria.
- I07 retains its observer scope with unverifiable launch causation. It does
  not expand to a custom launcher. K03 needs real-world acceptance. SysV
  execution remains unauthorized, and L05 deployment and privileged
  acceptance remain deferred.
- N01 requires actual CI runs. Windows remains deferred, including the
  historical MSVC Monitor risk. A successful local regression does not
  establish that historical Linux integration failures were fixed.
- This audit verifies the local Linux packaging and consumption portion of
  N03. Final project audit and outstanding checks remain open.

## Evidence for this revision

Twenty-two local links were checked. Prefix `work/sdk-audit.tff64S` held 48
headers, one static library, and four CMake files, with no executables.
Exact 0.2.0 was accepted and 0.2.1 was rejected. Six consumers, 12/12
installed SDK tests, direct g++ linking and self-test with exit status 0,
and 200/200 Linux suite tests passed. Logs:
`work/linux-delivery-*-verified.log`. An earlier shell-variable attempt
lost the prefix suffix when crossing PowerShell/WSL. Its log showed only
files already updated in the workspace. The test was repeated with a
literal path and an early prefix check was added. N03 remained open;
the official count at this checkpoint was 60/75 (80%).
