# Memoria framebuffer libre NVIDIA en Windows

Actualizacion revision55: [loader System32/legacy](WINDOWS_NVML_LOADER.md)
anade la ubicacion fija NVSMI mediante ProgramFilesX64 de Windows, sin PATH ni
entorno. Solo ausencia habilita fallback, nunca denegacion/binario invalido.
Windows130/130 ambas toolchains y Linux111/111 Release; aun sin positivo fisico
NVIDIA. El contrato de memoria/identidad siguiente no cambia.

Revision54, M04. `Monitor.sample(include_gpus=true)` puede completar el campo
existente `GpuInfo.memory_available_bytes` con los bytes **sin asignar reportados
por NVML v1** para una GPU NVIDIA inequívoca. No se calcula capacidad DXGI menos
uso PDH. No es presupuesto del llamador, headroom DXGI, capacidad compartida,
memoria BAR1, ni garantía de que una nueva asignación de ese tamaño vaya a funcionar.

## Fuente y alcance

Se usa `nvmlDeviceGetMemoryInfo` con el layout v1: tres enteros64 (total, free,
used). Se conserva free como entero64, incluso por encima de2^53. Se rechazan
sentinels de indisponibilidad, éxito sin salida, free/used mayores que total y
una terna que no cumpla free = total - used. Cero reportado correctamente,
también total/free/used cero, es conocido; los fallos no se convierten en cero.

El resultado es `Support::available` como lectura reportada por esa API, no una
certificación del asignador. Bajo WDDM Windows administra gran parte de la
memoria: el ámbito NVML no equivale a una promesa de asignación ni al uso PDH.
No se cambian `dedicated_memory_bytes`, `dedicated_memory_used_bytes`, el alias
`memory_used_bytes`, shared, engines, temperaturas, presupuestos ni versión de
driver. No sumar/comparar esos proveedores como una instantánea contable única.

Cada muestra vuelve a consultar. El proveedor se ejecuta DESPUÉS del reset de
memoria libre del proveedor PDH. Las filas NVIDIA se reinician a desconocido
antes de intentar la lectura; un error posterior no conserva su valor anterior.
Las filas de otros vendors, o sin vendor ID disponible, no se modifican. Monitor
crea inventario DXGI nuevo en cada muestra; `include_gpus=false` no llama aquí.

## Correspondencia, no por modelo ni orden

Solo filas con hardware_vendor_id disponible e igual a0x10de son candidatas.
La cadena es DXGI LUID -> CUDA LUID/UUID -> NVML root GPU UUID:

1. CUDA enumera dispositivos (máximo256). `cuDeviceGetLuid` y
   `cuDeviceGetUuid_v2` se leen dos veces con semillas distintas para detectar
   salida ausente/parcial y cambios. Se exige máscara de nodo de un solo bit.
2. LUID destino, LUID fuente y UUID CUDA deben ser únicos. No se asignan IDs
   vacíos/malformados o duplicados, incluso en otra capitalización hex o si
   una fila duplicada pertenece a otro vendor. No se elige el primero/máximo.
3. KMT abre ese LUID exacto y comprueba PHYSICALADAPTERCOUNT=1 antes/después
   de la lectura. Adaptadores físicos enlazados o topología desconocida no se
   publican, aunque CUDA solo muestre un nodo por sus filtros de visibilidad.
4. El UUID de16 bytes se presenta en formato GPU-8-4-4-4-12 para
   `nvmlDeviceGetHandleByUUID`. Se exige roundtrip exacto con
   `nvmlDeviceGetUUID`, sin aceptar MIG-/sufijos/truncados. Se detecta el alias
   de un handle NVML entre UUIDs diferentes.
5. Tras obtener memoria se revalidan LUID/máscara/UUID CUDA, UUID NVML y count
   físico KMT; al final se revalida también el count CUDA. Solo entonces se
   publican los resultados preparados. Cambios de identidad/inventario pueden
   invalidar toda la consulta; un fallo aislado de memoria no borra otra GPU válida.

Los enteros de dispositivo CUDA son handles locales de enumeración, nunca IDs
públicos. LUID/UUID cero no se rechazan por ser cero si ambas lecturas lo
escriben y la correspondencia es válida. Un UUID MIG no se presenta como memoria
de todo el adaptador. CUDA oculto/incompatible, TCC sin LUID útil o APIs antiguas
pueden dejar el campo desconocido. Las relecturas no son una transacción contra
hotplug/ABA ni garantizan una topología que el driver no reporte correctamente.

## Dependencias, estado del proceso y recursos

No hay headers CUDA/NVML, import libs, CLI, toolkit, instalador, descarga,
servicio o helper obligatorio. ABI C interna: CUDA WINAPI, NVML cdecl, UUID16B
y memoria v1 de24B. El wrapper nativo se limita a callers Windows64 bits.

nvcuda.dll y gdi32.dll se solicitan con LOAD_LIBRARY_SEARCH_SYSTEM32. NVML
prioriza esa busqueda y, desde revision55, admite la ubicacion legacy fija
ProgramFilesX64/NVIDIA Corporation/NVSMI solo ante ausencia, con ruta validada
y flags DLL_LOAD_DIR|SYSTEM32. Se verifican los13 entrypoints antes de inicializar.
No se busca por cwd/PATH ni entorno. Ver los limites de confianza/diagnostico
del [loader](WINDOWS_NVML_LOADER.md): no certifica firmas/ACL/modulos previos.
No se instala ni cambia PATH. Tener una NVIDIA no garantiza soporte NVML o los
simbolos requeridos; la compatibilidad del driver no se deduce de su ubicacion.

Se llama `cuInit(0)` y se balancea cada `nvmlInit_v2` exitoso con un intento de
`nvmlShutdown`, también al salir por error/excepción. No hay API cuShutdown:
no se intenta destruir el estado global CUDA que podría usar el host. La
inicialización de esos drivers puede cargar componentes/preparar hardware y
tener estado/hilos internos propios; no se promete impacto cero ni restauración
del estado interno del driver. PCInsight no crea contextos CUDA, no retiene el
primary context, no asigna buffers, no lanza kernels ni modifica contexto actual,
afinidad, variables de entorno, clocks o configuración. Tampoco crea workers
propios. Las referencias de módulos y handles KMT propios se liberan por RAII;
los handles NVML de consulta son prestados. Shutdown fallido no se reintenta ni
se eleva; no se promete que el proceso quede globalmente sin inicializar.

NVML_NO_PERMISSION y KMT STATUS_ACCESS_DENIED se distinguen como
permission_required. Un código CUDA numéricamente igual NO se interpreta como
un código NVML. Otros fallos/ABI incompleta/topología ambigua dan unsupported
con razón. Monitor se debe serializar como antes. No hay timeout que pueda
interrumpir un driver bloqueado.

## Verificación revision54 y límite físico

Release: Windows MinGW129/129, MSVC129/129 y Linux111/111. Veinte repeticiones
del nuevo test por compilador Windows. Tres consumidores instalados externos
reconstruidos/relinkados/ejecutados, sin incluir headers NVIDIA ni nuevos enlaces
del paquete. Fuente: `src/windows_nvml_memory.cpp`; pruebas:
`tests/windows_nvml_memory.cpp`. Registros iniciales/prototipo conservados en
`work/nvml-free-*`; finales `nvml-free-final-*`, repeticiones `nvml-free-release-repeat-*`.

La API inyectada comprueba asociación de dos GPUs/LUID high negativo, cifras
dinámicas/recuperación/cero/2^53, no-write/parcial, cardinalidades/límites,
identidades/máscaras/UUID mutados, root/MIG/alias, topología enlazada/cambiante,
permisos, símbolos faltantes, fallos independientes, limpieza KMT y balance
init/shutdown. Los tests anteriores de Monitor/presupuesto/consumidor ya no
exigen incorrectamente que TODOS los Windows carezcan de memoria libre: solo
aceptan un valor nuevo con vendor0x10de/procedencia NVML y soporte disponible.
Sus comprobaciones directas de PDH/DXGI siguen exigiendo que no inventen free.

Host físico actual: AMD RX7900XT. Se comprueba que se conserva su ausencia
de lectura NVIDIA, el probe opcional de loader/ID inválido queda desconocido
por DLLs ausentes en System32, y Monitor off/on/off no inventa datos. **No hay
positivo físico NVIDIA, WDDM/TCC, multigpu, MIG, linked adapters ni hotplug**;
la prueba inyectada no los certifica. No se cierra M04 por este bloque. AMD/Intel
global free y la cobertura física restante siguen pendientes. Linux mantiene
su proveedor NVML previo, sin cambios de semántica. Sin API/layout/vtable ni
formatos nuevos; comportamiento adicional opcional bajo include_gpus existente.

Fuentes primarias: [consultas NVML y alcance WDDM](https://docs.nvidia.com/deploy/nvml-api/latest/api/group__nvmlDeviceQueries.html),
[layout/unidades v1](https://docs.nvidia.com/deploy/nvml-api/latest/api/structnvmlMemory__t.html),
[identidades CUDA](https://docs.nvidia.com/cuda/cuda-driver-api/cuda_driver_api/group__CUDA__DEVICE.html),
[ejemplo NVIDIA de correspondencia UUID](https://github.com/NVIDIA/nvbandwidth/blob/main/inline_common.h),
[ubicaciones NVML Windows](https://docs.nvidia.com/deploy/nvml-api/nvml-api-reference.html),
[count físico KMT](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/ne-d3dkmthk-_kmtqueryadapterinfotype).
