# Frecuencias CPU Linux: actual y base no son intercambiables

`CpuInfo.current_ghz` y `base_ghz` usan ahora un lector cpufreq propio acotado.
Se retiraron `stod` sobre texto sin validar, promedios de un subconjunto de CPU
que casualmente respondieron y la lectura de base exclusivamente desde CPU0.
SMBIOS Current Speed ya no se publica como base: el parser SMBIOS conserva su
campo interno, pero este proveedor no lo usa como sustituto de base_frequency.
Tampoco se usan frecuencias máximas, límites BIOS o el nombre comercial del CPU
para inventar la frecuencia base.

## Velocidad actual

Se leen los CPU de `/sys/devices/system/cpu/online`, incluyendo IDs dispersos,
sin presumir que CPU0 existe o está online. Cada CPU tiene tres fuentes separadas
bajo `cpuN/cpufreq`: `cpuinfo_cur_freq`, `scaling_cur_freq`, `base_frequency`.

Para `current_ghz` se prefiere cobertura completa de `cpuinfo_cur_freq`. Si no
existe para todos los CPU online, se permite cobertura completa de
`scaling_cur_freq`, con nota explícita de que es un valor del controlador y
puede representar el P-state solicitado, no el reloj real del hardware.
Nunca se mezclan fuentes entre CPU para rellenar huecos. Si ninguna fuente
cubre el conjunto completo, el promedio queda ausente; no parcial ni cero.

La cifra es la media aritmética por procesador lógico online, en GHz. Una
política compartida se observa a través de cada CPU asociado: tiene peso por
número de CPU online, no peso uno por policy ni por núcleo físico. No se
interpreta como frecuencia efectiva de una aplicación, porcentaje de carga,
media ponderada por trabajo, máximo turbo ni una medición simultánea de todos
los CPU. Incluso lecturas de la misma política pueden variar durante el recorrido.

Los valores presentes llevan `Support::estimated` y la nota identifica la
fuente y agregación. Se suman en long double antes de dividir, sin suma uint64
que pueda desbordarse. El resultado se comprueba finito/positivo. No se impone
un rango comercial arbitrario de CPU: validar el formato no certifica hardware.

## Velocidad base

`base_ghz` exige un `base_frequency` válido e idéntico en todos los CPU online,
que además coincida entre dos lecturas consecutivas por CPU. Bases heterogéneas,
fuentes ausentes y cambios de configuración no se convierten en una media ni
en la base del primer CPU. La ausencia de base no borra una velocidad actual
válida. La base presente se etiqueta estimated y no equivale a una verificación
directa del BIOS o de la especificación comercial del fabricante.

La base es un dato de configuración, no una medida dinámica del reloj actual.
No se cambia porque `current_ghz` suba o baje. Se vuelve a consultar al muestrear
para no congelar un dato obsoleto ante cambios de perfil/hardware: no hay caché
permanente que garantice un valor único durante toda la vida de Monitor. Si la
configuración cambia y después estabiliza, puede publicarse la nueva base.

## Estabilidad y límites

Se verifica `online` antes/después de cada recorrido y se requieren dos
inventarios con el mismo conjunto online y filas válidas/únicas. Los valores
actuales pueden cambiar entre lecturas: se usa la segunda muestra completa;
exigir igualdad de los relojes dinámicos sería incorrecto. La base sí exige
concordancia. El primer inventario sirve para validar estructura, conjunto y
base; no es un intervalo con el que se calcule una tasa de frecuencia.

Esto no bloquea hotplug ni cambios de policy/driver; no es snapshot atómico y
no detecta todo cambio seguido de reversión. No se valida una identidad de
policy persistente ni se prueba que los CPU/policies permanezcan iguales
entre cada syscall. Los CPU ocultos/offline, afinidad/cpuset y cuotas cgroup
quedan fuera del promedio. Cada aplicación reemplaza ambos campos y limpia
valores anteriores que ya no sean válidos.

Parser: entero decimal uint64 positivo en kHz, sin signos, fracciones, exponentes,
unidades extra, NaN/Inf, NUL, overflow ni texto residual; admite whitespace
exterior. Archivos de frecuencia hasta 64 caracteres. Lista online hasta
64 KiB/4096 IDs, con rangos normalizados y sin duplicados; lectura acotada.
Errores por archivo quedan como faltantes; lista online inválida/cambiante
invalida todo el inventario. Son límites de trabajo, no timeout de syscalls.

Sin utilidades, paquetes adicionales, acceso directo a registros/MSR/BIOS ni
escrituras a governors/turbo/frecuencias. Si la VM/driver no expone cpufreq,
los campos quedan ausentes. No se implementan otras fuentes como ACPI CPPC
nominal_freq ni telemetría efectiva por ciclos en esta unidad. No cambia el
layout público ni la implementación Windows.

## Verificación

Fixtures portables: límites numéricos, conversión GHz, sumas mayores que uint64,
conjunto online disperso, filas duplicadas/faltantes, cambios de reloj válidos,
selección de fuente completa, base heterogénea/cambiante y borrado de valores
anteriores. En Linux, árbol sysfs privado con CPU0 offline y symlink de policy
compartida verifica ponderación por CPU, archivos malformados/excesivos/ausentes,
CPU sin cpufreq y rechazo de max como base. Release conserva asserts.

Verificación del 2026-10-15: Linux69/69 y Windows89/89 Release, instalación y
consumidores externos exit0. Prueba focalizada Linux repetida con salida
detallada: 32 CPU online; actual/base ausentes en WSL sin estas fuentes,
ausencia end-to-end comprobada. No se fuerza carga, turbo ni cambios de governor.
La obtención de frecuencias positivas nativas en Linux físico sigue pendiente;
las cifras positivas y sus conversiones quedaron probadas mediante fixtures.

Fuentes primarias: [interfaz y semántica CPUFreq del kernel](https://www.kernel.org/doc/html/latest/admin-guide/pm/cpufreq.html)
y [base_frequency en Intel Speed Select](https://www.kernel.org/doc/html/latest/admin-guide/pm/intel-speed-select.html).
La política de cobertura completa, agregación y ausencia ante bases distintas
es de PCInsight; no es una garantía adicional del ABI del kernel.
