# K03: selección y control OpenRC públicos

Revisión159, 2026-11-02. Unidad de integración pública, no aceptación OpenRC real.
Encabezado instalado `pcinsight/linux_openrc_control.hpp`; no modifica layouts
públicos existentes, Monitor, SystemActions ni los lectores de datos. La excepción
de control148 sigue opt-in, sin herramientas de recopilación ni elevación.

## Uso desde el GUI del consumidor

```cpp
#include <pcinsight/linux_openrc_control.hpp>

pcinsight::LinuxOpenrcControlConfig config;
config.enabled = usuario_habilito_control_openrc; // false por defecto
config.runtime_path = ruta_absoluta_confiable_del_runtime;
config.init_directory = directorio_absoluto_confiable_init_d;
// owner_uid=0 y allow_sticky_ancestors=false por defecto.

auto seleccion = pcinsight::LinuxOpenrcService::open(config, "mi-servicio");
if (seleccion.value && usuario_confirmo_restart) {
    auto solicitud = (*seleccion.value)->request(
        pcinsight::ServiceAction::restart, true);
    // Conservar INCLUSO si solicitud.result indica un error incierto.
    if (solicitud.runner) pendientes.push_back(solicitud.runner);
}
```

`pendientes` pertenece al consumidor. Desde su timer/worker, `runner->wait(0)`
consulta sin espera de polling; los syscalls siguen sin hard deadline. Cuando
el Metric tiene valor y exited=true, mostrar exit_code o signal y retirar ese
handle ya recolectado. Un error de observación requiere diagnóstico, no lanzar
otra solicitud automáticamente. No convertir exit0 en «servicio running»;
el modelo de pruebas devuelve3/7/11 precisamente para comprobar esa separación.
La interfaz gráfica y su build pertenecen al consumidor; no se requiere crear
ahora el CMake de su GUI. El pequeño proyecto consumer incluido es solo prueba
del SDK instalado, no un requisito de integración de la aplicación.

## Selección, consentimiento y caducidad

`open` es solo metadatos/FD: nunca lanza runtime, script, rc-service ni shell.
Config es confiable y se copia; modificarla después no cambia/revoca selecciones
existentes. init_directory debe ser absoluto/no vacío y name un basename literal
ASCII acotado: sin slash, NUL, opciones, espacios/metacaracteres ni dot/dotdot.
No se convierte un índice de inventario ni una marca started en autorización.

La selección conserva FD de runtime/script y guardas de sus directorios, no
solo strings. Cada request exige consentimiento independiente, cerrado a tres
verbos. Falta de consentimiento y verbos inválidos no consumen la selección.
Antes de liberar un hijo se revalidan los archivos ORIGINALES, rutas/metadata y
contexto. Reemplazo, modo/metadata cambiado, directorio o proceso/namespace/UID
distinto => stale_target, nada despachado y selección revocada permanentemente.
Restaurar el nombre no la revive. No se reabre silenciosamente otro objetivo.

Al devolver un hijo, la selección queda consumida incluso ante ACK incierto;
no permite un segundo intento. Para otra acción el GUI debe abrir una selección
nueva tras nueva intención explícita; ello no resuelve por sí solo una acción
previa de resultado desconocido. `close()` revoca futuras solicitudes, sin
cancelar/recolectar el runner, ni terminar el servicio. Los métodos de selección
se serializan; el host mantiene las restricciones de fork/FD/contexto/reapers
descritas en [el ejecutor](LINUX_OPENRC_CONTROL.md).
close conserva los pins hasta destruir la selección; libera ese objeto si ya
no necesitas su nombre/histórico. UID/GID reales y efectivos se fijan/revalidan;
grupos suplementarios y cambios de credenciales siguen sujetos a serialización
por el host, no se promete snapshot de todos sus privilegios.

## Resultado y recursos

LinuxOpenrcControlResult conserva ActionResult, runtime_may_have_executed y runner
opcional. Mensaje/booleano de dispatch no son prueba de ejecución ni estado vivo.
El runner posee el pidfd; process_id es diagnóstico, no permiso para señales
por PID. wait(0..30000ms) retiene resultado histórico y reap por P_PIDFD, sin
fallback de PID ni reaper oculto. El consumidor debe conservar cada runner hasta
recolectarlo. Destruir la selección no lo cancela. Abandonar un runner vivo puede
dejar zombie. terminate_runner exige confirmación y solo señala al runner; NO
equivale a ServiceAction::stop ni toca grupo/árbol/dependencias.

Se reservan envoltura pública, resultado y mensajes de ACK antes del dispatch;
las transferencias posteriores del handle/mensajes usan movimientos sin asignar.
Así un fallo al reservar esos objetos no abandona un hijo liberado. Se conservan límites,
entorno mínimo, cierre de FD, NO_NEW_PRIVS y presupuesto ACK2s del ejecutor158.
No hay auto-detección, shell fallback, instalación, privilegios nuevos ni conexión
al bus del host. En plataformas sin backend el resultado es unsupported, no
cero inventado. Existencia de selección no garantiza soporte del kernel/runtime.

FD no congela contenido y OpenRC reabre el script por nombre: no snapshot atómico,
garantía ABA ni defensa contra owner/root hostil. Callbacks pthread_atfork y base
confiable del runtime/scripts/config/dependencias siguen bajo control del host.
Es una API experimental hasta aceptación de versiones OpenRC reales.

## Verificación de la unidad

`tests/linux_openrc_control_public.cpp` importa SOLO encabezados públicos. Usa el
ELF modelo privado158, nunca OpenRC real, y cubre default-off/nombres inválidos,
consentimiento, tres verbos, configuración copiada, cambio de script/runtime/modo/
directorio, revocación persistente/close, selección consumida, contexto tras fork,
propiedad del runner, salida/timeout/terminación explícita y FD/limpieza.
`tests/openrc_consumer` compila ese mismo programa contra el SDK instalado sin
include privado. Asserts Release, alarma20s y CTest30s permanecen activos.

Pendiente K03: aceptación aislada OpenRC/runit/s6 reales y decisión de control
SysV (no autorizado por excepción OpenRC). No se repite la misma integración
como pendiente ni se confunde el modelo con aceptación física. L05 despliegue
aplazado, I07 observador y Windows aplazado se conservan.

Evidencia final159: Linux188/188, cinco tests OpenRC repetidos diez veces (50
ejecuciones); MSVC20 recorridos portables, tres consumidores SDK reinstalados/
relinkados exit0, incluido el programa nuevo sin includes privados. FD/limpieza
correctos; builds finales sin warnings de código. D9025 inicial MSVC por
/UNDEBUG frente a /DNDEBUG es esperado para mantener asserts Release activos.
Logs `work/openrc-public-*`, especialmente *-delivery.log, build-delivery-clean.log
y los recorridos focales finales de entrada pública.
Sondeo read-only PATH: command -v runsv s6-supervise openrc-run rc-service,
exit1/salida vacía; no demuestra ausencia global y no se ejecutó/instaló ninguno.
Consulta de permiso SysV emitida y pendiente, no autorización implícita.
No CI remota ni aceptación física. K03 E, C60/E10/P3/B2+3X excluidas:80%/20%.
