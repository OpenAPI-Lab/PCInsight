# Motores DRM por cliente: lectura y muestreo explícitos

API pública en pcinsight/linux_drm.hpp. El consumidor proporciona PID y número
de descriptor del proceso objetivo; no se abre ese dispositivo GPU ni se crea
un cliente. No modifica Monitor ni escanea todos los procesos automáticamente.

```cpp
#include <pcinsight/linux_drm.hpp>
pcinsight::LinuxDrmClientSampler client;
auto first = client.sample(pid, fd); // contadores, aún sin porcentajes
// Más tarde, desde el temporizador del consumidor:
auto later = client.sample(pid, fd);
if (later.value) {
    for (const auto& e : later.value->engines) {
        // e.name es nativo; comprobar counter_kind antes de leer el contador.
        // Mostrar e.utilization_percent solo cuando tenga value.
    }
}
```

read_linux_drm_client_usage ofrece el snapshot acumulado sin estado. El sampler
retiene una sola identidad, no lanza threads ni duerme. reset descarta la base.
No es thread-safe. sample_interval usa steady_clock y no es un intervalo atómico
del hardware. Error de lectura elimina la base; después hace falta otra muestra.

## Identidad y formato

Solo procfs y sysfs de lectura. Exige proceso vivo con PID/start_ticks estables,
descriptor character-device perteneciente al subsistema DRM, PCI sysfs válido
y drm-pdev coincidente. Relee fdinfo para comprobar cliente/driver/PCI y vuelve
a comprobar dispositivo y proceso. Cambios detectados, permiso denegado o
identidad no probada no producen datos. No garantiza atomicidad frente a carreras.
Los límites de tamaño no imponen un deadline a un kernel/driver bloqueado.

El parser acotado acepta drm-driver, drm-client-id y drm-pdev, y tiempos
drm-engine-* en ns. Usa capacidad publicada o uno si falta. Conserva los nombres
nativos y todos los motores observados, sin imponer dos compute. No transforma
gfx/render en 3D exclusivo. Rechaza duplicados relevantes, unidades diferentes,
signos, overflow uint64, capacidad cero/huérfana, registro >64KiB, línea >4096B,
nombre >128B o más de128 motores. LINUX_DRM_CYCLES.md añade pares de ciclos/reloj
GPU con base tipada separada; LINUX_DRM_MAXFREQ.md añade ciclos/frecuencia máxima.

Porcentaje: delta de tiempo activo dividido por intervalo y capacidad, por100.
Se publica estimated: es la observación de ese cliente, no carga total de GPU.
Primera lectura, identidad/capacidad/motor nuevos, reloj no avanzado o delta
mayor que el intervalo/capacidad dejan porcentaje ausente, sin recortar a100.
Un contador que retrocede conserva el máximo anterior en el estado interno y
espera al driver; el snapshot sigue mostrando el contador raw realmente leído.

## Cobertura y pendientes

La interfaz es opcional del driver, no una garantía de cualquier kernel/distro.
Sin interfaz/permisos/PCI: no hay valor; cero solo se publica si se midió delta0.
Windows devuelve unsupported, sin cambiar su proveedor PDH.

Los descriptores compartidos pueden representar el MISMO cliente. Para futuras
agregaciones se deben deduplicar por PCI/client-id; no sumar filas por PID/fd.
Un sampler mantiene base al cambiar a un descriptor duplicado del mismo proceso
y cliente. Cambiar de proceso reinicia la base aunque comparta ese cliente.
LINUX_DRM_AGGREGATE.md añade deduplicación y suma de tasas de clientes de un lote
explícito. No proporciona descubrimiento automático de descriptores,
porcentajes de GPU completos ni equivalencia con Task Manager. No crea la ventana
de60s; el consumidor puede conservar sus muestras explícitas.

LINUX_DRM_DISCOVERY.md añade el barrido automático limitado y explícito. Pendientes:
otros drivers/clases y cobertura global; LINUX_DRM_CLASSES.md añade taxonomía
AMDGPU/i915/Xe y LINUX_DRM_MONITOR.md implementa
integración opt-in y asociación única al inventario GPU. También pendientes:
NVIDIA propietario y validación física AMD/Intel/NVIDIA. WSL disponible no expone
DRM físico. No se certifica hardware con datos sintéticos. Se leen descriptores
existentes; no se abre una GPU para fabricar un caso positivo.

## Verificación

Parser y estado: varias clases/compute, capacidad, delta/zero, high-water y
recuperación, overflow, cambios de cliente/PID/start/driver/PCI/motor, descriptor
duplicado, errores y reset. Reader API inyectada: proceso/dispositivo/cliente
revalidados y permisos; consulta Linux nativa rechaza un pipe propio como no DRM.
Linux Release43/43, Windows56/56; consumidores de paquetes instalados compilan
y ejecutan la API pública. Positivo físico DRM/sysfs sigue pendiente.

Fuente: [interfaz DRM de estadísticas por cliente](https://www.kernel.org/doc/html/latest/gpu/drm-usage-stats.html).
