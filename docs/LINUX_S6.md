# s6: registro de estado nativo

Desde revisión134 existe el [control FIFO s6](LINUX_S6_CONTROL.md), separado del
registro raw. Entrega completa NO confirma vida/readiness ni finalización.
Revisión135 añade [inventario explícito con loggers](LINUX_S6_INVENTORY.md),
sin convertir estos registros en prueba de servicios vivos.

Revisión133, unidad de K03. API pública `pcinsight/linux_s6.hpp`:

```cpp
auto record = pcinsight::read_linux_s6_status_record("/service/mi-servicio");
// Comprobar value/support/note. Es un REGISTRO, no prueba de servicio vivo.
```

`decode_linux_s6_status_record(span)` valida el formato actual de 35 bytes y
`read_linux_s6_status_record(path)` lo lee directamente, sin ejecutar s6-svstat,
otros programas, scripts o bibliotecas s6/skalibs. No instala dependencias, crea
directorios, adquiere locks, abre FIFOs para I/O, escribe órdenes o envía señales.
No se integra automáticamente con Monitor ni con ServiceInfo/systemd/runit.

## Contrato de datos

Devuelve dos marcas TAI64N de 12 bytes sin conversión a Unix/uptime, PID reportado
u64 big-endian, wait status u16 big-endian y cuatro flags: paused, finishing,
want_up y ready_reported. No son el layout ni los estados enumerados de runit.
Solo se aceptan 35 bytes exactos, nanosegundos menores a 1e9 en ambas marcas y
bits conocidos del byte flags. Otros tamaños/bits son UNKNOWN, no adaptación
automática de versiones. `ready_changed_at_tai64n` conserva readystamp raw: no
se presupone que sea exclusivamente el momento en que la app estuvo lista.

El PID se conserva como dato u64 sin truncarlo a PID local. Puede ser ajeno al
namespace observador; durante finish puede referirse al ejecutor de finish, no
al daemon principal. NO se usa como identidad, para señales o como liveness.
El wait word puede describir una salida anterior; no es un exit code actual.
Se preservan flags sin inventar relaciones entre ellos: PID cero con ready=true
es un caso válido del supervisor. `ready_reported` NO significa app viva/sana.

**Available significa registro binario leído/validado**, no supervisor vivo,
estado actual, readiness efectiva ni éxito de una solicitud. El archivo puede
quedar tras morir el supervisor. Por diseño se devuelve ese registro incluso
sin supervisor/FIFO/lock presente; no se lo transforma en running. No hay campo
de liveness conocido, sondeo de lock ni autenticación de peer en esta unidad.

## Selección y consistencia

Servicio absoluto explícito de confianza, máximo32768 bytes/sin NUL. openat2
NO_MAGICLINKS para pin de directorio y supervise; admite links ordinarios de
directorios. Status debe ser un nodo regular real de tamaño35, no un link.
Primero se fija con O_PATH/NOFOLLOW y se verifica tipo/tamaño; solo se reabre
**ese FD propio** por /proc/self/fd para I/O, verificando dispositivo/inodo.
No se abre para leer un path unchecked que pudiera cambiar a FIFO/device.

Dos lecturas acotadas de hasta36 bytes, con metadata mtime/ctime/tamaño estable
por lectura y contenido idéntico entre ellas. Luego se revalida selección de
servicio/supervise y contexto boot/procfs/PID/inicio/UID/namespaces/eUID. Errores,
truncado, selección/contexto cambiados o datos distintos -> UNKNOWN, no cero/down.
Root faltante no se crea. Requiere openat2/procfs utilizable; no fallback inseguro.
No garantiza snapshot atómico, detectar todo ABA, latencia máxima de I/O ni que
el archivo no cambie después. Host serializa fork/setns/credenciales/FD contexto.
Consulta por llamada, sin hilo, polling implícito o caché de último dato bueno.

## Evidencia y pendientes

`tests/linux_s6.cpp`: tamaños0..34/36 inválidos, endian, TAI límite/inválido en
ambas marcas, todas combinaciones de flags conocidas y bits reservados, PIDu64
sin truncado, PID0/readytrue, lectura nativa de árbol privado sin supervisor,
root ausente sin crear, alias normal, status link/FIFO/directorio rechazados
sin bloqueo, formato runit20 rechazado, repeticiones y FD sin fuga. La prueba nativa
contrasta también permission_required con archivo privado sin lectura si el
usuario de prueba no es root (no se finge esa comprobación bajo root). MSVC solo
contratos puros y fallback no-Linux. SDK consumidor enlaza header/símbolos nuevos.
No se afirman pruebas deterministas de cada carrera ni aceptación s6 real.

La unidad133 no incluía control; revisión134 añade el [handle FIFO](LINUX_S6_CONTROL.md)
con consentimiento. El inventario llegó en revisión135 y la [observación de
condiciones](LINUX_S6_OBSERVATION.md) en136. K03 sigue abierta por evidencia de
liveness/aceptación real e integración;
SysV y otros pendientes siguen intactos. OpenRC control con scripts/runtime se
mantiene aplazado sin autorización explícita de la excepción132: «prosigue» no
la concede. Recolección continúa sin aplicaciones externas. I07 observador intacto.

Fuentes primarias del formato: [header s6](https://raw.githubusercontent.com/skarnet/s6/master/src/include/s6/supervise.h),
[unpack](https://raw.githubusercontent.com/skarnet/s6/master/src/libs6/s6_svstatus_unpack.c),
[supervisor](https://raw.githubusercontent.com/skarnet/s6/master/src/supervision/s6-supervise.c).
El [chequeo s6_svc_ok](https://raw.githubusercontent.com/skarnet/s6/master/src/libs6/s6_svc_ok.c)
usa el lock; esta unidad no lo replica ni afirma equivalencia con él.
