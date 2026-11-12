# OpenRC: lector nativo de marcas de estado

Desde revisión131 existe también el [catálogo init.d/runlevels](LINUX_OPENRC_CATALOG.md),
separado de estas marcas. No implica control, estado efectivo o aceptación real.

Revisión130, unidad de K03. API pública `pcinsight/linux_openrc.hpp`:

```cpp
auto inventory = pcinsight::query_linux_openrc_markers("/run/openrc");
if (!inventory.value) { /* mostrar support/note, no inventar cero */ }
else {
    // Consultar complete, diagnostics y disponibilidad por entrada.
    // entries describe marcas, NO todos los servicios instalados ni PIDs vivos.
}
```

El llamador selecciona explícitamente un árbol runtime de confianza. No se
autodetecta OpenRC ni se cambia Monitor/proveedor systemd. La consulta es
read-only por syscalls: no ejecuta rc-status, rc-service, shell, scripts o librc,
ni instala dependencias. No crea el root o categorías ausentes. No usa variables
de entorno para elegir el árbol. El host serializa fork/setns/cambios de credenciales.

Se observan siete categorías: started, stopping, starting, inactive, failed,
hotplugged y wasinactive. La máscara propia usa respectivamente bits 1, 2, 4, 8,
16, 32 y 64: **NO son los bits RC_SERVICE de librc**. Solo se aceptan entradas
symlink; no se lee, abre, valida ni sigue su destino. Una marca con destino roto
puede aparecer. Un archivo regular, directorio, FIFO o device se rechaza sin
abrirlo, hace incompleto el inventario y deja desconocidos los estados derivados.
Se omiten nombres ocultos. Las categorías deben ser directorios reales, no links.
El root admite links normales pero no magic links. Requiere openat2; si no está
disponible no hace fallback inseguro. No hay autenticación de OpenRC por el path.

`marked_state` describe únicamente una marca primaria no ambigua entre las
cuatro primeras categorías. Si faltan todas, hay conflicto, bits no reconocidos
o la consulta es parcial/inestable, queda UNKNOWN. `decode_linux_openrc_markers`
expone esta regla pura, sin consultar el sistema. Las flags complementarias
permanecen en la máscara; no prueban procesos caídos o readiness.

**started marcado NO equivale a running vivo.** No hay PID o identidad de proceso
verificados, tiempo de actividad, descripción, grupo Windows, scheduled o crashed.
No existe inferencia de stopped por ausencia: las apps detenidas/sin marcas no
pertenecen al universo de este censo. Empty significa ninguna marca observada,
no cero servicios instalados, y no demuestra que OpenRC esté instalado/activo.

Se hacen dos pasadas sobre descriptores de directorio, comparando conjuntos de
nombre/máscara, metadata de categorías y selección del root. Se comprueba contexto
boot/procfs/PID/inicio/UID/namespaces/eUID antes y después. Contexto cambiado
descarta el resultado; cambios de árbol/errores/bounds conservan observaciones
raw con `complete=false`, soporte estimated y estados derivados UNKNOWN. Esto
no es snapshot atómico ni detecta todo ABA/cambio entre muestras; tampoco ofrece
deadline duro de I/O. Evitar presentar estas observaciones como API rc_service_state.

Límites por pasada: 16384 entradas visitadas contando ocultas y dot, máximo de
nombres solicitado 1..4096 (1024 por defecto), payload de nombres 8MiB. Como solo
hay dos pasadas, el trabajo y diagnósticos quedan acotados. Resultados por nombre
ordenados; truncado/error nunca se anuncia como censo completo. Categoría que no
existe se observa vacía; categoría inaccesible/no-directorio no equivale a vacía.

## Evidencia y trabajo pendiente

`tests/linux_openrc.cpp`: decoder cuatro estados/flags/conflictos/unknown,
argumentos/bounds, root ausente sin creación, root vacío sin cambios, symlinks
rotos y ocultos, orden y flags, límite visible, archivo/FIFO rechazados sin
bloqueo, categorías symlink/FIFO rechazadas, magic link y recuento FD sin fuga.
Datos y árbol son PRIVADOS bajo /tmp; no se ejecuta OpenRC ni toca servicios reales.
MSVC solo comprueba contratos puros/fallback no-Linux, no soporte Windows nuevo.

K03 sigue abierta: inventario de servicios instalados/runlevels, PID/liveness,
control OpenRC real e integración no están implementados por este lector; tampoco
es aceptación en un sistema OpenRC real. runit real y adaptadores s6/SysV siguen
pendientes. No se debe escribir una marca para simular start/stop del servicio.

## Fuentes primarias revisadas

El [código librc](https://raw.githubusercontent.com/OpenRC/openrc/master/src/librc/librc.c)
crea las marcas con symlinks y consulta estado con comprobaciones adicionales;
su consulta de descripción emplea shell, por lo que NO se invoca aquí. Además,
algunas aperturas librc crean directorios; este observador no replica esa conducta.
El [header público](https://raw.githubusercontent.com/OpenRC/openrc/master/src/librc/rc.h.in)
define estados/flags y runtime Linux; las categorías se contrastaron con el
[header interno](https://raw.githubusercontent.com/OpenRC/openrc/master/src/librc/librc.h).
No se copia la implementación ni se asume equivalencia con su API.
