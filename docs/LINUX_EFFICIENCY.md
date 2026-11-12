# Preset de prioridad CPU/E/S Linux

Estado actual revisión41: alcance avanzado limitado aceptado2026-10-19,
ver
[LINUX_EFFICIENCY_DECISION.md](LINUX_EFFICIENCY_DECISION.md).
L03 se cierra con ese límite, NO identidad checked ni cobertura universal.
La llamada genérica está desactivada; solo el flujo avanzado con reconocimiento
explícito sigue disponible. Las secciones revision17/legacy conservan evidencia
histórica y no describen un bypass público actual.

## Flujo por threads y recibo de restauración (L03 parcial, revisión17)

La API aditiva `<pcinsight/linux_efficiency.hpp>` incorpora
`apply_linux_efficiency_unchecked(pid, expected_start_ticks, true)` y
`restore_linux_efficiency_unchecked(receipt, true)`. El último argumento es
reconocimiento explícito del riesgo PID/TID numérico; su valor predeterminado
false rechaza la operación sin mutar. No sustituye a
`SystemActions::set_efficiency_mode_checked`: esta continúa no soportada en Linux.
Revision41 desactiva el acceso genérico al preset legacy descrito más abajo.

El flujo nuevo enumera `/proc/PID/task`, captura el nice y el valor ioprio crudo
de cada thread, y conserva `start_ticks` de proceso y threads. Verifica contexto
procfs/PID del caller, lista y tokens antes de mutar; rechaza captura incompleta,
IDs duplicados/reservados, prioridades inválidas o cambios observados. Límite
4096 threads, lectura stat64KiB y hasta3 reintentos EINTR por lectura. No usa
utilidades externas, paquetes nuevos, suspensión, ptrace, helpers ni elevación.

Pide nice=max(original,10) e ioprio idle, omitiendo writes ya satisfechos.
Comprueba identidad y valor actual antes de cada campo, read-back después de
escribir, prioridades de todos los threads capturados al final y lista final.
Si la lista difiere antes de escribir, no inicia cambios; si difiere después,
informa cobertura parcial, no éxito. Una lista igual en dos instantes NO prueba
que no hubo threads intermedios. No sigue nuevos threads, descendientes ni exec;
exec no cambia necesariamente start_ticks. Nice e ioprio no garantizan ahorro.

El recibo conserva originales exactos por thread (incluida clase E/S NONE=0,
no sustituida por best-effort4), valores pedidos y flags `nice_pending`/
`io_pending`. Cada write exitoso deja su flag activo incluso si su read-back
falla; no se pierde el recibo tras errores. Restaurar solo trata esos campos:
si ya están en su original, limpia el flag sin escribir; si están en el valor
pedido, solicita el original y verifica; si observa otro valor, rechaza el
conflicto sin sobrescribirlo. Valida TODO el recibo antes de operar, incluidos
duplicados, rangos y correspondencia leader/token. No acepta recibos no confiables.

Se detiene en el primer error; no es una transacción ni un rollback automático.
Una restauración puede devolver error con E/S ya restaurada y CPU pendiente por
CAP_SYS_NICE/RLIMIT_NICE. Usar el **recibo devuelto** para reintentar: los campos
terminados tienen flags false; los restantes permanecen pendientes. No prometer
que restaurar podrá tener éxito sin permisos adicionales. Un thread terminado
puede impedir el resto del recorrido; no se omite silenciosamente.

Los tokens solo mitigan cambios detectables: las syscalls de prioridad no
aceptan pidfd y queda carrera entre lectura y write, incluso entre leer prioridad
y restaurarla. No llamarlo checked, seguro frente a reutilización ni compare-and-
swap. Un ioprio compartido por CLONE_IO puede tener efectos fuera del conjunto
capturado; se conserva/restaura el write registrado, sin fingir aislamiento.
El recibo es efímero del boot/namespace actual, no identidad persistente ni
autorización. El caller debe retenerlo; volver a aplicar crea otro recibo con
los valores de ese momento y no recupera los originales de una llamada anterior.
Estas funciones libres no pasan por AuditedActions; la integración que las use
debe registrar solicitud, resultado y recibo explícitamente si necesita auditoría.

Ejemplo (el PID/token provienen de un muestreo del boot/namespace actual):

```cpp
#include <pcinsight/linux_efficiency.hpp>
auto update = pcinsight::apply_linux_efficiency_unchecked(pid, start_ticks, true);
// Guardar update.receipt incluso si !update.result: puede haber cambios parciales.
// Restaurar después SOLO si el usuario solicita hacerlo; no en un destructor.
auto restored = pcinsight::restore_linux_efficiency_unchecked(update.receipt, true);
// Conservar restored.receipt si la restauración no terminó.
```

Windows devuelve not_supported para estas funciones; el modo EcoQoS existente
no cambia. API aditiva sin modificar layouts/virtuales existentes; recompilar y
relinkar consumidores para usar los símbolos nuevos. En revision17 L03 seguía E
porque este flujo no ofrece identidad checked ni seguimiento de threads futuros.
Revision41 registra aceptación expresa de ese límite; no declara implementadas
esas garantías ni concede crédito fraccionario al recibo.

Verificación2026-10-17: suites finales Linux85/85 y Windows105/105 Release,
ambos consumidores instalados recompilados/relinkados/ejecutados exit0 y
20/20 repeticiones consecutivas de linux-efficiency en Linux. Assertions activas.
Fixtures prueban captura incompleta/cambiante, IDs/tokens/recibos inválidos,
límites, prioridades -1/19, errores de contexto/API/lectura/write/read-back,
conflictos de restauración, cambios parciales, reintentos idempotentes y E/S
compartida simulada. Windows ejecuta lógica inyectada y rechazo nativo, no syscalls Linux.
Nativo Linux: hijo privado de2threads nice19 sin promoción CPU, E/S original
restaurada exactamente; otro hijo de2threads nice5/19, leader llevado a10 y
restauración CPU denegada errno13 sin elevar, E/S del leader restaurada y flag
CPU pendiente. Caller sin cambios, hijos recolectados. No procesos ajenos.
Registros: `work/efficiency-threads-build-{linux,windows}.log`,
`work/efficiency-threads-suite-{linux,windows}.log`,
`work/efficiency-threads-native-linux.log` y `work/efficiency-threads-repeat-linux.log`
en la raíz de trabajo (no se incluyen intermediates en el paquete).

## Preset legacy del hilo principal (interno/histórico desde revisión41)

La corrección original de `SystemActions::set_efficiency_mode(pid, enabled)`
en Linux se conserva como algoritmo interno de regresión. Desde revision41,
esa llamada pública devuelve not_supported sin mutar; no reutilizarla para
aplicar/deshacer ni como sustitución del recibo. El algoritmo histórico era
un preset de
prioridades del hilo principal, **no Windows EcoQoS**, política energética del
hardware ni garantía de menor consumo. No invoca nice/renice/ionice externos.

- enabled=true: nice=max(nice actual, 10), E/S clase idle (data 0). Un proceso con
  nice 19 conserva 19; activarlo nunca pide elevar su prioridad CPU a 10.
- enabled=false: nice=0 y E/S best-effort 4. Son valores fijos explícitos, no
  restauración de prioridades anteriores. Elevar prioridad CPU puede requerir
  CAP_SYS_NICE/RLIMIT_NICE; no se solicita elevación ni se instala un helper.

## Errores y cambios parciales

La versión anterior ignoraba el resultado de ioprio_set y anunciaba prioridades
“restauradas” después de imponer 0/best-effort 4. Ahora lee ambas prioridades
antes de mutar, rechaza PID 0/1/self/fuera de INT_MAX, API incompleta, nice fuera
de -20..19 y codificaciones E/S no reconocidas. No interpreta getpriority=-1
como error salvo que errno esté establecido.

Primero solicita E/S, después CPU, omitiendo writes ya satisfechos. Si E/S falla,
no solicita CPU. Si CPU falla después de cambiar E/S, devuelve el error real con
mensaje de **resultado parcial**. No realiza un rollback automático que pueda
fallar por permisos, perjudicar cambios concurrentes o afectar un PID reutilizado.
Lee ambas prioridades al final; discrepancia/lectura fallida no se anuncia éxito.
Lecturas satisfactorias solo confirman valores en esos instantes, no una
transacción atómica ni su persistencia frente a cambios externos.

EPERM/EACCES -> permission_denied, ESRCH -> not_found,
ENOSYS/EOPNOTSUPP -> not_supported, EINVAL -> invalid_argument; resto system_error.
No hay procesos/threads ocultos, creación de archivos, ni cambios en el scheduler
del dispositivo, cgroups o prioridades del caller.

## Alcance y seguridad

Las syscalls PRIO_PROCESS/IOPRIO_WHO_PROCESS operan con PID/TID numérico y no
aceptan pidfd. Esta variante conserva riesgo de reutilización de PID antes o
entre syscalls. `set_efficiency_mode_checked` permanece not_supported en Linux:
no se presenta un simple stat-check seguido de setpriority(PID) como protección
equivalente a terminación pidfd. Un preset con identidad estable/referencias
persistentes necesita un diseño separado; no queda cerrado por esta unidad.

Se aplica al leader/TID indicado, no a todos los threads, nuevos threads ni
descendientes. Nice en Linux es por thread. Un contexto E/S CLONE_IO compartido
puede propagar efectos a otros threads/procesos que comparten ese contexto.
La efectividad de E/S depende del scheduler/dispositivo y tipo de operaciones;
aceptar/leer una prioridad no prueba throttling efectivo. No se cambia afinidad,
clocks, scheduling class CPU ni CPU quota. El API no tiene deadline global.

Desde revision41, `ProcessInfo::efficiency_mode_available` Linux publica false
con available para el camino genérico desactivado. La antigua heurística
propietario/root se retiró: no certificaba capabilities/RLIMIT_NICE/seccomp ni
scheduler. El bool false no declara inexistentes las syscalls de prioridad;
la opción avanzada se maneja por separado y nunca como fallback automático.
`efficiency_mode_requested` sigue reservado al proveedor EcoQoS Windows; no se
deduce una solicitud “de modo” a partir de prioridades Linux coincidentes.

IOPRIO usa los syscalls nativos y constantes del ABI (process=1, idle=3,
best-effort=2, shift=13). Headers/kernel sin esos syscalls devuelven not_supported
antes de cambiar CPU. Prioridades con flags/hints desconocidos fallan cerradas;
no se reinterpretan ni se eliminan a ciegas. No se agregan dependencias externas.

## Verificación histórica del preset legacy

2026-10-14: Linux Release 18/18, Windows Release 29/29; paquetes instalados y
consumidores externos reconstruidos/ejecutados. En la prueba nativa sin elevar,
disable devolvió permission_denied (errno 13) con E/S modificada y CPU aún nice 10:
el estado parcial se comprobó leyendo ambas prioridades, no solo el mensaje.

Pruebas inyectadas: nice -1 válido, nice 19 conservado, idempotencia, preset false,
permisos/ausencia/formato inválido, fallo E/S sin CPU, fallo CPU con E/S parcial,
verificación denegada/discrepante y API incompleta. Se comprueba el orden de writes.
Prueba nativa Linux solo sobre dos hijos temporales: nice 0->10 + idle, repetición,
desactivación con éxito o permiso denegado explícito y nice 19 sin promoción.
Todos los hijos se recolectan; nice/E/S del caller se comprueban sin modificaciones.
AuditedActions conserva las dos etapas y el resultado. Windows solo ejecuta la
lógica inyectada portable, no se afirma cobertura Linux nativa por ese build.

## Revisión41 y migración del consumidor

Aceptar el alcance no activa ninguna operación ni concede privilegios. El GUI
debe mantener desactivada por defecto la opción avanzada, mostrar la advertencia
de PID/TID reutilizable y obtener reconocimiento antes de pasar true. Guardar
siempre el recibo devuelto, también en errores; restauración requiere otra acción
explícita y puede fallar parcialmente. No usar set_efficiency_mode(pid,false)
para restaurar: desde esta revisión está desactivada y antes imponía defaults.
No modificar una subclase de SystemActions para eludir esta política del backend.

Cambio de comportamiento0.x compatible en firma/layout/vtable pero no en
efectos del método genérico; recompilar/relinkar y probar clientes. No convertir
el error not_supported en opt-in silencioso. La variante checked sigue no
disponible. Sin garantía de ahorro, cobertura de nuevos threads/descendientes,
identidad estable ni restauración sin permisos. Sin helper/elevación/ptrace/
cgroup nuevos. Evidencia actual en work/efficiency-optin-*.log.
Regresión final: Linux108/108,Windows122/122 Release,20repeticiones Linux y
ambos consumidores instalados recompilados/relinkados/ejecutados exit0.
Builds finales sin warnings; no procesos ajenos ni prioridades del caller
modificados y ningún hijo privado pendiente de recolectar.

Referencias: [getpriority/setpriority](https://man7.org/linux/man-pages/man2/setpriority.2.html),
[ioprio_get/set](https://man7.org/linux/man-pages/man2/ioprio_set.2.html).
