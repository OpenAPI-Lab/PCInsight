# Identificación de medio, protocolo y transporte Linux

E06: proveedor de identificación explícita por sysfs, sin ejecutar utilidades,
abrir dispositivos /dev, enviar comandos de identificación, escribir atributos
ni requerir paquetes nuevos. Se usan los datos que el kernel ya expone.
No es un certificado del medio físico oculto detrás de un puente/VM/RAID.
El2026-10-19 el usuario respondió «prosigue» a la pregunta concreta de aceptar
este alcance para E06: reportar lo declarado y conservar desconocido si el medio
está oculto, sin convertir NVMe automáticamente en SSD. Aceptación solo E06;
no se extiende a L03 u otras decisiones pendientes.

## Contrato público

DiskInfo.linux_details contiene Metric<LinuxDiskDetails>. Cada campo tiene su
propia disponibilidad y nota; la presencia de la estructura solo confirma la
lectura comprobada del contexto del dispositivo, no que todos sus campos existan.

| Campo | Significado |
|---|---|
| kernel_subsystem | Nombre reportado por device/subsystem, no transporte deducido |
| device_protocol | SCSI/NVMe del subsistema, o ATA/SATA de IDENTIFY en SAT VPD89 |
| transport | Protocolo de puerto destino SCSI o atributo transport del controlador NVMe |
| media_type | Clasificación reportada por identificación; estimated, no pista de cola |
| nominal_rotation_rpm | RPM nominales reportadas; nunca velocidad medida en vivo |

reported_media_type refleja linux_details.media_type. Los strings legacy
media_type (rotational/non-rotational/unknown) y bus_type (subsistema) conservan
su contrato anterior: no se sustituyen silenciosamente por otro concepto.
El GUI debe consultar los Metric y sus notas. No concatenar todas las etiquetas
como si fueran una única conexión física: SATA + SAS pueden describir capas
distintas de un mismo camino. Windows deja linux_details ausente.

La implementación inicial(revision14) amplió DiskInfo/SystemSnapshot en API0.x:
recompilar biblioteca y TODOS
los consumidores, incluidos los instalados. Sin cambio del formato de historial.

## Fuentes y validación

SCSI: se leen binarios device/vpd_pg89, vpd_pgb1 y vpd_pg83. El kernel publica
estas páginas desde su caché VPD; no se solicita un rescan ni se emiten comandos
SCSI desde la biblioteca. [Publicación VPD del kernel](https://github.com/torvalds/linux/blob/v6.12/drivers/scsi/scsi_sysfs.c).

Todas las páginas requieren código correcto, tipo de periférico directo o
zoned block, qualifier conectado y longitud declarada igual al archivo completo.
Un dato truncado no se acepta como prefijo válido.

- SAT89: tamaño572B, comando IDENTIFY DEVICE0xec, payload512B desde offset60,
  dispositivo ATA, versión ATA7+ con bit15 no reservado. Se verifica checksum
  si aparece firma0xa5. Word76 con bits de velocidad SATA positivos y valor no
  reservado permite protocolo SATA; sin ello se conserva ATA, no se inventa PATA.
  Word217=1 da SSD estimated, siguiendo la clasificación ATA;0401..fffe da HDD
  y RPM nominales.0,ffff y2..0400 no informan medio.
  [Payload SAT89 y características B1 de libata](https://github.com/torvalds/linux/blob/v6.12/drivers/ata/libata-scsi.c),
  [Campos/helpers ATA del kernel](https://github.com/torvalds/linux/blob/v6.12/include/linux/ata.h).
- B1: mínimo64B. Tasa nominal big-endian en bytes4..5. Un valor1 se publica como
  non-rotating, no necesariamente SSD; una tasa0401..fffe como HDD estimated y
  RPM nominales. Una contradicción entre tasas válidas ATA y B1 invalida ambos
  campos de medio/RPM, no se elige arbitrariamente una página.
- VPD83: recorrido completo de descriptores, longitudes y reservados comprobados.
  Solo PIV=1 y asociación target-port con descriptores binarios EUI/NAA/relative-port
  admitidos se usan. Protocolos concordantes: Fibre Channel, Parallel SCSI,
  IEEE1394, SRP, iSCSI, SAS, ATA. Ausencia, conflicto, dato malformado o protocolo
  no admitido deja transport ausente. No se interpreta un identificador de LUN
  como puerto. [Identificadores de protocolo](https://github.com/torvalds/linux/blob/v6.12/include/scsi/scsi_proto.h),
  [PIV y asociación de puerto en el target kernel](https://github.com/torvalds/linux/blob/v6.12/drivers/target/target_core_spc.c).

NVMe: el subsistema establece el protocolo, no la tecnología de almacenamiento.
Se acepta transport=pcie/tcp/rdma/fc/loop desde el mismo directorio de controlador.
No se llama PCIe a un controlador TCP ni SSD a todo NVMe. Un namespace multipath
que no exponga este controlador directamente puede quedar sin esos datos.
[ABI NVMe transport](https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-nvme).

## Lectura e identidad

Se fijan directorios con descriptores, y se comparan inode/dev del bloque y su
device con sus rutas al terminar. major:minor requerido y diskseq opcional se
validan/comparan antes/después. Subsistema y páginas/transport se releen; un cambio
observado invalida linux_details completo, sin retener el medio anterior.
El parseo no se basa en sda/nvme*/ata* ni en strings de modelo/vendor.

Atributos: apertura read-only, CLOEXEC/NONBLOCK/NOFOLLOW, fstat regular, EINTR y
límites efectivos de lectura.89 hasta572B; B1/83 hasta65539B; dev/diskseq/transport
hasta128B; link hasta4095B. FIFO/directorio/symlink de atributo no se lee como
archivo regular. EACCES/EPERM observados se distinguen como permission_required;
no se deduce ese error de una simple ausencia.

No es snapshot atómico: cambios A->B->A intermedios, reemplazo de atributos con
el mismo contenido, backing oculto y reutilización de identidad sin diskseq no
son detectables universalmente. Los límites de lectura no son un deadline de
driver. No se busca un ancestro por nombres ni se atraviesan capas para inventar
el medio de dm/md/loop/virtio/USB. Firmware/puentes pueden mentir o emular datos.

## Verificación y alcance

tests/linux_disk_identity.cpp comprueba parsers portables con páginas sintéticas,
checksum, límites/rangos, conflictos, transportes separados y limpieza de valores.
En Linux el mismo lector nativo procesa un árbol sysfs privado sintético, con
FIFO, archivo sobredimensionado y cambios deterministas de major:minor/diskseq,
VPD, subsistema, device y directorio del bloque durante la lectura. Esos positivos
no son hardware físico ni páginas de un disco real. La integración Monitor lee
/sys/block real, sin escrituras, y verifica coherencia de los campos comunes.

E06 todavía no declara clasificación física exacta universal: medio de NVMe o
SCSI non-rotating sin identificación tecnológica adicional y backing de capas
virtuales pueden seguir desconocidos. No se cierra esa parte solo por devolver
unsupported. Hardware físico adicional sigue en N02. No cambia E01..E05/E07.

2026-10-16: Release final Linux84/84 y Windows104/104 aprobadas. Paquetes
reinstalados; ambos consumidores externos recompilados/ejecutados exit0 con el
nuevo layout. Ubuntu WSL:4filas con contexto fijado y0reportes de medio, sin
inventar tipo de hardware detrás de discos virtuales. Positivos SATA/SSD/HDD/
SAS/NVMe de este test son fixtures; no validación física de esas tecnologías.

Primera suite Linux83/84: linux-app-history falló por informe ausente, no la
identificación de discos. Su prueba nativa usa include_disks=false. Causa no
demostrada; logs preservados en work/disk-identity-failed-linux-tests.log y
...-list.log. Se añadió salida de nota antes de su aserción original; luego3/3
repeticiones y suite84/84 pasaron sin cambiar lógica/aserciones del historial.
No se atribuye ese fallo al CPU ni se declara corregido por E06; queda en N03.
Los primeros enlaces también usaron un objeto anterior a la nueva firma interna
del hook de prueba; recompilación completa posterior sin errores ni warnings.
No se cambió el reloj del host ante avisos transitorios de clock skew de MSFS.

Ese estado E correspondía a la espera de proveedor adicional o aceptación del
límite. La respuesta2026-10-19 permite el cierre E06 por la alternativa de
aceptación ya prevista en el criterio, no por haber implementado una fuente
exacta universal. No se agrega ningún ID ni crédito parcial al conteo.

## Revisión40 (2026-10-19)

La aceptación conserva las etiquetas reported/estimated y unknown; no autoriza
inventar tipo físico, buscar nombres de modelos para adivinarlo ni atravesar
capas virtuales como si fueran dispositivos físicos exclusivos. La validación
física de otras tecnologías continúa N02, sin usar positivos Windows como
prueba del proveedor Linux.

Auditoría/corrección de diagnóstico: sin reporte válido y con página ATA/B1
denegada, medio Y RPM conservan permission_required y un motivo explícito.
Una página independiente válida sigue aportando su reporte aunque la otra esté
denegada. Non-rotating no se convierte en RPM0. Tests adicionales comprueban
que VPD/transportes de otra capa no clasifican virtio/USB/block/PCI como disco
físico; NVMe/TCP con VPD ATA ajena no adquiere SSD/HDD; reemplazar una muestra
SSD por NVMe sin evidencia borra el reporte anterior, y permisos/ausencia nunca
retienen clasificación vieja. El alias reported_media_type conserva valor,
soporte y nota del campo interno, verificado también por consumidor instalado.

Para el GUI: comprobar metric_is_known(disk.linux_details), después el Metric
de cada campo. Mostrar «medio desconocido» si media_type carece de valor válido,
y conservar support/note; no usar disk.media_type legacy como fallback a SSD.
Una estructura disponible con todos los campos físicos ausentes no certifica
que el disco carezca de medio ni que su rendimiento/capacidad sean cero.

Sin cambios de layouts/API/ABI virtual/SystemSnapshot/historial, nuevas
dependencias, acceso /dev, cambios de discos/configuración ni elevación.
Positivos ATA/SATA/SSD/HDD/SAS/NVMe siguen fixtures, con el lector nativo sobre
sysfs privado; no hardware físico certificado. N03 conserva los fallos
históricos CPU/apphistory, no resueltos por repetir una suite verde.
Evidencia de esta revisión en work/disk-reported-*.log.
Regresión: Linux108/108,Windows122/122 Release;10repeticiones focalizadas Linux,
consumidores instalados ambos recompilados/relinkados/ejecutados exit0, builds
sin warnings. Nativo WSL4contextos fijados/0medios reportados, unknown conservado.
