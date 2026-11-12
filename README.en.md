# PCInsight

**Language:** [Español](README.md) · English

PCInsight is a standalone C++20 SDK for building system monitors and task
managers, with Linux as its primary platform. This repository contains the
library, examples, and tests. It does not include the GUI or the larger project
that uses the SDK.

**Status: 0.2.0 in development.** The Linux SDK is ready to try and integrate,
but it does not yet cover every feature or guarantee support on every
distribution. Available metrics depend on kernel, firmware, drivers, and
permissions. A missing value is not zero. The SDK does not yet promise a
stable 1.0 ABI. Windows support is partial and is not the focus of this release.

## Get started

- [Use PCInsight from C++, without adding CMake to your application](docs/GETTING_STARTED_CPP.en.md).
- [See what works, what is partial, and what is not implemented](docs/SUPPORT_STATUS.en.md).
- [Verify Linux hardware and share your results](docs/LINUX_HARDWARE_EVIDENCE.en.md).
- [Contribute code, tests, documentation, or hardware reports](CONTRIBUTING.en.md).
- [Report a security issue privately](SECURITY.en.md).

## Build and try it on Linux

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/pcinsight-cli
```

CMake builds the SDK itself. Your application can link the resulting
`libpcinsight.a` directly. See the [complete C++ example](examples/getting_started.cpp)
for a runnable program.

```cpp
#include <pcinsight/monitor.hpp>
#include <chrono>
#include <thread>

pcinsight::Monitor monitor;
monitor.sample(); // Establish a baseline for rate calculations.
std::this_thread::sleep_for(std::chrono::seconds(1));
auto snapshot = monitor.sample();

if (pcinsight::metric_is_known(snapshot.cpu.utilization_percent)) {
    double cpu_percent = *snapshot.cpu.utilization_percent.value;
}
```

For a GUI, use `pcinsight::Sampler` to collect samples in the background.
Its default interval is one second, and its history covers the last 60 seconds
by monotonic timestamp, subject to configured capacity and collection time.
Use those timestamps for the graph and show unavailable readings as gaps.
CPU utilization and rates use differences between samples; the first sample
normally has no rates. See the [Sampler contract](docs/SAMPLER_WINDOW.md)
and [metric availability contract](docs/METRIC_AVAILABILITY.md).

## Linux coverage

PCInsight exposes CPU, memory, process, disk, network, GPU, session, startup,
service, and observed application history data where supported. Collection
uses system interfaces; it does not need to run tools such as `dmidecode`,
`lspci`, `systemctl`, or `nvidia-smi`. Some optional providers depend on
interfaces or runtime libraries already available on the host.

Read-only sampling is separate from actions such as ending a process,
changing startup entries, logging out, or controlling a service. Those actions
require explicit requests and appropriate permissions. Installing the SDK
does not start network or notification capture or reconstruct past activity.
See the [support matrix](docs/SUPPORT_STATUS.en.md) before wiring actions to a GUI.

Part of the validation used WSL or simulated data. Those tests do not establish
support on physical hardware. A report for a single device or metric is useful;
see the [hardware verification guide](docs/LINUX_HARDWARE_EVIDENCE.en.md).

## Technical references

These detailed technical references are currently in Spanish.

- Performance: [fields and units](docs/LINUX_PERFORMANCE_REQUIREMENTS.md) · [C++ example](examples/linux_performance.cpp)
- Processes and details: [fields, identity, and actions](docs/LINUX_PROCESS_REQUIREMENTS.md) · [C++ example](examples/linux_processes.cpp)
- App history: [CPU, network, and notifications](docs/LINUX_APP_HISTORY_REQUIREMENTS.md) · [C++ example](examples/linux_app_history.cpp)
- Startup: [fields and actions](docs/LINUX_STARTUP_REQUIREMENTS.md) · [C++ example](examples/linux_startup.cpp)
- Users and sessions: [scope and actions](docs/LINUX_USERS_REQUIREMENTS.md) · [C++ example](examples/linux_users.cpp)
- Services: [providers, fields, and actions](docs/LINUX_SERVICES_REQUIREMENTS.md) · [C++ example](examples/linux_services.cpp)
- SDK: [Linux delivery guide](docs/LINUX_02_HANDOFF.md) · [header/library compatibility check](docs/SDK_HEADER_COMPATIBILITY.md) · [version policy](docs/SDK_VERSION_POLICY.md)

The [Spanish README](README.md) includes additional implementation notes and
references.

## Project origins and contributions

The project maintainer designed PCInsight and defined its requirements. The
initial implementation was vibe-coded with Codex under the maintainer's
direction. Contributions are welcome, whether written by hand or with AI
assistance, including bug fixes, tests, documentation, and hardware validation.
See the [contribution guide](CONTRIBUTING.en.md).

## License

The [MIT license](LICENSE) applies to PCInsight, including its SDK,
documentation, and examples. It does not set the license for the separate GUI
or other projects that use the SDK. Keep the copyright and license notices
when redistributing the library or a substantial portion of it.
