# Windows: inventario de dispositivos de disco

Implementado 2026-10-13. SetupAPI enumera GUID_DEVINTERFACE_DISK presente;
las rutas se abren mediante CreateFileW con acceso solicitado cero y sharing
de lectura/escritura. Solo se realizan consultas de propiedades/geometry.
No se monta, formatea, particiona ni escribe en discos.

`SampleOptions::include_disks=true` activa el inventario; false evita todas
estas consultas. Por ahora se vuelve a enumerar en cada muestra, para reflejar
dispositivos presentes. No es un snapshot atómico frente a hotplug.

## Datos disponibles

- id: ruta de interfaz de dispositivo devuelta por Windows, sin interpretarla
  ni sustituirla por un índice PhysicalDrive potencialmente reutilizado.
- model: VendorId/ProductId del STORAGE_DEVICE_DESCRIPTOR, cuando están
  presentes y son cadenas ASCII válidas. No se verifican fabricante o firma.
- bus_type: SATA, SAS, SCSI, ATA, NVMe, USB, RAID, Virtual, File-backed virtual
  o Storage Spaces según el descriptor. Valores no reconocidos quedan vacíos.
- capacity_bytes: tamaño OS-reportado del dispositivo mediante
  IOCTL_DISK_GET_DRIVE_GEOMETRY_EX. No es capacidad formateada de volúmenes
  ni capacidad física bruta de los chips de un SSD.
- media_type: `seek-penalty` o `no-seek-penalty` cuando el controlador publica
  la propiedad IncursSeekPenalty. Son etiquetas literales de esa propiedad;
  no se presentan como clasificación garantizada HDD/SSD. Un bus NVMe o soporte
  de TRIM por sí solo no se usa para inventar el medio físico.

Monitor añade la clasificación reportada HDD/SSD/SCM mediante el proveedor
descrito en WINDOWS_DISK_MEDIA.md. Cuando se obtiene, media_type refleja ese
tipo; reported_media_type añade soporte y diagnóstico por permisos/ausencia.

La enumeración representa endpoints de disco del OS; RAID, virtualización o
controladores pueden ocultar discos integrantes. Un dispositivo inaccesible
conserva su id con métricas ausentes; capacidad muestra permission_required
si la apertura/consulta falla por acceso denegado. Modelo/bus/medio vacíos
no distinguen ausencia de configuración de un fallo de lectura.

## Integridad y recursos

Fallo global de enumeración/detalle o registro inválido lanza system_error
sin publicar inventario parcial. Máximo 4096 interfaces; detalle hasta 64 KiB;
descriptor hasta 1 MiB. Se validan conteos/tamaños retornados, terminación de
cadenas y offsets; no se lee fuera de los buffers proporcionados. Handles
de disco y conjunto SetupAPI se cierran en todas las rutas. No se ejecutan
utilidades externas ni se solicitan privilegios adicionales.

La consulta de geometry usa un buffer de 4096 bytes; si un controlador exige
más o no acepta la consulta, capacidad queda ausente. Los IOCTL individuales
fallidos dejan sus campos sin dato sin eliminar el dispositivo. No se promete
un timeout propio para llamadas síncronas de drivers.

## Verificación y pendientes

Fixtures inyectados: propiedades y capacidad, tamaño/path inválido, descriptor
con offset fuera de rango, apertura denegada, geometry denegada/negativa,
fallo de enumeración posterior y cierre de recursos. Lectura nativa local:
5 dispositivos, tres SATA y dos NVMe, capacidades de aproximadamente
500 GB, 500 GB, 4 TB, 1 TB y 1 TB. No se alteró ningún disco.
Windows Release 19/19 y Ubuntu WSL 6/6. Rendimiento dinámico ya implementado
mediante proveedor PDH separado; ver WINDOWS_DISK_PERFORMANCE.md.

La asociación de volúmenes, capacidad de filesystem y system disk/page file
están implementadas en WINDOWS_VOLUMES.md, con soporte/ausencia explícitos.
Clasificación WMI implementada; actualización2026-10-19: positiva5/5discos
(2HDD/3SSD,SATA/NVMe) sin UAC/cambios de permisos. El acceso denegado previo
es evidencia histórica, no estado actual. Ver WINDOWS_DISK_MEDIA.md y
WINDOWS_NETWORK_IDENTITY.md para cierre M03 y límites/topologías no verificadas.
No se usa IOCTL_DISK_PERFORMANCE:
el proveedor dinámico consulta PDH y documenta sus límites. M03 cierra los
proveedores soportados, no una garantía de todas las topologías físicas.

Referencias: [SetupDiGetDeviceInterfaceDetailW](https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdeviceinterfacedetailw),
[descriptor de almacenamiento](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-storage_device_descriptor),
[IOCTL de propiedades](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_storage_query_property),
[geometry](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_drive_geometry_ex).
