# N03: App history Linux para el GUI

Revisión 162, 2026-11-06. Traza R14 a H01/H02/H04/H05 y la acción L01.
CPU, red y notificaciones son tres informes independientes. Una etiqueta,
ruta o fecha coincidente no prueba que representen la misma app o periodo.
La biblioteca no inventa contadores para llenar todas las columnas de una fila.

| Dato pedido | API y unidad | Alcance y verificación |
| --- | --- | --- |
| Name | `LinuxAppHistoryEntry.id/name` | CPU y red agrupan por ruta exacta observada del ejecutable; `name` es etiqueta, no paquete ni publisher. Una sustitución en la misma ruta puede interrumpir la atribución sin crear otra fila. [CPU](LINUX_APP_HISTORY.md), [red](LINUX_NETWORK_APP_HISTORY.md). |
| CPU time | `snapshot.linux_app_history` → `observed_cpu_time`, ns acumulados | Del intervalo válido de procesos visibles desde el inicio de observación. No porcentaje ni ventana gráfica de 60 s. Procesos breves, invisibles o anteriores pueden no contar. [Prueba](../tests/linux_app_history.cpp). |
| Network | `snapshot.linux_network_app_history` → `network_bytes`, B acumulados | Payload TCP/UDP instrumentado de procesos seleccionados y atribuibles; no throughput de interfaz, bytes en cable ni total de la app. CPU/notificaciones ausentes. [Prueba](../tests/linux_network_app_history.cpp). |
| Notifications | `LinuxNotificationObserver::poll/report` → `apps[].accepted_notify_calls`, llamadas | Respuestas exitosas de Notify observadas y correlacionadas con el servidor fijado. Reemplazos incluidos; `replacement_calls` es subconjunto, no sumar. No equivale a avisos mostrados, distintos o leídos. [Contrato](LINUX_NOTIFICATIONS.md), [prueba privada](../tests/linux_notifications_native.cpp). |
| Identidad de notificaciones | `sender_unique_name` y `declared_desktop_entry` | Conexión D-Bus en su observación y pista no autenticada del remitente, posiblemente ausente. Nunca unir con CPU/red por pista, texto de app o nombre de conexión de otra sesión. [Prueba de correlación](../tests/linux_notifications.cpp). |
| Run new task | `SystemActions::launch_process` | Acción explícita del usuario, no recolector ni autoriza captura. El lanzamiento no crea automáticamente un registro atribuible de historial. [Contrato](LINUX_LAUNCH.md). |

## Lectura y presentación

- CPU: conservar un `Monitor` y activar `include_app_history`. Ocultar `include_processes` no impide la lectura interna de procesos. Desactivar el historial borra su acumulación; no usar ese flag como pausa inocua. La primera observación puede no producir CPU conocido; dos observaciones válidas con delta cero sí pueden hacerlo.
- Red: requiere ambas opciones `include_app_history` e `include_linux_process_network` y captura F03 ya autorizada en modo observado (selección manual o admisión visible). Los flags no adjuntan BPF. El modo estricto/declarado no alimenta este informe. No pedir una muestra con el flag de red desactivado a un Monitor compartido si se pretende mantener su captura.
- Notificaciones: observador independiente y consentimiento informado. Construirlo no conecta; `start` explícito puede requerir libsystemd local y permisos del bus. Consumir `poll` desde el mismo hilo. Mostrar pendientes, expiradas, rechazadas, llamadas sin acuse y límites de sondeo como diagnóstico, no como aceptadas. No registrar títulos, cuerpos u otro contenido.
- Revisar `metric_is_known` tanto en el informe exterior como en cada contador. Ausencia no es cero, lista vacía conocida no es fallo y un informe con valor incoherente/denegado no autoriza mostrar sus filas. `app_history_view(snapshot)` selecciona el informe CPU nativo; para Linux red, pasar su Metric explícitamente. `AppHistoryOrigin::linux_observed` por sí solo **no distingue CPU de red**: conservar la procedencia junto a la vista. [API](../include/pcinsight/app_history.hpp), [pruebas](../tests/app_history_api.cpp).

Para el GUI, mantener secciones por fuente. Mostrar `period_start` y `period_end`
con cada periodo; no reconciliar automáticamente reloj, cobertura o identidad.
Si el informe CPU falta, `note` puede distinguir un retroceso del reloj de una
marca temporal inválida o un exceso de observaciones. Mostrarlo como diagnóstico,
no usar su texto como código estable ni interpretar el informe ausente como cero.
Un periodo observado no reconstruye actividad previa ni equivale a un total
vitalicio. Los índices del ejemplo son posiciones de salida, no claves estables.

## Guardado explícito, no captura

`LinuxHistoryStore` guarda filas observadas con valor, soporte y nota, no el
estado del informe exterior ni permisos, baselines o recursos de captura. CPU
y red requieren archivos separados. El merge usa `(id, period_start)` normalizado
a milisegundos: repetición del mismo periodo reemplaza, no suma; disminución o
pérdida de contador conocido en una actualización válida se rechaza. No llamar
`save` tras un fallo de carga ni convertir un informe ausente en lista vacía
para reemplazar el archivo. Serializar load/merge/save y retener periodos enteros.
No persiste `LinuxNotificationReport`. [Formato y límites](LINUX_HISTORY_STORE.md),
[pruebas](../tests/linux_history_store.cpp).

`HistoryStore` legacy conserva datos raw; `import_legacy_history` conserva solo
identidades/periodos como evidencia para presentación: **todos** los contadores
quedan desconocidos incluso con raw positivo. No migrar esos números a medición
ni mezclar formatos. [Contrato legacy](HISTORY.md), [pruebas](../tests/history_store.cpp).

## Ejemplo SDK

[linux_app_history.cpp](../examples/linux_app_history.cpp) solo usa encabezados
públicos y el formateador de los ejemplos. Sin argumentos muestra ayuda;
`--self-test` usa fixtures con ruta y periodo coincidentes, cero conocido,
importación legacy y reporte denegado; `--sample` hace dos lecturas Linux con
historial CPU. Puede observar ejecutables visibles aunque no muestre la tabla
de procesos. No inicia captura de red, conecta el observador D-Bus, guarda
historial ni ejecuta acciones. El [consumidor externo](../tests/app_history_consumer/CMakeLists.txt)
valida el SDK instalado; no impone CMake al GUI. La muestra es un smoke, no un
oráculo de totales ni nueva validación física de F03/H05.

Verificación: Linux Release 194/194, dos pruebas del ejemplo repetidas diez
veces, diez recorridos portables MSVC y consumidor del SDK instalado con
self-test y muestra exit0. 16 enlaces locales comprobados. Logs
`work/app-history-contract-*`. Sin cambios de biblioteca, ABI ni formatos;
N03 sigue abierto, no se da por resuelto ningún fallo histórico por este pase.
