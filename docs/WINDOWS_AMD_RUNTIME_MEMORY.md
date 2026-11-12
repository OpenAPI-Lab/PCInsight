# Consulta experimental de memoria del runtime AMD (Windows)

Revision56. El usuario eligio explicitamente el2026-10-21 anadir una consulta
experimental opt-in, separada de la memoria global, despues de explicar el
posible estado interno/recursos GPU y el alcance dependiente del runtime.

## API explicita, no activada por Monitor

```cpp
#include <pcinsight/windows_amd_runtime_memory.hpp>
auto view = pcinsight::query_experimental_windows_amd_runtime_memory();
for (const auto& gpu : view.gpus) {
    // reported_free_bytes y reported_largest_block_bytes son estimated.
    // No asignarlos a GpuInfo.memory_available_bytes ni llamarlos free global.
}
```

Solo esta llamada explicita ejecuta el proveedor. No hay opcion activada por
defecto, registro automatico, worker, timer, fallback en Monitor/Sampler ni
almacenamiento en historial. `GpuInfo` y `SampleOptions` no cambian de layout.
La consulta devuelve un tipo nuevo independiente WindowsAmdRuntimeMemorySnapshot,
con filas WindowsAmdRuntimeMemory: adapter_id de sesion DXGI, dos Metric<uint64_t>
en bytes y CL_DRIVER_VERSION opcional. No version comercial/DXGI UMD del driver.
El GUI decide si ofrecer y sondear esta capacidad experimental por separado.

Snapshot estimated significa que al menos una fila tiene estimaciones, no que
todas se hayan leido. Inspeccionar cada Metric. Sin datos, unsupported; fallo
del loader por acceso, permission_required. Inventario DXGI completo sin AMD:
available y lista vacia. No Windows: unsupported/lista vacia, no inventario
vacio exitoso de hardware. DXGI puede lanzar system_error como Monitor. Datos
ausentes no se convierten en0; cero escrito por una consulta valida si cuenta.

## Evidencia nueva del formato y su limite semantico

La especificacion AMD registrada identifica una consulta de memoria libre en
KBytes, pero no detalla todo el resultado. La investigacion de esta revision
aclara lo que antes figuraba como falta de formato: los bindings actuales
Khronos usan vector<size_type> y el codigo publicado AMD CLR describe dos
size_t: total reportado y bloque mayor. No se presupone que todos los drivers
AMD instalados sean ese codigo exacto.

Este decoder Windows64 solicita directamente los dos size_t (16B), verifica
exito/tamano exacto, inicializa ambos con sentinels y exige bloque<=total. No
usa la consulta de tamano0 como layout definitivo: el codigo AMD revisado puede
responder un solo elemento cuando el buffer es menor que el par. No hay
fallback al escalar. Conversion KiB*1024 comprobada contra overflow; enteros
completos, incluso >2^53, sin double ni clamp a capacidades DXGI.

El backend PAL publicado primero prepara recursos de heaps y calcula uso del
proceso/caches, puede incorporar informacion presupuestaria Windows y aplica
reservas/ajustes. Por tanto, saber el formato NO convierte el resultado en
memoria global fisica libre. Solo se exponen las dos cifras reportadas por el
runtime, siempre estimated y con notas experimentales; tampoco certifican que
una asignacion del bloque reportado vaya a funcionar.

Fuentes primarias revisadas:
[extension registrada](https://registry.khronos.org/OpenCL/extensions/amd/cl_amd_device_attribute_query.txt),
[tipo en bindings Khronos](https://github.com/KhronosGroup/OpenCL-CLHPP/blob/main/include/CL/opencl.hpp),
[dos elementos y comportamiento del buffer AMD](https://github.com/ROCm/clr/blob/develop/opencl/amdocl/cl_device.cpp),
[calculo/recursos del backend PAL](https://github.com/ROCm/clr/blob/develop/rocclr/device/pal/paldevice.cpp).

## Correspondencia, recursos y seguridad de alcance

Se usa el inventario DXGI completo y el helper compartido de correspondencia
D3D11/OpenCL de WINDOWS_OPENCL_INTEROP.md. Exige dispositivos raiz enumerados,
asociacion observada uno-a-uno y topologia compatible con toda la lista DXGI;
no modelo/orden. NO es prueba de identidad fisica LUID ni de todos los nodos de
un adaptador enlazado. AMD0x1002 debe estar disponible en DXGI y coincidir con
CL_DEVICE_VENDOR_ID; la extension cl_amd_device_attribute_query debe aparecer
como token exacto. CL_DRIVER_VERSION es acotado, no vacio, revalidado tras leer.
Se revalidan vendor/extension, y se vuelve a obtener el mapa de asociaciones
antes de publicar. Cambio/ausencia/ambiguedad deja unknown; no hay transaccion
atomica contra hotplug/ABA ni promesa sobre drivers que falseen sus identidades.

OpenCL.dll se solicita con System32 search y sus entrypoints se resuelven
dinamicamente; no SDK/link OpenCL/CLI/instalacion. El modulo y referencias DXGI
propios se liberan por RAII, IDs OpenCL prestados. No se llama clCreateContext,
ni se crean buffers, se compilan kernels o se cambia un contexto del host desde
PCInsight. **El runtime puede hacerlo internamente/preparar heaps**, conservar
estado/hilos/recursos y afectar consumo; no se promete impacto0 o restauracion
de su estado. No se cambia config/env/clocks/privilegios. No hay timeout contra
un driver bloqueado. El caller controla frecuencia/serializacion de consultas.

## Verificacion y que no se cierra

Windows MinGW131/131 y MSVC131/131, Linux111/111 Release. Veinte repeticiones de
AMD experimental y del proveedor interop anterior por cada toolchain Windows.
Tres consumidores externos instalados reconstruidos/relinkados/exit0; el
consumidor Linux comprueba unsupported, Windows NO invoca la consulta experimental
por defecto. Header nuevo instalado, sin dependencias de SDK exportadas.

Positivo nativo AMD RX7900XT en las dos toolchains, CL_DRIVER_VERSION3679.0(PAL,LC).
Una primera muestra MinGW reporto21435580416B y bloque21413273600B. Son datos
dinamicos del runtime, no constantes esperadas ni memoria global certificada.
La cifra reportada supera incluso la capacidad dedicada DXGI observada en este
host: no se recorta ni se declara que ambos proveedores midan lo mismo. Despues
de la llamada, Monitor mantiene memory_available_bytes AMD desconocido.

Pruebas inyectadas: dos GPUs/cantidades distintas, cero, enteros grandes,
overflow, sentinel/no-write/parcial, retorno escalar/tamano incorrecto,
bloque>total, query fallida, extension sufijo/vendor incorrecto o cambiado,
driver ausente/cambiado, topologia/asociacion cambiada, duplicados y API ausente;
fallo aislado no borra otra estimacion valida. Se comprueba liberacion de todos
los handles/factories realmente devueltos, tambien en retornos tempranos.
El primer test asumio siempre4 adapters cerrados: un rechazo temprano abre menos;
se corrigio a comparar abiertos/cerrados sin eliminar la comprobacion. Logs
iniciales preservados: `work/amd-runtime-*`; finales final-*, repeticiones repeat-*,
instalacion install-* y consumidor consumer-*. MSVC D9025 por /UNDEBUG esperado.

La extraccion del helper de asociaciones conserva las pruebas del proveedor
de version OpenCL previo y su staging antes de publicar. No anade esta consulta
de memoria a ese proveedor automatico. API0.2 adicional, sin cambiar layouts,
vtable ni formatos existentes. No soporte fisico de otros AMD/UMA/linked/drivers,
positivo NVIDIA, Intel free o matriz universal. **M04 permanece abierta** por
memoria global/cobertura pendiente; esta estimacion no sustituye ese criterio.
