# Swap activo por dispositivo Linux

Unidad: proveedor de DiskInfo.contains_page_file en Monitor/include_disks. En
Linux el nombre legacy significa **áreas de swap activas asociadas directamente
al dispositivo**: incluye particiones y archivos admitidos. No busca archivos
inactivos ni prueba la configuración del próximo arranque. No es un pagefile de
Windows ni contabilidad de bytes de swap por app. Sin nueva API/layout público.

Actualizacion2026-10-19: E05 cerrado tras positivo con archivo swap REAL activo
en ext4/WSL. Se conserva el alcance documentado: particiones directas y archivos
ext2/3/4, asociacion estimated, sin soporte universal de topologias/filesystems.
La evidencia de2026-10-16 mas abajo es historica, no el estado vigente.

Antes solo se consultaba stat sobre entradas block-device; los archivos se
ignoraban y rutas/mapeos no identificados acababan como false para todos los
discos. Ahora se conserva disponibilidad y se distingue sí/no/desconocido.

Actualizacion2026-10-16 (E05 E->E, sin cierre): referencia de archivo con tamano
comprobado y revalidacion de metadatos/ruta tras resolver el disco. Flujo nativo
de archivo regular probado con inventario SINTETICO. El entorno solo tiene swap
en particion /dev/sdc; sigue pendiente archivo swap activo real end-to-end.
No se activo/desactivo swap ni se presume aceptado un alcance menor.

## Captura y estado

Fuente /proc/swaps, con parseo completo antes de consultar rutas. Se requieren
header Filename/Type/Size/Used/Priority y cinco campos por entrada. Tipos admitidos
file/partition, tamaño/uso uint64 sin signos, uso no mayor que tamaño y prioridad
int32. Rutas absolutas decodifican040/011/012/134 una sola vez; NUL, escapes
desconocidos, tipos desconocidos y rutas duplicadas se rechazan. No hay salidas
parciales por errores de parseo.
[Manual proc_swaps](https://man7.org/linux/man-pages/man5/proc_swaps.5.html),
[fuente kernel de referencia v6.12, swap_show](https://github.com/torvalds/linux/blob/v6.12/mm/swapfile.c).

Tras resolver las rutas, se vuelve a leer la lista. Se exige la misma configuración
(ruta/tipo/tamaño/prioridad), sin depender del orden. Used puede variar: no es un
cambio de configuración. Si la segunda lectura falla, es inválida o difiere,
el informe queda ausente para volver a muestrear, sin publicar flags parciales.
Dos observaciones coincidentes NO forman una transacción ni prueban continuidad
entre ellas; swapoff/swapon intermedios pueden ser invisibles.

Con informe estable:

- Un disco único identificado entre los destinos conocidos obtiene true estimated.
- Los otros obtienen false estimated solo si todas las entradas se resolvieron,
  sus discos aparecen inequívocamente en el inventario y la política de capas lo
  permite. Lista válida sin áreas activas permite false para todos los IDs válidos.
- Si una entrada falla por permisos, ruta, topología o asociación, los positivos
  conocidos se conservan y los demás quedan ausentes, no false. Denegación
  explícita produce permission_required para los desconocidos; resto unsupported.
- Discos de destino faltantes/duplicados impiden negativos globales. Un ID de
  inventario duplicado o inválido tampoco se publica como positivo.

En nativo, un dispositivo cuya ruta sysfs incluye virtual, con slaves visibles
o con consulta de capas fallida, no permite excluir los otros discos. Mantiene
el positivo directo y deja los demás desconocidos. No reparte swap entre discos
físicos de RAID/LVM/device-mapper/loop ni promete descubrir todos los soportes
ocultos por drivers. Los flags siguen siendo estimados y su nota explica alcance.

## Asociación nativa y límites de identidad

Se abre cada pathname con O_PATH|O_CLOEXEC y se usa fstat del mismo descriptor.
file exige archivo regular y toma st_dev; partition exige block-device y toma
st_rdev. Nunca se abre un dispositivo raw para leer sus datos. Un tipo diferente
al listado se rechaza. Descriptor cerrado por RAII, también ante errores.

En archivos, size_kib*1024 debe caber en uint64 y no superar st_size no negativo.
Es un limite inferior de coherencia, NO tamano exacto con header, prueba de
extents asignados, ausencia de holes, firma swap ni elegibilidad para activacion.
Used>Size y resolver interno ausente tambien se rechazan en consulta directa.
Al terminar el mapeo se repiten fstat del fd y fstatat del pathname: deben
coincidir dev/inode/mode/rdev/size/nlink/mtime/ctime, incluidas fracciones ns.
No se compara atime. Desaparicion/sustitucion/truncamiento/cambio de metadatos
durante la consulta deja ausencia, no positivo ni negativo inventados. Un
resultado de resolver con identidad de disco invalida tampoco se publica.

Soporte inicial de archivos: familias ext2/ext3/ext4 identificadas por fstatfs
del mismo descriptor. Otros filesystems, incluidos XFS/Btrfs/F2FS, quedan
desconocidos hasta un proveedor de topología específico. Las particiones no
necesitan un filesystem montado. Se reutiliza resolve_linux_block_device:
/sys/dev/block/major:minor, padre de partición y coincidencia con /sys/block,
sin asumir que el nombre contenga sd/nvme ni una ruta con componente literal block.

Importante: /proc/swaps publica nombres, no un token inode verificable del objeto
swap activo para compararlo con el fd. El fd fija **la ruta que se abrió**, no
demuestra que sea el inode que el kernel usa como swap. Renombrados, sustitución
de rutas, namespace/chroot o reuso de dispositivos pueden causar ausencia o una
asociación estimada incorrecta. Las dos listas iguales no eliminan esa carrera.
No usar este informe como autorización de acciones destructivas ni identidad de
seguridad. El caller debe mostrar estimated, conservar notas y volver a muestrear.

La revalidacion de2026-10-16 detecta cambios entre sus extremos, pero NO prueba
que una sustitucion anterior al primer open corresponda al inode activo original
ni detecta todos los cambios ABA. El fd no se usa como permiso para swapoff u
otra accion. El callback de resolver anadido es interno y permite forzar carreras
en pruebas; la ruta de produccion sigue usando resolve_linux_block_device.

No instala dependencias ni llama swapon/swapoff, helpers, shells, montajes o
elevación. No lee contenido de archivos swap ni otros archivos usados como
referencia. No hay nuevas escrituras automáticas ni persistencia de rutas.
Esto describe al proveedor de produccion. La prueba MANUAL autorizada descrita
abajo si inicializa y activa exclusivamente su archivo privado.

## Límites y compatibilidad

Inventario acotado a16MiB al leer,100000entradas,64KiB por header/fila,16KiB por
ruta codificada. IDs de disco hasta4096B, sin NUL/slashes/punto/punto-punto.
Se valida todo antes de las consultas de referencias. Flags calculados con
índice de conteos, sin bucle cuadrático por cada disco. Estos límites no son un
presupuesto total de RAM ni deadline de syscalls; hay copias y conjuntos de claves.

El consumidor Linux debe comprobar Metric.value.has_value(): un false presente
no es lo mismo que desconocido. Algunos casos antes false available pasan a
ausente o estimated: corrección de semántica, no pérdida de una medición fiable.
Windows mantiene su proveedor de page files. Sin cambio de formato de historial.
La política de capas es conservadora; no hay una promesa universal de ubicación
física, ni validación de todos los filesystems/drivers/entornos Linux.

## Verificación

2026-10-15: Release Linux64/64 y Windows84/84; paquetes reinstalados y consumidores
externos reconstruidos/ejecutados exit0. Nuevo test linux-swap-disks con assertions
activas en Release: header/escapes/tipos/números/rangos, rutas duplicadas y límites
de entrada/filas/rutas, configuración repetida/cambiada, variación de Used/orden,
segunda lectura fallida, API incompleta y mapa inaccesible/inválido. Flags sí/no/
ausente, positivos conservados, falta/duplicado de destino en inventario, lista
vacía conocida y capas sin negativos inferidos probados con fixtures.

Ubuntu WSL nativo: inventario activo existente produce1true y27false estimated,
0unknown en esta pasada; Monitor/opt-out verificados. Prueba de ruta relativa,
tipo character-device rechazado y filesystem proc no soportado. Una referencia
ordinaria /etc/passwd se abrió SOLO para metadatos y se asoció a sdd mediante su
st_dev; no se leyó su contenido ni se afirmó que fuera swap activo. No se creó
ni activó ningún archivo de swap. Por tanto no se afirma validación end-to-end
de un archivo swap activo nativo nuevo: el flujo file se cubre con fixtures y
sondeo de referencia regular. Otros filesystems/RAID, swaps renombrados, permisos
nativos denegados y otros kernels/libcs requieren cobertura adicional.
Windows ejecuta fixtures portables y sus regresiones, no captura Linux nativa.

2026-10-16: Linux82/82,Windows102/102 Release, paquetes reinstalados y
consumidores externos relinkados/ejecutados exit0. Sin nuevo layout/API publica,
sin warnings en build Linux final. Nuevo tests/linux_swap_path.cpp:
archivo privado ordinario de2MiB en /tmp ext4, referencia8:48->sdd, flujo
sample_linux_swap_disks + lector nativo + flags usando inventario SINTETICO.
Verifica sustitucion de pathname, truncamiento, unlink, chmod y escritura
durante resolver; size menor/overflow/Used>Size, tipo incorrecto/FIFO/resolver
ausente/ID invalido. Incompleto borra positivo anterior y deja unknown, no false.
En otro filesystem o sin mapeo directo, el positivo se omite explicitamente y se
comprueba ausencia; esta pasada WSL SI ejecuto el caso positivo de referencia.

NO es prueba de archivo swap activo: no se ejecuto swapon, swapoff, mkswap,
montajes ni lectura de contenido de swap. Solo se escribieron fixtures privados
ordinarios, eliminados por el test. Inventario real WSL: /dev/sdc partition,
4194304KiB,used0,priority-2. E05 sigue E hasta probar archivo activo real y
topologias admitidas relevantes. No se declara resuelto el riesgo CPU previo N03;
la suite de esta unidad no fallo, pero eso no determina su causa anterior.

### Positivo nativo autorizado2026-10-19

Nuevo tests/linux_swap_active_native.cpp, ejecutable Linux
pcinsight-linux-swap-active-native. NO registrado en CTest. Sin argumento exacto
--authorized-private-swap devuelve77 antes de crear archivos; requiere permiso
humano separado y root. No es una accion publica de la libreria.

Prueba acotada: MemAvailable>=512MiB, espacio libre>=256MiB, /tmp ext2/3/4 con
mapeo directo. mkdtemp0700, archivo exclusivo0600/O_NOFOLLOW,64MiB asignados y
escritos, fsync. Inicializa header v1 segun
[union swap_header de Linux v6.6](https://github.com/torvalds/linux/blob/v6.6/include/linux/swap.h),
con tamano de pagina nativo y firma SWAPSPACE2; sin mkswap ni dependencias nuevas.
swapon/swapoff son llamadas nativas, solo para esa ruta. No fuerza presion de
memoria, no toca swap existente, fstab, montajes ni configuracion permanente.

WSL kernel6.6.114.1, ext4 /dev/sdd: archivo activo65532KiB (header4096B), used0,
disco sdd. /proc/swaps REAL -> read_linux_swap_path_disk -> informe nativo ->
Monitor::sample/include_disks: contains_page_file=true, Support::estimated.
No se sustituye el inventario por fixtures. swapoff exitoso, ausencia en lista,
unlink y rmdir comprobados; configuracion previa /dev/sdc4194304KiB/priority-2
preservada (Used es volatil). Prueba manual exit0; guard sin flag exit77.
Log work/swap-active-native-linux.log. No se leyo contenido mientras estaba activo.

La limpieza normal y por excepciones intenta desactivar antes de borrar,
comprueba ausencia en inventario e identidad dev/inode/tipo antes de unlink.
Si no puede confirmar desactivacion/identidad, conserva el archivo e imprime
su ruta. Un SIGKILL, caida del host o terminacion externa no permite garantizar
RAII: no ejecutar esta prueba desatendida ni sin autorizacion explicita.

E05 E->C por este positivo y la cobertura previa de particion/parseo/carreras/
topologia conservadora. NO prueba XFS/Btrfs/F2FS, reparto fisico RAID/LVM,
kernel-inode universal ni todos los kernels. No se promueve estimated a exacto.
Sin cambios al proveedor/API/layout/historial de produccion.
Regresion Release Linux102/102 y Windows116/116 aprobada; paquetes reinstalados
y consumidores externos compilados/ejecutados ambos exit0. Builds sin warnings.
Logs work/swap-active-{build,suite,install,consumer}-{linux,windows}.log.
