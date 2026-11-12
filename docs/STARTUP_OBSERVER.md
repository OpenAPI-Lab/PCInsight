# Observación incremental de startup impact

Subtarea terminada 2026-10-14: StartupImpactObserver (startup_observer.hpp),
poll-driven y con asociación explícita. Registra baseline por proceso, incorpora
muestras sucesivas, cierra registros y devuelve resultados/estado de cobertura.
No descubre asociaciones, no inicia apps, no crea thread/timer/daemon ni CLI.
Monitor.startup_entries.impact sigue not_measured: esta API no completa por sí
sola el observador automático del arranque ni inventa impacto de todo el boot.

```cpp
pcinsight::StartupImpactObserver observer(std::chrono::seconds(60));
auto baseline = pcinsight::read_startup_process_sample(pid, linux_start_ticks);
if (baseline) observer.register_process(entry_id, *baseline.value);
// El scheduler/GUI del caller llama periódicamente observer.poll().
auto rows = observer.observations();
// Al finalizar la fase: observer.close(); no hace una lectura final escondida.
```

Cada identidad PID/start_ticks/native_context solo se registra una vez, incluso
si otro entry_id intenta apropiársela. Se permiten varios procesos del mismo ID
como filas separadas; **no sumar automáticamente**: kernel I/O puede incluir hijos
esperados y duplicarse respecto de filas de hijos. No se agregan por app ni se
validan entry_id/Exec/cgroups. PID reutilizado con nuevo token/contexto no recibe
contadores del registro anterior; poll con stale_target/not_found cierra ese registro.

Window por proceso desde timestamp de su baseline, default60s, >0 hasta1h.
Se cierra automáticamente al aceptar una muestra cuyo intervalo alcance la window.
Se conserva el intervalo real, incluido overshoot del polling; no interpola ni
promete exactamente60s de consumo. window_complete significa duración alcanzada
sin gaps en las observaciones admitidas, no observación completa del boot/app.
Una lectura fallida no avanza timestamps de muestras. Desde la corrección del
2026-10-14, poll también comprueba el reloj al terminar cada intento: a partir
del deadline baseline+window, admite ese último intento y cierra el registro,
incluso si falla, lanza una excepción o no entrega una muestra válida. No vuelve
a leerlo. El cierre por reloj NO rellena muestras ni certifica window_complete:
sin un último delta válido que cubra la ventana, low/medium siguen desconocidos.
High parcial observado se conserva como límite inferior; last_error no se borra.
poll() usa steady_clock::now después de cada lectura; poll(now) permite un tiempo
explícito del mismo reloj para tests/replay deterministas. El caller debe usar
tiempos coherentes con las muestras y crecientes, no system_clock ni Unix epoch.
Una lectura lenta puede terminar después del deadline: no hay cancelación de I/O,
timer interno ni garantía de duración máxima global de poll. Las ventanas de
procesos con baselines diferentes expiran independientemente; el observer permite
nuevas asociaciones hasta close(). observe_sample es ingestión, no scheduler:
no consulta el reloj real; sin nuevos samples, llamar poll o close para finalizar.
Si no llama poll/observe_sample/close, no ocurre trabajo automático por el paso del
tiempo. close es idempotente, no realiza más I/O y bloquea nuevas asociaciones;
para otra fase crear otra instancia. Filas auto-cerradas nunca vuelven a muestrearse.

Mientras la ventana sea corta o gapped, no se certifica low/medium. High observado
se conserva como límite inferior estimado de ese proceso/intervalo. Cierre temprano,
proceso desaparecido, errors/throws del lector, contador decreciente/reset y samples
con métricas ausentes/protected dejan cobertura incompleta. Incluso si se recupera
una lectura, sampling_gap es sticky y no se oculta con low posterior. last_error
describe el último intento; puede volver a none al recuperarse, sin borrar gap.
Permission_required de métricas se conserva en impacto unknown cuando corresponde.
No se inventa cero ni se reconstruye el consumo no observado después de la salida.

Snapshots devuelven sample_count (baseline incluida), closed, window_complete,
sampling_gap, last_error y measurement opcional. Con solo baseline no hay delta.
Raw CPU/E/S observados siguen disponibles aunque clasificación sea unknown; no
confundir esos datos parciales con la app/árbol completos. Classifier/policy y
proveniencia nativa tienen los límites de STARTUP_IMPACT.md.

Interfaces son serializadas por mutex; poll y readers se ejecutan bajo el mismo
lock, por lo que snapshots pueden esperar lecturas lentas. Reader inyectado permite
tests/adaptadores propios; NO debe reentrar en el observer ni esperar a un thread
que necesite su lock. Excepciones del reader se convierten en fallo system_error
por registro y poll continúa otros registros, sin copiar texto sensible. Caller
coordina destrucción/lifecycle; no se garantiza un deadline global de poll/FS.
observe_sample permite ingestión explícita: identity/time inválidos no se aplican.
Copias de samples se preparan antes del commit para no corromper estado si fallan.

Límites:4096 registros,8MiB de strings retenidas (más overhead fijo acotado), ID32KiB,
context1024 bytes, cada nota4096. Excesos/policies/duración inválidos rechazados,
no evicción silenciosa. Chronología creciente, overflow de timepoint/interval y
monotonicidad de counters se comprueban. Constructor inválido lanza invalid_argument;
operaciones retornan ActionResult/poll counters de attempted/updated/failed.
Un último intento fallido en el deadline cuenta como attempted/failed; los polls
posteriores de ese registro no incrementan contadores ni llaman al reader.
No hay persistencia/retención histórica adicional ni asociación de seguridad.

Verificación Release: Linux29/29 y Windows40/40; header instalado y consumidores
externos ejecutados. Tests portable: baseline, delta parcial, cierre automático
con overshoot, cierre temprano/idempotente, double attribution, PID/context cambiados,
denegación/gap/recovery, desaparición, reader exceptions, counter reset, unknown,
límites/policies/timepoint y snapshots concurrentes. Linux realizó poll del proceso
de prueba propio con identidad de Monitor y contadores nativos, cerró ventana y no
volvió a leer. No startup ni aplicaciones reales modificados; Windows no tiene un
lector nativo de esos counters por esta unidad (reader Linux -> unsupported).

Corrección verificada Release Linux31/31 y Windows42/42, con paquetes instalados
y consumidores externos ejecutados. Regresión del cierre por deadline:
denegación persistente antes/en el límite,
reader que lanza, última lectura correcta, error final después de high parcial,
baselines escalonadas, tiempo inyectado máximo sin resta/overflow, reloj real y
ausencia de reintentos tras cerrar. API instalada consumida con ambas variantes.

Pendiente: correlación automática fiable de app/procesos, observador temprano en
login, captura de vida corta/descendientes, agregación sin doble cuenta,
persistencia y enlace coherente con inventario. No cerrar el padre startup impact.

El enlace explícito posterior ya se implementó mediante STARTUP_OVERLAY.md; no
equivale a descubrimiento/registro automático ni captura temprana certificada.

STARTUP_CHILD_EXIT.md añade lectura final no-reaping para hijos del consumidor
y finish_process para incorporar/cerrar una identidad sin cerrar las otras.
Captura breve universal y procesos ya recolectados siguen pendientes.

STARTUP_STORE.md implementa ahora guardado/carga explícitos de observaciones
cerradas como capturas históricas, no checkpoints reanudables ni recorder de boot.
