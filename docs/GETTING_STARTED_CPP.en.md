# Use PCInsight from C++ on Linux

**Language:** [Español](GETTING_STARTED_CPP.md) · English

PCInsight is a C++20 library. To read metrics, include its headers and link
`libpcinsight.a` into your program. You do not need to finish the GUI or adopt
CMake in your application.

## 1. Build the standalone SDK

You need a C++20 compiler, CMake 3.20 or later, and your platform's build
tools. GCC 13.3 and Clang 18.1.3 with libstdc++ were tested on Ubuntu WSL
x86-64. This does not guarantee support on every distribution or
architecture. From the PCInsight repository root:

```sh
cmake -S . -B build/sdk-build -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF -DPCINSIGHT_BUILD_TESTS=OFF -DPCINSIGHT_BUILD_EXAMPLES=OFF
cmake --build build/sdk-build --parallel 2
cmake --install build/sdk-build --prefix "$PWD/build/sdk"
```

Do not use `sudo`; the SDK goes into `build/sdk`, not a system directory.
For Clang, add `-DCMAKE_CXX_COMPILER=clang++` to the first command and use
a fresh build directory. Build and link your application with compatible tools.

If you received a binary package, check its `SHA256SUMS`, extract it, and use
the resulting `sdk` directory instead. A checksum does not authenticate the
publisher. The binary must match your architecture, runtime, and standard
library. When in doubt, build from source.

## 2. Compile a program without CMake

The [complete example](../examples/getting_started.cpp) has a `main`, reads
CPU, memory, and the number of visible process rows, and exits after three
samples. From the repository root:

```sh
SDK="$PWD/build/sdk"
g++ -std=c++20 -O2 -I"$SDK/include" examples/getting_started.cpp \
  "$SDK/lib/libpcinsight.a" -pthread -ldl -o build/mi_monitor
./build/mi_monitor
```

If CMake installed the library in `lib64` or another library directory,
adjust the path. You can copy the example into your project and use an
absolute path to the SDK. For Clang, use `clang++` and an SDK built with
that toolchain. For multiple source files, compile them normally and place
the static archive after your objects on the link command. Do not mix
headers and libraries from different revisions, even if both say 0.2.0.

The sampling code looks like this:

```cpp
pcinsight::Monitor monitor;
pcinsight::SampleOptions options;
monitor.sample(options); // Establish a baseline.
std::this_thread::sleep_for(std::chrono::seconds(1));
auto snapshot = monitor.sample(options);
if (pcinsight::metric_is_known(snapshot.cpu.utilization_percent)) {
    double cpu = *snapshot.cpu.utilization_percent.value; // 25 = 25%, not 0.25.
}
```

The complete example includes the headers and `main`. Missing data is not
zero. Check `support` and `note`; `estimated` is not an exact measurement.
The first sample normally has no rates. Do not subtract wall-clock timestamps
to calculate rates; the provider uses monotonic time.

## 3. Add panels and actions

- Enable `include_disks`, `include_networks`, and `include_gpus` for disks, network adapters, and GPUs. The example disables them to keep the first query small.
- Request services, users, startup entries, and app history with `include_services`, `include_user_sessions`, `include_startup_entries`, and `include_app_history`. Reading does not authorize actions or start privileged capture.
- For IP addresses with availability status, include `pcinsight/linux_network_addresses.hpp` and call `query_linux_network_addresses()`. This is a separate query, not an atomic join with a Monitor snapshot.
- For charts, include `pcinsight/sampler.hpp` and use `Sampler::start`, `latest_timed`, `timed_history`, and `stop`. Windows are time-based, not exactly 60 rows. Do not block the GUI thread with `sleep_for` or synchronous collection; the example loop is for a console program.
- Actions are separate in `pcinsight/actions.hpp`. Do not start or stop services, terminate processes, or change settings just because a panel is visible. Review the [support matrix](SUPPORT_STATUS.en.md) before wiring up buttons.

## 4. Troubleshooting

| Symptom | What to check |
| --- | --- |
| `pcinsight/monitor.hpp` not found | Point `-I` at `sdk/include`, not `sdk/include/pcinsight`. |
| `sdk_headers.hpp` not found | Use a built and installed SDK. This header is generated; copying the source `include/` directory is not enough. |
| `undefined reference` | Link the correct archive after your sources or objects, and keep `-pthread -ldl` for the tested setup. |
| `pcinsight_require_headers_...` fails | The headers and library do not match. Rebuild or use a complete SDK from one revision. |
| CPU utilization or rates are initially unknown | Take a second sample after an interval. A reset counter or loss of continuity also requires a new baseline. |
| A metric stays unknown | Read `support` and `note`. A source, driver, or permission may be missing; do not run as root automatically. |

## Optional CMake integration

Only if your application already uses CMake:

```cmake
find_package(pcinsight 0.2.0 EXACT CONFIG REQUIRED)
add_executable(mi_monitor main.cpp)
target_link_libraries(mi_monitor PRIVATE pcinsight::pcinsight)
```

Configure with `-DCMAKE_PREFIX_PATH=/path/to/sdk`. CMake is optional for
consumers. The [version policy](SDK_VERSION_POLICY.md) (Spanish) explains
why the 0.x SDK does not yet promise a stable 1.0 ABI.
