# Memoria dinámica GPU Windows

Revision56: [consulta AMD experimental](WINDOWS_AMD_RUNTIME_MEMORY.md), por
eleccion explicita del usuario, agrega tipos/llamada separados y opt-in para
estimaciones del runtime. No completa memory_available_bytes ni se activa en
Monitor/Sampler. Nueva evidencia primaria aclara size_t[2], pero no suprime el
problema de alcance/presupuestos/caches/recursos internos del runtime.

Actualizacion revision54: [NVML Windows](WINDOWS_NVML_MEMORY.md) puede completar
memory_available_bytes DESPUES de esta etapa PDH, solo para una NVIDIA root
inequivoca por CUDA LUID/UUID y KMT count1. No resta capacidad DXGI menos PDH
ni altera los contadores/capacidades de abajo. Memoria sin asignar reportada por
NVML, no garantia allocatable ni presupuesto del llamador. Sin positivo fisico
NVIDIA aun; AMD/Intel y los fallos siguen desconocidos. Las referencias siguientes
a free desconocido describen esta etapa PDH, no la nueva fuente independiente.

WindowsGpuMemory, integrado en Monitor con include_gpus, lee PDH local en inglés:
GPU Adapter Memory(*) / Dedicated Usage y Shared Usage. No ejecuta herramientas,
no crea dispositivo D3D, no carga un benchmark y no instala servicios. Solo usa
PDH del sistema y los LUID del inventario DXGI de WINDOWS_GPU_INVENTORY.md.

## Campos y alcance

- dedicated_memory_used_bytes: uso dedicado reportado por el contador de adaptador,
  en bytes, para una instancia física inequívoca bajo ese LUID.
- shared_memory_used_bytes: uso de memoria de sistema compartida, separado.
- memory_used_bytes legacy: alias del uso DEDICADO anterior, NO suma total de
  dedicated+shared ni uso del propio proceso de PCInsight.
- dedicated_memory_bytes/shared_memory_bytes conservan CAPACIDADES estáticas de
  DXGI, no cambian a significar uso. dedicated_system_memory_bytes sigue separado.
- memory_available_bytes queda desconocido: restar el uso PDH de una capacidad
  DXGI no certifica memoria libre/allocatable ni presupuesto de un proceso.

Los nuevos contadores son de ADAPTADOR, no GPU Process Memory. No se suman
procesos (pueden compartir allocations y duplicar el conteo). Tampoco se usa
IDXGIAdapter3::QueryVideoMemoryInfo como memoria global: su CurrentUsage y Budget
son del proceso llamador. Un consumidor que precise presupuestos por proceso
necesitaría otro contrato explícito; no se disfraza ese ámbito como total GPU.
Actualización: WINDOWS_GPU_PROCESS_BUDGET.md añade ese contrato separado para
el llamador/nodo0 (LOCAL/NON_LOCAL), sin rellenar memory_available_bytes global.

PDH se pide como entero64 con NOSCALE/NOCAP100: bytes completos, sin escalado
automático ni double que pierda precisión por encima de2^53. Cero válido del
contador se conserva; item/status negativo, ausente o inválido no produce cero.
El uso puede superar una capacidad DXGI de distinto ámbito/reservas; no se clampa
ni se calcula un free negativo/ficticio. Consultas dedicated/shared son separadas,
no una instantánea atómica entre sí, con inventario o con otros adaptadores.

## Identidades y lifecycle

Nombres aceptados: luid_0xHHHHHHHH_0xLLLLLLLL_phys_N, hex upper/lower y decimal
N representable en32 bits. Se enlazan a dxgi-luid:high:low, no por orden/modelo.
_Total y nombres de contadores por proceso no se atribuyen a ninguna GPU.
Dos instancias con el mismo LUID, sean duplicadas o distintos nodos físicos,
dejan ese recurso desconocido: no hay suma automática de linked adapters.
No se promete cobertura de nodos que un driver no exponga. Formato LUID alterado
o sufijos ambiguos se rechazan, no se toma una coincidencia parcial.

Cada Monitor conserva sus propias dos queries/counters. Se requiere primera
colección de preparación y otra nueva para publicar valores, aunque memoria
sea gauge y no tasa temporal; nunca se fabrica un delta. Fallo open/add/collect/
array descarta solo esa query; recovery exige preparación y colección nuevas.
Fallo de Shared no borra Dedicated válido, y viceversa. Item inválido/instancia
ausente deja esa GPU/recurso desconocido; una siguiente muestra válida puede
recuperarlo sin conservar números de una captura anterior.

Cambio del conjunto de LUID, lista vacía o include_gpus=false cierra ambas queries.
Reactivar requiere preparación; no aparecen valores retenidos mientras estaba
deshabilitado. Identidades repetidas/vacías en input no se aplican. El inventario
DXGI sigue fallando explícitamente si la enumeración falla; un fallo de PDH solo
afecta métricas opcionales, no elimina tarjetas del inventario. Fallos de permiso
son permission_required y los demás unsupported con status PDH en la nota.

Límites por array4MiB/16384 items/nombre128wchar; size/count, punteros dentro del
buffer/alineación/terminación, status por item y bytes no negativos comprobados.
Si la segunda llamada indica crecimiento o falla, no hay bucle de realloc ni
map parcial: se reintenta en una próxima muestra con query nueva. Queries y
counters se cierran en reset/destrucción; no hay threads/timer ocultos, no se
garantiza deadline frente a un proveedor PDH lento. Caller serializa su Monitor.
Campos nuevos GpuInfo0.x requieren reconstruir consumidores. Linux mantiene
sus fuentes previas y memory_used_bytes existente; estos campos específicos
no se rellenan allí por esta unidad.

## Verificación y pendientes

Windows47/47 y Linux35/35 Release; instalación, reconstrucción y ejecución de
consumidores externos en ambos sistemas. Tests con checks activos: asociación
LUID signed/hex, valores nuevos que suben/bajan, cero real, int64 máximo sin
double, error de permisos/open/add/collect/format, nulos, recovery, independencia
dedicated/shared, instancias ausentes/duplicadas/multinode/per-process, formato
inválido, bytes negativos, límites/punteros/count/nombres y cierre de recursos.
Monitor verificado con reset al alternar include_gpus.

Prueba nativa x64 en AMD Radeon RX7900XT: muestra inicial dedicada1625387008 y
compartida177475584 bytes; son cantidades observadas, no valores esperados ni
límites permanentes. Se leyó también a través de Monitor sin carga gráfica propia.
Otros drivers, WDDM antiguos, linked/multigpu y hotplug físico no se certifican.
Pendientes: GPU engines/utilization, temperatura, memory libre con ámbito válido,
fecha/ubicación del driver y runtimes. No cerrar GPU Windows completo.

Actualización: WINDOWS_GPU_ENGINES.md incorpora motores y máximo observado
estimado por LUID/nodo/índice. Los demás pendientes anteriores se conservan.

Fuentes: [PDH arrays y flags](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcounterarrayw),
[QueryVideoMemoryInfo y ámbito del proceso](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/nf-dxgi1_4-idxgiadapter3-queryvideomemoryinfo),
[VidMm y duplicación al sumar procesos](https://devblogs.microsoft.com/directx/gpus-in-the-task-manager/).
