# L05 — admisión por conexión y presión de colas

Revision156, 2026-11-02. Unidad interna implementada y probada. No configura el
broker/OS ni instala/activa helper. L05 sigue E; ejecución OFF por defecto.

## Alcance y orden

PrivilegedAdmission es un token bucket compartido por los dos endpoints de una
CONEXIÓN RECEPTORA, con otro bucket por remitente único asignado por D-Bus. No
es por UID, persona, proceso o sistema entero. Abrir otra conexión de cliente
cambia su key pero NO reinicia el bucket compartido del receptor. Abrir otro
receptor nuevo sí crea otro límite; la futura configuración de helper debe
controlar cuántos receptores existen. No hay una supuesta identidad de payload.

admit_request comprueba conexión propia, mensaje actual y tipo1, toma sender del
header del broker y lee contadores locales de cola, sin RPC de identidad/polkit.
Rechazo => no crea presupuesto155 ni hace preflight/identidad/Authority/Manager
del flujo solicitado. Sigue existiendo el coste local de validar la conexión,
metadatos y emitir la respuesta. Gates OFF/busy de control salen aún antes por
su ruta barata; no se consideran admitidos ni consumen cuotas.

## Configuración protegida e inmutable por receptor

En PrivilegedBusEndpoint, solo desde el mecanismo C++, nunca desde el mensaje:

| Parámetro | Defecto | Rango |
| --- | ---: | ---: |
| Burst compartido / recarga por segundo | 64 / 16 | 1..512 / 1..1024 |
| Burst por sender / recarga por segundo | 16 / 4 | 1..512 / 1..1024 |
| Senders almacenados | 64 | 1..256 |
| Expiración idle | 300s | 1..300s |
| Umbral cola read / write | 64 / 64 mensajes | 1..1024 |

Idle debe ser al menos el tiempo de recarga completa del bucket sender, para
que expirar/recrear una key no regale créditos anticipados. No se deshabilita
el guard con0: configuración inválida rechaza la conexión. Los perfiles de
regresión usan explícitamente burst256/rate512; NO cambian los defaults reales.

Créditos enteros en microtokens, sin floating point; reloj monotónico/mutex y
conservación de fracciones. Se carga primero el bucket compartido: rechazos por
sender o tabla llena consumen esa cuota también. Tabla llena rechaza keys nuevas,
sin expulsar activas. Solo entradas idle expiran; no se almacena una key en cada
rechazo cuando la cuota compartida está agotada. Reloj inválido/retroceso o fallo
de asignación deshabilita la instancia; no se restablece por petición del cliente.
No hay cola de solicitudes diferidas, retries, espera hasta recarga o fallback.

El guard no asegura equidad: clientes con muchas conexiones pueden evadir el
límite sender o llenar la tabla hasta su TTL; la cuota compartida y memoria
acotada siguen protegiendo ese receptor. No se promete inmunidad global a DoS.

## Qué significa presión de cola

sd_bus_get_n_queued_read/write observan mensajes ya pendientes en el objeto bus.
Si un contador supera el umbral, se rechaza trabajo antes de RPCs costosas. Error
o símbolos ausentes => rechazo, no suposición0. No se hace flush/drain/close de
forma implícita ni se cancelan operaciones anteriores.

NO es un límite duro de cola, bytes, mensaje, socket, broker o memoria antes de
la decodificación de sd-bus. Los contadores se leen DESPUÉS de recibir el mensaje;
un atacante podría consumir recursos de transporte antes del handler. El host
debe definir backpressure/cierre y respuestas acotadas bajo presión de escritura;
este guard no limita replies generados por otro código ni tráfico de señales.
La configuración de transporte/broker y sus límites previos a recepción quedan
pendientes; no se modifica la configuración del host para simular esa garantía.

## Evidencia156

tests/linux_privileged_admission.cpp: cuota compartida/sender, carga de rechazos,
recarga parcial/exacta e idle, tabla llena sin evicción activa, TTL seguro,
presión read/write inyectada y fronteras, sender inválido, configuración inválida,
retroceso submicrosegundo/fallo de reloj y32 threads con exactamente una admisión.

tests/linux_privileged_receiver.cpp añade modo --admission en un broker PRIVADO
REAL/libsystemd REAL, Authority y Manager SIMULADOS. Política burst compartido3/
sender2/rate1: autoriza una solicitud, ejecuta una operación MODELO y rechaza tres
adicionales. Control y auth-only comparten créditos; nuevo sender del segundo
cliente no evade cuota compartida. Rechazo de control sin budget/dispatch, auth
sin check, contadores congelados:5peticiones/2checks/1Manager/1Subscribe. Ningún
servicio real afectado. Lectura normal de contadores de cola nativa verificada;
no se provocó una inundación de colas reales en esta unidad.

El modo de regresión155 mantiene56peticiones/32checks/18Manager/2Subscribe con
perfil alto explícito; conserva deadlines, jobs, cleanup y señales falsas.
Linux185/185 +150 ejecuciones L05 (15tests10 veces;20brokers entre ambos modos),
MSVC20 admisión portable/args/fallback y2 consumidores SDK reinstalados/relinkados
exit0. FD/cleanup correctos, builds inicial/full/final limpios. Logs
work/privileged-admission-*. Sin API/layouts/formatos públicos, Monitor/SystemActions
o CI remota nuevos. No instalación de helper, elevación, polkit/aceptación reales.

Actualización157: [contrato de frontera](LINUX_PRIVILEGED_TRANSPORT.md) verificado
solo en broker privado con límites. No sirve como garantía por helper en el bus
compartido. Se pidió decisión entre aplazar despliegue L05/seguir K03 y ampliar a
canal Unix privado; no se elige por silencio. Evidencia156 anterior histórica.

Plan histórico156: cerrar contrato de límites de transporte/despliegue y sus dependencias
antes de aceptación real. No instalar/activar/elevar ni tocar servicios reales
sin autorización específica. OpenRC148 solo control explícito intacto;
SysV/K03/I07/UAC conservados. C60/E10/P3/B2+3X excluidas:80%/20%.

Fuentes primarias: [contadores de cola sd-bus](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_get_n_queued_read.html)
y [ABI sd-bus](https://raw.githubusercontent.com/systemd/systemd/main/src/systemd/sd-bus.h).
Bucket, límites y política de rechazo son contratos PCInsight.
