# Windows: volúmenes, sistema y page files

Implementado 2026-10-13 mediante APIs nativas de volumen y PSAPI. El proveedor
se activa con include_disks y publica campos del inventario de discos. No
modifica particiones, montajes, archivos ni configuración de paginación.

## Capacidad formateada

FindFirstVolume/FindNextVolume enumera nombres GUID; cada volumen se abre
sin solicitar acceso de escritura ni lectura de archivos y se consulta
IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS. Las extensiones identifican sus discos.
Se suma una sola vez por nombre de volumen, sin depender de letras/montajes.

Para un volumen NTFS accesible, FSCTL_GET_NTFS_VOLUME_DATA aporta TotalClusters
y BytesPerCluster. Su producto mide capacidad de clusters del sistema de
archivos, independientemente de cuotas; no equivale al tamaño bruto del disco
ni a todos los bytes asignados a la partición. Se valida multiplicación/suma.

Si esa consulta no está disponible, GetDiskFreeSpaceEx proporciona capacidad
visible para el usuario. Puede estar limitada por cuotas; si contribuye a la
suma, formatted_capacity_bytes se marca **estimated**. Si todos los volúmenes
contribuyentes usan datos NTFS, se marca available con esa definición explícita.
Disco sin volúmenes observados y con inventario completo produce cero.

Un volumen que abarca varios discos deja la capacidad de los discos afectados
sin dato: no se reparte un tamaño lógico arbitrariamente entre dispositivos.
Volumen inaccesible, buffer inválido, fallo de enumeración, overflow o suma
superior a capacidad física impide presentar una suma parcial como total.
Virtualización/thin provisioning/snapshots pueden necesitar más metadatos:
no se promete equivalencia exacta con capacidad de asignación física.

## Indicadores

system_disk identifica los discos de la extensión del volumen que contiene
GetWindowsDirectory. **No clasifica la partición EFI/boot del firmware**.
Solo ofrece false cuando la consulta de ese volumen y su asociación tuvo éxito.

contains_page_file usa EnumPageFilesW para los page files instalados y obtiene
sus volúmenes mediante GetVolumePathName/GetVolumeNameForVolumeMountPoint.
Se normaliza el prefijo DOS-device cuando PSAPI lo entrega. Un page file
confirmado produce true. False exige enumeración y asociación completas; fallos
de acceso o rutas que no se pueden resolver dejan desconocido. No se infiere
la presencia de page file a partir de su nombre predeterminado ni del registro.

Los indicadores se consultan independientemente de capacidad. Un fallo de
filesystem no borra una asociación sistema/page file ya confirmada. Los valores
representan las lecturas observadas; volumen/paginación/hotplug pueden cambiar
entre consultas, que no forman una transacción atómica.

## Límites y pruebas

Hasta 4096 volúmenes, 2048 extensiones por consulta en buffer de 64 KiB,
1024 page files y rutas hasta 32768 caracteres. Se validan tamaños, offsets,
extensiones no negativas y duplicados; no se adivina reparto de RAID/MPIO.
Handles de búsqueda/volumen se cierran en todas las rutas. Las excepciones
del callback se capturan dentro del callback sin cruzar la frontera Win32.
Sin número de dispositivo único, el disco conserva métricas sin dato.

Fixtures cubren suma y fallback estimado, NTFS independiente de permisos de
GetDiskFreeSpaceEx, cero sin volúmenes, volumen multidisco, permisos/errores,
inventario fallido a mitad, buffers malformados, nombres duplicados, page file
ausente y cierre/caché de handles. Lectura real en el equipo local: capacidades
en 5/5 discos, un disco del volumen Windows y un disco con page file.
Windows Release 19/19; Ubuntu WSL 6/6. No se modificaron volúmenes/page files.

Actualización2026-10-19: WMI positivo5/5discos, sin UAC/cambios de permisos.
M03 cierra proveedores soportados con evidencia en WINDOWS_NETWORK_IDENTITY.md.
El reparto de capacidad multicapa no se adivina; otras topologías/filesystems/
cuotas/permisos/hotplug físicos no se anuncian verificados por estos positivos.

Referencias: [extensiones de volumen](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_volume_get_volume_disk_extents),
[datos NTFS](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-ntfs_volume_data_buffer),
[capacidad y cuotas](https://learn.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getdiskfreespaceexw),
[page files instalados](https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-enumpagefilesw).
