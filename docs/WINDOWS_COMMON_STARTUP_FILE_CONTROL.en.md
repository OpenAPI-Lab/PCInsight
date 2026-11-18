# Common Startup files: explicit all-user scope

Revision 47, 2026-10-19. The Startup file provider also supports inventory
IDs starting with `winfolder:common:`. It resolves them through
FOLDERID_CommonStartup, not FOLDERID_Startup, hardcoded English paths, or
profile scanning. FOLDERID_Startup still applies only to `winfolder:user:`.
Run/HKLM IDs are not treated as files, and StartupApproved is not implemented.

## API and scope separation

```cpp
// Read-only; does not create the folder or a backup:
auto shared = pcinsight::list_windows_common_startup_file_backups();
// list_windows_startup_file_backups() still covers ONLY the current user.

// Query a common inventory entry or a row from the backup listing:
// auto observed = pcinsight::read_windows_startup_file_registration(common_entry.id);
// Clearly disclose the ALL USERS scope and obtain consent.
// auto result = actions.set_windows_startup_file_registered(*observed.value, false);
// true restores; AuditedActions is recommended for recording the attempt and outcome.
```

Reads and actions select the folder by exact ID prefix, verify that the
encoded path is a direct entry in the corresponding folder, and compare
observed identities before changing anything. The common listing emits
common IDs; they are not converted to user IDs or automatically merged
with the user listing. An ID from one scope with a path from another
cannot retarget outside the selected folder. If Windows redirects both
folders to the same object, the same file can appear with distinct IDs
for each scope. This does not establish that they are separate apps or
make IDs an authorization boundary.

The current Monitor/Sampler opt-in still reads ONLY user backups. The
host queries the common listing separately when needed. No snapshot
fields or options or automatic common refresh are added. SystemActions
and AuditedActions retain the same explicit method, without a new virtual
method or AuditOperation value. The target retains the common prefix.

## Permissions and impact

**Removing or restoring a common file can affect startup for ALL USERS.**
The host must disclose that scope before asking for consent. This is a
0.x behavior change: a valid common ID that previously failed can now be
acted on if the caller already has permission. Generic Windows
set_startup_enabled does not route to this mechanism, and existing
listings are not silently expanded. Update and rebuild consumers for
the additional function. This component introduces no new layouts,
virtual ABI changes, or persisted formats.

The provider uses the current process or thread's rights, never an
elevated token acquired by the library. No UAC, automatic impersonation,
ACL changes, privilege enabling, elevated retries, or fallback to
personal Startup when common Startup fails. Read denial leaves the Metric
without a value and with permission_required; action denial returns
permission_denied. Read access does NOT establish removal or restoration
rights. Errors do not become enabled/disabled states or empty lists.

The backup is PCInsight.Disabled.v1 inside the selected scope's folder,
not a private store for the user calling the function. Do not delete the
backup or change its permissions. The provider does not authenticate
authors or signers, establish that content is safe, or verify the
protection offered by file or store ACLs. In particular, a privileged
helper must not accept an ID, observation, or actor_label as authorization
to publish files supplied by untrusted users into common Startup. L05
(helper, requester, and consent) remains pending; this API does not replace it.

The [checked file contract](WINDOWS_STARTUP_FILE_CONTROL.en.md) is unchanged:
one handle-based rename without replacement, no copying or deleting
contents, and no KTM. Indices, timestamps, and size are not cryptographic
identity or ABA protection. The provider does not change or stop processes
or alter Windows approval or other sources. Only local drive paths are
accepted; UNC, network drives, and final-component reparse points remain
unsupported. The same row and text limits, estimated/non-atomic states,
and no-partial-result error behavior apply. The store may remain empty
after restoration, with no automatic cleanup. The library does not
explicitly apply new ACLs or guarantee that no one else can modify it later.

## Evidence and validation limits

tests/windows_startup_file_control.cpp adds two distinct private folders,
with unique fixture names even within one millisecond. Internal
injection redirects only Known Folder lookup by scope; NtCreateFile and
rename are real. Tests cover successful audited common removal, listing,
and restoration; conflicts without overwrite; rejection of the wrong
prefix with a correct path and of the wrong root without retargeting;
exclusion of common entries from user listings; invalid enum values;
and preservation of an injected enumeration permission error. Bytes
are checked on removal and restoration. Fixture and real-directory
permissions are unchanged.

The new public function uses real Known Folder lookup only for reads in
verification. The installed consumer links it and checks prefixes when
rows exist. No real common entries are created and no files are run.
A positive result in a writable fixture does NOT establish actual
ProgramData permissions, UAC behavior, other policies or ACLs, other
filesystems, or that a non-elevated user can write to real common Startup.
This component did not test elevated mutation or a real ACL denial.
permission_required is checked through the injected enumeration API.
The kernel remains responsible for authorizing actions.

Logs: work/startup-common-*.log. This completes common scope for the
FILE provider, not HKLM Run, StartupApproved, or all M05 criteria.

Release validation: Windows 125/125 and Linux 110/110. Twenty repetitions
of each Windows control/audit/monitor test (60 runs). Both installed
consumers were rebuilt, relinked, and run with exit code 0; builds had
no warnings. Linux checks the stub and regressions, not a Common Startup
provider. Private fixtures were cleaned up. Real common Startup, ACLs,
configuration, and privileges were not changed.
