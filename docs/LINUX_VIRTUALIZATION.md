# Virtualización Linux: firmware y acceso KVM son evidencias distintas

Revisión 171: [evidencia y criterios OEM](LINUX_FIRMWARE_VIRTUALIZATION_EVIDENCE.md).
La comprobación WSL no encontró un atributo de firmware visible; se conserva
el resultado desconocido hasta validar el mapeo en un equipo compatible.

La antigua condición `exists("/dev/kvm") => virtualization_enabled=true` se retiró:
un nombre existente no prueba firmware habilitado, permisos ni una API funcional.
Tampoco se puede concluir BIOS deshabilitado a partir de un dispositivo ausente.

## Contrato público

`CpuInfo.virtualization_enabled` permanece sin valor/unsupported en este
proveedor Linux: no implementa una fuente directa del estado de firmware.
No se reutiliza como bandera de KVM, hypervisor activo, estar dentro de una VM,
capacidad vmx/svm de CPU ni soporte de virtualización anidada. El proveedor
Windows sigue usando su bandera de firmware; no cambia en esta unidad.

Nuevo `CpuInfo.linux_kvm_api_version` (`Metric<unsigned>`): publica 12 con
Support::available solamente después de una consulta KVM_GET_API_VERSION exitosa
en un descriptor validado contra el registro KVM de sysfs. Confirma esa consulta
de API, no que crear/ejecutar una VM, todas las extensiones o un tipo de máquina
concreto vayan a funcionar. No habilita hardware, firmware ni módulos.

Un error de permisos al leer el registro inicial, abrir el dispositivo,
consultar metadatos o ejecutar el ioctl se clasifica permission_required cuando
el errno obtenido es EACCES/EPERM. Ausencia, ABI inesperada, inconsistencias y
otros fallos quedan sin valor; nunca se convierten en false/0 medidos para KVM
o firmware. El fallo de la comprobación final de registro es una muestra
inestable/no verificable, no un resultado positivo de API.

El nuevo campo amplía CpuInfo y por tanto SystemSnapshot: **recompilar la
biblioteca y todos los consumidores de esta API0.x**, también en Windows. No
mezclar objetos/binarios compilados con layouts distintos. En Windows el campo
Linux queda ausente. No se cambian formatos de persistencia de historial.

## Consulta nativa

1. Leer `/sys/class/misc/kvm/dev`: máximo64 bytes, major:minor uint32 decimales
   estrictos; sin signos, texto residual, NUL, números desbordados o partes vacías.
2. lstat de `/dev/kvm`: exigir dispositivo de caracteres con el major/minor
   registrado antes de abrir. Abrir con O_RDONLY/CLOEXEC/NONBLOCK/NOFOLLOW.
   No se aceptan symlinks finales por esta política conservadora.
3. fstat del mismo fd: debe ser dispositivo de caracteres y su major/minor debe
   coincidir con el registro KVM leído. Archivos, directorios, FIFO y otros
   dispositivos se rechazan antes de emitir el ioctl.
4. Emitir únicamente KVM_GET_API_VERSION. Releer el registro y exigir el mismo
   major/minor antes de aceptar la versión12. Otro valor es ABI no compatible.
5. Cerrar descriptores mediante RAII, también en errores o excepciones de recursos.

El ioctl estable se obtiene con `_IO(0xAE,0x00)` de sys/ioctl.h; no se añade una
dependencia de headers linux/kvm.h ni de herramientas/libvirt/QEMU. No se emite
KVM_CREATE_VM, KVM_RUN, escrituras, consultas MSR ni carga de módulos. Lecturas
del registro interrumpidas se reintentan hasta3 veces; otras operaciones fallan
cerrado sin bucles indefinidos de reintento.

Se confía en el sysfs/devfs del entorno; no es autenticación criptográfica de
un kernel, driver o mount adversarial. La comparación antes/después no prueba
ausencia de todos los cambios y reversiones, aunque el ioctl usa el mismo fd
que se inspeccionó. Un sysfs oculto puede impedir validar KVM aun si hay un
dispositivo accesible. O_RDONLY puede no ser aceptado por alguna implementación:
no se reabre con escritura para superar ese fallo. Los límites no son timeout
de syscalls. Un resultado válido es puntual, no garantía para otra credencial,
namespace, aplicación o instante. Cada muestra limpia los valores anteriores.

## Verificación y límites

Fixtures portables: parseo major/minor, identidad de dispositivo, clasificación
API12 frente a ABI inesperada, errno de permisos vs indisponibilidad y limpieza
de un true antiguo de firmware/datos KVM anteriores. El camino positivo portable
prueba clasificación, no simula haber recibido una respuesta de hardware real.

Fixtures Linux privados: archivo regular llamado kvm, directorio, FIFO, symlink,
dispositivo ausente y registro inválido/excesivo. `/dev/null` se usa solo como
dispositivo de caracteres con identidad diferente: se rechaza antes del ioctl.
No se crean nodos de dispositivo ni VMs. Monitor/consumidor instalado comprueban
separación de métricas; el consumidor Windows comprueba campo Linux ausente.
Una observación negativa nativa no valida el camino positivo en hardware.

Pasada final del 2026-10-15: Linux71/71 y Windows91/91 Release; instalación y
consumidores externos recompilados exit0. Prueba focalizada Linux repetida con
salida detallada: registro KVM ausente en WSL (errno2/ENOENT), API sin valor y
firmware desconocido. Consulta positiva nativa de API12 aún no validada en este
entorno; se requiere hardware/entorno Linux que exponga KVM. La prueba no carga
módulos ni modifica permisos para obtener un resultado positivo.

No implementa aún lectura directa de firmware (MSR/BIOS), detección de hypervisor
activo, listado de guests, capacidad por CPU/arquitectura ni prueba de creación
de VM. No declara completada la virtualización universal solo por esta corrección.

Referencias primarias: [API KVM](https://www.kernel.org/doc/html/latest/virt/kvm/api.html)
y [UAPI KVM](https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/linux/kvm.h).
La separación pública, límites y política de verificación son de PCInsight.
