# OpenGL Linux: contexto EGL actual, lectura opcional

Revision19: este proveedor EGL se complementa con GLX directo y sondeo de
capacidades declaradas de perfiles en [LINUX_CURRENT_GLX.md](LINUX_CURRENT_GLX.md).
La misma opcion opt-in consulta EGL/GLX sin crear contextos; si ambas APIs
afirman tener contexto actual, rechaza la atribucion por dispatch ambiguo.
`GpuInfo.linux_opengl_details` separa contexto de maximos declarados. Nuevo
layout GpuInfo0.x: recompilar TODOS los consumidores. Evidencia anterior abajo
conservada como historica; GLX ya no es pendiente desde revision19.

`SampleOptions.include_current_opengl_context=true` permite rellenar
GpuInfo.opengl_version con GL_VERSION del contexto desktop OpenGL actual en
el hilo que ejecuta Monitor.sample. Por defecto false: no carga EGL/GLX ni consulta
el contexto. También requiere include_gpus=true.

```cpp
pcinsight::SampleOptions options;
options.include_current_opengl_context = true;
// Ejecutar aquí, en el hilo que ya tiene el contexto EGL/GLX OpenGL del GUI actual.
auto snapshot = monitor.sample(options);
```

El valor describe ese contexto, **no la versión máxima soportada por la GPU**,
el loader ni la certificación de aceleración hardware. Un contexto antiguo
puede devolver una versión menor que la capacidad del driver. No se consulta
OpenGL ES como si fuera desktop GL. Otras GPU sin ese contexto quedan sin valor.
Sampler llama desde su propio hilo: no hereda el contexto del hilo del GUI.

## Identidad

libEGL.so.1 mediante dlopen local, sin SDK ni enlace EGL/OpenGL. Solo funciones
de consulta: contexto/display actuales, tipo del contexto (EGL_OPENGL_API),
extensiones cliente y dispositivo, y EGL_DEVICE_EXT. Se comprueba soporte
EGL_EXT_device_query/base antes de consultar el dispositivo. Se rechazan
dispositivos EGL_MESA_device_software.

Se consulta nodo render si anuncia EGL_EXT_device_drm_render_node, o nodo DRM
primario mediante EGL_EXT_device_drm. Para la ruta /dev/dri/cardN o renderDN:
stat debe demostrar char device; /sys/dev/char/major:minor/subsystem debe resolver
a /sys/class/drm; device debe resolver a BDF PCI válido. No se iguala N al índice
de GPU. Se revalida dev/inode/rdev y, tras GL_VERSION, se repite la resolución
PCI y se comprueba que contexto/display actuales sigan iguales. Solo una GPU
DRM/NVML con esa ubicación available recibe el dato. Duplicados, ubicación
estimada/ausente, otro dominio o identidad cambiante no se atribuyen.

Strings acotadas: extensiones64KiB, ruta4KiB, GL_VERSION1KiB, NUL y formato
major.minor o major.minor.release desktop comprobados, con información del
proveedor separada por espacio. Cada componente debe ser decimal sin signo,
representable como uint32; major distinto de cero. La cadena completa se
conserva, incluyendo release/proveedor. Sin contexto, API/extensión, permisos/identidad
o formato válidos: dato ausente, no versión cero ni default de otra GPU.
Una denegación explícita al resolver el nodo se distingue permission_required.

## Efectos y límites

No crea, hace current, cambia de API, libera ni destruye contextos, superficies
o displays; no EGLInitialize/Terminate, BindAPI, MakeCurrent ni carga gráfica.
No threads auxiliares, procesos, conexiones X11/Wayland nuevas ni root/DRM master. El único recurso
adquirido es la referencia de biblioteca, liberada por shared_ptr/dlclose.
No se leen/consumen eglGetError/glGetError, pero una consulta fallida del driver
puede actualizar su estado de error: opt-in no significa invisibilidad absoluta.
No se impone deadline a un driver ni snapshot atómico frente a hotplug.

Crear/terminar un display automáticamente no sería neutral: un EGLDisplay puede
estar compartido con el GUI. Esta unidad no hace ese sondeo. Revision19 agrega
GLX actual y máximos declarados de perfiles mediante queries, sin creación de
contextos independientes. No afirma sondeo headless, otros buses, Windows ni
validación física Linux. No hay cache global de otro contexto. G07/N02 y M04
conservan sus alcances físicos/Windows. La API0.x exige recompilar TODOS.

## Verificación histórica EGL (anterior a revisión19)

Linux Release41/41 y Windows55/55; paquetes y consumidores instalados ejecutados.
Pruebas inyectadas: contexto desktop y PCI correcto, otro dominio, ausencia de
contexto/display, ES, queries fallidas, extensiones con sufijo/nulas/truncadas,
software, nodos ausentes, render->primario y primario solo, permisos, identidad
no probada/cambiante/duplicada, contexto/display cambiados, versión inválida/ES/
sin NUL/no ASCII, API ausente, default opt-out e include_gpus=false.
Regresión 2026-10-14: se reprodujo primero el rechazo incorrecto de versiones
con release (por ejemplo 4.6.0 NVIDIA fixture). Corregido con casos positivos
de dos/tres componentes y negativos de componentes vacíos, signos, overflow,
cuarto componente, sufijos no separados y ES. Linux41/41 y consumidor instalado
reverificados; el fixture representa formatos, no certifica un driver NVIDIA.
Fixture libEGL.so.1 solo exporta consultas y eglGetProcAddress: carga dinámica
positiva 4.6(Core Profile) fixture con resolución PCI inyectada. No simula que
se haya validado stat/sysfs positivo real ni un contexto hardware real.

Consulta nativa WSL sin contexto actual y sin DRM: dato ausente, no fallback
software. **Validación física con GUI EGL/Linux y GPU DRM pendiente.**

Fuentes: [device query](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_query.txt),
[device DRM](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_drm.txt),
[render node](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_drm_render_node.txt),
[formato GL_VERSION](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetString.xhtml).
