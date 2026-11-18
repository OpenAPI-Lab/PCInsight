# Linux SDK 0.2.0 surface

This is a header inventory, not a 1.0 ABI promise or a list of frozen
symbols. **Candidate baseline** means the GUI can start here.
Platform-specific fields, legacy values, and extensions in these headers
retain their own limitations. Including a header neither grants permissions
nor guarantees availability.

| Category | Headers under `pcinsight/` | Integration rule |
| --- | --- | --- |
| Candidate baseline: reading | `monitor.hpp`, `sampler.hpp`, `types.hpp`, `metric.hpp`, `app_history.hpp` | Use reports with availability. Do not promote legacy values to known measurements. |
| Candidate baseline: actions | `actions.hpp`, `audit.hpp` | Invoke only on explicit request. Auditing is not authorization; do not fall back to a more permissive operation. |
| SDK integration | `sdk_headers.hpp` | Generated at build time. The optional link check does not certify a complete ABI. |
| Persistence and specialized actions | `history_store.hpp`, `linux_history_store.hpp`, `startup_store.hpp`, `startup.hpp`, `account_management.hpp`, `linux_session_control.hpp`, `linux_startup_publisher.hpp` | Select format and scope explicitly. Do not confuse logout with disconnect or account management with privilege elevation. |
| Advanced Linux inventory and queries | `linux_drm.hpp`, `linux_filesystem_capacity.hpp`, `linux_network_inventory.hpp`, `linux_network_addresses.hpp`, `linux_tcp.hpp` | Depend on provider, sources, and permissions. Preserve diagnostics and scope. |
| Advanced Linux capture and attribution | `linux_network_admission.hpp`, `linux_notifications.hpp`, `linux_process_network_selection.hpp`, `linux_process_network.hpp`, `linux_user_network.hpp` | Opt-in according to the API. Do not extrapolate to global traffic, unobserved apps, or earlier history. |
| Alternative Linux supervisors | `linux_openrc.hpp`, `linux_openrc_services.hpp`, `linux_openrc_control.hpp`, `linux_runit.hpp`, `linux_runit_services.hpp`, `linux_s6.hpp`, `linux_s6_services.hpp`, `linux_services.hpp`, `linux_sysv.hpp`, `linux_sysv_services.hpp` | Projection and control are distinct. Use the selected provider and consult [acceptance](LINUX_K03_ACCEPTANCE.en.md). Do not enable SysV scripts by analogy. |
| Advanced startup observation | `startup_birth_impact.hpp`, `startup_birth_observer.hpp`, `startup_birth_overlay.hpp`, `startup_candidates.hpp`, `startup_executable_matches.hpp`, `startup_impact.hpp`, `startup_observer.hpp`, `startup_overlay.hpp`, `startup_recorder.hpp` | An association or measurement does not certify that the OS caused the launch. Do not promise complete startup coverage. |
| Opt-in with explicit risk | `linux_efficiency.hpp` | The unchecked path is not an automatic substitute for the rejected generic action. |
| Windows outside Linux scope | `windows_amd_runtime_memory.hpp`, `windows_history_store.hpp`, `windows_network_identity.hpp`, `windows_services.hpp`, `windows_startup_file_control.hpp` | Inclusion in the SDK does not indicate a Linux provider. The experimental AMD query retains its separate contract. |

This inventory does not freeze, remove, or change the advanced categories.
Before declaring 1.0, define the subset of supported symbols and profiles
and verify it according to the [version policy](SDK_VERSION_POLICY.en.md).

## Verified minimal read-only consumer

The [test consumer](../tests/linux_abi/gui_read_consumer.cpp) links the
marker, the `Monitor` constructor and destructor, `Monitor::sample`,
the `app_history_view(SystemSnapshot)` projection, and the `Sampler`
constructor and destructor, `start`, `stop`, `running`, `last_error`,
`latest_timed`, and `timed_history`. It verifies a coherent window without
requiring every field to have data. It does not perform actions or start
privileged capture. Unused methods and compatibility with previously
compiled objects are outside the test.

From the PCInsight root, using an installed SDK 0.2.0 prefix:

```sh
g++ -std=c++20 -O2 -DNDEBUG -I"$SDK/include" \
  tests/linux_abi/gui_read_consumer.cpp "$SDK/lib/libpcinsight.a" \
  -pthread -o /tmp/pcinsight-gui-read-test
/tmp/pcinsight-gui-read-test
```

Verified profile: Linux x86-64 on Ubuntu WSL with GCC 13.3 and libstdc++,
a Release static library, and a rebuilt consumer. No CMake is required
on the consumer side. Revision 196 also built the read and result consumers
with Clang 18.1.3 and `-stdlib=libstdc++` against the GCC 13.3 static library
from revision 194. Both ran successfully ten times on Ubuntu WSL x86-64.
This is evidence for those interfaces and that toolchain pair, not a Clang
build of the library, a libc++ trial, or a 1.0 ABI promise.

Revision 197: a second local build produced the Release static library
with Clang 18.1.3 on Ubuntu WSL x86-64. Both Clang consumers using libstdc++
linked and ran against that SDK. This tested the library and consumers,
not the whole suite or libc++.

Revision 198: the Clang 18.1.3 Linux Release suite passed 197/197 in the
same environment, with the optional recorder disabled. An unused internal
constant was removed; the public contract did not change.
