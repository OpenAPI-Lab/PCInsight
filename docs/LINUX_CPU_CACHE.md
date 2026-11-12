# Cachés CPU Linux: instancias compartidas, no solo CPU 0

`CpuInfo.l1_bytes/l2_bytes/l3_bytes` son bytes de instancias distintas que sysfs
expone como alcanzables por los CPU en línea. Se suman cachés Data/Instruction
separadas; una caché compartida por varios hilos se cuenta una sola vez. No son
la capacidad por hilo, ni la suma ingenua de todas las filas `index*`.

El lector anterior usaba solo `cpu0/cache`: podía omitir otros núcleos/sockets y
fallaba si CPU 0 no estaba disponible. Ahora lee `online` y `cpuN/cache/index*`
para todos los CPU enumerados, directamente con C++/sysfs, sin lscpu, dmidecode,
subprocesos, instalaciones adicionales ni acceso al BIOS o escrituras al kernel.
No hay fallback que invente datos cuando el kernel/hipervisor los oculta.

## Identidad y cobertura

Cada fila requiere `level`, `type`, `size`, `shared_cpu_list`. La identidad usada
es (nivel, tipo, lista de CPU compartidos normalizada), no el número de `index`
ni un nombre/modelo comercial. No depende del atributo opcional `id`.

Para publicar un nivel:

- Todos los CPU en línea deben tener el mismo conjunto de tipos para ese nivel.
- Unified no puede mezclarse con Data/Instruction en el mismo nivel.
- Solo se admite una fila por (CPU, nivel, tipo). Topologías no representables
  por esta política conservadora quedan desconocidas, no se suman a ciegas.
- Cada grupo debe contener al CPU que lo reporta; todos sus miembros en línea
  deben reportar exactamente el mismo grupo y tamaño para ese tipo/nivel.
- Tamaños y suma deben ser positivos y caber en uint64. No se devuelve un total
  parcial si falta un miembro, hay solapamientos contradictorios o discrepancias.

Los miembros fuera de línea pueden figurar en una lista compartida; no se exige
leer sus directorios. Su caché solo se incluye si también es alcanzable desde
un CPU en línea. Un nivel completamente ausente queda sin valor, no en cero.
La cobertura se valida por nivel: una incoherencia L1 no borra un L2 intacto.
Un fallo de lectura/parseo que impida conocer el inventario invalida toda la
lectura; no se presume qué nivel escondía el archivo ilegible.

Dos inventarios completos consecutivos deben coincidir; además `online` debe
coincidir al inicio/final de cada lectura. Esto detecta cambios observables,
no es una transacción del kernel: un cambio y reversión entre lecturas puede
escapar. Valores presentes llevan `Support::estimated` y nota de alcance.
Cada llamada limpia los tres valores anteriores antes de publicar el resultado.

## Límites y alcance

CPU lists: rangos decimales uint32, sin duplicados/solapamientos, hasta 4096 IDs
por lista y 64 KiB de texto. Se normaliza el orden. `size` acepta el formato
entero positivo con sufijo K, convierte unidades de 1024 bytes y comprueba
overflow; no acepta signos, texto residual ni unidades adivinadas.

Hasta 64 índices y 128 entradas de directorio visitadas por CPU; hasta 65536
filas y 1048576 IDs compartidos acumulados por inventario. Cada archivo tiene
límite de 64 KiB (`size`, 64 caracteres); nivel válido 1..64, publicación solo
L1..L3. Superar límites produce ausencia, no truncamiento. Son límites de
trabajo/memoria, no timeout de syscalls ni soporte garantizado de todo servidor.

No cuenta cachés de CPU exclusivamente offline u ocultos; no descubre una caché
omitida de forma consistente en todos los directorios. No es capacidad disponible
para un proceso, cuotas cgroup, afinidad/cpuset ni una garantía sobre hardware
físico en contenedores/VM. Un sysfs ausente o restringido sigue siendo ausencia.
Sistemas heterogéneos sin una firma común pueden quedar sin total aunque parte
de la topología sea conocida. No se cambian campos públicos ni su layout.

## Verificación

Fixtures portables: dos sockets, SMT, L1 separada, L2 por núcleo y L3 compartida;
IDs dispersos, L1 unificada, CPU 0 offline, subconjunto online, orden de filas,
niveles ausentes, grupos/tamaños/tipos contradictorios, duplicados, ceros,
unidades/números inválidos, overflow y límites. En Linux, árbol sysfs privado
prueba además lectura completa, fallos de archivos, tamaño excesivo y limpieza
de valores anteriores. Release conserva asserts.

Pasada final del 2026-10-15: Windows87/87 y Linux67/67 Release; instalación y
consumidores externos exit0 en ambos sistemas. Prueba focalizada Linux repetida
con salida detallada y límites finales, también exit0.

Pasada nativa Ubuntu WSL: inventario estable con 32 CPU
online y 128 filas; L1=1048576B, L2=16777216B, L3=33554432B, todos estimated.
Son los valores expuestos por la VM, no una auditoría física independiente.
No se hizo hotplug, ni se cambió configuración o afinidad de CPU.

Referencias primarias: [ABI sysfs CPU del kernel](https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-devices-system-cpu)
y [implementación cacheinfo del kernel](https://raw.githubusercontent.com/torvalds/linux/master/drivers/base/cacheinfo.c)
para atributos y formato `size`/listas. La política de completitud y el alcance
estimated son decisiones de PCInsight, no garantías adicionales del ABI.
