# Correlacion de inicio y finalizacion de E/S de disco

Revision62, M02, 2026-10-21. Une el [indice temporal](WINDOWS_TRACE_IDENTITY.md)
con eventos normalizados de inicio/completion mediante un IRP opaco. Bloque
privado, sin nueva metrica de Monitor/Sampler ni API/ABI/persistencia publica.
No hubo ETW real, UAC, archivos de carga ni reintento de elevacion cancelada.

## Decoder TDH

decode_windows_disk_operation conserva el decoder de completion anterior y
agrega ReadInit12/WriteInit13 de DiskIo; Read10/Write11 siguen como completions.
Solo classic version3, anchos32/64 exactos por marcas del evento, timestamp
positivo y payload/funciones presentes. Otro GUID/opcode, incluido FlushInit15,
es ignored. Version no soportada o datos ambiguos no producen un evento valido.
El soporte de init version3 aqui se prueba por fixtures, NO por captura nativa.

IssuingThreadId/TransferSize/DiskNumber conservan propiedades uint32. El IRP se
obtiene por nombre Irp con TDH:4bytes en evento32 o8en evento64, cero-extiende
32bits, conserva todos los bits64 y rechaza longitud incorrecta, cero o puntero
todo-unos del ancho correspondiente. Se generalizo el lector escalar de dos
lecturas/seeds; valores coincidentes con los seeds siguen siendo validos si
ambas lecturas los escriben completamente. Sin offsets/pointer truncado a32
en un evento64 ni cambios en la consulta de completion de revision60.

IRP es exclusivamente una clave interna opaca: nunca se desreferencia ni se
usa como identificador permanente de una operacion/proceso. No exportarlo en
metadatos publicos ni persistir direcciones de kernel. El filtro opcional de
hilo puede descartar otros emisores antes de recuperar cantidades/IRP.

## Correlador acotado

WindowsDiskCorrelation recibe una edad maxima en ticks QPC (no segundos
implicitos) y limite de pendientes1..100000. sample recibe epoca, watermark,
eventos y WindowsTraceIdentity de cobertura reconciliada. Exige identidad ready,
misma epoca, cobertura hasta el watermark y complete=true del caller. Eso es
una **precondicion**, no una prueba de que ETW fue completo. Falta de cobertura,
perdida declarada, cambio de epoca o reset descartan pendientes anteriores.

El productor futuro debe ordenar/reconciliar el flujo antes de llamar sample.
Cada lote contiene eventos en [watermark anterior, nuevo watermark), orden
QPC no decreciente. Watermark debe crecer; tiempos negativos/cero, fuera de
ventana, orden regresivo, IDs/punteros invalidos o disco invalido en completion
invalidan el lote entero y borran pendientes. Limite100000eventos por lote.
No se publica un prefijo exitoso si falla una operacion posterior del mismo lote.
Excepciones de asignacion tambien reinician el estado antes de propagarse.

Al iniciar una operacion guarda IRP, TID, tipo read/write, timestamp y la
identidad resuelta **en ese inicio**. Si no se resuelve identidad, no la inventa
usando el header o una consulta actual: la finalizacion quedara unmatched.
Una segunda iniciacion para un IRP pendiente es ambigua e invalida el lote,
sin sobreescribir el emisor original. Exceso de pendientes tambien invalida.

Completion exige mismo IRP/TID/tipo y tiempo estrictamente posterior al inicio.
Mantiene al propietario capturado aunque proceso/hilo ya hayan terminado o
sus IDs se hayan reutilizado. No vuelve a resolver al propietario al finalizar.
Consume la entrada una sola vez; un completion huerfano/duplicado sin pending
cuenta unmatched, nunca genera una segunda fila. Reuso del IRP tras completion
con un inicio posterior si es posible. Un duplicado viejo posterior a ese reuso
se rechaza por tiempo/ventana regresivos, bajo el contrato de timestamps reales.
No autentica ni detecta falsificacion de timestamps por un caller arbitrario.

Edad mayor al maximo deja completion unmatched. Al terminar cada lote elimina
pendientes demasiado antiguos y reporta discarded, no un cero de actividad.
No expira pendientes por el watermark final ANTES de procesar completions
anteriores validos dentro del lote. El objeto no es thread-safe; serializar
sample/reset. Tablas y resultados se mantienen acotados, sin historico perpetuo.

## Salida y limites

processed significa que se procesaron pares positivos, **no** que se midio
todo el disco de cada proceso. Cada fila contiene TraceOwner/epoca/generacion,
IRP interno, tiempos QPC inicio/fin y bytes/disco/tipo de completion. Los bytes0
validos se conservan; no se calculan tasas ni porcentajes, ni se agregan filas
de cero para procesos sin eventos. unmatched/discarded indican huecos; no
autorizan presentar una ventana como completa aunque ambos sean0.

Sigue pendiente el productor nativo de ciclos de vida y de inicio de I/O,
reconciliacion temporal/reordenamiento/drenaje/perdidas, validacion real de
versiones/IRP/cargas y adaptador publico opt-in. El controlador de revision59
aun habilita solo DISK_IO, NO DISK_IO_INIT: no se declara conectado al nuevo
decoder/correlador ni se cambian sus flags silenciosamente. La prueba privada
de revision60 no prueba este camino nuevo. M02 general disco/red sigue abierta.

## Verificacion

Fixtures portables: pares normales/en lotes distintos, finalizacion despues
de terminar/reutilizar PID/TID, tipo/TID incorrectos, IRP duplicado/reutilizado,
huerfanos/duplicados, tiempo/ventana/epoca/cobertura/perdida/reset, TTL exacto/
excedido, bytes0/uint32max, ticks int64max, presupuesto y100000pendientes/limite
de entrada. Flujo determinista de10000pares contrasta cantidades e identidades.
Fixtures Windows TDH32/64/init/completion/IRP entero/tamanos/sentinels/errores y
pipeline inyectado TDH->indice temporal->correlador: header PID/TID equivocados,
emisor antiguo conservado despues de reutilizar sus IDs. Ningun fixture activa
ETW. Primer build fallo por nombre local far, macro heredada del SDK; se renombro
a extreme_identity sin modificar la logica. Logs iniciales conservados.
Suites Release finales Windows136/136 MinGW y MSVC, Linux115/115. Veinte
repeticiones de correlador y veinte del decoder por cada uno de los tres
entornos; tres consumidores externos instalados/reconstruidos/ejecutados exit0.
work/disk-correlation-* conserva builds/suites/repeticiones/instalacion/consumidores.
MSVC mantiene D9025 esperado por /UNDEBUG; sin otros warnings finales.

Fuentes primarias: [DiskIo_TypeGroup2](https://learn.microsoft.com/en-us/windows/win32/etw/diskio-typegroup2)
documenta init/IRP/hilo emisor; [DiskIo_TypeGroup1](https://learn.microsoft.com/en-us/windows/win32/etw/diskio-typegroup1)
documenta completion/cantidades y correlacion por IRP. TDH y origen QPC siguen
los contratos descritos en [WINDOWS_DISK_EVENTS.md](WINDOWS_DISK_EVENTS.md).
