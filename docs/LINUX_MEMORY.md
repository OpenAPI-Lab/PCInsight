# Memoria Linux: disponibilidad y aritmética validadas

Unidad: lectura dinámica de /proc/meminfo y cálculo de RAM usada/disponible,
porcentaje, caché, commit y zswap. Sin utilidades externas, elevación, escrituras,
cambios de swap ni generación de presión de memoria. Firmware/SMBIOS conserva su
proveedor aparte; no se añade lectura universal del BIOS.

Actualización C03: el nuevo informe MemoryInfo.linux_details amplía categorías,
partición estimada, zswap original/pool, zram por dispositivo y nombre explícito
installed_minus_usable_bytes. Ver LINUX_MEMORY_COMPOSITION.md. No convierte Slab
en pools Windows ni diferencia instalada-utilizable en reserva exclusiva del
hardware. C03 sigue abierta hasta resolver/aceptar ese límite. Nuevo layout0.x:
recompilar biblioteca y TODOS los consumidores.

## Total con disponibilidad explícita

Nuevo MemoryInfo.reported_total_bytes, Metric<uint64_t> al final del struct.
Preferirlo a total_bytes para comprobar disponibilidad. Linux lo obtiene de
MemTotal, que representa RAM utilizable publicada por el kernel, no RAM física
bruta instalada. Falta de MemTotal o valor0 deja el Metric ausente. El scalar
legacy total_bytes conserva0 como placeholder cuando no hay total: **no es un
cero medido ni debe usarse para inferir disponibilidad**.

Windows también llena reported_total_bytes cuando PSAPI GetPerformanceInfo
ofrece PhysicalTotal/PageSize válidos, comprobando el producto del total contra
uint64. No se revalidan todos los otros productos PSAPI en esta unidad. Backends
custom pueden dejar el nuevo Metric ausente; no se deduce de su scalar legacy.
API0.x ampliada: recompilar biblioteca y consumidores por el layout de MemoryInfo.

## Parser y fuente

El lector nativo acumula como máximo1MiB; errores de lectura no producen mapa
parcial. Parser: hasta10000líneas,64KiB por línea y sin NUL. Se seleccionan
MemTotal, MemAvailable, Cached, SReclaimable, Committed_AS, CommitLimit y Zswap;
C03 añade MemFree/Buffers/AnonPages/Shmem/SUnreclaim/Dirty/Writeback/Zswapped.
Para esos campos se exige nombre con colon, uint64 decimal completo sin signo,
unidad kB exacta y ningún token extra. kB se convierte a1024B solo después de
comprobar rango. Duplicados o un campo seleccionado mal formado rechazan todo
el mapa; no se publica lo parseado antes del error. Campos desconocidos se
ignoran para admitir extensiones y contadores sin unidad como HugePages_Total.

Campo seleccionado ausente no genera un valor0. La aplicación del mapa limpia
los datos dinámicos anteriores aunque se reutilice MemoryInfo, sin borrar sus
metadatos de firmware. Fallos del stream no se etiquetan como denegación de
permisos sin evidencia; una denegación explícita inyectada se conserva en las
métricas de lectura directa. No se cachea una muestra válida como si fuera nueva.

## Significado de los valores

- available_bytes: estimación MemAvailable del kernel, Support::estimated; no
  MemFree ni memoria que quede dentro de una cuota cgroup.
- in_use_bytes: MemTotal-MemAvailable, estimated. Si falta un operando o available
  supera total, no se resta: uso/porcentaje quedan ausentes con nota. La lectura
  available inconsistente tampoco se anuncia como válida. Porcentaje requiere
  total positivo y queda en0..100; available0 produce100%, available=total produce0%.
- cached_bytes: Cached+SReclaimable, estimated. Ambos campos deben existir; suma
  fuera de uint64 o mayor que un MemTotal conocido deja ausencia. Excluye Buffers
  y no equivale al campo Cached de Windows. No se suman componentes ausentes como0.
- committed_bytes y commit_limit_bytes: lecturas independientes Committed_AS y
  CommitLimit, no RAM residente. Se permite committed>limit: no es una resta de
  memoria usada y la política de overcommit puede permitir superar ese límite.
- compressed_bytes: proxy estimated de Zswap, solo memoria consumida por su
  backend comprimido. No mide Zswapped (tamaño original), zram ni toda la RAM
  comprimida de Linux/Windows. No añadirlo otra vez a in_use como si fuese memoria
  física adicional; esta API no produce una identidad exacta de composición.
- paged_pool/nonpaged_pool: ausentes, conceptos Windows sin equivalencia aportada
  por este proveedor; no se renombran Slab/SReclaimable para simularlos.

MemAvailable y composición de los campos siguen sus definiciones del kernel;
MemTotal, Cached y zswap no son equivalencias de los indicadores de Windows.
[Definiciones oficiales de meminfo](https://docs.kernel.org/filesystems/proc.html).

Un campo válido0, por ejemplo Zswap/Cached/Committed_AS, sí conserva su valor0;
no se confunde con ausencia. Los datos derivados llevan notas de alcance y
estimación. Una inconsistencia de relación invalida los campos dependientes,
sin borrar lecturas independientes válidas de commit o firmware.

hardware_reserved_bytes solo calcula firmware instalado menos MemTotal si ambos
están establecidos y firmware>=visible. No se usa el placeholder total0 para
afirmar que toda la RAM instalada está reservada. Diferencia0 conocida puede
publicarse estimated; la diferencia puede incluir límites de arranque, no solo
reservas físicas. La validación de todas las variantes de firmware es otro alcance.

## Límites de interpretación

No hay fallback MemAvailable inventado en kernels que no lo publiquen: se necesita
otro proveedor. Meminfo refleja la vista del kernel disponible al caller, no una
cuota cgroup ni necesariamente la memoria asignable dentro de un contenedor.
Los campos no forman una transacción con SMBIOS ni snapshots de procesos. Cambios
concurrentes pueden requerir otra muestra. Los límites del parser no son un
presupuesto de RAM total ni deadline de lectura. No se cierra toda la composición
de memoria, compresión universal, cgroups o inventario de módulos físicos.

## Verificación

2026-10-15: Release Linux66/66 y Windows86/86; paquetes reinstalados y consumidores
externos reconstruidos/ejecutados exit0. Nuevo test linux-memory con assertions
activas: unidades/campos/números/duplicados, límite de conversión kB, NUL/límites
de texto/líneas, cada campo opcional omitido, disponibilidad frente a cero,
available>total, porcentajes0/100, suma caché fuera de rango/superior a total,
committed>limit válido y limpieza de valores previos ante error. Metadata de
firmware conservada al aplicar el proveedor dinámico.

Ubuntu WSL nativo:16557035520B de RAM visible,15836041216B disponibles y4,35461%
de uso estimado en la pasada registrada. Relaciones uso+disponible=total dentro
del mismo snapshot y rango de porcentaje comprobados. Solo lectura, sin presión
de memoria. Windows nativo verifica reported_total_bytes available igual al total
legacy; consumidor instalado en ambas plataformas comprueba el campo público.
No se inyectaron errores nativos de PSAPI/procfs, no se provocó hotplug de RAM ni
compresión real: los bordes/ausencias son fixtures portables. Otros kernels/libcs,
cuotas cgroup y composición/compresión adicionales requieren más cobertura.
