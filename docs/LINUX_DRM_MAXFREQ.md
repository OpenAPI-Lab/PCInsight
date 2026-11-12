# DRM: ciclos normalizados por frecuencia máxima

El lector, sampler, agregador y pipeline opt-in Monitor admiten el par
drm-cycles-N / drm-maxfreq-N cuando no hay drm-total-cycles-N. No ejecutan
utilidades, abren dispositivos GPU ni cambian configuración del driver.
Se mantienen los controles de proceso/PID/start_ticks, descriptor, cliente y PCI.

## API y semántica

LinuxDrmEngineUsage.counter_kind vale cycles_max_frequency; busy_cycles conserva
el contador y max_frequency_hz la frecuencia normalizada a Hz, ambos available.
total_cycles está ausente. El porcentaje se publica estimated, calculado así:

    100 * delta(busy_cycles) / elapsed_seconds / max_frequency_hz / capacity

No es tiempo activo: una carga activa durante todo el intervalo a la mitad de la
frecuencia máxima puede dar50%, no100%. Tampoco mide rendimiento útil, ni carga
global GPU. No convertir estos ciclos a nanosegundos como si fueran tiempo activo.
busy_nanoseconds solo es interpretable como base de tasa si counter_kind es
nanoseconds; comprobar siempre el discriminante.

Si el mismo motor publica ns y el par ciclos/maxfreq, se selecciona el par
normalizado por frecuencia y se conserva el contador ns raw como metadato.
Si publica ns y ciclos/total, se sigue prefiriendo la base ns como antes.
No se mezcla ninguna de las tres bases al calcular un porcentaje o una suma.

El agregado usa summed_client_frequency_percent (nunca summed_client_active_percent
ni summed_client_cycle_percent). Expone counter_kind y max_frequency_hz. La suma
observada de clientes sigue siendo estimated, puede superar100 y NO sustituye
GpuInfo.utilization_percent. Clientes de una clase con frecuencias, capacidades o
bases distintas no se suman; los campos incompatibles quedan ausentes.

## Formato y continuidad

Se aceptan unidades explícitas Hz, KHz y MHz, con espacio/tab entre entero y
unidad, respetando mayúsculas de la especificación. No se adivinan unidades:
un valor sin sufijo se rechaza conservadoramente, aunque el formato general
permita omitirlo. No se aceptan fracciones, signos, frecuencia cero, unidades
desconocidas, basura posterior ni overflow uint64 al escalar. Los mismos límites
de64KiB/4096B por línea/128 motores siguen activos. maxfreq huérfano, duplicado o
junto con total-cycles para el mismo motor rechaza el registro. Estos controles
también se aplican si hay un contador ns alternativo: no se ignora un maxfreq
malformado para producir una tasa aparentemente válida.

La primera lectura y cambios de identidad, capacidad, base o frecuencia máxima
crean una base nueva. Un intervalo CPU no positivo no produce porcentaje;
para timestamp igual se retiene el ancla busy previa. Retroceso del reloj CPU
reinicia base. Retroceso del contador busy conserva high-water y espera al driver.
Un delta cero con intervalo positivo es cero medido. Resultado superior a100 o
no finito queda ausente, sin recorte. Los cálculos restan enteros antes de
convertir y no multiplican ciclos/frecuencia en uint64.

Copias del mismo cliente deben concordar en frecuencia normalizada y contador;
duplicados no se cuentan dos veces. Porcentajes suministrados por el consumidor
se ignoran y recalculan desde raw. La API0.x ampliada requiere recompilar
biblioteca y consumidores juntos.

## Cobertura

El formato no garantiza que el driver publique estos campos ni que habilite
profiling. No se escribe sysfs para activarlo. Se conserva la restricción PCI
del lector: esto no añade soporte a GPU de plataforma sin drm-pdev/PCI, como
muchos dispositivos Panfrost. No se usa BIOS para inventar un contador ausente.
No certifica hardware Linux físico, NVIDIA propietario ni cobertura global.

Pruebas: regresión del parser primero fallida; unidades/conversión/overflow,
porcentajes50/0/100, capacidad, intervalos subsegundo, reloj igual/retrocedido,
high-water, cambio de frecuencia, uint64 límite, duplicados/conflictos,
frecuencias y bases mezcladas, input adulterado y correlación Monitor por PCI.
Resultado final Release: Linux49/49 y Windows62/62; bibliotecas/headers
instalados y consumidores externos reconstruidos y ejecutados en ambos.
La verificación Linux se ejecutó en WSL, no en el backend Windows.

Fuente: [especificación DRM de ciclos y frecuencia máxima](https://www.kernel.org/doc/html/latest/gpu/drm-usage-stats.html).
Limitación de profiling/plataforma: [documentación oficial Panfrost](https://www.kernel.org/doc/html/latest/gpu/panfrost.html).
