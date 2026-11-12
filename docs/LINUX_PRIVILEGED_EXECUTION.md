# L05 — ejecución cerrada, opt-in y sin reintentos

Revision153, 2026-11-02. Flujo interno implementado y probado con gestor simulado.
OFF por defecto, no instalado/registrado, no conectado a SystemActions o Monitor.
L05 sigue E: no aceptación real de polkit/root/helper ni despliegue endurecido.

## Dos métodos distintos

El decoder cerrado se comparte sin ampliar el payload: path /org/pcinsight/Helper,
interfaz org.pcinsight.Helper1, firma uusb, versión1/op1..3/unit/intent, sin campos
de identidad/UID/actionID/comando. AuthorizeSystemService sigue solo observando:
NUNCA llama al gestor, aunque esté habilitado el switch de ejecución. El nuevo
execute_owned_privileged_request solo admite ControlSystemService del dispatch
actual. No acepta un reporte previo como grant ni reutiliza autorización guardada.

PrivilegedExecutionGate es configuración INTERNA del mecanismo, no payload:
enabled=false, permit_interaction=false y Manager UID sin configurar por defecto.
Atomic flag rechaza ejecución solapada/reentrante; no sustituye la serialización
del bus ni hace seguro usar/cerrar el mismo objeto desde varios hilos a la vez.
El guard se libera también ante error. No hay shell, argv, entorno ni método
arbitrario del cliente; únicamente StartUnit, StopUnit y RestartUnit.

## Flujo por solicitud

1. Switch/UID/reentrancia y decoder/preflight con copia de política/rules actuales.
2. Conexión propia y contexto de identidad nativo exactos, con generación de política.
3. GetNameOwner del Manager y UID desde el broker; nombre único distinto del
   solicitante y Authority, UID configurado. No activación/búsqueda alternativa.
4. Autorización NUEVA con subject de transporte y solicitud congelada. Coordinador
   aplica rechazo estricto; callbacks observan contexto/gate/política/conexión.
5. Reobservación del Manager/caller y comprobaciones de vigencia antes del envío.
6. Mensaje al OWNER ÚNICO ya verificado, método cerrado según operación, unidad
   literal y modo fijo replace; autostart0 y flag interactivo del mensaje0.
7. Último chequeo de switch/config/política/conexión, UN sd_bus_call de hasta5s.
8. Reply tipo/sender/firma o/cookie correlacionados, cuerpo completo y job path
   numérico bajo /org/freedesktop/systemd1/job/, hasta1024 bytes. Revalida contexto.

Un job receipt correlacionado significa job_queued, NO servicio arrancado/parado,
éxito final o job completado. No se usa el resultado de observación como ticket
reutilizable ni se afirma consentimiento humano por el bit de intención.

## Resultados honestos y frontera de concurrencia

Antes de dispatch_attempted, un rechazo/fallo implica que este flujo no intentó
el envío al Manager. Desde justo antes de invocar sd_bus_call, cualquier error,
respuesta inválida o cambio de contexto se informa dispatch_unknown: la operación
PUDO ejecutarse. No se asegura ausencia de efectos, no se reintenta ni se hace
rollback/cancelación automática. El GUI deberá tratar la incertidumbre como tal,
no como fallo seguro que autorice un retry invisible.

El chequeo final NO es una transacción atómica distribuida con polkit, cambios
de política, desconexión del cliente o systemd. Un cambio posterior a esa frontera
no cancela una operación en vuelo. Los postchequeos pueden detectar cambios pero
NO deshacerlos. Revocación/invalidation inmediata/atómica, hard deadline agregado,
rate/queue limiting, cierre operativo de jobs y endurecimiento del proceso siguen
pendientes antes de afirmar seguridad de un helper desplegado. Tampoco hay
aislamiento de dependencias/Conflicts de una unidad: la allowlist restringe el
target literal, no todos los efectos de un job systemd configurado por el admin.

Implementación privada en src/linux_privileged_identity.cpp reutiliza el ABI y
consultas ya validados; contrato en src/linux_privileged_execution.hpp. No se
exportan headers SDK, cambian layouts/formatos ni habilitan rutas existentes.

## Evidencia153

Actualización154: [seguimiento asíncrono](LINUX_PRIVILEGED_JOBS.md) añade match y
Subscribe antes de dispatch, recibo uint32 canónico, datos consultables sin tocar
bus desde otro hilo y deadline/limpieza acotados. job_queued sigue siendo recibo,
no finalización: el objeto job da el resultado observado. Evidencia153 histórica.

tests/linux_privileged_execution.cpp comprueba defaults de mecanismo/decoder y
fallback portable. tests/linux_privileged_receiver.cpp amplía el broker privado
REAL/libsystemd REAL: cuarta conexión y thread con Manager SIMULADO, además de
Authority SIMULADA. Ningún servicio real se lanza/detiene/reinicia: el Manager
solo verifica método/unit/mode, incrementa contadores y devuelve respuestas.

Cada recorrido:41 solicitudes al receptor,20 checks de Authority,8 llamadas al
Manager simulado. Casos nuevos: OFF defecto, UID no configurado, busy, op inválida,
unidad fuera de lista, falta de intención, campo UID extra, Manager UID erróneo,
denegación sin dispatch, start/stop/restart encolados, endpoint auth-only sin
dispatch aun enabled, invalidación durante auth sin acción, job path inválido,
firma incorrecta, política invalidada desde Manager tras la aceptación simulada,
error D-Bus tras aceptación simulada y recuperación final. Todos los casos de
respuesta desconocida tienen un solo incremento Manager y dispatch_attempted,
sin receipt ni retry. Conserva revocación del ancla152 y FD/cleanup privados.

Linux181/181 y110 ejecuciones L05 (11tests10 veces,10brokers), MSVC20 defaults/
argumentos/fallback, dos consumidores SDK reinstalados/relinkados exit0. Clockskew
inicial del build completo preservado; incremental final limpio y suite final181
sin tocar reloj/mtime. work/privileged-execution-*. No polkitd/busdelhost/dialogos/
elevación/serviciosreales/instalación/activación/aceptación/CI remota.

Siguiente unidad: límites del receptor y seguimiento de jobs sin confundir envío
con finalización; despliegue/cargador/polkit reales pendientes. No instalar/activar/
elevar ni actuar sobre servicios reales sin autorización específica. OpenRC148
solo control explícito intacto; SysV/K03/I07/UAC conservados.
Conteo oficial C60/E10/P3/B2+3X excluidas:60/75=80%,15/75=20% abiertas.

Fuentes primarias: [Manager systemd](https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.systemd1.html)
y [sd_bus_call](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_call.html).
El gate, protocolo del helper y política conservadora de incertidumbre son
contratos PCInsight, no una réplica de toda política de systemd/polkit.
