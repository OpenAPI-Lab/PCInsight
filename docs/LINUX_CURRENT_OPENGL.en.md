# OpenGL on Linux: optional read of the current EGL context

Revision 19: direct GLX and declared profile capability queries in
[current GLX](LINUX_CURRENT_GLX.en.md) complement this EGL provider. The
same opt-in option queries EGL and GLX without creating contexts.
If both APIs claim a current context, attribution is rejected because
dispatch is ambiguous. `GpuInfo.linux_opengl_details` separates the
context from declared maxima. The new 0.x GpuInfo layout requires
rebuilding all consumers. Earlier evidence below is historical; GLX
support was added in revision 19.

`SampleOptions.include_current_opengl_context=true` lets the provider
fill GpuInfo.opengl_version with GL_VERSION for the current desktop
OpenGL context on the thread calling Monitor.sample. It defaults to
false, so EGL/GLX is not loaded and the context is not queried.
It also requires include_gpus=true.

```cpp
pcinsight::SampleOptions options;
options.include_current_opengl_context = true;
// Run on the thread that already owns the GUI's current EGL/GLX OpenGL context.
auto snapshot = monitor.sample(options);
```

The value describes that context, **not the GPU's maximum supported
version**, loader version, or proof of hardware acceleration. An older
context can report a lower version than the driver supports. OpenGL ES
is not queried as desktop GL. Other GPUs without that context remain
without a value. Sampler calls on its own thread and does not inherit
the GUI thread's context.

## Identity

The provider loads libEGL.so.1 with local dlopen without an EGL/OpenGL
SDK or link dependency. It uses only queries for the current context
and display, context type (EGL_OPENGL_API), client and device
extensions, and EGL_DEVICE_EXT. It checks EGL_EXT_device_query/base
before querying the device and rejects EGL_MESA_device_software devices.

It queries the render node when EGL_EXT_device_drm_render_node is
advertised, or the primary DRM node through EGL_EXT_device_drm. For
/dev/dri/cardN or renderDN, stat must establish a character device,
/sys/dev/char/major:minor/subsystem must resolve to /sys/class/drm, and
device must resolve to a valid PCI BDF. The node number is not equated
with a GPU index. The provider revalidates dev, inode, and rdev, then
repeats PCI resolution after GL_VERSION and confirms the same current
context and display. Only one DRM/NVML GPU with that available location
receives the value. Duplicates, estimated or absent locations, other
domains, and changing identities are not attributed.

Strings are bounded: extensions to 64 KiB, path to 4 KiB, and
GL_VERSION to 1 KiB. NUL termination and desktop major.minor or
major.minor.release format are checked, with vendor information
separated by a space. Each component must be unsigned decimal and
fit in uint32; major must be nonzero. The full string is retained,
including release and vendor. Missing context, API or extension,
permission or identity, or a valid format yields absence, not version
zero or a default from another GPU. Explicit denial when resolving
the node is classified as permission_required.

## Effects and limits

The provider does not create, make current, release, or destroy a
context, surface, or display, or change the API. It does not call
EGLInitialize/Terminate, BindAPI, or MakeCurrent or run a graphics
workload. It adds no helper threads, processes, new X11/Wayland
connections, root, or DRM master. The only acquired resource is a
library reference released by shared_ptr/dlclose. It does not read or
consume eglGetError/glGetError, but a failed driver query can change
its error state: opt-in does not mean absolute invisibility. There
is no driver deadline or atomic snapshot against hotplug.

Automatically creating or terminating a display would not be neutral:
an EGLDisplay can be shared with the GUI. This work does not perform
that probe. Revision 19 adds current GLX and declared profile maxima
through queries, without creating independent contexts. It does not
claim headless probing, other buses, Windows, or physical Linux
validation. There is no global cache from another context. G07/N02
and M04 retain their physical and Windows scope, respectively. The
0.x API requires rebuilding all consumers.

## Historical EGL verification (before revision 19)

Linux Release passed 41/41 and Windows Release passed 55/55; packages
were installed and installed consumers ran. Injected tests cover
desktop context and matching PCI, other domains, missing context or
display, ES, failed queries, suffixed/null/truncated extensions,
software devices, missing nodes, fallback from render to primary and
primary-only, permissions, unverified/changing/duplicate identity,
context or display changes, invalid/ES/non-NUL-terminated/non-ASCII
versions, unavailable API, default opt-out, and include_gpus=false.

The 2026-10-14 regression first reproduced incorrect rejection of
versions with a release component, such as the 4.6.0 NVIDIA fixture.
The fix covers positive two- and three-component versions and rejects
empty components, signs, overflow, a fourth component, unseparated
suffixes, and ES. Linux 41/41 and the installed consumer were
reverified. The fixture represents formats; it does not certify an
NVIDIA driver. The libEGL.so.1 fixture exports only query functions
and eglGetProcAddress. It verifies dynamic loading with a 4.6
(Core Profile) fixture and injected PCI resolution, not a positive
real stat/sysfs result or a real hardware context.

The native WSL query had no current context or DRM and returned
absence without software fallback. **Physical validation with a
Linux EGL GUI and a DRM GPU remains pending.**

Sources: [device query](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_query.txt),
[DRM device](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_drm.txt),
[render node](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_drm_render_node.txt),
and [GL_VERSION format](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetString.xhtml).
