# Capacidad de sistemas de archivos montados en Linux

Unidad: corrección del proveedor de DiskInfo.formatted_capacity_bytes. El campo
se conserva por compatibilidad, pero en Linux su valor es **una estimación de la
suma de sistemas de archivos montados, visibles y asociables**, no una lectura
universal de toda la capacidad formateada. El caller debe mostrar ese alcance.
No se inspeccionan sistemas de archivos desmontados ni se leen dispositivos raw.

Actualizacion2026-10-19: la API explicita separada de E07 lee metadata primaria
de regiones ext2/3/4 seleccionadas, montadas o no; ver LINUX_FILESYSTEM_CAPACITY.md.
NO cambia el campo/proveedor montado descrito aqui, ni suma capas/aliases como
un total fisico universal. Capacidad declarada incluyendo metadata y statvfs
visible no son conceptos intercambiables.

Antes, un disco sin volumen asociado publicaba0 available, indistinguible de una
capacidad realmente medida como cero. Se sumaban productos sin comprobar overflow
y se descartaba un alias antes de saber si su statvfs había funcionado. Ahora:

- Sin medición asociable, formatted_capacity_bytes está ausente con nota, no0.
- f_blocks*f_frsize y la suma por disco tienen comprobaciones uint64; f_frsize0
  es inválido. Un producto válido0 conserva su valor opcional0.
- Se agrupa por dispositivo de filesystem major:minor, sin sumar de nuevo bind
  mounts/alias de ese mismo dispositivo. Se normalizan major/minor decimales.
- Se intenta otro alias si una ruta falla. Dos alias que dan capacidades distintas
  o tipos de filesystem incompatibles invalidan ese dispositivo.
- Si un dispositivo de filesystem ya asociado al disco no se puede medir, no se
  publica como total la suma de sus otros dispositivos asociados accesibles.

El resultado correcto tiene Support::estimated y nota de alcance. Fallos de
acceso nativos EACCES/EPERM se conservan como permission_required; otros fallos
de lectura, conflicto, rango o topología no soportada quedan unsupported.
No se publica un vector/mapa parcial por un error de parseo de mountinfo.

## Fuente, asociación y lectura nativa

La lista viene de /proc/self/mountinfo, en la vista del proceso llamador. Se
validan los campos obligatorios, el separador y sus campos finales; los opcionales
desconocidos no se interpretan. Rutas absolutas decodifican040/011/012/134 en una
pasada: un backslash decodificado no inicia otra sustitución. NUL, escapes no
reconocidos o estructura inválida se rechazan.
[Formato oficial mountinfo](https://man7.org/linux/man-pages/man5/proc_pid_mountinfo.5.html).

La asociación sigue /sys/dev/block/major:minor. Una partición se asocia a su
padre mediante la presencia de partition; se exige que el padre coincida con
un elemento de /sys/block. No depende de encontrar un componente literal block
en la ruta canónica: esa suposición no cubre todos los diseños sysfs. No distribuye
capacidad de device-mapper/RAID entre discos físicos ni suma capas subyacentes.
Dispositivos no asociables se omiten del mapa; eso no demuestra que el disco
físico no tenga otros sistemas de archivos ni que esté sin formatear.

Cada consulta abre la ruta con O_PATH|O_CLOEXEC (sin lectura de contenido ni
creación), comprueba st_dev con fstat frente a mountinfo y calcula capacidad
con fstatvfs del mismo descriptor. Un overmount visible con otro dispositivo se
rechaza; el descriptor se cierra por RAII también ante errores. No hay una llamada
statvfs por ruta después de validar otro objeto distinto.
[Referencias de open/O_PATH](https://man7.org/linux/man-pages/man2/open.2.html),
[fstatvfs y unidades de capacidad](https://man7.org/linux/man-pages/man3/statvfs.3.html).

Tipos admitidos inicialmente:ext2/ext3/ext4, vfat/msdos/exfat, ntfs/ntfs3,
iso9660/udf y squashfs. Es una política conservadora del proveedor, no una
afirmación de que otros sistemas no publiquen statvfs. XFS, Btrfs, F2FS, EROFS,
bcachefs y tipos desconocidos necesitan cobertura/proveedor de topología propio;
si están asociados a un disco se deja su agregado ausente. tmpfs/overlay y otros
mounts sin asociación de bloque no se convierten en capacidad del disco raíz.
No se requiere instalar aplicaciones para ninguno de estos caminos.

## Límites y compatibilidad

Hasta16MiB de mountinfo,100000filas,65536B por fila,1024campos por fila y16384B
por ruta codificada. Lector nativo acotado antes de acumular más de16MiB. Límite
de identidad de disco4096B sin slash/backslash/NUL. No es límite de RAM total:
se conservan copias, mapas y filas; consultas síncronas no tienen deadline global.
Un filesystem no responde dentro de un plazo garantizado solo por usar O_PATH.

Sin nueva API pública ni cambio de formato persistente. Windows conserva su
proveedor de volúmenes. Callers Linux deben comprobar valor/Support/nota: ausencia
ya no significa0B. La prueba de coincidencia de dispositivo no es identidad
duradera ni snapshot atómico de mountinfo, sysfs y todas las capacidades. Resize,
reuso de dispositivos o cambios concurrentes pueden obligar a volver a muestrear.

Se estima capacidad total del filesystem visible, no espacio libre ni capacidad
bruta/BIOS. Cuotas y vistas de montaje pueden influir en lo publicado. No se
reconstruyen volúmenes ocultos/desmontados ni capacidades de todos los soportes
físicos. La suma puede diferir del tamaño de particiones. system_disk y page file
mantienen aquí el código legacy: su seguridad semántica sigue pendiente aparte.
Actualización posterior: LINUX_SYSTEM_DISK.md corrige la asociación directa de /;
page file y topologías adicionales conservan pendientes separados.

## Verificación

2026-10-15: Release Linux62/62 y Windows82/82; paquetes reinstalados y consumidores
externos reconstruidos/ejecutados exit0. Nuevo test linux-formatted-capacity con
assertions activas: estructura/escapes y normalización, deduplicación de aliases,
reintento tras denegación, capacidades conflictivas/tipos discordantes, rechazo
de suma parcial, cero medido frente a ausencia, overflow de producto/suma,
fragmento0, proveedores incompletos, topologías no cubiertas y límites de
texto/filas e identidades inválidas. Input inválido falla antes de consultar filesystems.

Nativo Ubuntu WSL: lectura fstatvfs de referencia O_PATH de / contrastada contra
statvfs, rechazo deliberado de dispositivo incorrecto y ruta relativa. Monitor
publicó1fila de disco medida estimated y27ausentes con nota (no27ceros). Sin mounts,
umounts, lecturas raw, cargas de disco ni elevación. Windows ejecuta fixtures
portables y regresiones de su backend, no captura mountinfo nativa. Denegaciones,
conflictos y topologías múltiples son fixtures, no cambios en montajes reales.
Otros kernels/libcs y asociación física NVMe/RAID adicional requieren cobertura.
