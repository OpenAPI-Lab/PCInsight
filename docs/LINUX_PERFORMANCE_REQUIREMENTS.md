# N03: contrato de rendimiento Linux para el consumidor C++

Revisión160, 2026-11-02. Trazabilidad del bloque **Performance** pedido por el
usuario, no certificación de todo el proyecto ni cierre de N03/G07/N02/B08.
Todos los miembros siguientes están en `include/pcinsight/types.hpp`, salvo las
APIs adicionales identificadas. Prefijos de cada tabla son objetos del
`SystemSnapshot` devuelto por `Monitor::sample` o retenido por `Sampler`.
El [ejemplo público](../examples/linux_performance.cpp) compila estos accesos
contra el SDK, sin incluir archivos internos. No contiene acciones ni captura BPF.

Correspondencia con IDs oficiales existentes (no nuevas subtareas): ventana y
muestreo A01/A02/A03; CPU B01..B08/N02; RAM C01/C02/C03/N02; disk E01..E07;
red F01/F02; GPU G01..G08, con validación física en G07. R02..R12 de
SEGUIMIENTO.txt son los requisitos de usuario cubiertos por esta matriz. La
entrega/documentación pertenece a N03; contar una fila no cierra su proveedor.

## Reglas para no inventar datos en el GUI

- Un `Metric<T>` es conocido solo si `metric_is_known` es verdadero. Revisar
  support y note: available y estimated no son lo mismo. Cero/false conocidos
  se muestran; ausencia, permission_required y unsupported se muestran desconocidos,
  nunca mediante value_or(0). Flotantes no finitos no se grafican.
- En estructuras dentro de un Metric, primero comprobar el padre y después
  cada hijo. El ejemplo propaga la ausencia del padre aunque contenga un objeto.
- Tasas requieren dos observaciones continuas del MISMO Monitor. Warmup, reset,
  cambio de identidad o pérdida de contadores generan huecos, no ceros. No crear
  un Monitor nuevo en cada frame para calcular porcentajes/tasas.
- Unidades nativas: B y B/s, GHz, MT/s, ms, segundos, Celsius y porcentaje0..100
  cuando así lo define el proveedor. Mostrar KiB=B/1024, MiB=B/1024², GiB=B/1024³;
  MB decimal=B/1e6. Mbps=B/s*8/1e6. Etiquetar la unidad elegida, no intercambiarlas.
- Datos declarados estáticos pueden cambiar por hotplug/configuración/driver.
  No prometer snapshot atómico de CPU/RAM/GPU/disco/red. Conservar identidad y
  calidad del dato, no asociar adaptadores por posición/nombre comercial.

## CPU — prefijo `snapshot.cpu`

| Pedido | Miembro / unidad | Contrato Linux y prueba |
| --- | --- | --- |
| Marca y modelo | reported_vendor, reported_model / texto | Identificadores reportados, no attestation. Preferibles a strings legacy vendor/model sin disponibilidad. [Identidad](LINUX_CPU_IDENTITY.md); [test](../tests/linux_cpu_identity.cpp). |
| Utilization dinámico | utilization_percent / % | Delta validado proc/stat; primer intervalo desconocido. No cuota cgroup. [Contrato](LINUX_CPU_USAGE.md); [test](../tests/linux_cpu_usage.cpp). |
| Speed actual | current_ghz / GHz | Media por CPU lógico online, fuente completa cpuinfo_cur_freq o scaling_cur_freq; esta última puede representar P-state solicitado. Estimated, no reloj instantáneo de aplicación. [Frecuencia](LINUX_CPU_FREQUENCY.md); [test](../tests/linux_cpu_frequency.cpp). |
| Base speed | base_ghz / GHz | base_frequency coherente entre CPUs/lecturas. Configuración, NO velocidad turbo, SMBIOS Current Speed ni verificación directa BIOS. Ausente si falta fuente/coherencia. Mismos contrato/test de frecuencia. |
| Sockets, cores, logical processors | sockets, physical_cores, logical_processors / conteos | Topología visible online, no inventar núcleos offline/ocultos. Logical processors son contextos hardware, NO el contador threads de procesos. [Topología](LINUX_CPU_TOPOLOGY.md); [test](../tests/linux_cpu_topology.cpp). |
| Processes y threads | processes, threads / conteos | Procesos visibles procfs y suma Threads visible; namespace/hidepid/permisos y no atomicidad limitan alcance. [Conteos](LINUX_COUNTS.md); [test](../tests/linux_counts.cpp). |
| Handles | handles / conteo estimado | Descriptores de procesos visibles, NO handles Windows ni archivos únicos; tablas compartidas pueden duplicarse. Mismos contrato/test de conteos. |
| Uptime | uptime / segundos | /proc/uptime: segundos enteros, incluye suspensión/offset del namespace; no duración del proceso GUI ni certificación del host físico. [Contrato](LINUX_UPTIME.md); [test](../tests/linux_uptime.cpp). |
| Virtualización firmware | virtualization_enabled / bool | **Sin proveedor firmware Linux: unsupported**, B08 abierto. No inferir true/false de /dev/kvm, vmx/svm o estar en VM. [Contrato](LINUX_VIRTUALIZATION.md); [test](../tests/linux_virtualization.cpp). |
| KVM, dato adicional separado | linux_kvm_api_version / versión | Consulta compatible sobre FD validado, puede requerir permiso. NO sustituye la bandera de firmware; no crea VM. Mismos contrato/test de virtualización. |
| L1/L2/L3 cache | l1_bytes, l2_bytes, l3_bytes / B | Instancias sysfs alcanzables por CPUs online, deduplicadas por compartición; L1 suma datos/instrucciones. Estimated, no caché oculta/offline. [Caché](LINUX_CPU_CACHE.md); [test](../tests/linux_cpu_cache.cpp). |

## RAM — prefijo `snapshot.memory`

| Pedido | Miembro / unidad | Contrato Linux y prueba |
| --- | --- | --- |
| Cantidad de RAM | reported_total_bytes / B | MemTotal utilizable visible al kernel, no capacidad física instalada. total_bytes legacy=0 no prueba medición. [Memoria](LINUX_MEMORY.md); [test](../tests/linux_memory.cpp). |
| Tipo DDR y form factor | type, form_factor / texto | Firmware/SMBIOS si accesible/publicado; no adivinar por frecuencia o CPU. [SMBIOS](SMBIOS.md); [test](../tests/smbios.cpp). |
| Speed y slots usados/totales | speed_mts / MT/s; slots_used, slots_total / conteos | Inventario SMBIOS y sus límites; no DIMMs inexistentes inferidos. Misma evidencia SMBIOS. |
| Usage e in use | utilization_percent / %; in_use_bytes / B | Estimated: (MemTotal-MemAvailable)/MemTotal y diferencia en B. No Working Set Windows ni suma RSS de procesos. [Memoria](LINUX_MEMORY.md); [test](../tests/linux_memory.cpp). |
| Available | available_bytes / B | MemAvailable, estimación kernel, NO MemFree ni cuota cgroup. Misma evidencia memoria. |
| Compressed | compressed_bytes / B | Alias estimado del pool Zswap, NO toda compresión Linux ni original Zswapped/zram. No sumarlo otra vez a in_use. Misma evidencia memoria. |
| Detalles de compresión | linux_details.zswap_pool_bytes, zswap_original_bytes; zram_devices[].memory.{original_data_bytes,compressed_data_bytes,allocated_memory_bytes} / B | Fuentes separadas, no agregar como particiones disjuntas. Ausencia no significa compresión desactivada. [Composición](LINUX_MEMORY_COMPOSITION.md); [test](../tests/linux_memory_composition.cpp). |
| Composition | linux_details.composition.{unavailable_estimate_bytes,reclaimable_estimate_bytes,free_bytes} / B | Partición estimada validada que suma MemTotal. NO estados Windows modified/standby/in-use/free. Misma evidencia composición. |
| Committed y límite | committed_bytes, commit_limit_bytes / B | Committed_AS / CommitLimit, NO RAM residente; overcommit puede superar límite. Misma evidencia memoria. |
| Cached | cached_bytes / B | Cached+SReclaimable, estimated; excluye Buffers, no copiar significado Windows. Misma evidencia memoria. |
| Paged/nonpaged pools | paged_pool_bytes, nonpaged_pool_bytes / B | **Unsupported**: no equivalencia Windows implementada; Slab no se renombra para rellenarlos. Misma evidencia memoria. |
| Categorías Linux adicionales | linux_details.free_bytes, buffers_bytes, page_cache_bytes, anonymous_bytes, shared_bytes, slab_reclaimable_bytes, slab_unreclaimable_bytes, dirty_bytes, writeback_bytes / B | Contadores superpuestos, NO sumarlos entre sí ni a composition. Misma evidencia composición. |
| Hardware reserved | hardware_reserved_bytes / B | Alias legacy estimated de installed_minus_usable_bytes, NO reserva exclusivamente hardware. Preferir nombre explícito. Misma evidencia composición. |
| RAM instalada y diferencia | linux_details.firmware_installed_bytes, installed_minus_usable_bytes / B | Firmware instalada menos utilizable, solo con entradas coherentes. Incluye límites kernel/boot/offline; desconocida sin fuente. Misma evidencia SMBIOS/composición. |

## Disk — cada `snapshot.disks[i]`

| Pedido | Miembro / unidad | Contrato Linux y prueba |
| --- | --- | --- |
| Identificador/modelo | id, model / texto legacy | Etiquetas del dispositivo observado, no identidad física universal. No usar strings vacíos como datos conocidos. [Identidad](LINUX_DISK_IDENTITY.md); [test](../tests/linux_disk_identity.cpp). |
| Active time | active_percent / % | Delta io_ticks/tiempo, estimated y acotado100; NO saturación garantizada del hardware. [Tasas](LINUX_DISK_RATES.md); [test](../tests/linux_disk_rates.cpp). |
| Transfer/read/write speed | transfer_bytes_per_second, read_bytes_per_second, write_bytes_per_second / B/s | Deltas de sectores512B, reloj monotónico, identidad/continuidad comprobadas. Convertir a KiB/s en el GUI. Misma evidencia tasas. |
| Average response | average_response_ms / ms | Promedio de lecturas/escrituras completadas, estimated; sin operaciones no existe promedio. No latencia end-to-end ni discard/flush. Misma evidencia tasas. |
| Capacity | capacity_bytes / B | Capacidad del dispositivo de bloques visible, no necesariamente bruto físico del fabricante. [Propiedades](LINUX_DISK_PROPERTIES.md); [test](../tests/linux_disk_properties.cpp). |
| Formatted capacity automática | formatted_capacity_bytes / B | Estimated: filesystems montados/asociables, deduplicados. **No garantiza toda capacidad formateada**, sobre todo desmontada. [Contrato](LINUX_FORMATTED_CAPACITY.md); [test](../tests/linux_formatted_capacity.cpp). |
| Formatted desmontada explícita | read_linux_filesystem_capacity / read_linux_disk_filesystem_capacity en linux_filesystem_capacity.hpp / B | Superbloque primario ext2/3/4 de regiones seleccionadas, opt-in de bloque; no todos los formatos, raw físico ni suma universal de capas. [Contrato](LINUX_FILESYSTEM_CAPACITY.md); [test](../tests/linux_filesystem_capacity.cpp). No se ejecuta en el ejemplo. |
| System disk | system_disk / bool | Asociación root visible y topología conocida, puede ser desconocida en stacks/namespace no resueltos. [Contrato](LINUX_SYSTEM_DISK.md); [test](../tests/linux_system_disk.cpp). |
| Page file | contains_page_file / bool | Swap activo asociable, no existencia de todo archivo swap inactivo. Ausencia/inaccesibilidad no se convierten en false. [Contrato](LINUX_SWAP_DISKS.md); [test](../tests/linux_swap_disks.cpp). |
| HDD/SSD/SATA/SAS/NVMe | reported_media_type y linux_details.{device_protocol,transport,media_type} / texto | Medio/protocolo/transporte separados, fuentes SAT/SCSI/NVMe cuando disponibles. No inferir SATA de scsi ni SSD de nombre. [Identidad](LINUX_DISK_IDENTITY.md); [test](../tests/linux_disk_identity.cpp). |
| Rotación | rotational / bool | Pista de cola del kernel, NO certificación HDD/SSD ni medio físico. Misma evidencia propiedades. |

## Ethernet/red — cada `snapshot.networks[i]`

| Pedido | Miembro / unidad | Contrato Linux y prueba |
| --- | --- | --- |
| Adapter name | name, id / texto | Nombre/identidad de interfaz observada, no fabricante. [Identidad](LINUX_NETWORK_IDENTITY.md); [test](../tests/linux_network_identity.cpp). |
| Marca/modelo | linux_details.{vendor,model} / texto | PCI/USB y etiquetas locales opcionales, reportadas/estimadas; no modalias o driver como modelo. Primero comprobar padre. Misma evidencia identidad. |
| Connection type | linux_details.connection_type / texto | Evidencia ARPHRD/wireless/virtual, no heurística de nombre ni prueba de Internet/enlace físico/velocidad. Misma evidencia identidad. |
| Send/receive/throughput | send_bytes_per_second, receive_bytes_per_second / B/s | Tasas estimadas de interfaz, sin techo comercial fijo. No son bits/s ni tráfico causal de apps. [test](../tests/linux_network_rates.cpp). Mostrar ambas; sumarlas solo como agregado etiquetado, no ancho de banda. |
| IPv4 e IPv6 | ipv4, ipv6 / listas de direcciones | No suponer una sola. [Lectura Linux por índice y consulta explícita con disponibilidad](LINUX_NETWORK_IDENTITY.md). `query_linux_network_addresses` distingue dump vacío completo de ausencia; el ejemplo lo presenta como consulta separada. Las listas legacy de Monitor no tienen un Metric individual: vacía no demuestra protocolo deshabilitado ni consulta completa. La prueba nativa cubre loopback y la sintética cubre una dirección etiquetada. |
| Acumulados adicionales | total_sent_bytes, total_received_bytes / B legacy | No usarlos como tasa ni como disponibilidad; no sustituyen los Metrics de send/receive. [test](../tests/linux_network_rates.cpp). |

## GPU — cada `snapshot.gpus[i]` y proveedores separados

| Pedido | Miembro / unidad | Contrato Linux y prueba |
| --- | --- | --- |
| Marca/modelo/identidad | vendor, model, id; pci_vendor_name, pci_device_name / etiquetas | Proveedor y diccionario PCI opcional; no inventar retail por ID. Strings legacy vacíos son desconocidos. [Nombres](LINUX_GPU_NAMES.md); [test](../tests/linux_gpu_names.cpp). |
| Utilization global | utilization_percent / % | Fuente gpu_busy_percent o NVML coherente, NO motor3D/copy/compute específico. [Telemetría](LINUX_GPU_TELEMETRY.md); [test](../tests/linux_gpu_sysfs.cpp). |
| 3D/copy/compute y todos los motores | engines[].name/utilization_percent y linux_drm.aggregates[].engines[] / % | No límite de dos compute en biblioteca. No fabricar motores desde overall. DRM fdinfo es alcance clientes observados; summed_client_active_percent puede exceder100 y **NO es utilización GPU global**. [DRM](LINUX_DRM_MONITOR.md), [clases](LINUX_DRM_CLASSES.md); [test](../tests/linux_drm_monitor.cpp). Descubrimiento DRM opt-in, apagado en ejemplo. |
| GPU memory usada | memory_used_bytes, dedicated_memory_used_bytes / B | VRAM/sysfs o framebuffer NVML según fuente; aliases documentados, no sumarlos. Misma evidencia telemetría. |
| Dedicated capacity | dedicated_memory_bytes / B | Total reportado del proveedor, no presupuesto de proceso. Misma evidencia telemetría. |
| GPU memory disponible | memory_available_bytes / B | NVML free o margen estimated total-used, NO garantía de poder asignar esa cantidad. Vulkan heaps/OpenCL max-allocation no sustituyen memoria libre global. Misma evidencia telemetría. |
| Shared capacity/uso | shared_memory_bytes, shared_memory_used_bytes / B | **Sin equivalencia aportada por estos proveedores Linux**, queda desconocido. No rellenar con GTT ni RAM total. Misma evidencia telemetría. |
| GTT adicional | linux_sysfs_details.{gtt_total_bytes,gtt_used_bytes,gtt_headroom_bytes} / B | Contabilidad del gestor GTT, distinta de shared Windows/VRAM/RAM libre. Comprobar padre/child. Misma evidencia telemetría. |
| Temperature | temperature_celsius / Celsius | Sensor GPU identificable/NVML; sensores ambiguos conservan detalles, no se elige el primero. linux_sysfs_details.temperature_sensors lista todos. Misma evidencia telemetría. |
| Driver version | driver_version / texto | Versión del módulo vinculado o NVML, no nombre de driver/kernel como versión. Misma evidencia telemetría. |
| Driver date | driver_date / fecha | **Sin fuente fiable registrada: desconocida**, no mtime ni placeholder DRM ni versión con apariencia de fecha. [Contrato](LINUX_GPU_DRIVER_DATE.md); [test](../tests/linux_gpu_driver_date.cpp). |
| Vulkan | vulkan_version / texto | Capacidad API reportada y asociada según proveedor; no DirectX, no prueba de aplicación gráfica. [Contrato](LINUX_GPU_VULKAN.md); [test](../tests/linux_gpu_vulkan.cpp). |
| OpenCL | opencl_version / texto | Runtime compatible/asociación disponible; no garantía de kernel ejecutable o memoria global. [Contrato](LINUX_GPU_OPENCL.md); [test](../tests/linux_gpu_opencl.cpp). |
| OpenGL | opengl_version, linux_opengl_details / texto/estructura | Contexto actual del caller, requiere include_current_opengl_context=true. No se crea uno, no se asocia por modelo ni se afirma máximo físico. Tests [EGL](../tests/linux_current_opengl.cpp) y [GLX](../tests/linux_current_glx.cpp). Ejemplo deja la opción apagada. |
| Physical location | physical_location / BDF PCI | Bus/device/function nativo comprobado, no índice de lista o serial. Misma evidencia telemetría. |
| Inventario/calidad | snapshot.linux_gpu_inventory_source / alcance | Éxito vacío no equivale a ruta deshabilitada/inaccesible; no certificar todos los adaptadores de todo namespace/driver. [test](../tests/linux_gpu_sysfs.cpp). |

## Ventana dinámica 0..60s

Usar `Sampler(interval, 0, options)`: capacidad automática floor(60s/interval)+1.
`timed_history()` entrega envelopes steady-clock para muestras cuyos inicios
siguen en [ahora-60s,ahora], también al estar detenido. No persistir esos puntos
como fecha wall-clock; no interpolar faltantes ni asumir exactamente61 valores.
`latest()` puede ser más antiguo que la ventana: mostrar edad, no graficarlo
como recién capturado. Start limpia historial, no promete todas las baselines
reiniciadas. [Contrato público](../include/pcinsight/sampler.hpp), [test](../tests/sampler_window.cpp).
El GUI dibuja/escala porcentajes y B/s; la biblioteca no renderiza gráficas.

## Uso y límites de la entrega

`pcinsight-linux-performance --self-test` verifica render sin SO: desconocido vs
cero/false, estimated, permiso denegado aun con valor, NaN, escape de controles,
padre ausente y más de dos motores. `--sample` toma dos muestras Linux de solo
lectura con el mismo Monitor. Sin argumento solo imprime ayuda, no recopila.
Salida terminal: campo, unidad, support, valor/unknown y note. No formato JSON,
ni serializador de UTF-8 no confiable. Índices de disco/red/GPU son locales a
esa salida, **no identidades estables para el GUI**. El ejemplo ilustra los
campos principales y compresión Linux; la matriz incluye APIs opcionales que
no ejecuta (regiones de bloques, clientes DRM, contexto OpenGL).

El [consumer de prueba](../tests/performance_consumer/CMakeLists.txt) compila el
mismo ejemplo usando solo el paquete instalado. Su CMake es de validación del
SDK; el usuario no necesita crear el CMake de su GUI antes de diseñarla.
Sin instalar dependencias, control de servicios, elevación, eBPF ni cambios BIOS.
No se promete obtener información que kernel/firmware/driver no exponen. Validación
física multiplataforma G07/N02 y virtualización firmware B08 siguen abiertas.
Esta unidad cubre Performance; trazabilidad de procesos/historial/startup/users/
servicios/acciones y aceptación final N03 no se dan por cerradas con ella.

## Evidencia160

Linux Release190/190; los dos tests del ejemplo repetidos diez veces (20
ejecuciones), MSVC diez recorridos del contrato puro. Consumer externo del SDK
instalado compilado/relinkado, self-test y --sample exit0. Build final sin warnings
de código y enlaces locales de la matriz comprobados. Logs work/performance-contract-*.
En WSL se observaron, entre otros, utilización CPU y memoria conocidas y frecuencia
actual/base ausentes con nota explícita; no se rellenaron con máximos o ceros.
No evidencia de otra distro/GPU física ni CI remota. La biblioteca productiva y
sus layouts no cambian: solo ejemplo/tests/documentación de consumo. N03 sigue E,
C60/E10/P3/B2+3X excluidas =60/75=80%,15/75=20% abiertas.
