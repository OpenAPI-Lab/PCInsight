# Run new task on Linux: process launch and error reporting

Functional unit: `SystemActions::launch_process` validates the request
before creating a child and receives chdir/execve errors through an
O_CLOEXEC pipe. The earlier version returned success immediately after
fork even if execvp or chdir failed. It also allocated C++ memory in the
child, and execvp could implicitly invoke a shell for executable text
in an unrecognized format.

## Contract

- Executable must not be empty. Paths and arguments are Linux byte strings;
  UTF-8 is not required. Embedded NUL is rejected in executable, arguments,
  and working_directory.
- Paths containing `/` are tried directly. Bare names are searched using
  PATH captured from the caller. An empty PATH or an empty component uses
  the child's working directory. If PATH is absent, the library queries
  libc for `_CS_PATH`; it does not assume the working directory is included.
  Relative paths resolve after applying working_directory.
- `arguments` contains literal arguments after argv[0]. Quoting, expansion,
  redirection, and shell syntax are not interpreted. The library uses
  execve, never execvp or system, so ENOEXEC does not invoke `/bin/sh`.
  The kernel can invoke an interpreter for a script with a shebang, and
  the consumer can explicitly request an interpreter. This is not a sandbox.
- An empty working_directory inherits the current directory. The parent
  does not call chdir. No glibc posix_spawn_file_actions extensions or
  external applications are required.
- The library prepares argv, candidate paths, and an environment copy
  before fork. The child uses only close/chdir/execve/write/_exit, without
  C++ allocations, streams, or locks.
- ENOENT/ENOTDIR map to not_found, EACCES/EPERM to permission_denied,
  E2BIG/EINVAL/ENAMETOOLONG/ENOEXEC to invalid_argument, and other errors
  to system_error. During a PATH search, EACCES does not prevent trying
  later entries. If none succeeds and one was inaccessible, the result
  preserves permission_denied.
- Elevated=true is rejected unless geteuid is 0. If already 0, the process
  uses its current credentials. It does not request sudo/pkexec, change
  UID/GID, or elevate privileges.

## PID and child ownership

`ActionResult::process_id` contains the PID on a successful launch on
Linux and Windows. Other operations and pre-launch rejections use zero.
Linux also returns the PID if reading the error channel fails unexpectedly.
The result is system_error, the child's state is unknown, and the
consumer retains responsibility for it. If the child reports an error,
the library reaps that exact PID with waitpid before returning, never
with waitpid(-1).

The consumer must reap successfully launched children with waitpid or
its existing supervisor. PCInsight installs no SIGCHLD handler, creates
no hidden reaper, and changes no global configuration. Creation does
not prove readiness: the application may fail immediately, even with
exit status 127, without making the launch retrospectively fail. A PID
does not protect against later reuse.

EOF on the pipe means no pre-exec error was received. It normally comes
from CLOEXEC closing the pipe, but a fatal signal or external failure
before exec can also close it without an error report. EOF is not an
absolute guarantee of exec or initialization. The call is synchronous
with no overall deadline. Fork/exec on slow storage, the host's
pthread_atfork handlers, or waiting for a failed child that was externally
stopped can delay it. Do not automatically cancel a child or close the
caller's resources as an assumed rollback.

## Limits and integration

At most 4,096 arguments and PATH candidates; each string is limited to
131,071 bytes, the request and candidate paths to 1 MiB each, and the
environment to 2 MiB or 32,768 entries. The kernel may impose a smaller
ARG_MAX or additional format or permission requirements. Duplicate PATH
or an oversized environment is rejected before fork. Linux requires
pipe2/O_CLOEXEC (kernel 2.6.27 or later). Failure of that API propagates
without an unsafe fallback.

The host must coordinate setenv/unsetenv, chdir, SIGCHLD handling and
reaping, and descriptor closes if other threads modify them. Credentials,
signal masks, process group, stdio, and descriptors without FD_CLOEXEC
are inherited. Set CLOEXEC for resources that should not leak to children.
No separate terminal, isolation, custom environment, or UAC helper is
provided. A public 0.x struct was expanded; rebuild the library and
compatible consumers.

## Verification

On 2026-10-14, Linux Release passed 16/16 and Windows Release passed 27/27.
Packages were installed, and external consumers were rebuilt and run on
both systems. The consumer checks the new API with an invalid request
without launching an application.

The Linux test uses its own C++ test executable, never an application
belonging to the user. It checks empty arguments, spaces, quotes, newlines,
Unicode, literal shell syntax, a private working directory without changing
the parent's, relative paths and PATH, empty or absent PATH, permissions,
invalid executable format without a shell, NUL and limits, 32 errors without
zombies, exit status 7 after successful creation, and four concurrent calls.
The AuditedActions decorator preserves the PID and emits both events.
Private files under /tmp are removed at the end. The Windows CreateProcess
test uses injection and verifies the structured PID without launching
a Windows application.

References: [fork and multithreaded restrictions](https://man7.org/linux/man-pages/man2/fork.2.html),
[execve](https://man7.org/linux/man-pages/man2/execve.2.html),
[pipe2/CLOEXEC](https://man7.org/linux/man-pages/man2/pipe.2.html),
[execvp and shell fallback](https://man7.org/linux/man-pages/man3/exec.3.html).
