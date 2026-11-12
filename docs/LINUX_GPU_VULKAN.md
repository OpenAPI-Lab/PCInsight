# Vulkan Linux: versión por GPU

Corregida la asignación de `vkEnumerateInstanceVersion` a cada `GpuInfo`:
describe el loader, no una tarjeta. `vulkan_version` ahora contiene la propiedad
física `apiVersion` solo tras probar correspondencia PCI única en ambos
inventarios. No hay fallback por nombre, vendor, orden ni versión del loader.

## Consulta

El proveedor se ejecuta después de combinar DRM y NVML. Usa ubicación física
available: BDF completo con dominio de 4 u 8 dígitos hex, normalizado mediante
el parser PCI de NVML. Ubicaciones estimadas, inválidas o duplicadas no reciben
una versión. El dominio importa: dos tarjetas con mismo bus/device/function en
dominios distintos se mantienen separadas.

Se abre libvulkan.so.1 con dlopen local y se resuelven todos los entrypoints antes
de crear una instancia 1.1. No comandos externos, descargas, instalaciones,
superficie, dispositivo lógico, colas ni workload. El driver puede inicializarse;
no es coste cero. Sin capas/extensiones explícitas; las implícitas las controla
el loader. Se comprueba VK_EXT_pci_bus_info por cada dispositivo antes de pedir
VkPhysicalDevicePCIBusInfoPropertiesEXT. Se validan bus <=255, device <=31,
function <=7 y dominio de 32 bits.

Límites: 256 dispositivos, 4096 extensiones por dispositivo y tres intentos por
VK_INCOMPLETE. Errores, buffers/counts incorrectos, handles nulos/duplicados y
extensiones sin NUL descartan la muestra Vulkan. Solo se publican resultados
tras validar la lista; no desaparecen las otras métricas. Dirección PCI repetida
entre ICDs/dispositivos es ambigua incluso si sus versiones coinciden. CPU se
omite. Versiones variante 0, major 1 se presentan major.minor.patch, sin limitar
el dato físico a la versión solicitada por la instancia.

Sin extensión/loader/API, identidad o versión válida: valor ausente con motivo,
no cero ni prueba de ausencia de soporte hardware. Los errores Vulkan no permiten
inferir siempre permisos. Instancia/biblioteca RAII, sin caché permanente ni
worker. No transacción atómica frente a hotplug ni deadline para driver bloqueado.

## Headers opcionales

CMake busca PCINSIGHT_VULKAN_INCLUDE_DIR y comprueba estructuras/macros. Sin
headers compatibles compila el resto y deja una nota `not built` en el campo.
También admite PCINSIGHT_HAS_LINUX_VULKAN_HEADERS=OFF. No enlace a libvulkan ni
headers Vulkan en la API pública; consumidor instalado sin esas dependencias.
Aquí se reutilizaron headers ya presentes del SDK local, copiados a work; no se
descargó ni instaló software. Sin headers, el dato global antiguo NO reaparece.

Cambio semántico intencional: un campo antes rellenado globalmente puede quedar
ahora sin valor si no se prueba identidad. Actualización OpenCL Linux:
LINUX_GPU_OPENCL.md también retira primera plataforma y vincula versión por PCI;
ambas implementaciones probadas, con validación física Linux pendiente.

## Verificación

Linux Release 37/37 y Windows 51/51; paquetes instalados y consumidores externos
ejecutados. Pruebas inyectadas: dos dominios PCI con versiones 1.3.123/1.4.123
frente a loader1.4.500, BDF4/8 hex, identidad ausente/estimada/inválida, duplicados
de ambos inventarios, extensión ausente/truncada, errores/límites, INCOMPLETE
persistente/recuperado, CPU, versión inválida y limpieza. Fixture libvulkan.so.1
prueba dlopen/dlsym y correspondencia real del proveedor con dos dispositivos
sintéticos. Build separado sin headers opcionales y su prueba pasan.

Loader nativo ejecutado en WSL: /sys/class/drm no expone tarjetas y no se obtuvo
correspondencia PCI. **Validación física Linux con GPU DRM pendiente**. Ni la
fixture ni AMD probado en Windows sustituyen esa validación. Otros buses o
drivers sin VK_EXT_pci_bus_info permanecen no disponibles.

Fuentes: [extensión por dispositivo](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_pci_bus_info.html),
[propiedades PCI](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDevicePCIBusInfoPropertiesEXT.html).
