# Eventos de disco ETW y prueba privada de atribucion

Revision62 agrega [decoder init/IRP y correlador](WINDOWS_DISK_CORRELATION.md).
Probado con datos inyectados, sin captura nueva; no confundir el positivo
privado de revision60 con validacion nativa del nuevo camino.

Revision61 agrega el [indice de identidad temporal](WINDOWS_TRACE_IDENTITY.md),
con pruebas portables de reutilizacion PID/TID. Aun no hay productor de ciclos
de vida/inicio de I/O ni correlacion IRP: no atribuir por hora de completion.

Revision60, M02, 2026-10-21. Continua el experimento temporal autorizado de
revision58/59. [Control/ciclo de vida](WINDOWS_DISK_TRACE_LIFECYCLE.md).
Este bloque NO integra disco/red por proceso en Monitor ni activa captura por
defecto. No modifica API/layouts publicos, vtables o persistencia.

## Decodificacion nativa

src/windows_disk_events.hpp/.cpp filtra el GUID DiskIo documentado y opcodes
Read10/Write11. Solo soporta version3 classic, con exactamente una marca de
punteros32/64, payload presente y timestamp positivo. No interpreta un evento
WPP ni adivina formatos/versiones anteriores; devuelve invalid/unsupported.
Otro GUID/opcode es ignored. El caller garantiza reloj QPC mediante sesion
ClientContext1 y consumidor RAW_TIMESTAMP; el decoder por si solo no autentica
el origen/reloj de un EVENT_RECORD que le proporcione otro caller.

Lee propiedades por nombre con TdhGetPropertySize/TdhGetProperty: IssuingThreadId,
TransferSize y DiskNumber, exigiendo4bytes por cada una. No hay overlays ni
offsets manuales que dependan del ancho de FileObject/IRP. Dos lecturas del
payload inmutable con seeds distintos deben coincidir; no-write/partial/mismatch
son error, sin reservar ningun valor uint32 como seed. TransferSize0 es un valor
valido, no falta de informacion. TID0/UINT32max y discoUINT32max no se atribuyen.
PID/ThreadId del header NO se usan como emisor de la operacion de disco.

El filtro opcional required_thread descarta otros emisores antes de leer
TransferSize/DiskNumber; tambien puede descartar un emisor no atribuible. Esto
reduce datos transitorios pero NO prueba cobertura completa de todas las E/S
del proceso. Faltas de esquema/TDH/funciones no producen contadores cero conocidos.
TDH es una biblioteca nativa incluida en Windows, no una herramienta/paquete
instalado por PCInsight. CMake enlaza tdh solo en Windows y conserva la dependencia
en el paquete estatico para consumidores. No requiere CLI ni SDK de terceros.
La resolucion de esquemas TDH puede depender de WMI y no tiene plazo duro propio.

## Conteo acotado, no resolver general de PID

accumulate_private_disk es portable y recibe una identidad de hilo cuya validez
garantiza el caller durante toda la ventana QPC inclusiva [begin,end]. Solo suma
completions de ese hilo y ventana; no suma todos los eventos del header PID.
Mantiene uint64 read/write bytes y cantidad de operaciones, sin convertir a
double; protege desbordamientos sin modificar parcialmente el total. Una
operacion valida de0bytes aumenta su cantidad, no sus bytes. Otros hilos o
tiempos quedan ignored; identidad/ventana/disco invalidos u overflow, invalid.
No deduplica/reordena eventos ni calcula tasas/intervalos conocidos o actividad0.

El probe usa el hilo principal propio, que permanece vivo ejecutandose hasta
despues de unir ProcessTrace. Abre un handle real y verifica GetThreadId y
GetProcessIdOfThread contra su propio PID. **Un handle por si solo no evita
reutilizacion del TID tras terminar el hilo**: aqui la validez viene ademas de
que es el propio hilo principal continuamente vivo, no de resolver TIDs ajenos
al recibir eventos tardios. No es un mapa general de hilos/procesos/sesiones.

## Carga propia y limpieza

El modo manual --authorized-private-io no forma parte de CTest. Crea un archivo
CREATE_NEW junto al ejecutable bajo work/, exclusivo y DELETE_ON_CLOSE,
NO_BUFFERING|WRITE_THROUGH. Consulta FILE_STORAGE_INFO, valida sectores/alineacion
del buffer VirtualAlloc64KiB y offsets/tamanos multiplos de sector. Escribe
16x64KiB, vuelve a0 y lee16x64KiB verificando todos los bytes de datos propios.
La ventana QPC rodea esas operaciones, no las consultas/creacion/registro de logs.
No se leen archivos ajenos ni se conservan sus rutas/contenidos/argumentos.

El cierre del handle elimina solo el archivo recien creado; despues se verifica
FILE_NOT_FOUND. Colision CREATE_NEW no abre/borra archivos existentes. El modo
conserva contadores del hilo propio, no el payload completo ni datos de otros
emisores. No hay fichero ETL. La captura conserva el limite/watchdog de revision59;
tras I/O espera hasta3s por entrega bufferizada, con watchdog independiente10s.
Si el SO bloquea I/O o TDH no se promete deadline duro del ejecutable completo.

STOP ahora conserva estadisticas de perdida de su sesion, conocidas solo si
la API devuelve exito: EventsLost/RealTimeBuffersLost/LogBuffersLost. No se
confunden con trafico de otros procesos. Cero en estas estadisticas no garantiza
que el consumidor haya drenado todos los eventos: CloseTrace puede cancelar
entrega pendiente. La prueba exige positivos >=1MiB por direccion, ausencia
confirmada de sesion/archivo, sin errores de esquema, perdidas reportadas0 y
salida limpia del consumidor. No publica estos totales como captura completa.

Los bytes observados de escritura pueden superar1MiB por metadata/otras E/S
del mismo hilo dentro de la ventana. No se demuestra atribucion a un unico
archivo, bytes persistidos en el medio, causalidad original de escrituras
diferidas/cache ni ausencia de doble conteo entre topologias de almacenamiento.
NO_BUFFERING no anula necesariamente caches internas del hardware.

## Verificacion y pendientes

Fixtures portables de ventana/identidad/cero/uint64/overflow y oraculo de10000
eventos; Windows simula TDH/tamanos/errores/sin escritura/parcial/mismatch,
GUID/opcode/version/flags/payload/timestamp invalidos, dos anchos y filtro de
emisor. No activan ETW real. El test de ciclo de vida conserva los contadores
de perdida, valores no-cero y validez ante STOP fallido.

Primera prueba MinGW: positivos read1048576/write1159168B, pero schemaError13;
archivo/sesion retirados. Origen exacto del13 no diagnosticado por campo, no se
afirma una causa raiz probada. Se mejoro filtro temprano y se preservo0bytes
valido; repeticion MinGW PASS read1048576/write1142784B,16reads/38writes,
schemaError0, tres perdidas0, STOP0/QUERY4201, ProcessTrace0/CloseTrace0,
archivo eliminado. Logs iniciales conservados, no reescritos como exitosos.

Suite final Windows134/134 con MinGW y MSVC, Linux113/113 Release;20repeticiones
del decoder y20del ciclo de vida por cada compilador Windows. Tres consumidores
externos instalados/reconstruidos/ejecutados exit0, incluida dependencia TDH
Windows. Registros work/etw-disk-events-verified-*, repeat/install/consumer.
D9025MSVC esperado por /UNDEBUG; sin otros warnings de compilacion finales.
El caso real MSVC --authorized-private-io NO se ejecuto: Start-Process devolvio
que la elevacion fue cancelada. No se reintento ni se usa evidencia antigua del
ciclo de vida como positivo nuevo. La envoltura PowerShell no paro ante ese
error y devolvio0: ese codigo es de la envoltura, no del probe, y se descarto
como evidencia de exito. Futuros lanzamientos deben usar -ErrorAction Stop y
exigir un proceso/evidencia nuevos antes de interpretar su ExitCode.

Falta resolver identidad temporal general hilo/proceso ante reutilizacion,
producir ventanas completas con drenaje/perdidas/desorden, validar carga de
otros procesos controlados y topologias, exponer opt-in adecuado e integrar
metricas sin sustituir red con E/S generica. M02 sigue E, sin nuevo cierre/ID.

Fuentes: [DiskIo_TypeGroup1](https://learn.microsoft.com/en-us/windows/win32/etw/diskio-typegroup1),
[TdhGetPropertySize](https://learn.microsoft.com/en-us/windows/win32/api/tdh/nf-tdh-tdhgetpropertysize),
[TdhGetProperty](https://learn.microsoft.com/en-us/windows/win32/api/tdh/nf-tdh-tdhgetproperty),
[file buffering/alineacion](https://learn.microsoft.com/en-us/windows/win32/fileio/file-buffering).
