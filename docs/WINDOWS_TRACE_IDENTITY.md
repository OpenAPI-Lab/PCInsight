# Identidad temporal de procesos e hilos de una traza

Revision62 agrega [correlacion por IRP](WINDOWS_DISK_CORRELATION.md) con pruebas
inyectadas y guarda la identidad al iniciar I/O. Sigue sin productor ETW de
ciclos/inicio ni adaptador de Monitor; el pendiente de correlacion de este
documento describe el estado historico de revision61.

Revision61, M02, 2026-10-21. src/windows_trace_identity.hpp/.cpp implementa el
indice temporal privado WindowsTraceIdentity, portable y probado sin capturar
ETW ni solicitar UAC. NO es aun un productor de eventos, mapa vivo del sistema,
metrica de Monitor o solucion completa de disco/red por proceso.

## Identidad y cobertura

Entrada: vidas de procesos PID/inicio/fin y vidas de hilos TID/PID/inicio de
generacion del proceso/inicio/fin, en un solo reloj QPC y epoca de captura.
Fin0 representa vivo hasta el watermark que garantiza el caller, no infinito.
La identidad devuelta contiene epoca, PID, TID y ambos inicios. Estos numeros
NO son FILETIME de Windows, ni tokens para terminate_process_checked ni otras
acciones. No convertir ticks QPC a una identidad de accion por redondeo.

replace requiere complete=true solo cuando el futuro productor ya haya
reconciliado cobertura de ciclos de vida y perdida/desorden hasta el watermark.
El indice no establece esa propiedad; un contador ETW de perdida0 por si solo
no la demuestra. complete=false borra respuestas anteriores y deja unavailable.
Tras una perdida el caller debe reconstruir cobertura/epoca apropiada, nunca
seguir usando el mapa previo como si fuera completo. La epoca la administra
el caller; no es un identificador global autenticado ni persistente.

Cada reemplazo admite hasta100000 registros totales proceso+hilo. Construye
tablas temporales, ordena por inicio y publica solo tras validar todo. Entrada
invalida/exceso o excepcion de asignacion no conserva respuestas anteriores.
No mezcla capturas parciales ni recorta los registros excedidos silenciosamente.
Conjuntos completos vacios son ready sin respuestas, no captura completa del SO
inferida por ausencia de procesos. El objeto no es thread-safe: usarlo en el
hilo recolector o con sincronizacion externa durante reemplazo/consulta.

Valida IDs, intervalos positivos/ordenados, fin no posterior al watermark,
generacion de proceso existente y contencion de cada hilo en su proceso.
Duplicados exactos y solapamientos de generaciones de un PID/TID invalidan el
reemplazo entero. Hilos de TIDs distintos pueden coexistir. Watermark regresivo
en la misma epoca activa invalida; una epoca nueva es espacio separado.
No inventa finales de hilo ausentes a partir de un proceso ya terminado.

## Consulta de emisor y operaciones tardias

owner_at_initiation(TID,issued_qpc) consulta la generacion que corresponde al
**inicio real de la operacion**, no a la recepcion del callback ni a la
finalizacion. No abre procesos/hilos actuales para resolver eventos anteriores.
owner_for_interval exige que todo [inicio,fin] pertenezca a las mismas vidas.
Es conservador: cruces de terminacion/reuso o limites exactos de inicio/fin/
watermark quedan sin respuesta, sin elegir arbitrariamente una generacion.

Por ejemplo, TID7 de PID10/generacion100 emite en150 y termina en240. Luego
TID7/PID10 se reutiliza en una generacion300, y la operacion anterior termina
en350. Consultar solo350 seleccionaria el proceso nuevo: seria incorrecto.
La consulta de inicio150 conserva la identidad anterior; exigir el intervalo
150..350 devuelve unknown. Para contar una finalizacion despues de terminar
su emisor, el futuro correlador debe guardar la identidad al inicio, asociarla
con la operacion y controlar reuso/duplicados/perdidas antes de completarla.

Esto NO se conecta automaticamente al [decoder de completions](WINDOWS_DISK_EVENTS.md):
ese decoder actual no proporciona el inicio de la operacion ni correlacion IRP.
No sustituir ese dato faltante por la hora de completion, tiempo de llegada,
hora actual ni una resta de latencia no validada. Tampoco usar DCStart/DCEnd
como fecha exacta de nacimiento sin reconciliar los limites de observacion.

## Pruebas y siguiente integracion

tests/windows_trace_identity.cpp usa fixtures portables, sin APIs de captura:
PID/TID reutilizados, proceso/hilo equivocado, lookup tardio, intervalos cruzados,
limites coincidentes, fin abierto, desconocido/complete=false/reset, desorden de
entrada, duplicados/solapamientos, contencion, invalidos/negativos, watermark
regresivo, cambio de epoca, int64max sin aritmetica desbordada, conjuntos vacios,
limite exacto100000/excedido y recuperacion. Oraculo lineal independiente para
20000 consultas contrasta el indice por busqueda binaria.

No se inicio ETW ni se reintento la elevacion MSVC cancelada en revision60.
Falta producir/reconciliar eventos de ciclos de vida e inicio de I/O, correlacion
de operaciones, cobertura/drenaje y adaptador publico opt-in. M02 permanece E.
No cambia API/layout/ABI/vtable/persistencia publica ni agrega dependencias.
Suites Release finales Windows135/135 con MinGW y MSVC, Linux114/114;
20repeticiones del indice en cada uno de los tres entornos y tres consumidores
instalados/reconstruidos/ejecutados exit0. Registros work/trace-identity-*.
MSVC conserva D9025 esperado por /UNDEBUG en el build inicial del nuevo test.

Fuentes de la decision: [Thread_TypeGroup1](https://learn.microsoft.com/en-us/windows/win32/etw/thread-typegroup1)
documenta PID/TID del payload y diferencia Start/End de rundown;
[Process_TypeGroup1](https://learn.microsoft.com/en-us/windows/win32/etw/process-typegroup1)
describe el alcance de vida de IDs; [DiskIo_TypeGroup2](https://learn.microsoft.com/en-us/windows/win32/etw/diskio-typegroup2)
documenta inicio de I/O/IRP/hilo emisor. Este bloque no decodifica aun esas clases.
