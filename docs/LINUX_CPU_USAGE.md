# Utilización CPU Linux: intervalos validados

Unidad: robustez de CpuInfo.utilization_percent y del denominador global usado
por ProcessInfo.cpu_percent. Sin nueva API/layout público, utilidades externas,
escrituras, elevación ni cambios de afinidad/prioridad. Modelo/fabricante/topología
estática, frecuencia, memoria e historial de aplicaciones no quedan cerrados por
esta corrección.

Actualizacion2026-10-16: D04 sustituye el denominador numerico de CPU por proceso
por su reloj propio y capacidad visible normalizada. linux_cpu_interval permanece
como guardia de contexto, no divisor de ticks de filas tomadas en otro instante.
Ver LINUX_PROCESS_CPU.md. El resto de este documento conserva la evidencia y
el diseno de la unidad global original; no describe una reversion de D04.

## Lectura y parseo

Antes se aceptaba el prefijo numérico que pudiera leerse de la línea cpu, sumando
sin control de overflow y restando idle sin comprobar retrocesos. Ahora se lee
/proc/stat con un límite de16MiB y se validan:

- Línea agregada cpu, presente una sola vez y antes de los otros campos.
- Entre4 y10 contadores uint64 decimales completos, sin signos/texto residual;
  líneas cpuN con el mismo número de columnas y N uint32 no duplicado.
- Al menos una CPU visible, máximo100000, con IDs ordenados antes de comparar
  muestras. Orden distinto de las líneas no implica cambio de CPUs.
- btime opcional, uint64 completo y no duplicado cuando aparece.

Los cuatro primeros campos permiten esquemas antiguos; campos opcionales ausentes
se representan internamente como0, no como una lectura de campos presentes. Un
cambio de esquema entre muestras invalida el intervalo. No se admite un prefijo
incompleto de una fila de esquema conocido ni columnas no numéricas. Nuevos
esquemas de más de10campos requieren actualizar este proveedor.

CPU/btime hasta64KiB por línea; otras líneas de /proc/stat se ignoran, sujetas al
límite total. El parser no valida estadísticas ajenas a esta unidad. Error de
apertura/lectura/límite/parseo devuelve ausencia de contadores, nunca una muestra
parcial. Una lectura CPU inválida limpia inmediatamente su baseline en Monitor.
No se deducen permisos específicos de un stream fallido.

## Intervalo y porcentaje

Se comparan dos muestras válidas con el mismo esquema, conjunto visible de CPUs
y btime (incluida su presencia). Cada uno de los ocho estados agregados usados
debe ser no decreciente. Retroceso de cualquiera, cambio visible de CPU/arranque/
esquema o suma de deltas fuera de uint64 deja el porcentaje ausente con nota.
Un total sin avance también es ausencia, no0% medido. Se vuelve a establecer una
baseline; después de una lectura inválida hacen falta dos lecturas válidas para
recuperar un porcentaje. No se interpolan intervalos inválidos.

Se suman **deltas de estados**, no totales absolutos: un total absoluto que no
cabe en uint64 no impide usar un intervalo pequeño representable. Se comprueba
la suma antes de cada adición. Idle es el subconjunto de deltas idle+iowait.
La resta total-idle está acotada por construcción; no hay wrap ni clamp para
ocultar una inconsistencia. Porcentaje=100*(total-idle)/total, calculado con
intermedio long double y publicado en rango0..100 como Support::estimated.
Un intervalo válido solo idle/iowait produce0; solo estados ocupados produce100.

Se usan user,nice,system,idle,iowait,irq,softirq,steal, sin sumar otra vez guest
ni guest_nice. La fórmula mantiene iowait como no ocupado y steal dentro del
resto: es una convención de contabilidad del scheduler, no medición de ejecución
física o utilidad ponderada por frecuencia. El kernel documenta que iowait puede
retroceder; rechazar ese intervalo no afirma un fallo de hardware o del kernel.
[Campos oficiales de proc_stat e incertidumbre de iowait](https://man7.org/linux/man-pages/man5/proc_stat.5.html).

## Procesos e integración

Desde D04, linux_cpu_interval es guardia de contexto para CPU por proceso, no
su denominador numerico. Sin intervalo global valido no publica esos porcentajes;
el ratio usa reloj propio, CLK_TCK y capacidad de todas las CPU visibles, no un
core ni cuota cgroup. Ver LINUX_PROCESS_CPU.md.

Desde H01, el informe separado linux_app_history acumula duracion utime/stime
con identidad de ejecutable y fracciones conservadas. Requiere contexto/boot y
unidades validos, pero no un delta global positivo: un retroceso de iowait global
no borra por si mismo intervalos CPU propios validos. No es porcentaje ni usa
el denominador global. Ver LINUX_APP_HISTORY.md; legacy app_history queda vacio.

Ocultar procesos no detiene la captura CPU global. Reactivar procesos sigue
exigiendo sus nuevas baselines, sin reutilizar vectores ocultos anteriores.
Discos/red mantienen sus relojes y proveedores independientes. Fallos de otros
subsistemas de Monitor conservan su contrato previo: esta unidad no introduce
una transacción de toda la captura.

Los IDs visibles/btime no son identidad global de host ni detectan todos los
hotplugs/reinicios intermedios. Las filas CPU no forman una lectura atómica de
hardware. La vista procfs puede no corresponder a cuota cgroup o CPUs accesibles
por afinidad. No hay ajuste de namespace, frecuencia ni límite de potencia. Los
límites son de datos, no RAM total ni deadline de procfs.

## Verificación

### Diagnóstico de intermitencia, 2026-10-16 (N03 parcial)

Los fallos históricos de linux-integration no guardaron la causa del rechazo;
no prueban por sí solos un retroceso de iowait. Ahora el cálculo y su guardia
comparten una única evaluación que conserva las mismas reglas, y la nota de
una métrica ausente distingue:

- Contadores actuales ausentes (error exacto de lectura/parseo desconocido) o
  baseline anterior ausente (primera muestra o baseline reiniciada).
- Contadores anteriores/actuales inválidos, cambio de esquema, CPUs o token boot.
- Primer estado que retrocede, con nombre y valores uint64 anterior/actual.
- Overflow de la suma de deltas, con el estado en que ocurre, o falta de avance.

Las notas son diagnóstico humano, no códigos estables para parsear. No se
atribuyen permisos ni reinicio real del host a partir de estos motivos. Si hay
múltiples problemas se informa el primero según el orden de validación. No hay
nuevo layout/API pública, logging automático de la biblioteca ni nuevas lecturas
del sistema. linux-integration imprime la nota del mismo Monitor que falla,
antes de la aserción original: no usa lecturas paralelas para inferir la causa,
no elimina la aserción ni reintenta hasta obtener un resultado favorable.

Fixtures verifican cada motivo, los ocho estados decrecientes con sus valores,
y que no se produzca porcentaje ni cambie el soporte de ausencia. Los cálculos
válidos de 0/50/75/100 y la recuperación siguen comprobados. Windows ejecuta
estos fixtures portables, no un proveedor Linux nativo.

En esta ejecución acotada, 100/100 repeticiones de linux-integration pasaron;
el fallo histórico NO se reprodujo. Eso valida el recorrido normal, no su causa
ni una corrección de la intermitencia. La captura precisa queda preparada para
su próxima aparición. N03 sigue abierta; no se inventa una subtarea cerrada ni
se amplía el denominador por esta instrumentación.
Suite completa Release Linux83/83 y Windows103/103 aprobadas, paquetes
reinstalados y consumidores externos relinkados/ejecutados con exit0.

2026-10-15: Release Linux65/65 y Windows85/85; paquetes reinstalados y consumidores
externos reconstruidos/ejecutados exit0. Nuevo test linux-cpu-usage, assertions
activas: parseo/esquemas/duplicados/columnas/rangos y límites, guest sin doble suma,
retroceso de cada estado (incluido iowait), cambio de boot/IDs/esquema,0/100,
intervalo sin avance, suma absoluta grande con delta válido, overflow de deltas
y secuencia de pérdida/recuperación de baseline.

Ubuntu WSL nativo:32CPUs visibles, porcentaje global3,52113% y del propio test
3,52113% en la pasada registrada. Bucle breve solo del test, sin lanzar/modificar
otros procesos. Primera muestra ausente y opt-out/reenable de procesos verificados.
Windows ejecuta fixtures del cálculo portable, no lectura procfs Linux nativa.
Hotplug/reset real, lectura procfs denegada e iowait decreciente reales no fueron
provocados: esos casos se cubren con fixtures. Otros kernels/arquitecturas/libcs
requieren cobertura adicional; no es validación de equivalencia con Task Manager.
