# OpenGL Linux: GLX actual y capacidades de perfiles declaradas

G05, revision19. Complementa [LINUX_CURRENT_OPENGL.md](LINUX_CURRENT_OPENGL.md)
con el mismo `SampleOptions.include_current_opengl_context=true` y
`include_gpus=true`. Default false no consulta ni carga EGL/GLX. Ejecutar sample
en el hilo con contexto actual: Sampler no hereda el contexto del GUI.

## Proveedor y datos

GLX directo con `GLX_MESA_gl_interop` y `GLX_MESA_query_renderer`, cargado de
libGLX.so.0 (fallback libGL.so.1). Sin headers/SDK/enlace GLX ni instalación de
librerías: usa las ya presentes; si faltan, no hay dato. Solo obtiene queries,
nunca glXCreateContext, MakeCurrent, XOpenDisplay ni funciones de destrucción.

`GpuInfo.linux_opengl_details` contiene:

- context_api/provider: origen EGL/GLX y mecanismo de atribución.
- context_version: GL_VERSION, igual al legacy opengl_version.
- reported_max_core_profile_version y
  reported_max_compatibility_profile_version: major.minor declarados por
  query_renderer, separados del contexto. Ejemplo: contexto4.2, core4.6,
  compatibilidad4.5. No son una certificación universal de la GPU.
- core_profile_supported/compatibility_profile_supported: false conocido al
  devolver0,0; máximo ausente, no versión0.0 fabricada. Fallo/respuesta parcial
  conserva unknown, no false. Un perfil fallido no elimina GL_VERSION válido.

EGL también rellena details, pero no sus máximos/flags de perfiles. Es sondeo de
capacidades **declaradas** del renderer identificado, no creación/ejecución de
perfiles. No exporta memoria/nombre/versión de implementación de esa extensión
como memoria libre global, nombre comercial o fecha/versión del driver kernel.

## Identidad y ABI

Requiere contexto/display actuales, GLX>=1.4, screen válido, contexto directo y
ambas extensiones por token exacto. Funciones de extensión mediante
glXGetProcAddressARB/glXGetProcAddress: nombre despachado
glXGLInteropQueryDeviceInfoMESA, no export directo de un vendor arbitrario con
contexto que podría pertenecer a otro vendor GLVND.

Solicita exclusivamente ABI v1 de Mesa device-info (7uint32,28bytes), nunca esa
estructura reducida con versión nueva. Inicializa ubicación/IDs con sentinels
y exige versión1 devuelta, BDF válida, IDs PCI16bit conocidos y aceleración
declarada=true. Los IDs de query_renderer deben coincidir con interop; no elige
GPU por nombre, orden, screen o vendor/device IDs solos.

Corrobora BDF leyendo vendor/device de /sys/bus/pci/devices/BDF por directorio
fijado, archivos sin seguir symlinks y lecturas acotadas64bytes. Reabre/verifica
dev/inode del directorio y relee atributos; distingue denegación explícita antes
de confirmar de ausencia. El callback de resolución inyectado no certifica sysfs
real. Tras GL_VERSION y perfiles repite interop completo, IDs, aceleración,
resolución PCI, contexto/display y screen. Exige exactamente una GPU inventariada
con ubicación available coincidente y concordancia de IDs adicionales presentes.

Ubicación estimated/duplicada, otro dominio, software/indirecto, ES, API/extensión
incompleta o cambio observado no se atribuyen. Si EGL/GLX afirman simultáneamente
tener contexto actual, el flujo combinado rechaza la ambigüedad de dispatch.
GL_VERSION usa la validación desktop/longitud/NUL/ASCII compartida con EGL.

## Efectos y alcance

Sin nuevas conexiones X11/Wayland, ventanas, procesos, helpers, threads,
elevación, cambios de API/contexto/display o funciones de export/flush de Mesa.
Solo se adquieren referencias de biblioteca, liberadas por shared_ptr/dlclose.
Las consultas GLX pueden comunicarse con el X server por el display del caller
y afectar su estado de error. No instala X error handler global, no consume
glGetError ni cambia el modelo de threading Xlib; caller conserva sincronización
y propiedad del display/contexto. No deadline frente a driver ni snapshot
atómico ante hotplug. Chequeos antes/después son observaciones, no un bloqueo.

Otros vendors GLX sin identidad comprobable quedan sin dato. Tener OpenGL/GLX
instalado no garantiza atribución por GPU. No abre displays ni crea contextos
para rescatar un entorno sin contexto actual. No hay cache de otro contexto.
Sondeo headless independiente y pruebas físicas no se afirman implementados.
Windows no usa este proveedor; su cobertura sigue en M04. G07/N02 siguen
responsables de hardware/driver/distribuciones reales, no cubiertos por fixtures.
API0.x cambia layout GpuInfo: recompilar y relinkar TODOS los consumidores.

Fuentes del ABI: [Mesa device-info/header](https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/include/GL/mesa_glinterop.h),
[dispatch Mesa GLX](https://chromium.googlesource.com/external/gitlab.freedesktop.org/mesa/mesa/%2B/ea72882df13e275473b2a9c83e93786b58fadc80/src/glx/glxcmds.c).
Especificación: [GLX_MESA_query_renderer](https://registry.khronos.org/OpenGL/extensions/MESA/GLX_MESA_query_renderer.txt).

## Verificación de revisión19 (2026-10-17)

Linux Release87/87 y Windows105/105; ambos consumidores instalados recompilados,
relinkados y ejecutados exit0. Cuatro tests EGL/GLX específicos y10 repeticiones
consecutivas de cada uno (40ejecuciones) aprobados, asserts activos Release.
Fixture libGLX.so.0 exporta solo consultas/get-proc, carga dinámica positiva
contexto4.2/core4.6/compat4.5 con identidad inyectada. Escenarios incluyen API
ausente, extensiones-sufijo, software/indirecto, IDs/rangos/ABI incorrectos,
permisos, no-coincidencia PCI, dominio distinto/duplicados/estimated,
contexto/display/screen/identidad cambiantes, versión inválida/ES/sin NUL,
perfiles fallidos/parciales/0,0 y ambigüedad EGL+GLX.

El lector nativo de PCI también se ejecuta con archivos privados tipo sysfs:
positivo, ausencia/IDs distintos/texto inválido o truncado, symlink de atributo,
reemplazo de directorio y cambio de atributos durante la lectura. Es prueba del
lector real sobre fixture, no hardware PCI positivo. Todos los fixtures se
eliminan de sus directorios temporales privados al terminar.
Nativo WSL: contexto GLX ausente,0atribuciones; no se inventa contexto software
ni se abre un display. Hardware físico/GPU real sigue pendiente en G07/N02.

Registros en la raíz de trabajo: work/glx-context-build-linux.log,
work/glx-context-final-build-windows.log, work/glx-context-suite-{linux,windows}.log,
work/glx-context-targeted-linux.log, work/glx-context-repeat-linux.log y
work/glx-context-native-linux.log. Primer build durante ediciones falló con
objeto anterior/nuevo símbolo de prueba; log glx-context-initial-build-linux.log
preservado. Reconstrucción estable posterior completó todo; warnings iniciales
clock-skew y NOMINMAX no aparecen en builds incrementales finales. No se cambió
el reloj. Los riesgos CPU/historial anteriores de N03 no se declaran resueltos.
