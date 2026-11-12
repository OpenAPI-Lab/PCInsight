# ETW disco: acceso y ciclo de vida de una sesion propia

Revision60 agrega [decoder y prueba privada de E/S](WINDOWS_DISK_EVENTS.md),
modo --authorized-private-io separado y estadisticas STOP. Las afirmaciones
de este documento sobre descartar TODOS los eventos describen los dos modos
de ciclo de vida originales, no el nuevo conteo privado. Sin integracion default.

Revision59, M02, 2026-10-21. El usuario respondio «prosigue» a la solicitud
explicita de revision58; se tomo como autorizacion para el experimento temporal
acotado, incluyendo elevacion si necesaria. [Alcance](WINDOWS_PROCESS_DISK_DECISION.md).

Este bloque implementa un controlador **privado** y un probe manual; no publica
disco/red por proceso, no agrega una opcion de captura continua ni cambia
Monitor/Sampler. No hay nuevos tipos/layouts publicos, vtables o persistencia.

## Controlador

src/windows_disk_trace.hpp/.cpp usa StartTraceW/ControlTraceW, interfaces nativas
ya enlazadas mediante advapi32, sin CLI, SDK externo en runtime ni instalacion.
El nombre fijo descriptivo `PCInsight Private Disk Test` y GUID propio no son
NT Kernel Logger/SystemTraceControlGuid. Nombre «Private» describe propiedad
del experimento, NO un logger in-process ni filtro de captura por PID.

StartTrace se intenta una sola vez por objeto. ALREADY_EXISTS/acceso denegado/
otros errores no adquieren propiedad y no provocan QUERY/UPDATE/STOP ajenos.
Solo el handle devuelto por StartTrace exitoso autoriza UPDATE/STOP; nunca se
detiene por nombre, ni se adoptan/limpian supuestos huerfanos automaticamente.

Se solicita SYSTEM_LOGGER|REAL_TIME|NO_PER_PROCESSOR_BUFFERING y, explicitamente,
STOP_ON_HYBRID_SHUTDOWN. Sin fichero ETL ni autologger, reloj QPC. Buffer64KiB,
min4/max64 (4MiB de techo solicitado). QUERY valida GUID/modos/flags exactos,
sin fichero, tamano y cantidad de buffers/techo aceptado <=64MiB. Si el SO
ajusta fuera de lo permitido se aborta y retira la sesion; no es una garantia
de que el SO nunca reserve transitoriamente mas antes de la comprobacion.

Primero se crea con EnableFlags0; el probe abre OpenTraceW e inicia ProcessTrace
en otro hilo antes de enable_disk(). El controlador exige ese orden al caller,
no tiene acceso a la propiedad del consumidor. UPDATE habilita unicamente
EVENT_TRACE_FLAG_DISK_IO y mantiene REAL_TIME explicitamente. QUERY revalida.
No se habilitan process/thread/network/file-name/stackwalk en este bloque.

stop() serializa llamadas concurrentes y hace un unico STOP por handle adquirido,
despues consulta el nombre (solo lectura). ERROR_WMI_INSTANCE_NOT_FOUND4201
confirma ausencia. Si otra sesion aparece con ese nombre o hay un error de
consulta, se informa limpieza no confirmada; nunca se vuelve a parar el nombre
ni se reintenta un handle potencialmente reciclado. stop_error/query_error se
conservan separados. El destructor intenta el mismo cierre solo si aun no se
intento. No hay recuperacion automatica de limpieza incierta: informar y parar
la investigacion antes de otra captura.

## Probe manual, no CTest

tests/windows_disk_trace_native.cpp requiere --authorized-lifecycle o
--authorized-watchdog; sin opcion sale77 sin crear evidencia/sesion. No se
registra con CTest. Las pruebas automaticas solo inyectan APIs simuladas.

El probe intenta habilitar SeSystemProfilePrivilege solo en su token, restaura
el estado al salir y no modifica grupos/politicas/configuracion. La ejecucion
elevada se solicito por UAC solo despues de fallar el token ordinario. Ningun
efecto de elevacion forma parte de Monitor.sample() ni del controlador.

El callback descarta absolutamente TODOS los eventos, sin leer ni almacenar
payload, rutas, contenidos, argumentos, identidades o contadores. Solo se guardan
metadatos de la sesion propia/errores de control en archivos CREATE_NEW junto
al ejecutable bajo work/. No son trazas ETL. No se crea un archivo de carga
de disco ni se accede a datos ajenos: todavia no es prueba de atribucion.

Un hilo watchdog separado del consumidor solicita STOP y CloseTrace a10s;
el modo de prueba de watchdog usa1,5s. Cierre normal tras1s, o espera2s para
ejercitar el watchdog. Se despierta/une el watchdog y se une el consumidor
tras cerrar la sesion/CloseTrace. No se promete un plazo duro ante bloqueo de
APIs del SO, ni limpieza frente a kill/crash del proceso o caida del sistema.
El presupuesto temporal autorizado es15s; los caminos observados terminaron
normalmente. Si se informa limpieza incierta no seguir capturando.

## Evidencia y limites

Token ordinario: AdjustTokenPrivileges1300, StartTrace5; ninguna sesion creada.
Elevado MinGW y MSVC: start/verify0, enable/verify0; modo0x12400100, flags0->256,
buffer64KiB,4reservados/max64, GUID coincidente. Cierre normal y watchdog en
ambos: STOP0, QUERY4201, ProcessTrace0, CloseTrace0, cleanup confirmed1, exit0.
Todos los eventos descartados. Esto demuestra control/configuracion/cierre,
NO llegada de eventos, atribucion PID/hilo, cantidades/tasas, ausencia de
perdida, semantica de cache/escritura diferida o cobertura universal de equipos.

Se conservaron fallos iniciales: macro ANSI en LookupPrivilegeValueW corregida
a literal wide; validacion inicial rechazo el flag de apagado hibrido por
defecto (13), corregido declarandolo explicitamente; UPDATE inicial con modo0
fallo87, corregido manteniendo REAL_TIME, sin relajar verificaciones. Todas las
sesiones creadas en esos intentos confirmaron retirada antes de reintentar.

Fixtures: colision/permisos/recursos/funciones ausentes, QUERY/UPDATE fallidos,
GUID/modos/flags/fichero/buffers invalidos, overflow y techo exacto64MiB,
STOP fallido/consulta incierta, destructor/idempotencia y ocho cierres concurrentes.
Los registros estan en work/etw-lifecycle-* y los .result_*.txt junto a cada
probe. Falta interpretar eventos, resolver identidades en el tiempo, controlar
perdidas y validar I/O propio antes de integrar disco por proceso en M02.
Suites Release finales: Windows133/133 MinGW y MSVC, Linux112/112; veinte
repeticiones del fixture por compilador Windows y tres consumidores externos
instalados/reconstruidos/ejecutados exit0. MSVC D9025 esperado por /UNDEBUG.

Fuentes primarias: [StartTraceW](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-starttracew),
[ControlTraceW y UPDATE REAL_TIME](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-controltracew),
[modos y apagado hibrido](https://learn.microsoft.com/en-us/windows/win32/etw/logging-mode-constants),
[buffers EVENT_TRACE_PROPERTIES](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/ns-evntrace-event_trace_properties).
