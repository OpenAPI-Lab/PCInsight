# Contadores DRM en el dominio de reloj GPU

El pipeline DRM acepta pares drm-cycles-N / drm-total-cycles-N, además de los
tiempos drm-engine-N en ns. Sirve para los registros que publica Intel Xe,
manteniendo los controles de PID/start_ticks, descriptor, cliente y PCI.
No abre una GPU, crea cargas ni necesita programas externos.

## Datos públicos y GUI

LinuxDrmEngineUsage.counter_kind distingue nanoseconds de cycles. Para cycles,
busy_cycles y total_cycles contienen los valores raw y su disponibilidad.
El campo legacy busy_nanoseconds NO es válido en ese caso: su cero inicial no
es una medición de tiempo ni una conversión. Comprobar siempre counter_kind.
Si el driver publica ns y ciclos/total para el mismo motor, se prefiere ns y se
conservan también los contadores de ciclos como metadatos, sin mezclarlos.

El porcentaje del cliente por ciclos es:

    100 * delta(busy_cycles) / delta(total_cycles) / capacity

capacity es la publicada o uno si falta. Se resta antes de convertir a coma
flotante, evitando overflow de la resta uint64. No depende de la duración CPU
entre muestras: puede calcularse aunque dos timestamps steady_clock sean iguales.
Un reloj CPU que retrocede sí invalida esa comparación por seguridad.

Los agregados separan explícitamente las bases:

| counter_kind | Campo con la suma observada |
|---|---|
| nanoseconds | summed_client_active_percent |
| cycles | summed_client_cycle_percent |
| cycles_max_frequency (LINUX_DRM_MAXFREQ.md) | summed_client_frequency_percent |

No se publica el otro campo como cero. Clientes con el mismo motor pero distinta
base/capacidad no se suman. La suma sigue siendo estimated de clientes visibles,
no utilización global GPU; puede superar100. shortest/longest_interval describen
intervalos CPU disponibles, no ciclos ni una ventana común. Pueden estar ausentes
en una muestra por ciclos válida con timestamps CPU iguales.

## Casos incompletos y reinicios

Primera muestra o cambio de base/capacidad/identidad: nueva base, sin porcentaje.
Denominador sin avanzar: dato ausente y se conserva el ancla previa de ciclos
ocupados; no pierde incrementos observados antes de que avance el reloj GPU.
Denominador que retrocede: nueva época/base, sin inventar continuidad ni unwrap.
Ciclos ocupados que retroceden con reloj GPU avanzado: retiene high-water y espera
recuperación. Deltas mayores que capacidad*denominador: ausentes, sin recorte100.
Un delta ocupado0 con denominador positivo sí produce cero medido.

Parser acotado: mismas reglas uint64/nombres/líneas/registros y128 motores.
Rechaza pares incompletos, duplicados, unidades en campos de ciclos, signos,
overflow, denominadores huérfanos y total-cycles junto con maxfreq para un motor.
LINUX_DRM_MAXFREQ.md añade la alternativa drm-maxfreq con base tipada separada,
conversión de unidades explícitas y porcentaje normalizado por frecuencia.
La deduplicación compara ambos contadores cuando la base seleccionada es cycles.

## Verificación

Pruebas de parser, lector de identidad inyectado, muestreo, deduplicación,
agregación, clasificación y Monitor por PCI. Incluyen capacidad2, reloj GPU
independiente de CPU, denominador inmóvil/reset, high-water, cercanía a UINT64_MAX,
conflictos y bases mezcladas. Se mantienen las regresiones ns existentes.
Linux Release48/48 y Windows61/61; paquetes y consumidores instalados ejecutados.
La aprobación automática interrumpió inicialmente la verificación Windows;
se reintentó por el mismo mecanismo de aprobación y terminó correctamente.

Sigue pendiente el positivo físico Linux Xe/AMD/NVIDIA: el WSL disponible no
expone DRM físico. Las fixtures no certifican compatibilidad del driver/hardware.
También pendientes otros proveedores y cobertura GPU-global. API0.x
ampliada: recompilar consumidores y biblioteca juntos.

Fuentes: [especificación DRM](https://www.kernel.org/doc/html/latest/gpu/drm-usage-stats.html),
[formato Xe](https://www.kernel.org/doc/html/latest/gpu/xe/xe-drm-usage-stats.html),
[publicación de capacidad/reloj Xe v6.12](https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/gpu/drm/xe/xe_drm_client.c).
