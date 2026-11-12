# Fecha del driver GPU Linux: fuentes y ausencia motivada

G08. La salida usa el campo existente `GpuInfo.driver_date` (Metric<string>).
Para los proveedores Linux actuales su value permanece ausente y support es
unsupported, con una nota que empieza por `Linux driver date unavailable:`.
No es fecha cero, string vacío conocido, consulta pendiente ni permiso denegado.
No se ha conseguido una fecha real de publicación/instalación del driver.

Esta unidad completa la política de fuentes/disponibilidad solicitada en G08,
cuyo criterio permite declarar indisponibilidad motivada. No cierra el resto de
telemetría GPU ni representa implementación de una fuente de fecha positiva.
El comportamiento Windows existente no cambia: allí la fecha del dispositivo
se consulta por su propio proveedor; no se deduce a partir de esta política.

## Fuentes revisadas

| Proveedor / escenario | Qué informa | Política de fecha |
| --- | --- | --- |
| DRM/sysfs, incluidos AMD/Intel/nouveau y otros drivers | Identidad, módulo/versiones según driver; ioctl DRM_VERSION conserva un campo histórico de fecha | No hay fuente de fecha de publicación/instalación validada en este proveedor. Campo DRM obsoleto/constante no se usa |
| NVIDIA NVML | La consulta de sistema usada obtiene versión del driver | No obtiene fecha de publicación/instalación; no parsear una versión como fecha |
| DRM+NVML fusionados por PCI | Se combinan las métricas disponibles de ambos | Nota específica de fuente combinada; no perder el diagnóstico al fusionar |
| Vulkan/OpenCL/EGL/GLX | Versiones API, contexto o información del renderer/driver según la interfaz | No convierten sus versiones o texto informativo en fecha del driver del kernel |
| Proveedor Linux todavía no registrado | No hay contrato de fuente de fecha | Ausencia explícita; no adivinar origen por modelo/nombre o por posición |

DRM actual conserva un placeholder por compatibilidad en su campo date. La
documentación histórica ya advertía que la fecha estática se actualizaba mal.
Por ello ni siquiera una cadena YYYYMMDD de un kernel antiguo demuestra la
fecha del módulo instalado ni la publicación de ese paquete. No se añade una
apertura del dispositivo/ioctl solo para consultar ese campo obsoleto.
Fuentes: [DRM actual, drm_version](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/drm_ioctl.c),
[documentación histórica DRM](https://www.kernel.org/doc/html/v4.18/gpu/drm-internals.html).

La API NVML usada devuelve un identificador de versión, no una fecha; un
string con aspecto de fecha se conserva como versión sin extraer calendario.
[NVML system queries](https://docs.nvidia.com/deploy/nvml-api/latest/api/group__nvmlSystemQueries.html).
CL_DRIVER_VERSION es una versión específica del proveedor; driverInfo Vulkan
es información del driver, sin contrato de fecha de publicación/instalación.
[OpenCL clGetDeviceInfo](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clGetDeviceInfo.html),
[Vulkan driver properties](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceDriverProperties.html).

## Datos que NO se sustituyen

No se infiere driver_date de BIOS/SMBIOS/VBIOS, mtime/ctime/birthtime de archivos,
fecha de instalación del paquete, texto de compilación de módulo, srcversion,
versión del kernel, versión API/gráfica, fecha de documentación o catálogo de
releases descargado. Esas fechas describen objetos/eventos diferentes; una
actualización o backport de distribución también puede conservar versiones.
No se ejecutan modinfo, glxinfo, nvidia-smi ni gestores de paquetes; no red,
helper, caché de fecha, SDK o dependencia nueva en el runtime de la biblioteca.

La política no afirma que ningún driver futuro o API particular pueda ofrecer
una fecha verificable. Agregar una fuente positiva requiere definir qué fecha
representa, vincularla al dispositivo/driver cargado, distinguir soporte/error
y probarla; no basta con encontrar números con forma de calendario.

## Integración y pruebas

Las filas DRM inicializan el diagnóstico antes de fusionar proveedores. NVML
aplica su nota al crear cada fila; una fusión PCI exitosa pasa a la nota de
origen combinado. Filas NVML separadas por ausencia/duplicación de identidad
conservan su fuente. Un fallo de init NVML no transforma ausencia de capacidad
de fecha en permission_required: ese error sigue en las métricas realmente
consultadas, como versión/utilización. No afecta al dato de fecha Windows.

`tests/linux_gpu_driver_date.cpp` cubre políticas DRM/NVML/combinada/desconocida,
ausencia frente a valor conocido, versión normal y cinco formatos incluyendo
texto con apariencia de fecha, fusión exacta, filas separadas/duplicadas,
init denegado y opt-out de GPUs. Las pruebas de lógica corren en Linux/Windows;
solo Linux consulta el Monitor nativo. El consumidor instalado Linux comprueba
valor ausente/support/nota de todas sus filas visibles, sin exigir GPU física.
Sin cambios de API/ABI/layout público ni formato de historial en esta unidad.

Verificación de cierre2026-10-17, revisión20: Linux Release88/88, Windows106/106;
paquetes reinstalados y ambos consumidores externos reconstruidos/relinkados/
ejecutados exit0. WSL0filas GPU en prueba nativa: no confundir ausencia de
dispositivo con cobertura física positiva de driver/fecha. Las pruebas de
política/merge corren con fixtures, sin instalar ni cambiar ningún driver.
Builds de esta unidad sin warnings. Logs en raíz de trabajo:
work/gpu-driver-date-build-{linux,windows}.log,
work/gpu-driver-date-suite-{linux,windows}.log y
work/gpu-driver-date-native-linux.log. G06/G07/N02/M04 siguen separados;
riesgos históricos CPU/historial de N03 no resueltos por esta suite aprobada.
