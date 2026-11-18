# Windows startup candidate executable signature

Independent opt-in: `include_startup_entries=true` and
`include_windows_startup_signature=true` (false by default). Returns
`StartupEntry.windows_signature` without treating CompanyName as a
verified publisher. Uses system Wintrust/Crypt32: no CLI, user-installed
SDK, entry execution, elevation, or change to its startup settings.

## Result and trust

`native_status` retains the LONG from WinVerifyTrust. Only **exactly
zero** means acceptance under this policy; it is not an HRESULT to test
with SUCCEEDED. `status` distinguishes trusted_local_policy, rejected,
no_embedded_signature, and unavailable. Known digest errors, explicit
distrust, untrusted roots or subjects, expiration, revocation, and
incorrect usage are classified as rejection. Other errors, including
unavailable providers and positive values, mean unavailable, not acceptance
or proof that a signature is absent.

TRUST_E_NOSIGNATURE is labeled no_embedded_signature only for this
file-mode query. It does not discover catalogs or prove the file is
unsigned in every sense. An observed native result can have available
support but unavailable status: the known datum is the provider error.
If the file cannot be opened or queried, the value is absent;
ACCESS_DENIED retains permission_required. Missing signatures do not
remove inventory entries.

`signer_name` is the display name of the first signer's leaf certificate
from WinTrust state, obtained **only after acceptance**. It is not a
unique identity, CompanyName, a guarantee of payload authorship,
reputation, or permission to run. An accepted signature may lack a readable
name. There is no fallback to VERSIONINFO resources. `source_executable`
and `policy` identify the source. A launcher EXE does not vouch for the
arguments, scripts, DLLs, or programs it loads.

## Policy and selection

WINTRUST_ACTION_GENERIC_VERIFY_V2, WTD_CHOICE_FILE, WTD_UI_NONE, and
window handle -1. WTD_CACHE_ONLY_URL_RETRIEVAL, WTD_REVOCATION_CHECK_NONE,
WTD_REVOKE_NONE, and WTD_DISABLE_MD2_MD4. Uses Windows local policy and
cache, without requesting provider network retrieval or checking
revocation. This does not establish current revocation status or cover
SmartScreen, antivirus, MOTW policy, all signatures on multiply signed
files, catalogs, or verification independent of host trust policy.
It does not change the certificate store.

Reuses [CompanyName's conservative selection](WINDOWS_STARTUP_PUBLISHER.en.md):
Run with an unambiguous literal EXE, a direct .exe, or a supported saved
local .lnk target (also requires the metadata opt-in). No PATH, variables,
UNC, extended prefixes, scripts, DLLs, .url handlers, or shell resolution.

The candidate is opened with GENERIC_READ/FILE_SHARE_READ and
OPEN_REPARSE_POINT. Directories, final-component reparse points, and
files larger than 128 MiB are rejected. The same open handle is passed to
WinTrust; it is always closed, as is the verification state through
WTD_STATEACTION_CLOSE, even on extraction errors. Provider certificates
and data are not freed separately. Restrictive sharing can prevent
reading files that have open writers. This does not guarantee future
launch identity, a universal snapshot, or subsequent immutability.
A mapped drive or intermediate reparse point can involve network I/O:
cache-only does not make every path local storage.

## Limits and use

Up to 50,000 rows, 16 distinct textual paths per sample, 128 MiB per
native file, and 16 MiB of retained text values (path/policy/name).
Per-call caching includes failures; signatures from previous samples
are not reused. Aliases and case are not canonicalized. Paths are limited
to 32,768 UTF-8 bytes and names to 16,384 bytes. Native names are limited
to 4,097 wchar_t units including the terminator, with strict UTF-8
conversion and no embedded NUL; extraction is limited to 64 certificates
in the chain and 64 signers. These are input and buffer limits, not a
provider I/O quota or deadline. This query is costly: do not run it on
every CPU/RAM tick. The host should choose a metadata polling interval
and present results as timestamped observations.

Startup status (unknown), impact (not_measured), CompanyName, and actions
are unchanged. Linux does not interpret these signatures; the native
reader is unsupported there. The 0.x API was extended: rebuild consumers
against installed headers and library.

## Verification

Portable tests cover zero, positive, rejected, and unknown codes; ignored
derived classifications; removal of names on failure; absent provider,
permissions, or status; independence from CompanyName; conservative
selection; caching including errors; 16/50,000/16 MiB limits; and invalid
labels. Injected Windows API tests check flags, GUID, UI, use of the same
handle, extraction only on success, provider certificates, invalid or
missing names, and cleanup on success, errors, and exceptions. A native
query checks the unsigned test EXE and optionally reads explorer.exe to
observe actual policy without running it. Also verified: independent
Monitor opt-in and opt-out and installed consumer result consistency.
The test does not change the registry, Startup folders, or trust stores.

Release results on 2026-10-15: Windows 70/70 and Linux 54/54. External
consumers were installed, rebuilt, and run with exit code 0 on both.
On the Windows host the fixture EXE returned no_embedded_signature;
explorer.exe returned LONG 0 and signer Microsoft Windows. This observes
that machine's local policy, not current revocation status or validity
on other hosts. Linux ran the portable pipeline and regression checks,
not native WinTrust. MSVC, catalogs, multiple signatures, and more
Windows hosts and policies remain unverified.

Official sources:
[WinVerifyTrust: LONG status and exact zero](https://learn.microsoft.com/en-us/windows/win32/api/wintrust/nf-wintrust-winverifytrust),
[WINTRUST_DATA: flags and cleanup](https://learn.microsoft.com/en-us/windows/win32/api/wintrust/ns-wintrust-wintrust_data),
[WINTRUST_FILE_INFO: file handle](https://learn.microsoft.com/en-us/windows/win32/api/wintrust/ns-wintrust-wintrust_file_info),
[WTHelperGetProvSignerFromChain](https://learn.microsoft.com/en-us/windows/win32/api/wintrust/nf-wintrust-wthelpergetprovsignerfromchain),
[CertGetNameStringW](https://learn.microsoft.com/en-us/windows/win32/api/wincrypt/nf-wincrypt-certgetnamestringw).
