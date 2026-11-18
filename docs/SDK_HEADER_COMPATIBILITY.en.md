# A05: Verify that SDK headers and the library match

**Language:** [Español](SDK_HEADER_COMPATIBILITY.md) · English

Revision 167, November 6, 2026. The SDK is still at 0.2.0. The package version
and `find_package(... EXACT)` do not distinguish two 0.x builds that differ in
a public type but share that version. The generated `pcinsight/sdk_headers.hpp`
header adds an explicit, optional link-time check without changing layouts,
vtables, existing API semantics, or persisted formats.

```cpp
#include <pcinsight/sdk_headers.hpp>

int main() {
    pcinsight::require_matching_sdk_headers();
    // Create the Monitor and other SDK objects next.
}
```

Compile with the installed SDK include directory and link its static archive.
The header is not in the source tree: use the headers in the installed prefix.
The build tree also exposes it under `generated/include` as an explicit include
directory for consumers that need the marker before installation. Your GUI does
not need CMake. Existing code that does not include or call the marker does not
gain this requirement.

The SHA-256 fingerprint covers the sorted relative names and bytes of the
public `.hpp` files, plus the marker template. Generation registers these files
as configuration dependencies. The function references a C symbol specific
to that fingerprint. An archive lacking the symbol fails at link time. When
the symbol matches, the call performs no I/O or actions. The wrapper has
internal linkage so another translation unit cannot substitute its marker.
Put the call in reachable code that is actually linked. This is not an
automatic check of every object or plugin in a process.

## Limitations

- The check compares the marker for a **packaged set**; it does not reread
  the consumer's headers. Copying just the right marker over unrelated headers
  or mixing in unchecked objects can hide a mismatch.
- The check is conservative: a comment or line-ending change can alter the
  fingerprint without changing the ABI. It is not a signature and does not
  authenticate a package.
- It does not detect different packing options, standard-library ABIs, flags,
  toolchains, architectures, or dependencies when the input bytes are the
  same. Use compatible toolchains and configurations and rebuild when changing
  the SDK. This does not replace ABI analysis or declare 1.0 stability.
- An older SDK without the marker cannot be checked with this call. When
  adopting the new header, also reinstall and link the new library.

The [test consumer](../tests/sdk_headers_consumer/CMakeLists.txt) uses only
the installed SDK. It compiles and links the real marker, compiles a different
synthetic marker, and requires rejection **at link time** on that symbol.
It does not treat a compilation error as ABI rejection or claim validation of
every historical revision. A05 remains open.

The test also links two translation units with matching markers, then repeats
with a different synthetic marker in the second unit. Both calls are reachable.
The mixed case must fail on the symbol in the second unit: a correct check
in `main` must not hide the other object's fingerprint. This does not detect
objects that omit their own include and check.

Verification: with the SDK installed on Linux and in MSVC Release, both
positive consumers exited with status 0. Both alternatives compiled but failed
to link on the expected symbol. On Linux, the older archive from audit 166 also
rejected the new marker; direct linking with g++ worked, and the suite passed
200 of 200 tests. WSL mount clock-skew warnings were retained without changing
the clock. Logs: `work/sdk-headers-*`. A05 was in progress; the official count
at this checkpoint was 60/75 (80%).

As a complementary check, the [Linux baseline](LINUX_ABI_BASELINE.en.md) records
selected signatures, sizes, and alignment for one profile. The
[version policy](SDK_VERSION_POLICY.en.md) distinguishes both checks from
a release byte manifest and a 1.0 ABI guarantee.
