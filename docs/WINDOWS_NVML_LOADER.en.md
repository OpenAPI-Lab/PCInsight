# Loading NVML on Windows: System32 and the legacy location

Revision 55, M04 component complete. The [NVML memory provider](WINDOWS_NVML_MEMORY.en.md)
no longer requires nvml.dll to be exclusively in System32. NVIDIA documents
System32 for DCH and an NVSMI location under Program Files for standard
installations ([NVIDIA reference](https://docs.nvidia.com/deploy/nvml-api/nvml-api-reference.html)).
This expands the accepted locations, not the verified physical coverage or
the set of compatible symbols and drivers.

## Search policy

1. Try nvml.dll with LOAD_LIBRARY_SEARCH_SYSTEM32. Success always takes
   priority: no Known Folder lookup and no other copy is opened.
2. Only MOD_NOT_FOUND, DLL_NOT_FOUND, FILE_NOT_FOUND, and PATH_NOT_FOUND
   allow a second attempt. ACCESS_DENIED, BAD_EXE_FORMAT, and any other
   error stop the search rather than being masked by another installed copy.
3. Windows resolves FOLDERID_ProgramFilesX64 through SHGetKnownFolderPath
   with DONT_VERIFY, without creating folders. It does not read ProgramFiles
   or ProgramW6432 from the environment or assume `C:\Program Files`.
   CoTaskMemFree releases the returned string, including on exceptions.
4. Require an ordinary absolute drive path, at most 32,767 wchar units
   including the suffix and NUL. UNC or device namespaces, relative paths,
   drive roots, traversal, empty components, ADS, control characters or NUL,
   forward slashes, wildcards, percent expansion, and components ending
   in a dot or space are rejected. An ordinary trailing separator is
   normalized. Unicode paths and other drives are allowed. Program Files
   configurations outside this conservative contract remain unknown rather
   than being guessed.
5. Append only `NVIDIA Corporation\NVSMI\nvml.dll`. Load that absolute
   path with DLL_LOAD_DIR | SYSTEM32 without changing PATH, the working
   directory, or the process's DLL directories. No scanning disks,
   DriverStore, apps, toolkits, or user folders; no installation or download.

The [Windows loading API](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw)
can fail because a dependency is missing, not just the main file.
MOD_NOT_FOUND enables the legacy attempt but does not identify which
file is missing. The policy controls requested paths; it does not verify
signatures, ACLs, or reparse points or neutralize modules already loaded
by the host. It trusts the Known Folders configuration and system loader.
Checking these flags alone does not authenticate the source path.

## Integration and errors

Only nvml.dll has this fallback. nvcuda.dll and gdi32.dll still use the
System32 search. Without CUDA, the collector does not try NVML; the loader
test can check its locations separately without initializing NVML. If the
DLL lacks required symbols, no other copy is tried and the provider remains
unknown. The remaining CUDA/NVML/KMT identity and v1 memory contract is
unchanged. The fallback does NOT make older drivers without UUIDv2 compatible.

The internal result returns an owned handle, Win32 error, and search
reason/source. The caller releases the reference with FreeLibrary through
RAII. Each LoadLibraryEx error is saved immediately before another API call.
An inconsistent failure with error 0 is normalized to GEN_FAILURE. Denied
Known Folder resolution and denied loading propagate permission_required
to the metric. The collector note includes the loader's Win32 diagnostic.
The error helper no longer adds a fabricated status 0 when no code was
supplied. No API, layout, vtable, version, or persistence-format changes.

## Evidence and remaining work

Windows MinGW 130/130 and MSVC 130/130, Linux 111/111 Release. Twenty
repetitions each of loader and memory tests per Windows toolchain (40 test
runs each). Three installed external consumers rebuilt, relinked, and run
with exit code 0, with no new link dependencies or public headers. Logs:
`work/nvml-legacy-final-*`, `nvml-legacy-repeat-*`, `nvml-legacy-install-*`,
and `nvml-legacy-consumer-*`. The first MinGW build found DWORD/initializer_list
constant type issues and a narrowing conversion. Explicit types fixed them;
the original log was kept. MSVC retains the expected D9025 from test
/UNDEBUG; warnings are not disabled.

Injected tests cover System32 priority, successful legacy loading and its
path and flags, both copies absent, errors and access denials without
bypass, failed Known Folder lookup, malformed paths, traversal, UNC, ADS,
control characters, NUL, limits, Unicode, another drive, a trailing separator,
a missing API, and error 0. The prior memory test retains its checks.
Native probe on an AMD host: System32 did not find NVML, and the legacy
attempt also failed with Win32=126. The location probe does not write DLLs
to System32 or Program Files or invoke nvmlInit/CUDA. Injected success does
not verify a real NVIDIA driver or its dependencies.

A positive physical NVIDIA result, driver and topology coverage, and
AMD/Intel free memory remain outstanding. M04 remains open and M05 stays
deferred. Linux's provider and semantics are unchanged. A [Known Folder
identifier](https://learn.microsoft.com/en-us/windows/win32/shell/knownfolderid)
does not certify the permissions or security of the folder's contents.
