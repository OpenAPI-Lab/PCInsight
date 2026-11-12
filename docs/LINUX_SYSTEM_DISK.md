# Indicador system_disk Linux con desconocido explícito

Unidad: corrección de DiskInfo.system_disk en Monitor/include_disks. En Linux
significa el dispositivo de bloques asociado directamente al sistema de archivos
montado en / en la vista del caller. No es partición EFI, ubicación del bootloader,
/boot, disco de arranque de firmware ni todos los soportes físicos de una pila.
No cambia API/layout público, no monta/desmonta ni escribe dispositivos.

Antes, al fallar la identificación del root, root_disk quedaba vacío y se devolvía
false available para todos los discos. Eso confundía «no identificado» con «no es
el disco del sistema». Ahora se conserva la incertidumbre por Metric:

- Con asociación conocida y exactamente una fila de inventario coincidente,
  esa fila es true y las demás false, con Support::estimated y nota de alcance.
- Si no se identifica / o la topología no está cubierta, todas quedan ausentes
  con nota, nunca una colección de false inventados.
- Si el root asociado falta o está duplicado en el inventario, las filas quedan
  ausentes para que el consumidor vuelva a muestrear, no se publica un «no» global.
- Una denegación explícita del proveedor de root se conserva permission_required;
  otros fallos de identificación se indican unsupported. Sin permisos no se eleva.

Comprobar system_disk.value.has_value() antes de interpretar el bool almacenado;
false presente significa «no es el dispositivo root directamente asociado».
No presentar ausencia como false en la GUI.

## Asociación y comprobaciones

Se reutilizan el lector acotado /proc/self/mountinfo, su parser de escapes y
el resolver de bloques de LINUX_FORMATTED_CAPACITY.md. El código de capacidad
montada usa los mismos helpers; su semántica de capacidad no cambia por esta unidad.

El root se abre O_PATH|O_CLOEXEC; fstat del mismo descriptor obtiene st_dev,
y RAII lo cierra. No se lee contenido del root ni se ejecutan comandos externos.
Las filas mountinfo cuyo punto de montaje decodificado es / se contrastan con
ese dispositivo. Se ignoran otras filas root de diferente dispositivo que no
coinciden con la referencia actual; conflicto de tipos en candidatas coincidentes
falla cerrado. Falta de coincidencia indica cambio/vista inconsistente, no false.
[Referencia mountinfo/st_dev](https://man7.org/linux/man-pages/man5/proc_pid_mountinfo.5.html),
[open/O_PATH](https://man7.org/linux/man-pages/man2/open.2.html).

Se exige la misma política conservadora de tipos de filesystem que la capacidad
montada:ext2/3/4, vfat/msdos/exfat, ntfs/ntfs3, iso9660/udf y squashfs. Otros tipos
(incluidos XFS/Btrfs/F2FS y overlay/tmpfs) requieren un proveedor/cobertura propios.
No afirma que el kernel no tenga información sobre ellos; este proveedor no
publica una asociación single-device que no pueda justificar con su contrato.

El dispositivo se sigue desde /sys/dev/block/major:minor, y una partición se
asocia al padre mediante partition. Se verifica coincidencia con /sys/block,
sin depender de un componente literal block en la ruta. Device-mapper puede
identificarse como dispositivo lógico directo; no se atribuye a sus slaves.
Nombre devuelto vacío, con slash/backslash/NUL, punto/punto-punto o más de4096B
es inválido. La identidad root numérica debe tener major/minor uint32 completos.

## Límites

Hereda límites de mountinfo:16MiB totales,100000filas,65536B por fila,1024campos
y16384B por ruta codificada. Valida todo el texto antes de consultar root o el
resolver; un parseo fallido no publica una asociación parcial. No hay caché del
root entre muestras ni un nuevo hilo de fondo.

No es snapshot atómico entre mountinfo, fd de /, sysfs y el inventario de discos.
Nombres/números pueden reutilizarse; cambios rápidos pueden requerir otro poll.
O_PATH fija la referencia del root para fstat, no la identidad física durante
toda la captura. Chroot/contenedores/namespaces pueden tener otro root que el
host; no se afirma que el resultado sea el disco del sistema host. No hay deadline
global ni garantía universal ante hotplug. Bootloader/EFI, multi-device/RAID y
topologías fuera de la lista no quedan cerrados. contains_page_file conserva
su alcance independiente; la unidad posterior LINUX_SWAP_DISKS.md corrige el
proveedor activo con disponibilidad explícita y límites de topología propios.

## Verificación

2026-10-15: Release Linux63/63 y Windows83/83, instalación y consumidores externos
reconstruidos/ejecutados exit0. Assertions activas en nuevo test linux-system-disk:
asociación root, candidatas ocultas, ausencia de /, mismatch de dispositivo,
topologías/tipos no cubiertos, denegación, identidad/API inválida, límites de
texto/filas, sí/no/ausente y root faltante/duplicado en inventario. Lecturas root
no ocurren cuando el parser falla; no hay fallback que publique false global.

Ubuntu WSL nativo: root sdd,1true y27false estimated,0unknown; st_dev de referencia
root contrastado con stat, integración Monitor y opt-out verificados. Solo lecturas.
Windows ejecuta fixtures portables, no captura root Linux nativa. No se probaron
overmounts reales, permisos root denegados, chroot ni hardware NVMe/RAID físico;
los fallos de topología/ambigüedad se cubren con entradas inyectadas.
