# Windows GPU: fecha del controlador y ubicación PCI

Unidad verificada 2026-10-14 (America/La_Paz). `Monitor` completa los campos
existentes `GpuInfo.driver_date` y `physical_location` después del inventario
DXGI, sin herramientas externas, WMI, lectura del BIOS ni instalación adicional.

## Identidad y datos

SetupAPI enumera interfaces presentes `GUID_DISPLAY_DEVICE_ARRIVAL`.
`D3DKMTOpenAdapterFromDeviceName` traduce cada ruta al LUID exacto de DXGI;
no se compara el modelo, vendor/device ID ni el orden de enumeración.
Las propiedades se leen del `SP_DEVINFO_DATA` devuelto para esa misma interfaz.
Se vuelve a abrir la ruta para comprobar su LUID antes de aceptar los datos.
Los handles KMT, el conjunto SetupAPI y Gdi32 se liberan por RAII.
Gdi32 se carga únicamente desde System32. Se usa un subconjunto ABI público
documentado, con comprobaciones de offsets; no hace falta instalar el DDK.

- `driver_date`: propiedad FILETIME DriverDate, convertida a `YYYY-MM-DD` UTC.
  Es la fecha INF DriverVer del controlador instalado, no instalación, descarga,
  firma, compilación ni firmware. Una FILETIME cero válida no significa ausencia.
- `physical_location`: solo para BusTypeGuid PCI. BusNumber y Address de tipo
  INT32 no negativo o UINT32, con tamaños exactos. Address contiene device en
  high word y function en low word; se validan device <=31 y function <=7.
  Formato `PCI bus 3, device 0, function 0`. No se inventa segmento PCI ni se
  confunde el identificador con una ruta persistente: el bus puede cambiar.

Cada muestra vuelve a enumerar; no hay caché permanente que oculte hotplug.
Fechas inválidas, otro bus o propiedades ausentes quedan sin valor. Un error
de propiedad afecta solo al campo correspondiente. Denegaciones se distinguen
como `permission_required`. No se eleva ni lanza helpers automáticamente.

## Fallos y límites

Se aceptan hasta 256 interfaces, rutas de hasta 64 KiB, estructuras alineadas,
tamaños y terminación NUL comprobados. Una enumeración incompleta o una ruta
que no puede vincularse invalida todos los metadatos de esa consulta: no se
publica un subconjunto que pudiera ocultar identidades duplicadas. No se elimina
el inventario DXGI ni se invalidan los otros proveedores de telemetría.
Dos nodos PnP diferentes con el mismo LUID son ambiguos: sus campos quedan
desconocidos. Repeticiones del mismo nodo se deduplican. El muestreo entre APIs
no es una transacción atómica frente a cambios de hardware/controlador.

## Verificación

Prueba `windows-gpu-devices`: identidad distinta/no encontrada, LUID compartido
entre nodos, permisos de conjunto/propiedad/KMT, error al terminar enumeración,
ruta no terminada, tamaños/tipos incorrectos, fecha fuera de rango, bus no PCI,
device fuera de rango, negativos, ceros válidos, cambio de LUID, límite de
interfaces, API ausente y cierres de recursos incluso en fallos.
Incluye consulta nativa y paso por Monitor. En AMD Radeon RX 7900 XT se obtuvo
`2026-08-17`, `PCI bus 3, device 0, function 0`. Son observaciones del equipo,
no constantes ni garantía de presencia en todos los controladores.

Regresión Release Windows 49/49 y Linux 35/35; consumidores externos de los
paquetes instalados reconstruidos/ejecutados en ambos sistemas. No se ha
validado hotplug físico, linked adapters reales ni hardware Intel/NVIDIA aquí.

## Fuentes primarias

- [Interfaz display y correspondencia KMT](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/guid-display-device-arrival)
- [ABI OpenAdapterFromDeviceName](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/ns-d3dkmthk-_d3dkmt_openadapterfromdevicename)
- [Cierre KMT](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/nf-d3dkmthk-d3dkmtcloseadapter)
- [DriverDate](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/devpkey-device-driverdate)
- [Codificación de bus/device/function](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/obtaining-device-configuration-information-at-irql---dispatch-level)
