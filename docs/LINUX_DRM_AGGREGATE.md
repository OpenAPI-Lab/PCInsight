# Agregación explícita de clientes DRM seleccionados

LinuxDrmGpuSampler, en pcinsight/linux_drm.hpp, consume un lote de resultados de
read_linux_drm_client_usage y mantiene sus propias bases. No necesita subprocess,
SDK, root ni dispositivos nuevos. El núcleo aritmético también funciona en Windows
con observaciones suministradas por el consumidor; el lector nativo DRM sigue
siendo exclusivo de Linux. No escanea /proc ni cambia Monitor/GpuInfo.

```cpp
pcinsight::LinuxDrmGpuSampler selected;
std::vector<pcinsight::Metric<pcinsight::LinuxDrmClientUsage>> observations;
// El consumidor decide qué clientes existentes observar (PID/fd conocidos).
observations.push_back(pcinsight::read_linux_drm_client_usage(pid, fd));
auto first = selected.sample(observations); // primera base, sin sumas aún
// En el siguiente tick, reconstruir observations con lecturas nuevas:
// auto next = selected.sample(observations);
```

Los resultados se agrupan por PCI completo (dominio incluido), y dentro de él
por client-id. Un cliente compartido entre varios PID/fd cuenta UNA vez. Las
copias se ordenan por timestamp y se selecciona una observación completa, la
más reciente: no se construyen contadores mezclando máximos de motores distintos.
Orden de motores y variantes equivalentes de dirección PCI se normalizan.
Copias del mismo cliente deben coincidir en driver, motores/capacidades y tener
contadores no decrecientes al avanzar el timestamp; en el mismo instante deben
coincidir exactamente. Contradicciones rechazan el lote entero y borran las bases.

## Semántica para el GUI

summed_client_active_percent es la **suma de tasas de clientes observados**, NO
utilización global de GPU. Se marca estimated y puede superar100: dos clientes
con tasas de80 suman160, sin afirmar que la GPU tenga160% de utilización.
Tampoco es una cota inferior instantánea: sus intervalos pueden no coincidir.
No usar esta suma directamente como la gráfica0..100 de utilización hardware.

Para pares busy/total por ciclos, counter_kind identifica la base y la suma va
en summed_client_cycle_percent, nunca en summed_client_active_percent. No se
mezclan bases distintas en una suma. Los ciclos no necesitan intervalo CPU;
shortest_interval/longest_interval solo incluyen intervalos CPU disponibles.
Ver [contadores por ciclos](LINUX_DRM_CYCLES.md) para fórmula y reinicios.

Cada GPU informa input_records y unique_clients. Cada motor conserva nombre
nativo, capacidad y observed_clients/measured_clients. Solo los clientes que
publican ese motor forman parte de esa clase; un motor no publicado no es cero.
Si alguno de esos clientes no tiene delta válido, no se publica una suma parcial.
Capacidades contradictorias entre clientes dejan capacidad y suma ausentes.
shortest_interval/longest_interval describen los intervalos que sí aportan deltas;
earliest_observation/latest_observation son timestamps actuales, no una ventana
común del hardware. No hay certificación de cobertura ni de procedencia de
observaciones construidas manualmente por el consumidor.

Las tasas se recalculan desde contadores raw; porcentajes/notas preexistentes
del input se ignoran. Se heredan high-water y validación del sampler de cliente.
Cambiar el proceso propietario seleccionado reinicia esa base por seguridad.
Clientes ausentes se retiran del estado; reaparecer requiere base nueva.
reset y lote vacío descartan las bases. La clase no es thread-safe.

## Límites y pendientes

Máximo2048 observaciones,128 motores/cliente y65536 registros de motor por lote,
incluyendo duplicados. IDs, PCI, nombres, capacidad y unicidad se validan antes
de agregar. Entrada inválida/ausente/estimated o permiso denegado rechaza el lote
sin truncarlo ni convertir errores en cero; permission_required se conserva.
Las estructuras copiadas omiten notas y porcentajes ajenos, conservando solo raw.

LINUX_DRM_DISCOVERY.md añade descubrimiento limitado automático como llamada
explícita; este agregador no hace IO. LINUX_DRM_MONITOR.md añade integración opt-in
con Monitor/Sampler y LINUX_DRM_CLASSES.md añade categoría nativa tipada para
AMDGPU/i915/Xe. LINUX_DRM_CYCLES.md añade pares busy/total por ciclos.
LINUX_DRM_MAXFREQ.md añade suma separada normalizada por frecuencia máxima.
Quedan pendientes otros drivers/clases, cobertura global y validación física
AMD/Intel/NVIDIA. Esta unidad no completa esa tarea agrupada.

Pruebas: deduplicación entre fd/procesos, dominios/alias PCI, orden temporal,
conflictos, ventanas diferentes, suma160 sin recorte, cero medido, motores
independientes, capacidad, clientes nuevos/retirados, high-water, límites y reset.
Linux Release44/44 y Windows57/57; paquetes instalados/consumidores externos.
Casos positivos sintéticos: no sustituyen hardware DRM físico, ausente en WSL.

Fuente de identidad y contadores: [estadísticas DRM por cliente](https://www.kernel.org/doc/html/latest/gpu/drm-usage-stats.html).
