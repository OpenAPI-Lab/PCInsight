# L05 — seguimiento asíncrono y acotado de jobs

Revision154, 2026-11-02. Unidad interna terminada, sin servicios reales. L05 sigue
E por límites del receptor, despliegue/proceso/cargador y aceptación polkit reales.
Ejecución continúa OFF por defecto, sin helper instalado/registrado/activado.

## Resultado separado del envío

PrivilegedExecutionResult conserva job_queued como hecho de recepción del job y
añade job, un objeto de datos compartido PrivilegedJobObservation. snapshot() es
thread-safe y NO usa sd-bus, no ejecuta acciones ni hace polling de servicios.
Solo informa de lo observado por el bucle de eventos del propietario de conexión.

Estados: pending, completed (JobRemoved done), failed (canceled/timeout/failed/
dependency/skipped) y unknown. succeeded es true para done, false para esos
resultados no exitosos y desconocido ante ambigüedad. El result bruto reconocido
o futuro se conserva separado; un valor futuro no se interpreta como fallo ni
éxito. Job completed NO significa servicio actualmente running; canceled/timeout
NO prueban ausencia de efectos ni cancelación efectiva de la operación.

## Suscripción y correlación

Antes del dispatch se instala un match JobRemoved del OWNER ÚNICO verificado,
path e interfaz exactos, y Subscribe al mismo owner si la conexión aún no tiene
esa suscripción. Subscribe autostart0/interactivemsg0, reply tipo/sender/firma
vacía/cookies correlacionados. Fallo => no se envía Start/Stop/Restart. Una única
suscripción exitosa se reutiliza por conexión/owner; al cambiar owner se retiran
observaciones pendientes antiguas y se subscribe al nuevo. No se usa un bus por
defecto. El cierre de la conexión elimina la suscripción del cliente, sin RPC
Unsubscribe desde destructores. La información de job no es un grant.

Callback comprueba bus, sender, tipo4/firma uoss, cuerpo completo y límites.
Exige path canónico uint32 no-cero y coincidencia de ID, path y unidad esperados.
El recibo del ejecutor ahora exige también ese path canónico (sin cero, leading
zero u overflow). Señales anteriores a seleccionar el recibo se guardan hasta64;
después se ignoran otros paths sin penalizar un sistema ocupado. Hasta4096 eventos
para el path observado; duplicados idénticos son idempotentes, conflictos y unidad
distinta en el mismo path => unknown. Alias de unidad sin resolver pueden dar
unknown: no se inventa equivalencia con el nombre primario de systemd.

La observación se prepara antes del envío; después de Subscribe se revalida otra
vez Manager/caller/política/conexión y el deadline antes del último checkpoint.
No se espera un job haciendo sd_bus_process dentro del handler. El host debe
seguir procesando el bus de forma serializada después de devolver el recibo;
sin bucle de eventos no hay nuevas confirmaciones.

## Tiempo, memoria y propiedad

Timeout de observación configurado por el mecanismo:1ms..30s, predeterminado30s,
con reloj monotónico. No es deadline agregado de autorización/handshake/IO. Al
vencer pasa a unknown, sin CancelJob, rollback ni retry. Señales tardías no
reviven una observación desconocida. Si vence antes del dispatch, no se envía la
operación. snapshot evalúa vencimiento; no se crea un thread/timer de fondo.

Slots nativos y sus callbacks son propiedad de la conexión (uso y cierre
serializados), NO del objeto de datos entregado al consumidor. Retirar/copiar el
resultado desde otro hilo no destruye un slot ni toca el bus. Máximo64 watches
pendientes por conexión; al preparar otra operación se retiran los terminados o
vencidos. Ese límite64 de watches está implementado pero no se estresó contra el
broker en esta unidad; el buffer temprano64 sí tiene pruebas de frontera.

Destructores del propietario retiran matches antes de cerrar bus/biblioteca.
Cierre/revocación detectada => pendientes unknown, sin cancelar la operación;
las finalizaciones históricas confirmadas se conservan en las copias de datos.
No hay ciclo de referencia con la conexión. Signal callback captura errores y
no propaga excepciones al ABI C. Lectura/eventos de estado usan mutex. El objeto
interno sigue siendo contrato C++ confiable, no frontera contra código malicioso
en el mismo proceso ni API instalada del SDK.

## Evidencia154

tests/linux_privileged_jobs.cpp: señales antes del recibo, done, cinco resultados
negativos, resultados futuros, duplicados/conflictos, otros5000 jobs ignorados,
unidad incorrecta, ID/path/rango uint32/leadingzero/overflow, deadline y evento
tardío, buffer64/65, cierre pendiente y conservación histórica.

tests/linux_privileged_receiver.cpp: broker/libsystemd REALES privados, Authority
y Manager SIMULADOS; ahora50 solicitudes al receptor,29 checks de Authority,
16 intentos modelados de control y2 Subscribe (un rechazo +uno exitoso reutilizado).
El Manager emite JobRemoved antes del recibo y usa IDs distintos por operación.
Prueba subscribe fallido sin dispatch, done/failed, alias inesperado, resultado
futuro, duplicado, contradicción, firma errónea, ausencia de señal/timeout y
señal forjada del cliente que no confirma nada. La revocación del ancla convierte
el último pendiente en unknown sin borrar un completed histórico. FD/cleanup
correctos. Ningún servicio real ni bus/polkit del host fue controlado.

Linux182/182 +120 ejecuciones L05 (12tests10 veces,10 brokers), MSVC20 FSM portable/
argumentos/fallback y2 consumidores SDK reinstalados/relinkados exit0. Builds
inicial/full/verificado/final limpios y suitefinal182. work/privileged-jobs-*.
Sin API/layouts/formatos públicos, cambios Monitor/SystemActions o CI remota.

Actualización155: [presupuesto compartido](LINUX_PRIVILEGED_BUDGET.md) implementado
para las fases RPC del handler; cooperativo, NO deadline absoluto del proceso/
transporte. El timeout de observación de jobs154 sigue siendo independiente.

Plan histórico154: límites/deadline agregado del receptor. Despliegue/cargador/política y
consentimiento polkit reales pendientes; sin instalar/activar/elevar ni tocar
servicios reales sin autorización específica. OpenRC148 solo control explícito
intacto, SysV/K03/I07/UAC conservados. C60/E10/P3/B2+3X excluidas:80%/20%.

Fuentes primarias: [Manager y JobRemoved](https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.systemd1.html)
y [matches sd-bus](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_match_signal.html).
Los estados, límites, ownership y política conservadora son contratos PCInsight.
