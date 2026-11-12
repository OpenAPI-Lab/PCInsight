# Historial de red observado por ejecutable Linux

Revision100, 2026-10-24. H04 conecta la fuente opcional F03 con
`SystemSnapshot::linux_network_app_history` en `Monitor` y `Sampler`.
El informe usa `Metric<vector<LinuxAppHistoryEntry>>`, con `network_bytes`
acumulados y CPU/notificaciones AUSENTES. No rellena enteros legacy.
No ejecuta aplicaciones de telemetria, instala paquetes ni guarda archivos.

## Uso y periodos independientes

Requiere AMBAS opciones `include_app_history` e
`include_linux_process_network`, mas una captura F03 previamente iniciada con
consentimiento explicito. Usar modo `observed_namespace_buckets` en seleccion
manual, o la ruta de admision visible. Un flag nunca autoriza ni inicia BPF.
Los modos strict/declared no producen este historial; su ausencia es explicita.
Privilegios/kernel/cobertura y limites de F03: LINUX_NETWORK_ACCEPTANCE.md.

```cpp
#include <pcinsight/monitor.hpp>
#include <pcinsight/app_history.hpp>
#include <pcinsight/linux_history_store.hpp>

pcinsight::Monitor monitor;
// Antes: la aplicacion pide consentimiento y configura seleccion/admisiones.
// monitor.refresh_linux_visible_process_network(consentimiento);
pcinsight::SampleOptions options;
options.include_app_history = true;
options.include_linux_process_network = true;
auto snapshot = monitor.sample(options); // llamada periodica del consumidor
auto view = pcinsight::app_history_view(snapshot.linux_network_app_history);
if (pcinsight::metric_is_known(snapshot.linux_network_app_history)) {
    // Mostrar bytes/Support/notas de cada fila, no convertir ausencia a0.
    // Persistir explicitamente con load/merge/save bajo bloqueo propio;
    // ejemplo completo y manejo de errores en LINUX_HISTORY_STORE.md.
}
```

`linux_app_history` sigue siendo CPU observado; `app_history_view(snapshot)`
selecciona ese informe CPU, NO mezcla los dos. Pasar el informe de red
explicitamente a la sobrecarga. El GUI puede mostrar ambas procedencias junto
a la misma ruta, pero no inventar un periodo compartido. Guardar CPU y red en
archivos distintos. No usar `merge` entre ellos por coincidir ruta/inicio:
periodos independientes pueden colisionar al normalizar a milisegundos.

Las filas agrupan por ruta absoluta exacta del ejecutable, entre procesos y
usuarios seleccionados. Ruta no significa paquete, nombre comercial, publisher
ni identidad permanente del binario. Una sustitucion en la misma ruta conserva
la fila historica pero rompe el intervalo. No se afirma causalidad/exclusividad
de sockets compartidos. No se incluyen bytes de procesos no seleccionados.

## Identidad y ventanas

Cada llamada realiza **un solo sweep F03**. Se reutiliza su metadata anterior
para leer ejecutables antes del sweep, y la metadata nueva para leerlos despues.
Cada lectura nativa verifica boot/observador/procfs/namespaces y PID/start/UID/
inode con un pidfd propio antes y despues de leer el ejecutable regular fijado.
Se compara ruta y stamp completo (device/inode/size/mode/mtime/ctime).
No se usa la lista CPU leida antes como si delimitara el sweep de red.

Primera muestra: registra actores, sin fila atribuida. Segunda estable:
baseline por ejecutable, bytes aun desconocidos. Tercera estable: primer delta
verificado. Actores nuevos o recuperados necesitan calentamiento nuevamente.
Solo se resta el contador acumulativo EXACTO de cada bucket/lane presente en
ambos extremos verificados; luego se suman TCP send/receive y UDP send/receive.
No se integran tasas ni se suman los acumulativos de cada snapshot. Send+receive
de loopback cuentan ambos; bytes de payload no equivalen a bytes wire ni a
todo el trafico de la aplicacion (async/kernel/offload y otros caminos excluidos).

Un bucket nuevo no aporta sus bytes iniciales retroactivamente. Un actor idle
puede demostrar cero para el subconjunto instrumentado tras un intervalo
verificado, sin inventar cookie0 ni un bucket ficticio. Un snapshot repetido
no suma bytes y rompe la continuidad; relojes invertidos, overflow, descenso de
contadores o buckets incoherentes rechazan la transaccion completa.

Cambio de epoch/boot/productor/observador, PID reutilizado, cambio de ejecutable,
metadata denegada o perdida de captura eliminan baselines, no totales confirmados.
No se recupera el trafico del hueco. Fallo de un ejecutable afecta su atribucion,
no inicia captura alternativa ni eleva permisos. El frame F03 original sigue
disponible con su propia cobertura. Cambiar seleccion requiere consentimiento
F03 normal; incluso actores retenidos pierden continuidad del historial.

Los chequeos son de extremos, no una traza de todos los exec: un exec y regreso
al mismo archivo (ABA) entre lecturas puede no detectarse. Por eso el informe y
los bytes son `estimated`, no una garantia exacta de aplicacion durante todo el
intervalo. No certifica todas las distros/kernels/drivers ni trafico universal.

## Memoria, apagado y persistencia

Maximos F03:1024 actores,16 buckets por actor/16384 total. Historial:4096 rutas,
65536 bytes por ruta,4MiB de identidades retenidas. Operacion transaccional:
un error global deja totales anteriores intactos y descarta continuidad; no hay
commit parcial ni wrap de uint64. Copias de trabajo acotadas, sin deadline duro.

Desactivar `include_app_history` borra este historial y la CPU observada.
Detener/desactivar solo la captura de red conserva los totales ya confirmados,
pero elimina baselines. Un `Monitor` nuevo inicia otro periodo; no se restauran
pidfds, permisos ni baselines desde un archivo. Serializar llamadas a Monitor;
Sampler usa su serializacion existente y conserva informes en su ventana.

`LinuxHistoryStore` conserva las filas, presencia/Support/notas y periodo.
Repetir una fila acumulativa REEMPLAZA, no suma; descensos/perdidas de valores
conocidos se rechazan. Guarda solo filas, no estado/nota exteriores ni metadata
F03. El consumidor decide archivo, bloqueo, frecuencia y retencion. Ver
LINUX_HISTORY_STORE.md para formato, limites, privacidad y errores.

Cambio de layout publico0.x: recompilar biblioteca y TODOS los consumidores,
incluido el GUI aunque aun no tenga CMake. No es una nueva dependencia runtime.

## Evidencia

`tests/linux_network_app_history.cpp`: warmup, deltas,50replays persistencia,
replay de frame, exec/stamp/denegacion/cambio durante sweep, PID/UID/start/inode/
boot/observer/productor, cambios de epoch, cookie nuevo, idle0, multiproceso,
overflow intervalo/acumulado, contador decreciente, buckets/flags/joins invalidos,
rollback, opt-out y gaps. Release conserva asserts.

`tests/linux_network_admission_native.cpp --authorized-private-admission`:
dos hijos UDP privados, fuente procfs/pidfds/BPF real y mismas rutas Monitor/
Sampler, seleccion solo de los hijos. Red por ejecutable258->516 bytes,
50replays conservan258, load/merge/save/load real conserva516. CPU/notificaciones
no promovidas. Temporales privados y sondas retirados; censo FD restaura baseline,
hijos salen normalmente. No captura global ni nuevo permiso sobre /proc completo.
Logs revision100 `work/linux-network-history-*`; resultado final en SEGUIMIENTO.txt.
