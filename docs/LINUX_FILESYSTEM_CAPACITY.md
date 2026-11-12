# Capacidad declarada fuera de montajes: E07

Consulta explicita read_linux_filesystem_capacity, independiente de mountinfo.
Lee la extension declarada por un filesystem ext2/3/4 dentro de una fuente y
region seleccionadas. No monta, formatea, repara, ejecuta herramientas externas,
instala dependencias ni eleva privilegios. No se activa automaticamente en Monitor.

## Uso y unidades

```cpp
#include <pcinsight/linux_filesystem_capacity.hpp>

pcinsight::LinuxFilesystemTarget target;
target.path = "/absolute/path/to/filesystem.img";
// offset_bytes indica el COMIENZO del filesystem; default0.
// region_length_bytes ausente usa el resto de la fuente.
auto result = pcinsight::read_linux_filesystem_capacity(target);
if (result.value) {
    const auto bytes = result.value->declared_capacity_bytes;
    // Mostrar tambien support/note: es metadata declarada, no espacio libre.
}
// Para un dispositivo de bloque seleccionado explicitamente por el caller:
// target.path = "/dev/..."; target.allow_block_device = true;

// Alternativa por DiskInfo.id: descubre las particiones del kernel via sysfs.
// false (default) NO realiza sondeos raw. true es consentimiento explicito.
auto disk = pcinsight::read_linux_disk_filesystem_capacity("selected-disk-id", true);
// disk.value->regions contiene disponibilidad por region.
// declared_total_bytes SOLO esta presente si todas las regiones son medibles
// y no se solapan. No es una suma de discos fisicos subyacentes.
```

No deben intercambiarse estas medidas:

| Campo | Significado |
| --- | --- |
| source_size_bytes | Extent logico del archivo fuente, o BLKGETSIZE64 del dispositivo seleccionado |
| declared_capacity_bytes | Bloques declarados por el superbloque por bytes/bloque; incluye metadata del filesystem |
| DiskInfo.formatted_capacity_bytes | Proveedor separado de capacidad visible en filesystems montados via statvfs, con su politica de agregacion |

Un filesystem de32MiB dentro de una fuente de48MiB sigue declarando32MiB.
No se sustituye ese resultado por el tamano de la fuente, el espacio libre,
la capacidad fisica/BIOS o la suma de todas las particiones ocultas.
Se conserva Support::estimated: se conoce la declaracion leida, no se certifica
que todas las estructuras del filesystem sean consistentes ni montables.

El proveedor montado anterior NO cambia de semantica ni empieza a leer raw:
ver LINUX_FORMATTED_CAPACITY.md. El GUI puede usar esta consulta separada para
la region seleccionada y rotular su alcance; no reemplazar ni sumar ambos
campos como si midieran cosas disjuntas. No hay un nuevo total universal por disco.

## Formatos y politica de mapeo

Se admite la familia ext con superbloque primario en offset1024 desde el inicio
de la region, revisiones0/1 y bloques1..64KiB. Se verifica geometria/cotas de
bloques/inodos/grupos, contadores de64bits solo con su feature y descriptor
coherente, dependencias journal/checksum/orphan-file y overflow de producto.
Bigalloc, compression, journal-device como fuente, snapshots/replicas y flags
incompatibles/readonly no conocidos por esta politica quedan ausentes. No se
afirma soporte de todas las opciones ext, XFS, Btrfs, F2FS, FAT, NTFS u otros
formatos desmontados. Features compat desconocidas no se usan para interpretar
estructuras; este lector no actua como fsck ni escribe el filesystem.

metadata_csum exige tipo CRC32C conocido y checksum primario valido. Si no hay
checksum configurado, superblock_checksum_valid esta ausente, NO true. CRC no
es firma/autenticacion. El UUID declarado es opcional en revision dinamica y
no se publica como UUID utilizable si es cero; clones pueden compartirlo.
No se deduplican fuentes por UUID. needs_journal_recovery y
orphan_entries_may_be_present solo exponen flags, no verifican la salud del FS.
No se lee el journal/orphan-file ni se intenta recuperarlos.

Fuentes soportadas: archivo regular de imagen o dispositivo de bloque con
opt-in separado. Puede ser el objeto de un disco sin particionar, una particion
o un dispositivo logico ya expuesto por el kernel. El caller selecciona la
fuente/region; no se adivinan offsets a partir de nombres y no se exploran
automaticamente MBR/GPT ni contenedores cifrados/comprimidos.

kernel_block_device identifica major:minor del objeto logico seleccionado.
disk_inventory_id, cuando sysfs lo resuelve, asocia ese objeto o el padre de su
particion con un ID del inventario /sys/block. Es estimated, no una distribucion
de capacidad sobre discos fisicos. Para DM/LVM/RAID/loop/capas complejas solo se
lee metadata del objeto logico accesible; no se siguen slaves ni se suman sus
soportes. Sin mapeo directo verificable, la asociacion queda desconocida, aunque
la declaracion del filesystem pueda estar presente. Imagen regular NO se asocia
al disco que almacena el archivo mediante st_dev: ese no es su volumen objetivo.

La consulta individual no suma fuentes. read_linux_disk_filesystem_capacity
selecciona el objeto /sys/block por ID y descubre sus particiones con dev/start/
size; si no hay particiones, sondea el objeto completo. No sondea ambos para
sumar dos veces el mismo almacenamiento. Cada probe exige el major:minor
esperado y size/region/asociacion coherentes. Una segunda enumeracion compara
disco/particiones/diskseq y tamanos; cambios invalidan el informe para reintentar.
El orden de enumeracion no altera la comparacion. Hasta128regiones/4096entradas,
sin nombres adivinados por prefijos. Se requieren nombres directos de kernel
soportados y sus nodos /dev; no se traducen nombres exoticos ni crean nodos.

Solo se publica declared_total_bytes si TODAS esas regiones tienen declaracion
valida y no se solapan. Alias/devices duplicados se rechazan; layouts con
particion extendida solapada no producen total. Una sola region desconocida/
inaccesible invalida el total, conservando disponibilidad por region. No se
mezclan mounts, imagenes, UUID clones, ni capas/slaves de otros objetos.
Un total fisico completo para capas complejas sigue siendo desconocido; el total
presente describe UN objeto logico seleccionado, no sus soportes fisicos.
Esta es la politica de mapeos/formatos soportados de E07, no reconstruccion
universal de almacenamiento ni certeza de que un disco este sin formatear.

## Acceso, errores y consistencia

Path absoluto acotado a32768B/sin NUL. O_RDONLY|O_CLOEXEC|O_NONBLOCK|O_NOFOLLOW,
solo regular/block; symlink final, FIFO, directorios y character devices se
rechazan. El opt-in de bloque permite lectura, no otorga privilegios; si el SO
deniega acceso, devuelve permission_required. No se utiliza sudo, polkit ni UAC.
La API no es un helper privilegiado ni sandbox de paths hostiles: el host debe
seleccionar fuentes de confianza y respetar permisos del usuario.

Offset/longitud deben caber en la fuente y en el rango de pread. Se leen dos
superbloques de1024B, se exige igualdad y se revalidan fd/pathname:
dev/inode/rdev/mode/nlink/size/mtime/ctime, capacidad de la fuente y diskseq si
esta expuesto. Se mantiene el fd fijado durante toda la consulta y se cierra
por RAII. Las denegaciones explicitas de diskseq tambien se conservan por campo.

Es comparacion de extremos, NO transaccion ni identidad durable: reemplazos
anteriores al primer open, cambios ABA, alias, resize/caches del kernel o
cambios de ancestros pueden escapar o producir ausencia. En un FS montado la
declaracion puede diferir del estado vivo; no se fuerza flush de caches.
No usar resultado/UUID/diskseq como autorizacion para formatear o modificar.
Una inconsistencia invalida el resultado; no recupera un numero viejo ni usa
tamano de dispositivo como fallback. No hay deadline total de I/O/RSS garantizado.

## Evidencia y compatibilidad

tests/linux_filesystem_capacity.cpp valida parser portable, geometria, unidades,
CRC32C con vector conocido, UUID/revision,64bits/overflow, features desconocidas,
dependencias orphan/journal y cotas; assertions activas en Release.
La politica por disco prueba multi-region conocida, denegacion/partial sin
total, device incorrecto, cambio de diskseq, solapamiento y device duplicado,
con fixtures deterministas. Estas topologias sinteticas no son prueba fisica
de RAID/LVM/particiones de todos los tipos.

tests/linux_filesystem_capacity_native.cpp crea imagenes privadas regulares
ext2/ext3/ext4 reales de32MiB como usuario normal, sin montarlas. SOLO el harness
usa mke2fs si ya esta instalado para generar fixtures, no para obtener datos.
La libreria distribuida no depende de ese programa y nunca lo ejecuta. No se
instala si falta: CMake omite ese target, sin afirmar positivo nativo.
Se prueba el API publico:32MiB declarados dentro de fuente48MiB, permisos,
symlink/FIFO/offset/region y checksum corrupto. Un fixture de subregion copia
solo metadata de una imagen real; NO se presenta como un segundo FS completo.
Solo se formatean archivos exclusivos en mkdtemp, nunca dispositivos reales.

Modo manual --read-root-block-metadata fuera de CTest: resuelve el dispositivo
actual de / via sysfs/stat y prueba el opt-in y lectura readonly. WSL expuso
8:48 -> sdd,1099511627776B declarados y de fuente, checksum valido. No se cambio
ningun montaje, filesystem, configuracion ni archivo de usuario; solo se leyo
metadata primaria. Primer rechazo conservador ro_compat=66667 incluia
ORPHAN_PRESENT: se verifico la especificacion y se anadio con sus dependencias,
sin relajar el rechazo de flags desconocidos/checksum. Diagnostico preservado
en work/filesystem-capacity-native-block-diagnostic-linux.log.
En el modo manual se contrasta tambien el API por disco SOLO si el root es un
objeto sin particiones: se repiten dos lecturas del mismo superbloque para su
region unica. Si se detectan particiones no se amplia ese test a otros destinos.

API adicional linux_filesystem_capacity.hpp; sin cambio a layouts existentes,
ABI virtual, SystemSnapshot ni historial. Windows prueba parser portable y
ausencia del lector nativo Linux; su proveedor de volumenes no se sustituye.

Verificado2026-10-19: Linux108/108 y Windows119/119 Release aprobadas; imagenes
nativas y dispositivo real WSL exit0. Consulta por disco via sysfs -> region
guardada -> suma completa logica tambien positiva en sdd sin particiones, sin
leer otros destinos. Consumidores instalados recompilados/relinkados/ejecutados
ambos exit0; builds finales sin warnings. Logs work/filesystem-capacity-*.
E07 P->C por proveedores/formatos/mapeos admitidos y politica de desconocidos
verificados, no por unicamente unsupported. N02 fisico y N03 siguen abiertos.

Referencias: [superbloque y flags ext del kernel](https://www.kernel.org/doc/html/latest/filesystems/ext4/super.html),
[checksums ext](https://www.kernel.org/doc/html/latest/filesystems/ext4/checksums.html).
