# L05 — presupuesto compartido de RPC y plazo acumulado

Revision155, 2026-11-02. Unidad interna terminada. Sigue siendo un plazo
COOPERATIVO con RPC acotadas, no preempción/realtime ni cancelación remota.
L05 sigue E; no helper instalado/activado, acciones públicas o servicios reales.

## Un presupuesto por solicitud

PrivilegedRequestBudget utiliza steady_clock, contador y mutex. Configuración
solo del mecanismo: timeout1ms..30s y1..128 operaciones RPC instrumentadas;
predeterminado5s/96. Configuración inválida, reloj no disponible/retroceso o
agotamiento rechazan nuevas llamadas. Nunca entrega timeout0, que sd-bus podría
interpretar como uso del timeout predeterminado. Cada claim deduce una operación
y entrega min(cap por llamada,5s,tiempo restante), sin extender el deadline.
El contador es diagnóstico, NO grant/ticket ni permiso reutilizable.

ControlSystemService captura la configuración y comparte la MISMA instancia a
través de consultas daemon/identidad, CheckAuthorization, Manager, AddMatch,
Subscribe y dispatch. Cambiar el gate/configuración en vuelo invalida el flujo.
AuthorizeSystemService a través de la conexión propia usa también un presupuesto
por solicitud5s/96. Adaptadores internos legacy/raw sin presupuesto explícito
conservan su contrato anterior; no se presentan como receptor endurecido.

El presupuesto empieza dentro del handler, no al aceptar la conexión ni recibir
el primer byte. Consume el tiempo transcurrido entre checkpoints, incluido trabajo
local; las consultas pueden seguir solo si queda tiempo. No es deadline de
handshake, carga inicial de configuración o transporte previo al handler.

## Aplicación al ABI nativo

Consultas que usan sd_bus_call_method y el AddMatch interno reciben temporalmente
un timeout predeterminado <=tiempo restante, mediante lease RAII. Se restaura
el timeout original en salida normal y por excepción. Uso de conexión siempre
serializado. No se cae a un timeout sin límite si faltan setter/getter. En llamadas
sd_bus_call explícitas se pasa directamente el tiempo restante. No hay retry.

La cuota cuenta puntos RPC instrumentados, incluyendo conservadoramente el
registro de match como uno. NO cuenta cada mensaje interno posible de libsystemd,
replies del receptor, RemoveMatch de limpieza o tráfico asíncrono de jobs.
Tampoco es rate limit global/por UID ni límite de cola/bytes/CPU de todo el proceso.

Sin presupuesto antes de dispatch => no intento de envío. Si vence tras el
intento => dispatch_unknown, aunque el Manager posteriormente responda positivo.
La operación PUDO ejecutarse; no rollback ni CancelJob ni cancelación de un agente
polkit garantizados. Una nueva solicitud recibe presupuesto NUEVO; no hereda uno
agotado ni reutiliza autorización anterior. Deadline de observación del job154
permanece separado y puede continuar después de terminar el handler.

## Límites de la garantía

No puede interrumpir dlopen/constructores, filesystem/procfs bloqueados, código C++
o scheduling del kernel. Una pausa después de calcular el timeout puede retrasar
la invocación; los chequeos posteriores conservan incertidumbre, no deshacen el
envío. Cleanup/retirada de slots y configuración del proceso no están convertidos
en transacciones temporales. Por eso NO se afirma un máximo absoluto de tiempo
real para todo el receptor ni una frontera atómica de revocación. Rate/admisión,
colas/tamaño antes de decodificar y despliegue/proceso/cargador siguen pendientes.

## Evidencia155

tests/linux_privileged_budget.cpp usa reloj controlado: reducción del restante,
cap por llamada, límite de conteo/128, bounds inválidos, vencimiento, retroceso,
excepción de reloj, lease restaurado tras éxito/excepción y setter fallido.
No modifica el reloj del sistema.

tests/linux_privileged_receiver.cpp conserva154 y añade seis solicitudes:
timeout0/rpc0 inválidos, rpc1 agotado antes de Authority, Authority con respuesta
retenida y200ms de presupuesto sin dispatch, Manager con respuesta retenida tras
aceptación simulada que produce dispatch_unknown y recuperación fresca. Las
retenciones tienen guard1s: el receptor debe devolver ANTES de liberar cada fake,
no basta con descartar tarde el resultado. Comprueba timeout nativo restaurado
exactamente a2s en el hilo propietario al terminar CADA solicitud. El Manager
cuenta un único intento en el caso incierto; no hay retry.

Por recorrido56 solicitudes,32checks Authority,18Manager y2Subscribe. Broker/
libsystemd REALES PRIVADOS, Authority y Manager SIMULADOS, ningún servicio real.
Linux183/183 +130 ejecuciones L05 (13tests10 veces,10brokers), MSVC20 presupuesto/
args/fallback y2 consumidores SDK reinstalados/relinkados exit0. FD/cleanup
privados correctos. Un aviso clockskew inicial preservado; incremental final
limpio y suitefinal183, sin tocar reloj/mtime. work/privileged-budget-*.
No API/layouts/formatos públicos, Monitor/SystemActions, polkit real, instalación,
activación, elevación, aceptación o CI remota nuevos.

Actualización156: [admisión por receptor/remitente y presión de colas](LINUX_PRIVILEGED_ADMISSION.md)
implementadas antes de RPCs; no son límites de bytes/pre-recepción. Evidencia155
anterior histórica; despliegue y transporte previos al handler siguen pendientes.

Plan histórico155: admisión y límites de transporte/cola del receptor antes de despliegue
real. Política/consentimiento polkit y proceso/cargador protegidos pendientes;
no instalar/activar/elevar ni controlar servicios reales sin autorización
específica. OpenRC148 solo control explícito intacto, SysV/K03/I07/UAC conservados.
Conteo oficial C60/E10/P3/B2+3X excluidas:60/75=80%,15/75=20% abiertas.

Fuentes primarias: [timeouts sd-bus](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_set_method_call_timeout.html)
y [sd_bus_call](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_call.html).
La cuota, reloj inyectable de prueba y política conservadora son contratos PCInsight.
