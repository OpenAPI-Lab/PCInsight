# DRM observado integrado en Monitor y Sampler

SampleOptions.include_linux_drm_clients=true activa el pipeline de descubrimiento
y agregación DRM. Por defecto false y además requiere include_gpus=true. No se
escanea procfs para DRM cuando falta cualquiera de estas opciones. No exige que
include_processes esté activado y no abre dispositivos GPU ni lanza utilidades.

```cpp
pcinsight::Monitor monitor;
pcinsight::SampleOptions options;
options.include_linux_drm_clients = true;
options.linux_drm_limits.max_processes = 4096;
options.linux_drm_limits.max_duration = std::chrono::milliseconds(100);
auto snapshot = monitor.sample(options); // primera base
// En ticks posteriores, reutilizar el MISMO monitor y options:
// snapshot = monitor.sample(options);
const auto& report = snapshot.linux_drm;
// report.discovery: cobertura/límites/permisos, siempre observada (estimated).
// report.aggregates: motores/clientes por PCI; no utilización global GPU.
```

Sampler recibe las mismas options en su constructor y conserva el informe en
latest/history. Mantiene su historial acotado como antes; la integración no
retiene vectores raw de PID/fd por muestra: LinuxDrmDiscoverySummary contiene
solo contadores, flags, timestamps y scope. Los vectores raw existen durante el
barrido y para las bases del agregador, no se copian al historial de snapshots.
LinuxDrmDiscovery hereda ese resumen y conserva observations en su API explícita.
Los tipos de datos DRM residen en types.hpp para evitar includes circulares;
linux_drm.hpp sigue exponiendo los lectores y samplers.

## Semántica y asociación

El informe linux_drm está separado de GpuInfo.utilization_percent y engines.
No reemplaza medidas DRM/NVML/PDH ni convierte sumas de clientes (que pueden
superar100) en gráficas de utilización0..100. Conserva errores/permisos/límites
del descubrimiento y contadores/ventanas por motor del agregador. Falta de datos
no es cero y un barrido sin errores no certifica cobertura global.

Cada grupo puede tener inventory_gpu_id SOLO si su PCI completo coincide con
exactamente una ubicación available del inventario actual y ese id de GPU es
único y no vacío. No se relaciona por índice, nombre, modelo, ubicación estimated
ni dominio PCI parcial. Grupos no asociados permanecen en el informe con identidad
PCI y el id de inventario ausente. La asociación no vuelve global a la métrica.

Se conservan bases por Monitor. Opt-out, include_gpus=false, cambio de límites,
cambio de identidades del inventario (no de orden), errores de descubrimiento,
lotes contradictorios y excepciones descartan las bases correspondientes.
Reactivar requiere otra muestra antes de calcular tasas. Excepciones del backend
también reinician el estado DRM antes de propagarse; Sampler conserva su manejo
habitual de errores. Windows con opt-in devuelve unsupported y no toca PDH.
Monitor no promete uso concurrente del mismo objeto.

El coste del barrido forma parte de la llamada sample. Su presupuesto es
cooperativo, no un timeout duro del kernel/driver. captured_at del snapshot no
significa captura atómica: el resumen DRM expone started_at/finished_at propios.

## Verificación

Fixture de integración: dos lecturas de un cliente duplicado producen10, no20;
modelo independiente del nombre, dominios PCI distintos, PCI duplicado/estimated,
inventario ausente, ids duplicados, orden/cambio de inventario, opt-out, supresión
GPU, límites, permisos, fallo/excepción, conflictos y recuperación de bases.
Comprueba que utilización global17 y el motor overall original no se alteran.
Monitor nativo y Sampler background latest/history probados en Linux y Windows.
Los positivos de correlación DRM son inyectados, no hardware Linux certificado.

LINUX_DRM_CLASSES.md añade engine_class para nombres conocidos AMDGPU/i915/Xe,
sin renombrar ni reinterpretar tasas. LINUX_DRM_CYCLES.md añade pares busy/total
por ciclos, con counter_kind y summed_client_cycle_percent separados de tiempo
activo. LINUX_DRM_MAXFREQ.md añade summed_client_frequency_percent para la base
ciclos/frecuencia máxima. Pendientes: otras clases/drivers,
cobertura/utilización hardware
global y validación física AMD/Intel/NVIDIA. Esta unidad cierra la integración,
no toda la tarea agrupada de motores. No nuevas dependencias ni instalaciones.
Los cambios en structs de API0.x requieren recompilar biblioteca y consumidores.
Resultado Release: Linux46/46 y Windows59/59; bibliotecas/headers instalados y
consumidores externos reconstruidos/ejecutados. Default opt-out probado también
en el consumidor instalado. Positivo físico Linux sigue pendiente.
