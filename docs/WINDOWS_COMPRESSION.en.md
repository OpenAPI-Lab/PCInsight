# Windows compressed memory: experimental provider

Implemented and tested on 2026-10-12. None of the public interfaces reviewed
provided a documented equivalent that guarantees the global figure displayed
by Task Manager. The provider uses an internal Windows query, so it is
**disabled by default**.

## Explicit build-time opt-in

```sh
cmake -S outputs/pcinsight -B work/build-compression -DCMAKE_BUILD_TYPE=Release -DPCINSIGHT_ENABLE_EXPERIMENTAL_WINDOWS_COMPRESSION=ON
cmake --build work/build-compression
ctest --test-dir work/build-compression --output-on-failure
```

The provider is enabled only for Windows x64 builds. On other architectures
or without this option, `memory.compressed_bytes` has no value and carries a
note. It does not change the public API or require additional software.

## Semantics and limits

The provider calls `NtQuerySystemInformation` with class
`SystemStoreInformation` (109), read operation `MemCompressionInfoRequest`
(22), and request version 3. The resulting field is `TotalCompressedSize`:
bytes of compressed content. It is published as `Support::estimated` with
a note distinguishing compressed content from the store's total resident
footprint and from Task Manager's field. Do not add it to `in_use_bytes`:
it is not a separate additional reservation.

The internal interface has no stability guarantee across Windows versions.
The implementation validates sizes, alignment, version, and response envelope,
but these checks do not guarantee future compatibility. A failed query or
unexpected structure leaves the value absent; access denial or missing
privilege yields `permission_required`. Zero is published only after a
successful query with a compatible response.

The symbol is resolved from `ntdll.dll`, which Windows has already loaded.
The provider does not load an external DLL, install a driver, enable
privileges, or call operations that flush, reconfigure, or disable compression.

## Verification

Windows Release: 7/7 tests with the option OFF and 7/7 with it ON. Linux WSL:
5/5. The experimental query returned 297,533,440 bytes on the development PC.
This is a dynamic observation, not an expected value for other runs.

Injected tests cover valid data, measured zero, an absent function, access
denial, missing privilege, rejected class or length, unsuccessful status,
changes in version/operation/pointer/length, and recovery without reusing
earlier data. Exact equivalence to Task Manager and coverage across Windows
versions have not been validated. This is not full, stable support.

Interoperability references consulted on 2026-10-12:

- [Internal interface definitions in System Informer/phnt](https://github.com/winsiderss/systeminformer/blob/master/phnt/include/ntexapi.h)
- [Query usage in System Informer](https://github.com/winsiderss/systeminformer/blob/master/phlib/native.c)
- [Microsoft's documented memory performance information](https://learn.microsoft.com/en-us/windows/win32/memory/memory-performance-information)
