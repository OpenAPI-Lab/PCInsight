# RSS por proceso Linux: unidades, disponibilidad e identidad

`ProcessInfo.memory_bytes` continúa siendo el campo público existente. En Linux
representa VmRSS de `/proc/PID/status` convertido a bytes, ahora con
Support::estimated y nota de alcance. No es memoria privada/única, PSS, memoria
virtual asignada, swap ni el equivalente exacto del contador de Windows.
Las páginas compartidas pueden contarse en varios procesos; sumar RSS de un
usuario no produce necesariamente su consumo físico único.

Se retiró stoull(VmRSS)*1024 sin validación, que podía aceptar prefijos, ignorar
unidades y desbordar la multiplicación o provocar que se perdiera la fila al
lanzar una excepción de conversión dentro del colector legacy.

## Parseo y disponibilidad

El proveedor propio reconoce VmRSS y Pid por clave/colon, valida un único valor
decimal uint64 con unidad exacta kB y comprueba el producto por1024 antes de
convertir. No acepta signos, fracciones, exponentes, unidades adivinadas, texto
residual ni duplicados de esos campos, incluso sin espacio después del colon.
Pid, cuando aparece, es uint32 positivo y único. En la lectura nativa es
obligatorio y debe coincidir con la identidad esperada.

VmRSS0kB es un cero reportado válido estimated. VmRSS ausente (posible en tareas
del kernel/procesos saliendo) no se convierte en cero. No hay fallback que deduzca
RSS de otros contadores, tamaño de ejecutable, memoria global o falta de permiso.
Un campo malformado/duplicado, un status sin PID correcto o una fuente inaccesible
dejan el Metric sin valor; no publican un valor parcial. El resto de campos
desconocidos de status se ignora, sujeto a los límites de texto.

Límites de contenido aceptado: status1MiB/10000líneas/64KiB por línea, stat64KiB;
NUL invalida status. Lecturas en bloques8KiB, sin reservar1MiB por cada proceso
pequeño. Un bloque que detecta exceso se descarta, sin publicar su prefijo.
EACCES/EPERM obtenido de syscalls se expresa permission_required; otros errores
o formato/rango inválido unsupported. Reintentos de read por EINTR limitados a3.
Los límites no son timeout de syscalls ni garantía contra bad_alloc global.

## Atribución al proceso

La fila inicial ya tiene PID/start_ticks de stat. Para recoger su RSS:

1. Abrir y fijar el directorio del proceso con O_PATH/O_DIRECTORY/CLOEXEC.
2. Leer stat mediante openat sobre ese directorio y comprobar PID/start_ticks
   contra la identidad esperada antes de leer memoria.
3. Leer status acotado por openat y validar Pid/VmRSS.
4. Volver a leer stat sobre el mismo directorio y exigir la misma identidad.

Los archivos finales no se siguen si son symlinks; los descriptores se cierran
por RAII también en errores/excepciones. Un proceso desaparecido, stat inválido
o discrepancia de PID/start_ticks no recibe la memoria leída. Start_ticks0 no
es un sentinel de error: puede ser un token válido al inicio del boot.

Es un token de vida PID/ticks del mismo boot y namespace, no identidad global
criptográfica ni generación de exec. La resolución de ticks es finita; no se
certifica que jamás exista una colisión antes de fijar el directorio. El pin
evita que accesos posteriores por ese fd cambien normalmente a otro directorio
por reutilización del nombre PID. No congela el proceso: RSS puede cambiar
durante/después de la lectura, y un exec conserva PID/start_ticks. Se confía en
procfs del entorno; los fixtures de archivos no son autenticación de procfs.

RSS del kernel puede actualizarse asíncronamente. No se pide igualdad de lecturas
independientes ni se presentan bytes como snapshot físico exacto. Para un dato
de smaps/PSS se necesita un proveedor diferente, no implementado aquí.

## Alcance de la integración

El colector asigna el Metric nuevo a cada fila en vez de interpretar VmRSS en
su bucle antiguo de State/Uid. No cambia la API/layout ni el backend Windows.
No elimina ni altera procesos, no fuerza carga de RAM, no lee /proc/PID/mem,
no usa utilidades ni instala dependencias. Monitor sin recolección interna de
procesos no ejecuta este camino.

Esta unidad no robusteció todo fill_processes: State/Uid, nombre, ELF, CPU,
tasas E/S y acumulación de historial conservan sus caminos/limitaciones propios.
Los demás campos de la fila no son un snapshot atómico con RSS. El manejo de
recursos/errores del colector legacy en conjunto sigue pendiente; no se declara
todo Processes/Details completo por validar esta métrica.

## Verificación

Fixtures portables: ausencia vs cero, kB exacto, overflow uint64/producto,
duplicados/texto/PID inválidos, límites e identidad antes/después, incluyendo
reutilización, desaparición y start_ticks0. Archivos privados Linux prueban
openat/lectura acotada, stat/status ausente o malformado, PID/start equivocados,
archivo excesivo y rechazo de symlink final. Cambios de identidad entre lecturas
se ejercitan en el guard puro, no con reutilización forzada de PID real.

Prueba nativa del propio PID: RSS positivo estimated, identidad esperada
concordante y token falso rechazado; integración Monitor conserva la fila propia
y etiqueta las memorias publicadas como estimated. No exige igualdad de los
valores dinámicos entre muestras. Denegación de permisos se prueba como
propagación de resultado sintético; no se alteran permisos de otros procesos.

Pasada del 2026-10-15: Linux73/73 y Windows93/93 Release; instalación y
consumidores externos exit0. Prueba focalizada Linux repetida con salida
detallada: propioPID1539/start_ticks5173, RSS4538368B, Monitor5976064B estimated;
token de inicio incorrecto rechazado. Son dos muestras de un proceso que cambia
su propio RSS al ejecutar pruebas/Monitor, no dos cifras que deban ser iguales.

Referencia primaria: [status/VmRSS y límites de precisión del kernel](https://www.kernel.org/doc/html/latest/filesystems/proc.html).
Validación, límites y política de atribución son decisiones de PCInsight.
