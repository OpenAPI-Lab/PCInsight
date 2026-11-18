# Windows: Run new task

Implemented on 2026-10-13 through `SystemActions::launch_process` and
`CreateProcessW`. This is an explicit action, separate from sampling. It
does not require external programs or implicitly open a shell.

```cpp
auto actions = pcinsight::make_system_actions();
pcinsight::LaunchRequest request;
request.executable = R"(C:\Program Files\Example\app.exe)";
request.arguments = {"--name", "text with spaces", ""};
request.working_directory = R"(C:\Work)";
auto result = actions->launch_process(request);
// Check result.error and result.message; do not assume the app is ready.
```

## Contract

- Inputs must be strict UTF-8 with no embedded NUL. The executable must have
  an absolute drive path or conventional UNC path. PATH and App Paths are
  not searched, and variables are not expanded. Relative paths, paths quoted
  by the caller, and device namespaces (`\\?\`, `\\.\`) are unsupported
  in this version.
- An empty `working_directory` inherits the caller's directory. If specified,
  it must be absolute. Environment and security context come from the calling
  process. The action does not change user or session or drop elevation the
  parent already has.
- Arguments are encoded individually using Microsoft C runtime rules,
  including empty arguments, quotes, and trailing backslashes. Windows
  passes a command line rather than an argv vector; programs with their own
  parsers can interpret it differently. The entire line is limited to 32,766
  UTF-16 code units plus NUL.
- `lpApplicationName` is always explicit and separate, rather than inferred
  from the first part of the command line. The action does not enable general
  handle inheritance or redirect stdin/stdout/stderr. The console and UI
  follow the executable's and Windows' normal behavior.
- There is no ShellExecute, file association, or script/batch interpreter.
  The caller can explicitly choose an interpreter but must then treat its
  arguments as potential code. This API **is not a sandbox**.
- `elevated=true` returns `permission_denied` without starting anything:
  the UAC helper is not implemented. The API also distinguishes access denial
  or required elevation, missing files or paths, invalid parameters or formats,
  and other Win32 errors.
- Success means Windows created the process, not that initialization finished
  or that the process will keep running. `message` includes an informational
  PID, not a durable identity or a handle for later actions. Both native
  handles close through RAII. The call does not wait for the child, and closing
  the handles does not terminate it.

The caller must authorize the action and trust the selected executable.
Path validation does not verify signatures or publishers or prevent concurrent
file replacement. The API does not add privileges, search for executables,
or derive authorization policy from argument text.

## Verification and remaining work

Revision 37 update, 2026-10-19: a native test successfully launched the test
executable as a private child, confirmed the arguments through an event, and
used checked cleanup. All 20 repetitions passed. See
[Windows service view](WINDOWS_SERVICE_VIEW.en.md). This does not validate
arbitrary parsers, UAC, or every toolchain. The tests below are the original
2026-10-13 evidence, not the current integration status.

Injected CreateProcessW and CloseHandle tests cover security parameters,
writable buffers, handle closure, permissions and errors, Unicode, invalid
inputs, the exact length limit, and rejected elevation. Explicit escaping
cases and 300 deterministic sets of arguments are checked against
CommandLineToArgvW (parsing only, no execution). These tests launched no
real application.

Windows Release passed 10/10 and Linux WSL passed 5/5. Pending at this
checkpoint: integration with a disposable executable in isolation, validation
with MSVC and other architectures, a UAC helper, and efficiency mode.
[Forced termination](WINDOWS_TERMINATE.en.md) is implemented; cooperative
shutdown remains pending. This unit does not close the broader process subtask.

References: [CreateProcessW](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw)
and [C command-line argument rules](https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments).
