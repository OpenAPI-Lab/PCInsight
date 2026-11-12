# Historial observado de CPU por ejecutable Windows

Opt-in existente `SampleOptions.include_app_history=true`. Windows entrega
`SystemSnapshot.windows_app_history`, un Metric de WindowsAppHistoryEntry, separado
del vector legacy app_history para no introducir ceros de red/notificaciones
como si fueran mediciones. El vector legacy continúa con su proveedor Linux;
en Windows sigue vacío. Linux no ejecuta este proveedor Win32 y el informe
windows_app_history queda unsupported con nota de plataforma.

## Qué mide

ProcessInfo.windows_cpu_time_100ns expone kernel+user acumulado por GetProcessTimes,
desde el nacimiento del proceso, con soporte/permisos explícitos. Es duración CPU
sumada entre threads, no tiempo de pared ni porcentaje; puede superar el tiempo
real en programas multihilo. Creación y contador proceden del mismo handle;
suma comprobada contra overflow. Conversión de texto de procesos a UTF-8 ahora
es estricta: errores no producen rutas con sustituciones silenciosas para agrupar.

El historial solo suma **deltas observados** de muestras consecutivas con mismo
PID, creación no cero y ruta exacta del ejecutable. La primera observación crea
baseline y CPU observado0; no incluye el consumo anterior a activar la captura.
PID reutilizado, cambio de ruta, contador decreciente o muestra previa ausente
no añade una diferencia. Permisos/contador/identidad/ruta no disponibles rompen
continuidad para ese PID. PID0 no cuenta como aplicación.

Agrupa procesos visibles de todas las cuentas por ruta exacta observada, sin
deduplicar case, aliases, hardlinks, versiones ni paquetes comerciales. No usa
solo el nombre del proceso como identidad fallback. Un archivo sustituido en
la misma ruta sigue en el mismo grupo; esto no es identidad de seguridad ni
atribución AppUserModelID. No contabiliza todos los procesos que nacen/mueren
entre polls, ni reconstruye CPU durante huecos o después de perder acceso.

Cada fila contiene id (ruta), name (primer nombre observado), observed_cpu_time
Metric<nanoseconds> estimated, network_bytes/notifications sin valor con notas,
y period_start/end. Period_end es la marca de la última muestra publicada,
también si esa aplicación ya no fue visible; no prueba consumo continuo ni el
instante de su última ejecución. Los contadores se conservan mientras dure la
captura y las filas de aplicaciones desaparecidas permanecen con sus sumas.

## Captura y resets

Windows recopila procesos internamente al pedir historial, aunque
include_processes=false; en ese caso snapshot.processes sigue vacío. CPU/RAM
globales y otros flags conservan su funcionamiento. Desactivar include_app_history
borra baselines y acumulados de este proveedor; reactivarlo inicia periodo nuevo.
Un fallo de enumeración de procesos rompe continuidad sin prometer recuperación
de los deltas faltantes. No escribe ficheros, lanza procesos, eleva permisos ni
instala dependencias. No son datos históricos recuperados de Windows anteriores
a esta instancia de Monitor ni equivalencia completa al historial del Task Manager.

Una muestra rechazada por duplicados, límites, reloj de pared hacia atrás u
overflow no publica informe parcial ni aumenta acumulados; borra continuidad.
Las sumas previas se conservan y el caller puede reiniciar captura con opt-out
para un nuevo periodo. No convertir ausencia de informe en CPU/red/notificaciones0.

## Límites y persistencia

Hasta10000 procesos por muestra,4096 rutas retenidas,16MiB texto de entrada,
4MiB de texto de identidad retenido (incluye claves/IDs/nombres), rutas32768B y
nombres16384B. Son presupuestos de datos, no RAM total ni deadline de APIs.
El acumulado de duración debe caber en nanoseconds int64; overflow se rechaza.
Copias transaccionales del mapa y de snapshots/historial de Sampler añaden memoria.
Muestrear más seguido reduce algunos huecos, no garantiza cobertura completa.

HistoryStore v1/v2 no cambia: acepta AppHistoryEntry legacy, **no** este informe
con disponibilidad por campo. WindowsHistoryStore ahora persiste/combina ese
informe en formato/archivo separado, preservando valor/soporte/nota y rechazando
destinos corruptos o legacy. Ver WINDOWS_HISTORY_STORE.md; no convertirlo a
legacy perdiendo la diferencia entre ausente y0. La aplicación coordina I/O.
Vista comun anadida en A01: app_history_view y AppHistorySample conservan la
disponibilidad/procedencia de este informe; no sustituyen WindowsHistoryStore.
LegacyAppHistoryEntry es alias explicito del payload antiguo. Importar v1/v2
no reconstruye mediciones conocidas; ver METRIC_AVAILABILITY.md.
Tampoco se añaden collectors de red/notificaciones. API0.x ampliada: recompilar
biblioteca y consumidores, incluidos quienes usen estructuras públicas por valor.

Fuente oficial: [GetProcessTimes: tiempos de usuario/kernel y unidades100ns](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes).

## Verificación

2026-10-15: Release Windows77/77 y Linux57/57; paquetes y consumidores externos
reconstruidos/ejecutados exit0 en ambos. Nativo Windows: captura del propio
ejecutable de prueba, incremento positivo93750000ns (93,75ms) en una pasada;
include_processes=false conserva historial y oculta vector de procesos, opt-out
borra informe y reactivar comienza con baselines0. La carga es un bucle breve
del propio test, sin lanzar ni modificar otras aplicaciones.

Pruebas portables: deltas100ns a nanosegundos, varias instancias por ruta, PID
reutilizado, desaparición/hueco/denegación, contador decreciente, cambio de ruta,
ausencia de identidad, PID0, límites de rutas/tamaño, PID duplicado, reloj hacia
atrás y overflow. El consumidor Windows pide historial sin exponer procesos y
verifica CPU/ausencia de red-notificaciones; Linux prueba agregador con fixtures
y regresiones de su proveedor, no GetProcessTimes nativo. Tests de HistoryStore
conservados sin alterar formato v1/v2. Otros hosts/MSVC requieren más cobertura.
