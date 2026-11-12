# E/S general de procesos Windows

Revision57, bloque M02. `Monitor` completa `ProcessInfo.windows_io` con
GetProcessIoCounters y tasas entre muestras de una misma identidad. No ejecuta
CLI, no instala componentes ni cambia privilegios. Este bloque no implementa
disco fisico o red causal por proceso: **no rellena** disk_bytes_per_second ni
network_bytes_per_second con estas cantidades.

## Datos y unidades

La nueva Metric<WindowsProcessIoDetails> contiene:

- counters: read_operations/write_operations/other_operations y
  read_transfer_bytes/write_transfer_bytes/other_transfer_bytes acumulados,
  todos uint64 completos del proceso. No se suman hijos ni objetos job.
- read_bytes_per_second/write_bytes_per_second/other_bytes_per_second:
  Metric<double> estimated, delta de cada contador de bytes dividido entre
  segundos monotonicos. Son aproximaciones en coma flotante, no enteros exactos
  ni bytes acumulados. Los contadores originales conservan precision >2^53.
- interval_seconds: Metric<double> estimated, ventana propia del proceso para
  esas tres tasas. No se reutiliza un intervalo global de CPU/disco ni se
  normaliza por cantidad de procesadores.

El padre puede estar available con contadores validos aunque las tasas de la
primera muestra esten desconocidas. Inspeccionar cada Metric. Un delta0 valido
produce una tasa0 conocida, no ausencia. Un lector ausente/denegado no produce
contadores0. Linux/otros backends dejan windows_io unsupported, sin valor.

Los contadores son de **toda la E/S del proceso** y pueden incluir archivos,
red y dispositivos. No son bytes persistidos en un disco, uso de espacio,
latencia, IOPS de un disco fisico, ni trafico exclusivo de una interfaz. Tampoco
permiten separar disco/red por restas. Una tuberia privada basta para elevar
estos contadores sin convertirlos en disco. El GUI debe rotularlos como E/S
general y no sumarlos con otros proveedores como una particion fisica.

Fuentes primarias: [GetProcessIoCounters y permisos](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprocessiocounters),
[campos/unidades IO_COUNTERS](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-io_counters).

## Identidad, errores y ciclo de vida

Se usa el handle real ya abierto por el recolector, no se vuelve a abrir por
nombre/PID para consultar otra instancia. El lector comprueba PID, fecha de
creacion FILETIME exacta y ausencia de fecha de salida antes/despues. Si no hay
identidad de creacion validada no publica E/S. El guard existente que descarta
procesos nacidos despues del inicio del snapshot sigue activo.

Dos lecturas nativas con buffers inicializados con patrones diferentes detectan
salida no escrita/parcial y regresion de cualquiera de los seis contadores.
Una salida indistinguible de su patron se rechaza conservadoramente: un valor
real coincidente con0xa5a5... en la primera lectura o0x5a5a... en la segunda
tambien podria quedar desconocido; esos numeros no son sentinels reservados
por Windows. Cero, uint64max y otras cantidades grandes si se conservan en las
pruebas. Exito de API no se convierte en0 ante salida ambigua.

ACCESS_DENIED se expone permission_required; otros errores/identidad cambiada/
salida ambigua, unsupported con razon. Fallo Win32 con error0 se normaliza a
GEN_FAILURE, no se acepta como identidad valida. No se activa SeDebugPrivilege,
helper ni UAC. El lector no adquiere/cierra handles del proceso: son prestados,
y el recolector conserva su cierre RAII anterior. No garantiza plazo maximo ni
una transaccion atomica con todos los metadatos/otros procesos.

El tracker exige dos muestras consecutivas legibles con PID/creacion iguales,
tiempo creciente y los seis contadores no decrecientes. PID reutilizado, primer
dato, perdida/denegacion, contador regresivo o tiempo no positivo dejan las
tasas desconocidas. El raw actual puede seguir conocido. Fallos legibles por
proceso no invalidan el resto. La tabla se reemplaza en cada captura para no
retener procesos desaparecidos/inaccesibles, con maximo100000 entradas; IDs
duplicados/exceso invalidan la captura del tracker y reinician su baseline.
PID0 del inventario puede tener un resultado desconocido, nunca una identidad
inventada para medir el proceso Idle.

Desactivar la captura **interna** de procesos borra el tracker, y reactivar
requiere baseline nuevo. include_processes=false solo garantiza que no salen
filas: usuarios o historial CPU habilitados pueden seguir necesitando la
captura interna compartida, como antes. Este bloque no agrega contabilidad E/S
a esos agregados/historiales ni los trata como disco/red por sesion o app.

## Compatibilidad y verificacion

Se anaden WindowsProcessIoCounters/WindowsProcessIoDetails y el campo
ProcessInfo.windows_io. **Cambia el layout/stride publico de ProcessInfo0.x**:
reconstruir biblioteca y TODOS los consumidores, no mezclar binarios previos.
No cambian vtables ni formatos de persistencia, no se declara API/ABI1.0.
Los snapshots de Sampler pueden transportar el campo por el tipo comun, sin
convertirlo en un contador acumulativo de AppHistory.

Windows MinGW132/132, MSVC132/132 y Linux112/112 Release. Test portable de
tracker en ambas plataformas, lector/probe nativo solo en Windows. Veinte
repeticiones por toolchain Windows. Tres consumidores externos instalados
reconstruidos/relinkados/ejecutados, comprobando el nuevo campo/ABI; Linux
comprueba que no inventa windows_io. MSVC mantiene D9025 esperado por /UNDEBUG.
La recompilacion completa tambien revelo C4312 en el fixture de volumes:
se corrigio su conversion int->HANDLE mediante uintptr_t sin cambiar valores
simulados2/3, fuente nativa ni comprobaciones del test. Log inicial conservado.
Registros work/process-io-*, builds/suites/repeticiones/instalacion/consumidores;
el primer intento de patch no encontro el bloque CMake y se rechazo entero,
sin aplicar cambios parciales; no fue un fallo de runtime.

Pruebas: baseline, cambio PID/creacion, tiempo igual/retrocedido/extremos sin
overflow de duracion, cero, raw>2^53/uint64max, error/denegacion/sin API/salida
no escrita o parcial, proceso terminado/cambiado alrededor de lectura,
contador regresivo, desaparicion/reset, duplicados y recuperacion. Probe con
handle de proceso propio y permisos QUERY_LIMITED_INFORMATION, tuberia privada
de1024B: read delta1024 y write delta1024 observados en ambas toolchains.
Monitor primera muestra/tasas posteriores/off-on comprobados, disco/red
especificos siguen sin valor. Solo handles privados, sin archivos nuevos,
instalacion, mutacion de otros procesos ni servicios.

Bloque general E/S completado; **M02 sigue abierta** por disco/red especificos,
atribucion y cobertura restantes. M04 queda con sus pendientes de fuente global/
hardware; la consulta AMD experimental de revision56 no cambia de alcance.
