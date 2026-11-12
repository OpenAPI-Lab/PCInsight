# Persistencia explícita de capturas de startup

Revision118: [PCISTART v3 para capturas de hijos](STARTUP_BIRTH_STORE.md) conserva
claves/metricas/availability/ventanas/umbrales sin coercion al esquema legacy.
Lectura v1/v2/v3, birth ausente en antiguos no equivale a vacio/zero. Recorder
con watch_births ahora guarda v3; recompilar SDK y consumidores.

Revision107: StartupImpactArchive agrega diagnostics opcional. El recorder
guarda diagnosticos historicos en v2; archivos v1 siguen legibles y conservan
diagnostics AUSENTE, no contadores0. Recompilar consumidores por campos nuevos.
Detalle de alcance y wire format: [STARTUP_DIAGNOSTICS.md](STARTUP_DIAGNOSTICS.md).

StartupImpactStore (startup_store.hpp) guarda/carga UNA captura histórica por
archivo, sin utilidades externas, dependencias nuevas, thread de mantenimiento
ni escrituras automáticas de Monitor. No reutiliza AppHistoryEntry: CPU/E/S de
startup tienen identidad, intervalo, cobertura y confianza diferentes al historial
acumulativo de aplicaciones. HistoryStore y sus archivos no se modificaron.

```cpp
observer.close();
pcinsight::StartupImpactArchive capture;
capture.capture_id = "login-capture-2026-10-14"; // identidad elegida por el caller
capture.native_context = accepted_native_context;
capture.recorded_at = std::chrono::system_clock::now();
capture.observations = observer.observations();
auto saved = pcinsight::StartupImpactStore::save(private_path, capture);
// Manejar saved.error y saved.replaced; no suponer rollback tras publicación.
auto loaded = pcinsight::StartupImpactStore::load(private_path);
// Si loaded falla, no hay archive ni filas parciales. No sobrescribirlo
// automáticamente como respuesta a un error de carga.
```

Solo se guardan observaciones cerradas. Se preservan ID, PID/start_ticks, contexto
nativo, sample_count, window_complete, sampling_gap, last_error, medición opcional,
CPU/E/S/intervalo y sus Support/notas/clasificación. No se fabrican ceros para
valores ausentes ni se promueve confidence. capture_id es un identificador del
consumidor (máximo256 bytes, sin NUL), NO un certificado de causa o autenticidad.
Todas las filas pertenecen al mismo native_context; duplicar identidad, mezclar
contextos, dejar ventanas inconsistentes o suministrar enums inválidos se rechaza.

## Historial no equivale a estado actual

No se guardan timestamps absolutos de steady_clock, baselines ni estado necesario
para reanudar un observer. Solo se guarda su intervalo real en nanosegundos y
recorded_at como tiempo UTC de system_clock con precisión de milisegundos.
PID, token y contexto son históricos: NO ejecutar acciones sobre procesos actuales
usándolos ni registrar las filas cargadas como procesos vivos. Otro arranque/PID
namespace tiene contexto diferente y el overlay con ese contexto actual falla.
Incluso en el mismo arranque, una captura anterior NO es consumo actual: el GUI
debe etiquetarla como captura histórica, mostrar recorded_at/capture_id y scope.
Nada se superpone automáticamente al inventario de Monitor.

La clasificación guardada conserva la política aplicada por el caller entonces;
no se recalcula al cargar, ni este formato conserva una política ejecutable.
La validación comprueba coherencia estructural/evidencia igual que el overlay,
no autentica contadores o recalcula umbrales elegidos por un consumidor.
No se suman capturas, procesos ni recursos. Save reemplaza el archivo elegido
con la captura completa: repetir el mismo save produce los mismos bytes, no
acumula deltas. Para mantener varias capturas, el consumidor elige rutas distintas
y su retención; no hay merge/prune ni borrado automático.

## Formato y límites

Formatos binarios v1/v2 little-endian: magic PCISTART, versión, capture_id/context,
recorded_at en ms, cantidad y observaciones, checksum FNV-1a de64 bits.
V2 inserta diagnosticos antes del checksum. Save sin diagnostics emite v1;
con diagnostics emite v2, incluyendo automaticamente LinuxStartupRecorder.save.
Lectores anteriores a107 rechazan v2: actualizar antes de usar capturas nuevas.
No se migra ni se reescribe nada por cargar. Reemplazar una captura sigue siendo
una operacion explicita del caller; no hay merge/checkpoint ni sidecar separado.
El checksum detecta corrupción accidental, NO manipulación deliberada.
No se aceptan versiones desconocidas, trailing data, registros truncados,
CPU negativo/overflow, tiempo no representable, enums o flags inválidos.
La carga devuelve el archivo entero validado o nada. Máximo16MiB de archivo,
4096 observaciones y8MiB de metadata conforme al overlay: IDs32KiB, contexto1024,
notas4096. No interpreta .desktop Exec ni guarda env/argv de otros procesos.
Una plataforma con ticks de reloj que no representen exactamente un intervalo
persistido lo rechaza en vez de convertirlo silenciosamente. Recorded_at no
puede ser negativo; el reloj del host puede estar errado, no certifica fecha real.

## Escritura, permisos y concurrencia

Caller elige un archivo y un directorio existente de confianza; no se crean
directorios de sistema/usuario ni se habilitan apps de startup. Load/save rechazan
directorios y symlinks/reparse points finales, no constituyen una sandbox para
ancestros de rutas hostiles ni una operación CAS contra escritores adversarios.
El consumidor debe serializar sus escrituras; saves concurrentes son last-writer-
wins. Load toma una instantánea histórica, no un lock persistente de la captura.

Linux crea un temporal exclusivo con modo0600 en un descriptor del directorio,
escribe/ fsync/close/renameat y fsync del directorio. Un error anterior a rename
conserva el destino previo y elimina solo su temporal. Si el fsync del directorio
falla después de rename, error se informa con replaced=true: el nuevo archivo
ya fue publicado, no se finge rollback. Cargar usa O_NOFOLLOW/O_NONBLOCK y exige
archivo regular/bounded; un FIFO se rechaza sin bloquear. La sustitución crea
archivo privado nuevo, no preserva xattrs/ACL/propietario del destino anterior.

Windows usa CREATE_NEW para temporal regular, FlushFileBuffers/close y MoveFileEx
REPLACE_EXISTING|WRITE_THROUGH. No mueve el viejo archivo a backup compartido.
Un destino bloqueado falla sin sobrescribirlo y limpia su temporal. Permisos
dependen de ACL heredada del directorio: usar una carpeta privada; no se instala
una ACL ni hay cifrado. Atomicidad/durabilidad dependen del filesystem/dispositivo;
no se garantiza supervivencia ante cualquier pérdida de energía. No se elimina
la captura existente ante errores de carga. No hay protección anti-tamper.

## Verificación

Round-trip desde observer cerrado, datos known/unknown/permission, filas sin delta,
checksum, cada truncación y cada corrupción de byte, versión/count/flags/enums y
CPUoverflow incluso con checksum recalculado, trailing data válido en checksum,
archivo excesivo, contexto de otro boot rechazado por overlay, save inválido
preserva archivo, capture nueva reemplaza sin sumas, temporales limpiados.
Linux prueba modo0600, symlink/FIFO y directorio no escribible con UID no-root.
Windows prueba reemplazo bloqueado con HANDLE real y conservación del archivo.
Consumidores instalados incluyen la nueva API sin efectos sobre archivos reales.

2026-10-14: Linux33/33 y Windows44/44 Release; paquetes instalados y consumidores
externos reconstruidos/ejecutados. Sin warnings nuevos en los builds verificados.

Pendiente: recorder temprano/origen automático fiable y procesos ajenos breves.
La persistencia no implementa esos colectores ni cierra el padre startup impact.
