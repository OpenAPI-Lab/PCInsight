# Rendimiento de discos Linux: continuidad y rango numérico

Unidad cerrada: corrección de tasas de lectura/escritura/transferencia,
active_percent y average_response_ms de DiskInfo. Captura nativa sysfs, sin
subprocess, utilidades instaladas, configuración de dispositivos ni elevación.
API pública sin cambios de layout; no añade E/S por aplicación ni identifica
automáticamente tipos físicos que el proveedor previo desconoce.

## Qué cambió

El backend anterior retenía contadores de diskstats al desactivar include_disks,
pero avanzaba el reloj global de Monitor. Al reactivar, podía dividir consumo
de una pausa larga por un intervalo corto. Restas uint64, multiplicaciones por
512 y sumas de bytes/operaciones/tiempos también podían desbordarse.

Ahora LinuxDiskRateTracker conserva exclusivamente la muestra anterior válida
por disco, con reloj steady_clock propio. include_disks=false borra baselines;
la primera captura tras reactivarlo no publica tasas. Muestra vacía o disco
omitido pierde continuidad. Error global de fill_disks limpia el tracker antes
de propagar la excepción. Una muestra rechazada por límite/identidad duplicada
no publica resultados parciales y también limpia continuidad.

Se exige nombre, dev (major:minor) y diskseq opcional coincidentes, tiempo creciente
y los diez contadores acumulativos usados no decrecientes. El campo in_flight
es una cantidad instantánea: puede bajar y no provoca reset. Tiempo igual/atrás,
cambio de identidad o contador decreciente dejan las cinco métricas sin valor
para ese intervalo y establecen una nueva baseline. Cambiar de presencia de
diskseq también rompe continuidad. Sin diskseq disponible en ambas muestras se
admite nombre+dev, sin prometer detección de toda reutilización entre polls.

## Fuente y significado

La captura lee /sys/block/<disco>/stat por dispositivo en vez del conjunto
/proc/diskstats. Se leen dev y diskseq antes/después de stat para descartar
cambios visibles. Campos ausentes/inválidos no eliminan el DiskInfo: conservan
inventario con métricas de rendimiento ausentes y nota. Los campos estáticos
no son un snapshot atómico con stat. La unidad posterior LINUX_DISK_PROPERTIES.md
corrige capacidad visible y separa pista de rotación de clasificación física.

Los sectores de estos contadores son de512B, no el sector físico del dispositivo.
Las tasas dividen deltas de sectores por segundos monotónicos y convierten a
bytes/s. La conversión y las sumas se realizan sin aritmética entera intermedia
que pueda desbordarse; valores fuera de uint64 quedan ausentes, no saturados.
Cada dirección y el agregado se validan por separado. Valores se redondean hacia
abajo; delta0 válido produce0. Las tres tasas tienen Support::estimated.
[Semántica oficial de los contadores y unidades](https://docs.kernel.org/block/stat.html).

active_percent estima delta io_ticks / tiempo transcurrido y se limita a100%.
No equivale a saturación del hardware. El kernel documenta limitaciones de la
contabilidad del tiempo de E/S, especialmente con peticiones concurrentes.
[Documentación de estadísticas de E/S](https://docs.kernel.org/admin-guide/iostats.html).

average_response_ms es (delta read_ms + delta write_ms) / (delta lecturas
completadas + delta escrituras completadas), etiquetado estimated. No incluye
discard/flush ni latencia extremo-a-extremo de la aplicación. Si el denominador
es0, el promedio es indefinido: se devuelve ausente con nota, no0ms ficticios.
Los tiempos de peticiones concurrentes pueden sumar más que el tiempo de pared.
[Definición de tiempos de peticiones](https://docs.kernel.org/block/stat.html).

diskseq, cuando se publica, aporta una secuencia de dispositivo; no hace la
lectura transaccional ni identifica permanentemente el medio físico. Puede cambiar
al sustituir el backing file de un loop device.
[ABI oficial diskseq](https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-block).

## Límites

Parser: mínimo11 campos, máximo64, todos uint64 decimales sin signos/texto
sobrante y hasta16384B de entrada; se calculan métricas con los primeros11.
Los adicionales se validan pero no se usan para throughput/response. Identidad
dev exige major/minor uint32 completos; nombre no vacío/sin NUL/hasta4096B.
Hasta100000filas; el inventario aplica el mismo máximo. No es presupuesto de RAM
total ni deadline de syscalls. El texto se carga antes de validarlo; estos límites
no endurecen el lector genérico frente a archivos arbitrarios fuera de sysfs.

No se reconstruye consumo perdido, ni se corrigen wraps ocultos sin descenso
visible. Nombre+dev puede reutilizarse cuando diskseq no es accesible; crear
otro Monitor al cambiar la vista de sysfs/namespaces. Todo corresponde a los
dispositivos visibles, incluidos virtuales; no sumar capas como si fueran
discos físicos independientes. No hay garantía de equivalencia visual a Windows.

## Verificación

2026-10-15: Release Linux60/60 y Windows80/80; reinstalación y consumidores externos
reconstruidos/ejecutados exit0. Nuevo test linux-disk-rates con assertions activas
en Release: parser/overflow/texto/longitud/columnas, tasas/activo/promedio,
in_flight decreciente válido, cada uno de los diez campos acumulativos reiniciado,
nombre/dev/diskseq cambiados/ausentes, desaparición, opt-out/reenable, reloj igual/
atrás/rango completo, ceros válidos, promedio sin operaciones ausente, tasas fuera
de rango y sumas grandes sin wrap, duplicados y límites de filas/identidad.

Nativo Ubuntu WSL:28 dispositivos con tasas/activo disponibles entre muestras,
incluyendo ceros medidos; opt-out/reenable verificado a través de Monitor. Solo
lecturas; no se generó una carga de disco ni se modificaron dispositivos. Windows
ejecuta fixtures del cálculo portable, no valida captura sysfs. Resets/hotplug
físicos y otros kernels/arquitecturas requieren cobertura adicional: los casos
de reinicio e identidad de esta unidad son fixtures, no desconexiones reales.
