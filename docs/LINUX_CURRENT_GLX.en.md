# OpenGL on Linux: current GLX context and declared profile capabilities

G05, revision 19. This complements the [EGL provider](LINUX_CURRENT_OPENGL.en.md)
with the same `SampleOptions.include_current_opengl_context=true` and
`include_gpus=true` options. The default false does not query or load
EGL/GLX. Call sample on the thread with the current context; Sampler
does not inherit the GUI thread's context.

## Provider and data

Direct GLX uses `GLX_MESA_gl_interop` and `GLX_MESA_query_renderer`, loaded
from libGLX.so.0 with fallback to libGL.so.1. It requires no GLX headers,
SDK, link dependency, or library installation. It uses libraries already
present; if they are missing, the data remains absent. It resolves only
query functions, never glXCreateContext, MakeCurrent, XOpenDisplay, or
destruction functions.

`GpuInfo.linux_opengl_details` contains:

- context_api/provider: EGL/GLX source and attribution mechanism.
- context_version: GL_VERSION, matching the legacy opengl_version field.
- reported_max_core_profile_version and
  reported_max_compatibility_profile_version: major.minor values
  declared by query_renderer, separate from the context. For example:
  context 4.2, core 4.6, compatibility 4.5. These do not universally
  certify the GPU's capabilities.
- core_profile_supported/compatibility_profile_supported: known false
  when the query returns 0,0. The maximum remains absent rather than
  becoming a fabricated version 0.0. Failure or a partial response
  remains unknown, not false. A failed profile query does not remove
  a valid GL_VERSION.

EGL also fills details, but not its profile maxima or flags. This
queries **declared** capabilities of the identified renderer rather
than creating or running profiles. The extension's memory, name, and
implementation version are not exposed as global free memory, a retail
name, or the kernel driver's date or version.

## Identity and ABI

The provider requires a current context and display, GLX 1.4 or later,
a valid screen, a direct context, and exact tokens for both extensions.
It resolves extension functions through glXGetProcAddressARB or
glXGetProcAddress. The dispatched name is glXGLInteropQueryDeviceInfoMESA,
not a direct export from an arbitrary vendor that may not own the
current GLVND context.

It requests only Mesa device-info ABI v1 (seven uint32 values, 28 bytes),
never passing that smaller structure with a newer version. It initializes
location and IDs with sentinels and requires returned version 1, a valid
BDF, known 16-bit PCI IDs, and declared acceleration=true. query_renderer
IDs must match interop. The provider does not choose a GPU by name,
order, screen, or vendor/device IDs alone.

It corroborates BDF by reading vendor and device under
/sys/bus/pci/devices/BDF through a pinned directory, without following
attribute symlinks and with reads bounded to 64 bytes. It reopens and
checks the directory's dev/inode and rereads the attributes. Explicit
denial is distinguished from absence before confirming the match.
An injected resolver callback does not establish real sysfs coverage.
After GL_VERSION and profile queries, it repeats full interop, IDs,
acceleration, PCI resolution, context, display, and screen checks.
Exactly one inventoried GPU must have a matching available location,
and any additional IDs present must agree.

Estimated or duplicate locations, a different domain, software or
indirect rendering, ES, missing API or extensions, and observed changes
are not attributed. When EGL and GLX both claim a current context, the
combined flow rejects ambiguous dispatch. GL_VERSION uses the desktop,
length, NUL, and ASCII checks shared with EGL.

## Effects and scope

There are no new X11 or Wayland connections, windows, processes, helpers,
threads, elevation, API/context/display changes, or Mesa export/flush
calls. Only library references are acquired and released through
shared_ptr/dlclose. GLX queries can communicate with the X server
through the caller's display and affect its error state. The provider
does not install a global X error handler, consume glGetError, or
change Xlib's threading model. The caller retains synchronization
responsibility and owns the display and context. There is no driver
deadline or atomic snapshot against hotplug. Before-and-after checks
are observations, not a lock.

Other GLX vendors without verifiable identity leave data absent.
Installing OpenGL/GLX does not guarantee per-GPU attribution. The
provider does not open a display or create a context to rescue an
environment without a current context. There is no cache from another
context. Independent headless probing and physical tests are not
claimed. Windows does not use this provider and remains under M04.
G07/N02 still cover real hardware, drivers, and distributions, not
covered by fixtures. The 0.x GpuInfo layout change requires rebuilding
and relinking all consumers.

ABI sources: [Mesa device-info header](https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/include/GL/mesa_glinterop.h)
and [Mesa GLX dispatch](https://chromium.googlesource.com/external/gitlab.freedesktop.org/mesa/mesa/%2B/ea72882df13e275473b2a9c83e93786b58fadc80/src/glx/glxcmds.c).
Specification: [GLX_MESA_query_renderer](https://registry.khronos.org/OpenGL/extensions/MESA/GLX_MESA_query_renderer.txt).

## Revision 19 verification (2026-10-17)

Linux Release passed 87/87 and Windows Release passed 105/105. Both
installed consumers were rebuilt, relinked, and exited 0. Four dedicated
EGL/GLX tests passed ten consecutive runs each (40 runs), with assertions
enabled in Release. The libGLX.so.0 fixture exports only query and
get-proc functions. Dynamic loading produced context 4.2, core 4.6, and
compatibility 4.5 with injected identity. Cases include missing APIs,
suffixed extension tokens, software or indirect rendering, invalid IDs,
ranges or ABI, permission errors, PCI mismatch, different domains,
duplicate or estimated location, context/display/screen/identity changes,
invalid/ES/non-NUL-terminated versions, failed, partial, or 0,0 profile
responses, and EGL+GLX ambiguity.

The native PCI reader also runs against private sysfs-like files. It
covers success, absence, mismatched IDs, invalid or truncated text,
attribute symlinks, directory replacement, and attribute changes during
reads. This exercises the real reader on fixtures, not real PCI hardware.
All fixtures are removed from their private temporary directories.
Native WSL had no current GLX context and zero attributions. No software
context was invented and no display opened. Physical GPU validation
remains under G07/N02.

Workspace-root logs: work/glx-context-build-linux.log,
work/glx-context-final-build-windows.log, work/glx-context-suite-{linux,windows}.log,
work/glx-context-targeted-linux.log, work/glx-context-repeat-linux.log, and
work/glx-context-native-linux.log. An initial build during edits failed
with an old object and new test symbol. glx-context-initial-build-linux.log
was retained. A subsequent stable rebuild completed; initial clock-skew
and NOMINMAX warnings did not appear in the final incremental builds.
The clock was not changed. Earlier N03 CPU and history risks are not
claimed resolved.
