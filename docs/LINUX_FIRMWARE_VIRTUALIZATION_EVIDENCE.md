# B08: evidencia de virtualización configurada en firmware

Revisión 171, 2026-11-06. Revisión de fuentes primarias y comprobación local
de solo lectura. No se añadió un proveedor supuesto ni se cambió el resultado
de `CpuInfo.virtualization_enabled`. B08 permanece abierto: necesita una fuente
vinculada al ajuste correcto y una prueba positiva en un equipo compatible.

| Fuente | Qué puede demostrar | Qué no debe publicar PCInsight |
| --- | --- | --- |
| [SMBIOS Type 4, Processor Characteristics](https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.4.0.pdf) | Características que el firmware atribuye al procesador, incluida capacidad de virtualización. | La posición del ajuste en UEFI/BIOS. |
| CPUID, `vmx`/`svm` o registro de CPU leído desde un invitado | Capacidades que ve ese entorno. El proveedor no lee MSR ni abre dispositivos para obtenerlos. | Estado del firmware físico, ni enabled/disabled por la sola presencia o ausencia de un flag. |
| [`KVM_GET_API_VERSION`](https://docs.kernel.org/virt/kvm/api.html) | Versión de la interfaz KVM consultada desde ese proceso; la métrica Linux ya la separa. | Estado del firmware o éxito al crear/ejecutar una VM. |
| [Clase `firmware-attributes` de Linux](https://github.com/torvalds/linux/blob/master/Documentation/ABI/testing/sysfs-class-firmware-attributes) | `current_value` permite leer el valor de un ajuste que el driver expone; `type` y `possible_values` describen sus valores. | Un nombre universal ni activación efectiva a partir del nombre, valor por defecto o una cadena parecida. |

## Candidato OEM y límites

La [referencia de Dell](https://dl.dell.com/topicspdf/command-configure_reference-guide4_en-us.pdf)
distingue su opción de virtualización del procesador de la opción VT-d de E/S.
El [driver Linux dell-wmi-sysman](https://github.com/torvalds/linux/blob/v6.12/drivers/platform/x86/dell/dell-wmi-sysman/sysman.c)
es una vía candidata. **Inferencia, no mapeo validado**: el nombre de una
opción en la CLI del fabricante no demuestra el basename que exportará ese
driver en un modelo concreto. Tampoco se traslada sin evidencia el nombre
`ProcVirtualization` de [la interfaz iDRAC](https://dl.dell.com/topicspdf/idrac9-lifecycle-controller-v32-series_cli-guide_en-us.pdf)
a sysfs. El [ThinkLMI de Lenovo](https://github.com/lenovo/thinklmi) es otra vía
del mismo tipo, no un alias universal. No se propone instalar esas CLIs ni
agregar una dependencia obligatoria.

El indicador de reinicio pendiente, donde existe, es del conjunto de ajustes
del proveedor; no identifica necesariamente el ajuste de virtualización. Un
valor observado de configuración tampoco debe interpretarse como comprobación
de que KVM funciona. No leer `default_value` como valor efectivo ni recorrer
atributos de autenticación, claves o contraseñas.

## Evidencia mínima para implementar un mapeo

1. Linux nativo con un driver ya presente: modelo y versión de firmware,
   kernel/driver y ruta exacta del atributo. Sin números de serie ni volcado de
   atributos. Una VM/WSL no confirma el ajuste del host.
2. Contrastar el identificador OEM con documentación y con el ajuste de
   virtualización **del CPU**, no IOMMU/VT-d/SR-IOV. Leer solo `type`,
   `possible_values`, `current_value` y, si aplica, `pending_reboot`.
   Comparar con el valor ya conocido en la interfaz del firmware. No se
   requiere cambiar el ajuste para recolectar una observación.
3. Aceptar solo un mapeo exacto del proveedor; cubrir valores válidos y
   desconocidos, permisos, ausencia y cambios con fixtures acotados. Si hay
   ambigüedad, cambio pendiente o ausencia, conservar desconocido. No usar
   KVM/SMBIOS como fallback para este booleano.

## Resultado local

WSL devolvió `ENOENT` para la clase `firmware-attributes`, las tres rutas OEM
sondeadas y tanto el registro sysfs de KVM como `/dev/kvm`. Esto se limita a
las rutas visibles en ese entorno, no prueba que el firmware físico esté
deshabilitado. El test focal `linux-virtualization` pasó diez veces: el caso
API12 es un fixture de clasificación, no una lectura positiva de hardware.
No hubo escrituras, elevación, carga de módulos ni creación de VMs. Sin
cambios de código productivo o ABI. Logs `work/firmware-b08-*.log`.
