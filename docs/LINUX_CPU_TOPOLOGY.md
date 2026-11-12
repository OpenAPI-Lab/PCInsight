# Topología CPU Linux: conteos online sin fallbacks inventados

`CpuInfo.logical_processors`, `physical_cores` y `sockets` ahora usan un lector
propio de sysfs. La implementación anterior contaba líneas de `/proc/cpuinfo`,
reutilizaba un `physical id` entre registros y, ante faltantes, devolvía tantos
núcleos como hilos y un socket. Esos fallbacks se retiraron. `/proc/cpuinfo`
sigue proporcionando vendor/model; ya no decide estos tres conteos.

## Fuentes y semántica pública

- `logical_processors`: número de IDs distintos en
  `/sys/devices/system/cpu/online`, no el ID máximo más uno.
- `sockets`: grupos de package distintos representados por CPU online, con
  `physical_package_id` no negativo y coherente en todos sus miembros online.
  No es un inventario de conectores de placa ni incluye paquetes solo offline.
- `physical_cores`: grupos de hermanos de núcleo distintos representados por
  CPU online, completamente contenidos en los grupos de package validados.
  No se deduce de los hilos ni de contar IDs numéricos de núcleo.

Se prefieren `topology/package_cpus_list` y `topology/core_cpus_list`. Solo si el
camino preferido no existe se usan `core_siblings_list` y `thread_siblings_list`,
respectivamente. Un archivo moderno presente pero ilegible o malformado no se
oculta con un alias antiguo. Los aliases antiguos no se consultan cuando el
moderno es legible y válido. Los valores `core_id`/`die_id` no son claves: pueden
repetirse sin colapsar grupos diferentes. El package ID puede ser disperso;
no se usa su valor máximo como cantidad. ID desconocido/negativo no significa
un socket medido. `0` sí puede ser un ID válido de un grupo, no un conteo cero.

Los tres valores presentes son `Support::estimated` y llevan nota de alcance:
son topología reportada por el kernel, no una auditoría física independiente.
Un kernel puede exponer topología por defecto o una VM puede exponer una
topología sintética; no se certifica SMT/hardware real a partir de ella.
No son el total instalado, la afinidad de un proceso, cpuset, cuota cgroup ni
garantía de disponibilidad efectiva de CPU para el llamador.

## Completitud, cambios y límites

Cada grupo debe contener al CPU que lo reporta. Los grupos deben ser disjuntos
y todos sus miembros online deben reportar el mismo grupo. Se rechazan también
solapamientos entre grupos distintos a través de miembros offline. Esos miembros
offline forman parte de la identidad, pero no se exige leer sus directorios.
Package IDs y grupos deben tener correspondencia uno a uno. Un núcleo que
cruza paquetes o queda fuera de su package no se publica como núcleo válido.

Si falla package, sockets y cores quedan ausentes. Si solo falla core, se
conservan sockets válidos. Topología individual ausente/inválida no convierte
la lista online válida en cero: logical puede seguir disponible. No se publican
sumas parciales para sockets/cores. Cada aplicación del resultado reemplaza los
tres campos y limpia valores anteriores si ya no se pueden validar.

Se comprueba `online` antes/después de cada lectura y se requieren dos
inventarios completos coincidentes. Un cambio entre ellos deja los tres campos
ausentes. No es una transacción del kernel: cambios y reversiones intermedios
pueden escapar. No se promete snapshot atómico entre estos campos y otros
proveedores CPU independientes, como utilización o cachés.

Límites: 4096 IDs por lista, IDs uint32, texto 64 KiB por archivo, package ID
int32 no negativo y como máximo 64 caracteres; hasta 1048576 referencias de
miembros acumuladas por inventario. Se reutiliza el parser estricto de listas
CPU de cachés (normalización, duplicados/rangos/overflow). La validación trabaja
por grupos únicos para evitar comparar cada lista con cada lista de cada CPU.
Superar límites produce ausencia, no truncamiento. No es timeout de syscalls.

Sin subprocesos, paquetes adicionales, lectura de BIOS ni escrituras al kernel;
no se modifica el layout público ni el backend Windows. Sysfs restringido o
ausente no tiene un fallback que invente cantidades físicas.

## Validación

Fixtures portables: dos paquetes, SMT, núcleos con un hilo, IDs dispersos,
CPU0 offline, subconjuntos online, filas desordenadas, IDs repetidos y grupos
contradictorios/solapados incluso offline; cobertura incompleta, presupuestos,
sentinelas, cambios entre lecturas y limpieza de resultados anteriores.
Fixtures de archivos Linux comprueban caminos modernos y aliases antiguos,
prioridad ante archivo moderno inválido, archivos ausentes/excesivos y CPU sin
directorio de topología. No se hace hotplug ni se cambia afinidad.

Pasada final del 2026-10-15: Linux68/68 y Windows88/88 Release; instalación y
consumidores externos exit0. Prueba focalizada Linux repetida con salida
detallada: online=32, core groups=16, package groups=1 en inventario estable
WSL, coincidente con `Monitor::sample`. Son datos expuestos por esa VM, no
validación física independiente de sockets/núcleos del host. Las pruebas de
cambios entre lecturas son fixtures, no un experimento nativo de hotplug.

Referencias primarias: [topología CPU sysfs](https://www.kernel.org/doc/html/latest/admin-guide/cputopology.html)
y [ABI estable de CPU](https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-devices-system-cpu).
Estos documentan las interfaces y sus valores por defecto; la política de
completitud y las restricciones anteriores son decisiones de PCInsight.
