# Capacidad visible y pista de rotación de discos Linux

Corrige capacity_bytes y evita una clasificación física ficticia HDD/SSD/NVMe
SSD. Solo lecturas nativas en include_disks de Monitor, sin subprocess, utilidades
instaladas, elevación ni writes sysfs. No cambia cálculo de volúmenes formateados,
system_disk o page file; esos campos no quedan revalidados por esta unidad.
Actualización posterior: LINUX_FORMATTED_CAPACITY.md corrige la suma de sistemas
de archivos montados con alcance estimated, sin afirmar cobertura desmontada.

## Capacidad

capacity_bytes es el tamaño del dispositivo de bloques visible al SO: size en
sectores de512B convertido a bytes. No es capacidad bruta de fabricación, espacio
libre ni suma de sistemas de archivos. Incluye dispositivos virtuales, sin afirmar
que se haya descubierto su medio físico subyacente.

Antes se usaba stoull sin comprobar consumo completo y se multiplicaba por512
sin verificar overflow. Ahora se exige un uint64 decimal completo sin signo,
con whitespace final opcional y hasta128B. Entrada vacía, NUL, signo, texto extra,
overflow o tamaño mayor falla con Metric ausente/unsupported y nota. Se comprueba
sectors<=UINT64_MAX/512 antes de multiplicar. Un size válido0 es cero publicado,
no ausencia. Tamaño válido available significa publicado por el dispositivo
visible, no prueba de capacidad física exacta. Una fila inválida no aborta por
conversión de capacidad el inventario de otros discos.

## Rotación y medio físico

Nuevo DiskInfo.rotational, Metric<bool>, al final del struct: true para
queue/rotational=1 y false para0, con Support::available como lectura de la pista
del kernel. Solo acepta literales0/1 y whitespace final, hasta128B. Ausente o
inválido produce Metric sin valor y nota. Es un atributo de cola modificable,
no una certificación de tecnología física.
[ABI oficial queue/rotational](https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-block).

| Fuente | media_type Linux | rotational |
|---|---|---|
| queue/rotational=0 | non-rotational | false, available |
| queue/rotational=1 | rotational | true, available |
| ausente/inválida | unknown | sin valor, unsupported |

Actualización E06: LINUX_DISK_IDENTITY.md añade un proveedor independiente de
identificación SAT/SCSI/NVMe. reported_media_type puede reflejar ese proveedor,
no este cálculo de pista. No se convierte la pista en HDD/SSD/SCM: RAM, loop,
dispositivos virtuales/apilados o fallos de lectura no se etiquetan como SSD.
El GUI puede mostrar la pista con su nota, no «SSD confirmado».

bus_type procede del nombre nativo de device/subsystem: nvme se presenta como
NVMe, otros identificadores se conservan, y ausencia/formato inválido da unknown.
Ya no se deduce NVMe por un prefijo del nombre del disco. Se aceptan identificadores
ASCII alfanuméricos con _ y -, hasta128B. scsi no prueba transporte SATA ni SAS;
la identificación reportada se consulta separadamente en linux_details, con
fuentes/límites documentados en LINUX_DISK_IDENTITY.md.

## Compatibilidad y límites

API0.x ampliada: recompilar biblioteca y consumidores porque DiskInfo creció.
Campo nuevo al final, sin reordenar los existentes. Windows conserva proveedores
de medio/bus/capacidad y rotational ausente, sin deducirlo automáticamente de
HDD/SSD. El consumidor instalado comprueba las nuevas etiquetas Linux.

Callers que comparaban media_type con HDD/SSD/NVMe SSD deben adaptar su GUI.
Donde antes se infería un medio ahora puede aparecer unknown: corrección de
semántica, no pérdida de un dato físico previamente confirmado. Comprobar
has_value para distinguir false de ausencia. No se adivinan permisos específicos
a partir de una lectura vacía.

El lector genérico carga texto antes de validarlo;128B es límite de aceptación,
no presupuesto de lectura frente a archivos arbitrarios. Modelo, size, pista y
subsistema no son un snapshot atómico ni identidad persistente. Volver a muestrear
ante cambios/hotplug. No promete SATA/SAS/SSD universal ni acceso directo al BIOS.

## Verificación

2026-10-15: Release Linux61/61 y Windows81/81; instalación y consumidores externos
reconstruidos/ejecutados exit0. Assertions activas en Release. Nuevo test portable:
capacidad0/normal/máximo permitido/primer overflow, signo/NUL/texto residual/
entradas sobredimensionadas, rotación0/1/ausente/inválida, filas reutilizadas sin
valores retenidos, subsistemas válidos/inválidos, nombre nvme-fake sin inferencia
de bus y pistas contradictorias sin inventar medio físico.

Prueba nativa Ubuntu WSL:28 filas estables contrastadas con lecturas sysfs antes
y después de Monitor; capacidad y pista/etiqueta coinciden, sin writes. Consumidor
Linux instalado verifica etiquetas y disponibilidad del campo público nuevo.
Windows ejecuta fixtures portables y sus regresiones de almacenamiento; no se
afirma captura sysfs nativa desde Windows. No se validaron medios físicos de
otras máquinas, hotplug ni archivos sysfs con permisos denegados reales; entradas
vacías/inválidas se cubren con fixtures, sin inventar permission_required.
