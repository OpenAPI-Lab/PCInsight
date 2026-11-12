# Inicio Windows: inventario de registros Run

Monitor llena startup_entries cuando include_startup_entries=true. Es opt-in,
sin ejecutar comandos, procesos auxiliares, cambios en el registro ni elevación.
Solo usa Advapi32 ya enlazada y APIs del sistema. No instala dependencias.

## Cobertura de esta unidad

Se enumera Software\Microsoft\Windows\CurrentVersion\Run en HKCU del caller y
HKLM. En Windows AMD64 se consulta HKCU compartido una vez y las vistas HKLM
64/32 con flags WOW64 explícitos, sin concatenar Wow6432Node. En Windows x86
nativo se consulta una sola vista de cada hive. ARM64/otras arquitecturas fallan
con not_supported al solicitar este proveedor: no se afirma cobertura de sus
vistas dependientes de arquitectura sin implementarlas/verificarlas.

Una fila por valor REG_SZ/REG_EXPAND_SZ no vacío y con nombre no vacío. name es
el nombre del valor, command es el texto raw convertido de UTF-16 a UTF-8.
No expande variables, resuelve ejecutables/atajos ni interpreta quoting o shell.
Se conservan incluso textos que no podrían lanzarse; inventario no prueba validez
del comando. source distingue hive/vista/tipo y advierte registration only.
El id es winrun:HIVE:VIEW:HEX con el nombre UTF-8 completo codificado en hex,
evitando colisiones por separadores dentro del nombre. Mismo nombre en vistas
distintas se conserva como registros distintos, no se deduplica por nombre/app.

status=unknown: estar en Run no certifica aprobación, habilitación efectiva,
elegibilidad, ejecución ni éxito. No se interpreta StartupApproved mediante
bytes privados ni se anuncia enabled/disabled basándose solo en presencia.
publisher queda vacío (nombre de valor no prueba editor); impact=not_measured y
las métricas observadas quedan ausentes. Los campos XDG siguen ausentes.
WINDOWS_STARTUP_PUBLISHER.md añade opt-in separado de CompanyName estimado para
EXE candidato explícito; no firma/autoría verificada ni inferencia desde el nombre.

WINDOWS_STARTUP_FOLDERS.md añade inventario de archivos en carpetas Startup
al Monitor combinado, sin resolver destinos de accesos directos. El lector Run
aislado no incluye esas carpetas. Siguen pendientes RunOnce, resolución de accesos directos, tareas programadas,
políticas, paquetes StartupTask ni otras hives de usuarios. No es inventario
completo de autoruns ni de todos los usuarios. Enable/disable/reset/properties,
aprobación, publisher verificado y medición de impacto Windows quedan pendientes. El id de
lectura no concede permiso de modificación; las acciones Windows siguen sin
proveedor para este tipo de entrada.

## Errores y límites

Clave inexistente se omite. Otros fallos de apertura/enumeración (incluido acceso
denegado o ERROR_MORE_DATA) propagan std::system_error Win32, sin devolver lista
parcial como completa. Monitor propaga el fallo. No hay elevación/reintentos
ocultos. Tipos no textuales, nombre por defecto vacío y comando vacío se omiten;
lista vacía solo significa que estas claves no reportaron registros incluidos.

Se aceptan textos no terminados usando la longitud en bytes explícita. Longitud
impar, NUL incrustado, UTF-16 inválido, duplicado exacto de nombre durante el
recorrido, buffers/longitudes inválidos y API incompleta fallan cerrados.
Límites:10000 valores por clave (también ignorados),16383 unidades UTF-16 por
nombre,65536 bytes por dato,16MiB acumulados raw y16MiB de salida. El límite
raw incluye valores ignorados, evitando crecimiento ilimitado del set de nombres.
Los comandos largos son texto observado, no una promesa de soporte al ejecutarlos.

Todos los handles se cierran incluso en excepciones. La lectura no es atómica:
modificaciones concurrentes pueden omitir/cambiar registros; el chequeo de
duplicados no detecta todas las carreras ni prueba consistencia de todo el hive.
Los límites no imponen timeout a una API del registro bloqueada. Cadenas devueltas
pueden contener rutas/argumentos sensibles; el consumidor decide su exposición.

## Verificación

Fixtures de usuario/sistema, AMD64 dos vistas/x86 una, Unicode/nombres con
separadores, REG_EXPAND_SZ sin expansión, tipos ignorados, datos no terminados,
duplicados, truncamientos/unidades UTF-16 inválidas, API incompleta, permisos,
límites por registro/inventario/payload, clave ausente y limpieza en fallo tardío.
La primera pasada detectó nombres duplicados accidentales en la fixture válida;
se corrigieron los datos de prueba conservando el rechazo de duplicados real.
Prueba nativa de solo lectura y Monitor opt-in/opt-out; no se crea, borra, cambia
ni ejecuta ninguna entrada real. La prueba no exige que existan registros.
MSVC, Windows x86 nativo y otros equipos siguen sin validación nativa; la selección
de vistas x86 se prueba con API inyectada, no se certifica con un build AMD64.

Resultado Release: Windows65/65 y Linux50/50; paquetes instalados y consumidores
externos reconstruidos/ejecutados en ambos. El consumidor Windows también recorre
Monitor con include_startup_entries. Linux verifica regresiones, no el registro
Windows; no se afirma cobertura nativa Windows mediante un build Linux.

Fuentes oficiales: [Run/RunOnce](https://learn.microsoft.com/en-us/windows/win32/setupapi/run-and-runonce-registry-keys),
[RegEnumValueW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regenumvaluew),
[claves compartidas y redirigidas WOW64](https://learn.microsoft.com/en-us/windows/win32/winprog64/shared-registry-keys).
