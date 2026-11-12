# Uptime Linux: lectura acotada y conversión entera validada

`CpuInfo.uptime` sigue siendo `Metric<std::chrono::seconds>`, obtenido del primer
campo de `/proc/uptime`. Se retiró `stod` seguido de conversión directa a entero:
podía aceptar un prefijo incompleto, lanzar excepciones y convertir valores no
finitos/fuera de rango sin una validación apropiada.

El nuevo proveedor no ejecuta `uptime`, `cat` u otras utilidades, no descarga
paquetes y no modifica relojes. Lee como máximo 256 bytes y valida exactamente
dos campos decimales no negativos con dos cifras fraccionarias, como los emite
procfs. Se admite whitespace exterior/entre campos, pero no campos extra,
signos, exponentes, NaN/Inf, NUL, decimales incompletos ni sufijos.

Cada parte entera se analiza como uint64; el primer campo además debe caber en
`chrono::seconds::rep`. No se multiplica por cien/nanosegundos ni se pasa por
double: el máximo representable y sus fracciones no sufren redondeo/overflow
intermedio. La fracción se valida y descarta: 1.99 significa 1 segundo completo,
no 2. `0.00` y `0.99` producen un cero válido con `Support::available`.

El segundo campo se valida, pero no se usa como uptime ni se resta del primero.
Su idle acumulado entre CPU puede superar legítimamente el uptime; no se aplica
una relación idle<=uptime ni se deriva un porcentaje de CPU de estos campos.

Un archivo ausente/ilegible, lectura fallida, exceso de tamaño o formato/rango
inválido deja el Metric sin valor y con nota. No se inventa cero ni se retiene
el último valor de una muestra válida. La conversión no lanza excepciones por
texto numérico inválido; esto no promete ausencia de excepciones de recursos
como bad_alloc en todo el programa. No se adivina permission_required a partir
de un error de stream sin un errno fiable.

## Alcance

Es el boottime que expone el kernel a este entorno, incluyendo suspensión y
posibles offsets de namespace temporal; no es la vida de Monitor, una sesión de
usuario, un proceso ni una certificación del encendido físico del host fuera
de una VM/contenedor. La API publica segundos completos, no centésimas ni una
fecha/hora de arranque basada en el reloj civil. No se usa CLOCK_MONOTONIC como
sustituto: su semántica de suspensión es distinta.

El proveedor es stateless, así que no oculta un reinicio/cambio de vista
forzando monotonía con un máximo histórico. La prueba de no retroceso corresponde
a lecturas consecutivas en el mismo WSL estable, no una garantía entre reinicios,
restauraciones de VM, cambios de namespace o fuentes procfs alteradas. Faltando
procfs, no hay fallback implementado en esta unidad. La lectura es acotada en
bytes, no tiene timeout de syscall. No cambia layout público ni backend Windows.

## Verificación

Fixtures portables: cero, truncación de fracciones, idle mayor que uptime,
máximo de seconds::rep, overflow uint64, negativos, NaN/Inf, texto residual,
truncaciones de un registro y límites de campo/archivo. Fixtures Linux: archivo
privado ausente, directorio, vacío, inválido o excesivo, tamaño exacto aceptado
y limpieza de valores anteriores. Release mantiene asserts.

La prueba nativa intercala `/proc/uptime`, dos muestras de Monitor y otra
lectura procfs; con CLOCK_BOOTTIME disponible, una pareja de lecturas del reloj
encierra la secuencia en segundos completos. Solo lectura: no se fuerza
suspensión/reanudación, reinicio, cambio de namespace ni de reloj.
El consumidor instalado comprueba uptime presente/no negativo en ambos hosts.

Pasada del 2026-10-15: Linux70/70 y Windows90/90 Release; instalación y consumidores
externos exit0. Prueba focalizada Linux repetida con salida detallada: 52 segundos
de uptime en WSL, dentro del intervalo CLOCK_BOOTTIME52..52; secuencia
proc/Monitor/Monitor/proc no decreciente. Es el valor de esa pasada, no uptime
actual ni una medición del encendido del equipo Windows anfitrión.

Fuentes primarias: [implementación proc uptime del kernel](https://raw.githubusercontent.com/torvalds/linux/master/fs/proc/uptime.c),
[documentación procfs](https://www.kernel.org/doc/html/latest/filesystems/proc.html)
y [manual Linux clock_gettime](https://man7.org/linux/man-pages/man2/clock_gettime.2.html).
La política de validación/límites es de PCInsight; no amplía garantías del kernel.
