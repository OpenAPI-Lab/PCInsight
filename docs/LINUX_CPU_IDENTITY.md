# Fabricante y modelo CPU: cobertura, procedencia y alternativa SMBIOS

Nuevos `CpuInfo.reported_vendor` y `reported_model` son Metric<string> con
disponibilidad, soporte y nota de procedencia. `vendor`/`model` legacy se mantienen
y reflejan el valor publicado; si no hay dato, quedan vacíos, no con un nombre
inventado. Nuevos miembros amplían CpuInfo/SystemSnapshot en API0.x: recompilar
biblioteca y todos los consumidores, también Windows. No mezclar layouts.

## Texto del kernel Linux

El lector anterior elegía el primer vendor_id/model name y leía sin límite de
tamaño. Ahora se reconocen registros iniciados por `processor` numérico uint32,
únicos, incluso con IDs dispersos. Se consideran `vendor_id` para vendor y
`model name`, `cpu model` o `cpu` para modelo. Los dos últimos permiten recoger
el texto de MIPS/PowerPC además del formato habitual x86/ARM32, sin ejecutar
utilidades ni usar una base externa de nombres.

Se publica una cadena solo si todos los registros de procesador observados
tienen el mismo valor normalizado y la misma clave fuente. Duplicados de un
campo o aliases simultáneos en un registro, cobertura incompleta y valores
contradictorios dejan ese Metric sin valor. No se escoge el primero ni se
rellena esa discrepancia con una cadena única de firmware. Vendor y model
son independientes: un fabricante común no prueba un modelo común.

La nota available identifica la clave y el alcance: texto reportado en registros
visibles de procfs, no certificación de hardware físico, modelo comercial o
uniformidad de CPU ocultos. No se relaciona atómicamente con sysfs online/CPU
topology ni se detecta todo hotplug. Un kernel/hipervisor puede suministrar
nombres genéricos o sintéticos; el programa no los convierte en otra marca.

Hardware, Model, machine, system type, nombres de placa/SoC o un CPU implementer/
CPU part numérico no se convierten en un fabricante/modelo textual. En ARM64
esos IDs pueden existir sin una cadena de producto; sin otra fuente, el nombre
permanece desconocido. No se implementa catálogo MIDR, device-tree de CPU,
RISC-V uarch/IDs ni decodificación universal de arquitecturas en esta unidad.

Procfs: límite16MiB total,250000líneas,64KiB por línea,4096registros CPU. Lectura
en bloques sin reservar16MiB para cada muestra pequeña. IDs duplicados/invalidos,
NUL o exceso de límites invalidan el inventario; no se filtran filas parciales
como completas. Líneas/campos desconocidos se ignoran. Solo claves seleccionadas
dentro de un registro CPU aportan nombres; no se inventa un registro para una
descripción global de plataforma o un esquema de arquitectura no reconocido.

## Nombres de firmware

Si todos los registros carecen de un nombre textual seleccionado, o procfs no
se puede leer/parsear, puede utilizarse una fuente independiente SMBIOS, marcada
estimated. Si procfs sí mostró mezcla, cobertura parcial o campo inválido, no
se usa firmware para esconderlo. Las notas de firmware no afirman correlación
con los CPU online del namespace del proceso.

El parser propio conserva el string-set de cada estructura: para Type4 central
y socket poblado extrae Manufacturer y Processor Version por sus índices1-based.
Índice0, fuera del string-set o cadena inválida dejan el campo desconocido; no
leen cadenas de otra estructura ni usan Socket Designation/Processor Family
como modelo. Solo hay resumen escalar si todas las estructuras centrales
pobladas reportadas concuerdan y tienen ese campo. Sockets no poblados no se
usan; una estructura Type4 demasiado corta invalida el resumen CPU para no
publicar un subtotal. Los tres placeholders exactos Unknown, Not Specified y
To Be Filled By O.E.M. se omiten. No es un filtro universal de todos los textos
OEM desconocidos ni una garantía de que el firmware sea verdadero/completo.

SMBIOS conserva el límite de tabla16MiB y validación de terminación de string-set;
las cadenas seleccionadas se limitan a1024 bytes. La interpretación existente
de RAM no cambia. Current Speed no se usa como identidad o frecuencia base de
Linux. El firmware se lee al construir Monitor; recrearlo tras cambios de
hardware/firmware para refrescar esta alternativa estática.

Si falta nombre de procfs y el acceso a firmware está denegado, se expone
permission_required, no una cadena vacía interpretada como medición. Cada muestra
reemplaza los cuatro campos y limpia valores anteriores ante falta de fuentes.
Una tabla no disponible no se descarga ni requiere dmidecode. Ambos proveedores
dependen de lo que exponga el entorno, no del BIOS físico del host de una VM.

## Texto y compatibilidad Windows

Texto CPU normalizado: UTF-8 válido, sin sobrelongitudes, sustitutos, escalares
fuera de rango o controles ASCII/C1; espacios/tabs se colapsan y se recortan.
No se translitera, traduce ni deduce un nombre comercial. No es un saneador
universal para HTML, terminal o bidi: el consumidor debe escapar según su GUI.

Windows llena los nuevos Metric desde sus cadenas existentes de registro
CentralProcessor/0 y los etiqueta estimated: un CPU representativo, no prueba
de uniformidad de todos los paquetes. No se cambió su lector de registro ni se
afirma que haya recibido la misma validación del parser Linux. Consumers pueden
usar los nuevos Metric en ambos backends, sin perder los strings legacy.

## Pruebas

Fixtures: texto x86/ARM32/MIPS/PowerPC, ARM64 sin nombre textual y fallback
SMBIOS, IDs dispersos, campos vacíos/duplicados/heterogéneos, cobertura parcial,
UTF-8/controles/límites, permiso firmware y limpieza. Tablas Type4 prueban
índices/ausencias/discrepancias, sockets vacíos, placeholders, truncaciones
en cada byte y estructuras cortas. Casos no-x86 y firmware positivo son fixtures;
no equivalen a una ejecución en hardware ARM/MIPS/PowerPC real.

Pasada del 2026-10-15: Linux72/72 y Windows92/92 Release; instalación y consumidores
externos recompilados exit0. Prueba focalizada Linux con salida detallada:
AuthenticAMD / AMD Ryzen 9 7950X 16-Core Processor en32registros WSL, concordantes
con Monitor. Fabricante/modelo de firmware positivo y arquitecturas no-x86
solo fixtures; no se presenta como validación nativa de esos sistemas.

Referencias primarias: [SMBIOS3.8, Type4](https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.8.0.pdf),
[cpuinfo ARM64](https://raw.githubusercontent.com/torvalds/linux/master/arch/arm64/kernel/cpuinfo.c),
[cpuinfo MIPS](https://raw.githubusercontent.com/torvalds/linux/v6.12/arch/mips/kernel/proc.c),
[cpuinfo PowerPC](https://raw.githubusercontent.com/torvalds/linux/master/arch/powerpc/kernel/setup-common.c).
Cobertura, límites, precedencia y disponibilidad son política de PCInsight.
