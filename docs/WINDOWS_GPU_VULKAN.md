# Vulkan Windows por adaptador

## Actualizacion revision53: ruta de cabeceras y MSVC

La deteccion Windows ahora acepta `PCINSIGHT_VULKAN_INCLUDE_DIR`, igual que
Linux, y busca tambien `VULKAN_SDK/Include` si ese entorno esta configurado.
Antes solo comprobaba el include implicito del compilador: MinGW encontraba
Vulkan, pero el build MSVC verificado en revision52 lo dejaba `not built`.
La ruta se usa exclusivamente para compilar la biblioteca y su prueba interna;
no se exporta al consumidor, no se enlaza el loader ni se descarga un SDK.

Ejemplo con una copia de cabeceras ya presente (ruta al directorio que contiene
`vulkan/vulkan_core.h`, no al archivo). En un build que ya habia guardado el
resultado negativo, limpiar solo esa prueba de CMake para que se repita:

```powershell
cmake -S outputs/pcinsight -B work/build-msvc-release `
  -U PCINSIGHT_HAS_VULKAN_HEADERS `
  -DPCINSIGHT_VULKAN_INCLUDE_DIR:PATH=C:/ruta/Vulkan-Headers/include
```

Se mantiene `-DPCINSIGHT_HAS_VULKAN_HEADERS=OFF` como desactivacion explicita,
incluso si la ruta existe. Sin cabeceras el resto de la biblioteca sigue
compilando; no se inventa una version ni se instala nada automaticamente.
El cache de comprobacion de CMake no se invalida automaticamente al cambiar
la ruta: usar `-U` como arriba o un build nuevo, no forzar `ON` sin comprobar.

Tambien se corrige una ambiguedad en el destino: dos filas GPU con el mismo
ID DXGI no reciben una misma lectura Vulkan. Ambas quedan desconocidas,
borrando el valor previo; otra fila unica conserva su lectura. Una fila de ID
vacio tampoco recibe resultado. Esto complementa la deteccion ya existente de
LUID repetidos entre dispositivos Vulkan, sin convertir hotplug en transaccion.

Verificacion local Release de esta unidad: Windows MinGW128/128 y MSVC128/128,
Linux111/111; veinte repeticiones de `windows-gpu-vulkan` por compilador Windows,
incluyendo los nuevos casos inyectados. MSVC y MinGW consultan nativamente la
RX7900XT y obtienen Vulkan1.4.349 por LUID; no es solo compilar el stub. Se
comprueba tambien el build MinGW con proveedor explicitamente OFF (1/1 prueba).
Tres consumidores externos instalados reconstruidos y ejecutados, sin ruta
Vulkan en el target CMake exportado. Registros: `work/vulkan-portable-*`.
MSVC conserva D9025 intencional por /UNDEBUG de tests; no se apagan warnings.
Sin API/layout/vtable ni formatos nuevos; M04 sigue abierta por memoria global
disponible y cobertura real de adaptadores/vendors, no certificadas por esto.

Investigacion acotada de memoria libre: la [extension AMD registrada en
Khronos](https://registry.khronos.org/OpenCL/extensions/amd/cl_amd_device_attribute_query.txt)
describe `CL_DEVICE_GLOBAL_FREE_MEMORY_AMD` en KBytes, pero no detalla ahi el
tipo/composicion del resultado. Esta revision NO implementa un decoder por
suposicion ni resta capacidad DXGI menos uso PDH. No se bloquea esta unidad
de Vulkan a la espera de resolver esa via.

## Contrato del proveedor

`Monitor.sample(include_gpus=true)` completa `GpuInfo.vulkan_version` con
`VkPhysicalDeviceProperties.apiVersion` del dispositivo físico correspondiente.
No es la versión del loader ni la versión habilitada por una aplicación. Se
conserva major.minor.patch de Vulkan variante 0, versión mayor 1; no se asigna la
versión más alta de todo el sistema a todas las tarjetas.

## Identidad y consulta

Se carga `vulkan-1.dll` solamente desde System32 y se resuelven los cinco
entrypoints core necesarios antes de crear una instancia. El loader existente
debe ofrecer Vulkan 1.1. La instancia solicita 1.1, sin capas ni extensiones
explícitas, sin superficies, dispositivos lógicos, colas ni carga gráfica. Las
capas implícitas configuradas por el sistema siguen bajo control del loader.
No se instala nada ni se ejecuta un programa externo.

La cadena `VkPhysicalDeviceProperties2 -> VkPhysicalDeviceIDProperties` devuelve
deviceLUID. Solo se compara con el ID DXGI si deviceLUIDValid es exactamente
VK_TRUE y deviceNodeMask tiene un único bit. No se empareja por nombre, vendor,
device ID, orden ni UUID inventado. Dispositivos CPU se omiten. Un LUID que
aparece en varios dispositivos (p. ej., nodos enlazados o ICDs alternativos) es
ambiguo y queda sin valor: no se escoge el primero ni el máximo.

La enumeración tiene límite de 256 dispositivos y tres intentos por
VK_INCOMPLETE. Errores, lista truncada, count fuera del buffer, handles nulos o
duplicados y metadatos de identidad malformados descartan toda la consulta.
Los resultados solo se publican tras validar la lista; no se oculta un duplicado
con una salida parcial. ID ausente o versión inválida dejan ese campo sin valor.
No desaparece el inventario DXGI ni se invalidan otros proveedores GPU.

Loader/entrypoints ausentes, versión 1.0 o errores Vulkan producen un valor
ausente, no versión cero ni prueba de que el hardware carezca de Vulkan. Una
denegación Win32 al cargar la DLL se marca permission_required. Los errores
Vulkan no tienen equivalencia fiable a permisos Win32 y no se reclasifican.
Instancia y módulo se liberan por RAII. No hay caché permanente ni workers;
hotplug/driver entre APIs no constituye una transacción atómica y un driver
bloqueado no tiene timeout impuesto por esta implementación.

## Construcción sin dependencia obligatoria

CMake comprueba que los headers Vulkan disponibles incluyan las estructuras,
funciones y macros requeridas. No descarga paquetes ni exige un SDK. Si faltan,
el resto de PCInsight compila y el campo queda sin valor con nota `not built`.
`-DPCINSIGHT_HAS_VULKAN_HEADERS=OFF` permite desactivar explícitamente este
proveedor al compilar. Las cabeceras y bibliotecas públicas instaladas no
dependen de headers Vulkan ni requieren enlazar vulkan-1. El consumidor tampoco
necesita instalar un loader si solo desea otras métricas.

## Verificación

Windows Release 51/51 y Linux 35/35; paquetes instalados y consumidores externos
ejecutados en ambos. Prueba `windows-gpu-vulkan`: GPU distintas con versiones
distintas, LUID signed high, loader 1.0/error, create fallido/nulo, cleanup,
API faltante, conteos fuera de límite/buffer, handles nulos/duplicados,
INCOMPLETE persistente/recuperado, LUID ausente/inválido/ambiguo, máscara inválida,
versión cero/otra variante, CPU y lista vacía. Consulta nativa y Monitor off/on/off.
También compilación separada con PCINSIGHT_HAS_VULKAN_HEADERS=OFF y su prueba.

RX 7900 XT nativa: Vulkan `1.4.349`. Observación del driver instalado, no constante.
No validación física de multiGPU, otros vendors, loader antiguo ni hotplug.
OpenCL/OpenGL Windows y temperatura permanecen pendientes.

Actualización Linux: LINUX_GPU_VULKAN.md elimina el fallback global del loader
y añade correspondencia PCI única, verificada con API/loader sintéticos. Falta
validación física DRM en Linux; la prueba Windows no la sustituye.

Fuentes: [deviceLUID y correspondencia DXGI](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceIDProperties.html),
[versión de dispositivo frente a instancia](https://docs.vulkan.org/spec/latest/chapters/extensions.html),
[exports core del loader Windows](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/loader/vulkan-1.def).
