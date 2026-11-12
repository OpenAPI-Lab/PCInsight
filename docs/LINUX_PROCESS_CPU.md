# CPU por proceso Linux (D04)

`ProcessInfo.cpu_percent` es una estimacion del tiempo CPU del proceso frente a
la capacidad de todas las CPU visibles en procfs, no el porcentaje de un core
ni una medida equivalente de utilidad/frecuencia de Windows.

## Calculo

```
100 * (delta utime + delta stime) / CLK_TCK / segundos_propios / CPUs_visibles
```

Se conservan utime/stime por separado, se comprueba su suma uint64 y se rechaza
el retroceso de cualquiera aunque aumente el total. El parser exige PID positivo,
estado alfabetico y texto sin NUL, ademas de sus limites previos. Guest ya esta
incluido en utime: no se suma otra vez. No se suman cutime/cstime de hijos.
Fuente/unidad: [proc_pid_stat, campos14/15](https://man7.org/linux/man-pages/man5/proc_pid_stat.5.html).

El reloj steady_clock propio se registra inmediatamente despues de leer/parsear
stat, antes de RSS/status/NSS/ELF. La tasa no divide ticks de esa fila por un
intervalo de otra fila o por el intervalo global de Monitor. Se usan segundos
long double a partir de ambas epocas para evitar overflow al restar durations
enteras a traves del rango completo. CLK_TCK debe ser positivo; no se sustituye
por1 si sysconf falla. Una CPU ocupada por completo en un host de32CPU representa
aproximadamente3,125% en esta convencion. Todos los threads del proceso estan
incluidos en la contabilidad de /proc/PID/stat; no se multiplica por thread_count.

## Continuidad y validacion

- Dos observaciones propias accesibles, mismo PID/start_ticks y tiempo creciente.
- Contexto global proc_stat valido en ambas muestras: mismo esquema, CPU IDs,
  boot time (incluida disponibilidad), estados no decrecientes y total con avance.
  Se reutiliza linux_cpu_interval como guardia, no como denominador numerico.
- Mismo CLK_TCK entre muestras. Cambios de contexto/unidades reinician la
  referencia; fuente global invalida elimina todas las referencias de procesos.
- Dato propio invalido, ausente o no confiable elimina su referencia. Permiso
  denegado se conserva como tal. Desaparicion/omision de PID lo olvida.
- Primera muestra o recuperacion tras ausencia: sin porcentaje, nunca0 inventado.
  Dos muestras validas con ticks constantes y contexto adecuado si producen0.
- Resultado finito entre0y100 con Support::estimated. Exceso/rango invalido no
  se clampa ni se convierte en100; intervalo descartado y nueva referencia.
- Desactivar toda captura interna de procesos reinicia el tracker. Usuarios o
  historial activados siguen necesitando captura interna aunque no se devuelvan
  filas de procesos; ocultarlas no significa que se haya detenido el muestreo.
- Max100000observaciones; PID0/duplicado/lote excesivo lanza invalid_argument y
  reinicia el tracker. Enumeracion nativa limita100000candidatos numericos antes
  de acumular mas filas; exceso aborta Monitor explicitamente, no trunca silencioso.

## Relacion con CPU global e historial

CpuInfo.utilization_percent mantiene su formula de estados del scheduler y su
propia ventana. D04 estima capacidad por tiempo propio y cantidad de CPU: NO
promete que la suma de filas coincida con CPU global ni con100, porque ventanas
y cobertura son distintas. Tampoco es tiempo de IRQ/steal causado por cada app.
Un iowait global decreciente puede invalidar el contexto conservador sin indicar
un error del kernel. Ver LINUX_CPU_USAGE.md.

La normalizacion no representa cuotas/afinidad de cgroups, CPU offline oculta,
frecuencia/turbo, potencia o rendimiento fisico; no convierte esa vista en una
medida del limite asignado al contenedor. Los CPU IDs se toman de proc_stat y
no son inventario fisico certificado. No se detecta todo hotplug intermedio ni
cambio A->B->A entre muestras. El token start_ticks es local al boot/namespace,
no generacion exec ni identidad universal. Muestreo secuencial, tick cuantizado
y latencias de lectura impiden atomicidad; intervalos muy cortos pueden quedar
sin ratio por cuantizacion/rango. No hay deadline global de procfs o del colector.

Actualizacion H01: `linux_app_history` acumula duracion observada por ruta de exe
con resto fraccionario exacto y disponibilidad. Comparte el helper utime/stime,
pero no el denominador porcentual ni la exigencia de delta global positivo.
Exige contexto/boot/CLK_TCK validos y continuidad de ejecutable. Ver
LINUX_APP_HISTORY.md. Legacy app_history queda vacio; HistoryStore no guarda
el nuevo informe. D04 no cambio layout; H01 si amplia SystemSnapshot0.x y exige
recompilar consumidores. A01 cerro despues con vista comun y alias legacy
explicito, METRIC_AVAILABILITY.md; A05/N03 siguen con sus alcances propios.

## Evidencia

Test linux-process-cpu (asserts activos en Release):0/25/50/100%, distintos
intervalos por fila en un lote, resets individuales con suma creciente, PID
reusado, dato ausente/permiso, sumas invalidas/overflow y rango,
contextos CPU/boot/esquema/hz cambiantes, relojes iguales/inversos/extremos,
lotes invalidos y recuperacion. PID/counters mismatched no se atribuyen.

Nativo WSL: propioPID1495,CLK_TCK100,32CPU visibles, ratio positivo2,87677%;
otro intervalo propio de carga observo12ticks y0,120041s en CLOCK_PROCESS_CPUTIME_ID.
Ventanas diferentes/cuantizadas: no se exige igualdad numerica exacta. Monitor
verifica primera muestra y opt-out/reanudacion. Solo carga breve del propio test,
sin procesos nuevos, cambios de afinidad/prioridad, contenedores o tareas ajenas.
Hotplug/reset/reuso/permisos son fixtures, no eventos fisicos forzados.
Regresion final Release2026-10-16:Linux77/77 y Windows97/97; paquetes
reinstalados y consumidores externos relinkados/ejecutados exit0. Windows
ejecuta calculos portables/fixtures, no la captura procfs nativa.
