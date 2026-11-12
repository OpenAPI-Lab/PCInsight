# OpenCL Linux: versión por GPU

Retirada la copia de CL_PLATFORM_VERSION de la primera plataforma a todas las
tarjetas. `GpuInfo.opencl_version` ahora conserva la cadena CL_DEVICE_VERSION
del dispositivo GPU vinculado por dirección PCI completa y única. Es versión
del dispositivo OpenCL, no de plataforma, loader, OpenCL C ni soporte de todas
las funciones opcionales de OpenCL 3.0.

## Identidad y API

El proveedor corre después de DRM+NVML. Solo admite physical_location available,
BDF completo con dominio de 4/8 dígitos hex. Compara dominio/bus/device/function,
no nombre, vendor, índice o primera GPU. Consulta todas las plataformas y sus
dispositivos GPU raíz; verifica CL_DEVICE_PLATFORM y CL_DEVICE_TYPE. Comprueba
el token exacto cl_khr_pci_bus_info por dispositivo antes de leer su estructura
PCI. Bus <=255, device <=31, function <=7, dominio completo de 32 bits. No se
inventan dominios faltantes mediante extensiones vendor parciales.

Una dirección repetida en DRM/NVML o en distintos dispositivos/ICDs es ambigua
y queda sin valor, aunque las versiones coincidan. Sin extensión, identidad o
versión válida no hay fallback global. No se confunde ausencia de datos con
ausencia de soporte hardware. Un error de versión afecta solo al dispositivo
ya identificado; fallos de inventario/identidad descartan toda la consulta para
no publicar un subconjunto que pudiera ocultar un duplicado.

## Límites y recursos

Hasta 128 plataformas, 256 dispositivos por plataforma y 4096 en total;
enumeraciones con conteos exactos y tres intentos si crece la lista entre
consultas. Handles nulos/duplicados y propietario contradictorio son inválidos.
CL_DEVICE_NOT_FOUND y CL_PLATFORM_NOT_FOUND_KHR dejan la lista correspondiente
vacía. Extensiones hasta64KiB, versión hasta1KiB, tamaños/NUL comprobados y texto
ASCII imprimible. Versiones requieren formato OpenCL major.minor seguido de
espacio/sufijo; OpenCL C o valores malformados no se aceptan como versión GPU.

dlopen libOpenCL.so.1 local y dlsym de tres funciones, sin enlace OpenCL,
headers/SDK obligatorios, instalación ni subprocess. El subconjunto ABI Linux
público usa enteros fijos, IDs opacos prestados y estructura PCI de16bytes.
No contextos, colas, subdevices ni kernels; no retain/release de IDs raíz
prestados. El único recurso adquirido es la biblioteca, liberada por RAII incluso
si la consulta falla. Sin caché permanente ni worker. El runtime puede inicializar
drivers; no hay plazo impuesto a uno bloqueado ni snapshot atómico frente a
hotplug. Los errores OpenCL no certifican por sí solos una denegación de permisos.

## Verificación

Linux Release39/39 y Windows51/51; paquetes y consumidores instalados ejecutados.
Dos plataformas, dos dominios PCI, versiones diferentes: OpenCL1.2 fixture-A y
OpenCL3.0 fixture-B. No se exporta clGetPlatformInfo en la fixture compartida:
la prueba dlopen/dlsym solo puede obtener CL_DEVICE_VERSION por dispositivo.
Pruebas API cubren errores/listas vacías, límites, crecimiento persistente,
handles/propietarios duplicados o nulos, tipo CPU, tamaños/tipos de texto
incorrectos, NUL ausente, tokens con prefijo/sufijo, PCI inválido/ambiguo,
fallo de versión aislado y API ausente. También configuración Linux sin headers
Vulkan opcionales y las dos pruebas OpenCL pasan.

Consulta nativa en WSL sin correspondencia GPU PCI; **validación física Linux
DRM/OpenCL pendiente**, igual que Vulkan. Los tests sintéticos no la sustituyen.
Drivers sin la extensión PCI estándar u otros buses quedan no disponibles.

Fuentes: [extensión PCI por dispositivo](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/cl_khr_pci_bus_info.html),
[estructura PCI](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/cl_device_pci_bus_info_khr.html),
[consulta de dispositivos](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clGetDeviceIDs.html),
[CL_DEVICE_VERSION](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clGetDeviceInfo.html).
