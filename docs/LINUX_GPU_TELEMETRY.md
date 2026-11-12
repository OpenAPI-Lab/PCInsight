# G06: auditoría de telemetría GPU Linux por proveedor

El lector DRM/sysfs reemplaza las conversiones std::stod/stoull sin validación
del inventario anterior. Una cifra vacía, negativa donde no corresponde,
truncada, con sufijo u overflow deja ese campo ausente: no aborta el resto del
snapshot ni se convierte en cero. Las lecturas nativas son acotadas4KiB, sin
seguir symlinks de atributos y con hasta3 reintentos de EINTR. No abre nodos DRM,
cambia políticas, ejecuta comandos, instala paquetes ni requiere root.

## Campos y fuentes

| Campo | DRM/sysfs | NVML |
| --- | --- | --- |
| utilization_percent | gpu_busy_percent entero0..100, utilización general declarada | Ventana de kernels activos reportada por NVML |
| dedicated_memory_bytes | mem_info_vram_total | Framebuffer total reportado |
| memory_used_bytes / dedicated_memory_used_bytes | mem_info_vram_used, alias explícito entre ambos campos | Framebuffer usado v1, incluye reserva del driver |
| memory_available_bytes | total-used, **estimated**, margen no atómico | Free reportado del mismo tuple NVML válido |
| shared_memory_bytes / shared_memory_used_bytes | Ausentes: no se renombran contadores GTT | Ausentes: estas consultas no proporcionan los pools compartidos de Windows |
| temperature_celsius | Solo un candidato edge/core/GPU identificable, no primer sensor arbitrario | Consulta NVML de temperatura GPU |
| driver_version | Atributo version del módulo realmente vinculado | Versión de sistema NVML |
| physical_location | Ubicación PCI del dispositivo, comprobaciones de directorio/binding | Identidad PCI del proveedor; asociación solo única/available |

No se presenta utilización overall como un motor 3D/copy/compute específico.
Los engines de DRM fdinfo son otro proveedor/alcance; no se suman aquí. Un cero
correctamente leído es conocido (0% busy,0bytes usados,0C); ausencia no es cero.
Un total conocido sin uso conserva total y deja margen desconocido. Si used
supera total, se rechazan uso/margen pero se conserva la capacidad leída. El
muestreo es secuencial; no una transacción frente a cambios del driver.

Fuentes: [AMDGPU interfaces de memoria/utilización](https://docs.kernel.org/gpu/amdgpu/driver-misc.html),
[hwmon AMDGPU](https://kernel.org/doc/html/latest/gpu/amdgpu/thermal.html),
[NVML queries](https://docs.nvidia.com/deploy/nvml-api/latest/api/group__nvmlDeviceQueries.html).

## Detalles Linux sin equivalencias inventadas

`GpuInfo.linux_sysfs_details` expone por Metric el nombre del driver vinculado,
nombre del módulo y su versión, VRAM total/usada/margen, GTT total/usada/margen
y una lista de sensores con ID/etiqueta/Celsius. Nombres de driver/módulo NO son
versiones. No se busca /sys/module/<nombre-driver> suponiendo que ambos nombres
coincidan: se sigue la vinculación driver/module. Módulo builtin o sin atributo
version produce ausencia, no fallback al nombre ni a uname.

GTT representa contabilidad del gestor Linux (capacidad/uso/margen de ese
gestor), no RAM libre, memoria físicamente dedicada ni la reserva shared de
Windows. El GUI puede mostrarlo como GTT con su nombre y nota. No se mezcla con
VRAM ni se fabrica shared_memory_bytes a partir de él. Los detalles conservan
la observación DRM aunque NVML aporte los campos generales por separado.

Todos los márgenes por resta son estimated; no prometen que una asignación de
ese tamaño tendrá éxito. No usa presupuesto de proceso, tamaño de heaps Vulkan
ni límite de asignación OpenCL como memoria global libre. Presupuestos LOCAL/
NON_LOCAL del API Windows permanecen separados y sin datos Linux fabricados.

Los sensores conservan etiqueta (edge/junction/mem/etc.) e identidad, hasta128
canales,32por hwmon. Se rechazan duplicados/IDs inválidos. El campo genérico
requiere un solo canal con etiqueta edge/GPU/gpu/core, o temp1 sin etiqueta del
driver amdgpu según su contrato. No se usa junction/mem como edge ni se elige
arbitrariamente entre dos candidatos. Etiqueta desconocida no impide exponer
la lectura en la lista. Temperaturas firmadas en miligrados se convierten a
Celsius; rango de cordura -273.15..1000C, no clamp de lecturas inválidas.

## Identidad y combinación

Inventario solo card+índice decimal, no conectores ni nombres card arbitrarios.
Directorio de tarjeta/device fijado con fd; verifica identidad de ambos por
dev/inode, ruta, vendor/device, binding driver/module y módulo/version antes/
después. No reutiliza caché global de otra tarjeta. Cambios observados descartan
la fila; otros atributos defectuosos son fallos por campo. Las comprobaciones
no bloquean hotplug ni garantizan snapshot atómico. Inventario acotado1024
entradas, con diagnóstico parcial si rechaza filas o excede límites.

NVML solo se asocia a DRM con BDF normalizada única en ambos y support available,
nunca ubicación estimated, nombre o índice. Su tuple total/used/free sustituye
el de DRM únicamente completo/available y coherente sin overflow: no toma
capacidad de un proveedor con uso/libre del otro. Errores/símbolos ausentes
conservan el fallback DRM; memory_used y dedicated_memory_used se actualizan
juntos. Shared no se inventa. Fechas siguen la política separada de G08.

`SystemSnapshot.linux_gpu_inventory_source` distingue opt-out, fuente inaccesible,
enumeración vacía completada y cobertura parcial DRM/NVML. Una fuente completada
sin su compañera es estimated; ambas completas available. No equivale a que se
hayan descubierto todas las GPUs del host, en contenedores o sin estos drivers.

## Alcance del cierre

Esta auditoría cubre los campos/proveedores DRM/sysfs y NVML implementados,
incluyendo ausencias reales y semántica compartida/disponible. No equivale a
telemetría universal de cualquier Intel/AMD/NVIDIA, memoria de cada proceso,
MIG/vGPU, pools Windows en Linux ni todas las fuentes alternativas de cada
driver. OpenGL/Vulkan/OpenCL, nombres PCI y fecha están documentados por separado.
G07/N02 conservan validación física/driver/distribuciones y M04 Windows su área.
Ausencias por falta de fuente no se convierten en valores estimados ficticios.
Nuevo layout GpuInfo/SystemSnapshot0.x: recompilar TODOS los consumidores.

## Verificación de cierre, revisión21 (2026-10-17)

Linux Release89/89, Windows107/107; paquetes reinstalados y consumidores externos
recompilados/relinkados/ejecutados exit0. Pruebas portables de parseo/cero/
overflow/sufijos/signos/permisos, temperatura negativa y selección/ambigüedad de
sensores, GTT separado, módulo sin versión y coherencia del merge NVML. Lector
nativo probado en árbol privado tipo sysfs: módulo con nombre distinto,
atributos inválidos/truncados/symlinks, cambio de identidad/vendor/version y
directorio durante lectura, inventario vacío/ausente. Un directorio con forma
BDF sin subsystem PCI probado NO recibe physical_location.

Primer test falló en ambos OS por una expectativa incorrecta nueva: -1 no es
válido para bytes o porcentaje, pero SI para miligrados Celsius. Se corrigió el
assert para exigir -0.001C sin alterar el parser de temperaturas para ocultarlo.
Registros iniciales preservados en work/gpu-sysfs-first-test-{linux,windows}.log.
Build final de esta unidad sin warnings (unused inicial Windows corregido).
Nativo WSL:0filas GPU, DRM enumerado vacío y NVML no disponible; fuente DRM/sysfs
estimated, no conclusión de ausencia física de GPU. G07/N02 no se cierran.

Logs: work/gpu-sysfs-build-{linux,windows}.log,
work/gpu-sysfs-suite-{linux,windows}.log y work/gpu-sysfs-native-linux.log.
Los riesgos CPU/historial anteriores de N03 no se declaran resueltos por estas
suites aprobadas. No se ejecutaron acciones de configuración/control de GPU.
