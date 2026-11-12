# Directorios y overrides efectivos de startup XDG

Unidad funcional terminada el 2026-10-14: resolver directorios XDG y elegir
una entrada por nombre de archivo, con precedencia del contexto del proceso.
`Monitor::sample(include_startup_entries=true)` usa este resolver nativo Linux.

## Contexto y precedencia

Se consulta el entorno del llamador: XDG_CONFIG_HOME para configuración personal
y XDG_CONFIG_DIRS para directorios del sistema, en orden. Solo se usan paths
absolutos; componentes relativos/vacíos se ignoran. Si no hay config home válido,
se usa HOME/.config; HOME inválido/ausente tiene fallback al home de geteuid vía
passwd/NSS. Sin directorio system válido, se usa /etc/xdg. Se agrega /autostart.
Paths duplicados se eliminan sin cambiar la prioridad.

La primera entrada con un nombre `.desktop` gana. Una copia personal Hidden=true
enmascara las copias del sistema: estas no reaparecen como enabled. Un archivo
ganador inaccesible o malformado queda unknown; no se vuelve a una copia inferior
como si el override no existiera. Los nombres son case-sensitive en Linux;
no se deduplica por Name ni por comando Exec.

`StartupEntry.id` sigue siendo el path seleccionado, compatible con la acción
que edita un archivo específico. `source` indica personal/system efectivo y
agrega un diagnóstico si no se puede demostrar precedencia/lectura.
El orden de salida es estable por nombre de archivo.

**Cambio de alcance:** ya no se mezclan indiscriminadamente todos los perfiles
del sistema. Es el contexto XDG de este proceso, no el entorno supuesto de cada
usuario ni necesariamente el de la sesión gráfica si el monitor corre en un
helper elevado. El consumidor debe ejecutarlo en el contexto que desea consultar.
No se captura ni modifica el entorno de otras sesiones. El host debe coordinar
modificaciones concurrentes de su propio environ; no cambiarlo mientras se lee.

## Errores y límites

Un directorio inexistente no impide resolver los siguientes. Otros errores de
enumeración invalidan la confianza en las entradas inferiores: unknown en vez
de enabled. Las entradas ganadoras de directorios superiores que ya se leyeron
siguen siendo válidas. Se conservan diagnósticos internos de enumeración.

Directorios/FIFO/otros objetos no regulares no son entradas de aplicaciones.
La lectura Linux usa O_NONBLOCK y comprueba el tipo del descriptor para evitar
bloqueos si un path cambia a FIFO. Symlinks a archivos regulares se pueden leer;
symlinks rotos conservan una entrada desconocida y no activan una copia inferior.
Esto no constituye un sandbox para leer paths manipulados en un helper privilegiado.

Hasta 64 directorios, paths de 32 KiB, 100000 entradas inspeccionadas y archivos
de 1 MiB. Metadata de salida limitada a 64 MiB de bytes de cadenas (no límite
RSS): si se excede se lanza error, sin publicar un payload ilimitado. Environment
de directorios excesivo también produce error. El Sampler captura esas excepciones.
El snapshot del filesystem no es atómico.

`XdgStartupInventory.complete` es interno y solo indica enumeración, no validez
de todos los archivos. El API público actual no expone un diagnóstico global si
ningún directorio puede enumerarse; una lista vacía no prueba cobertura completa.
Los estados de las entradas que sí se devuelven conservan la incertidumbre.

## Criterio de cierre y evidencia

Release Linux 14/14 y Windows 26/26. Fixtures verifican defaults/personalización,
paths relativos, duplicados, prioridad entre directorios system, Hidden personal,
override malformado/inaccesible, directorio inexistente/no-directory, límites,
permisos de directory/file, symlink roto y FIFO sin bloquear.

Integración Linux normal: 3 entradas efectivas nativas, sin duplicados por nombre.
Otra prueba cambia **solo** XDG_CONFIG_HOME/XDG_CONFIG_DIRS del proceso de prueba
a directorios temporales propios: Monitor devuelve únicamente el override
personal disabled y oculta la copia system. Se restaura ese entorno al terminar.
No se editaron archivos de autostart reales ni configuración del sistema.
Opciones include_startup_entries false/true/false verificadas.
Paquetes instalados y consumidor CMake externo ejecutados correctamente en
Linux y Windows después de los cambios finales.

Este cierre no evalúa OnlyShowIn/NotShowIn/TryExec, condiciones específicas del
escritorio, publisher/startup impact ni creación de overrides personales desde
la acción enable/disable. Status sigue siendo configuración de flags, no prueba
de que la app vaya a ejecutarse en el siguiente login.

Actualización posterior: OnlyShowIn/NotShowIn/TryExec se implementaron como campo
separado en [XDG_ELIGIBILITY.md](XDG_ELIGIBILITY.md). Condiciones específicas,
publisher/impact y creación de overrides continúan pendientes.

Referencias primarias: [precedencia Autostart XDG](https://specifications.freedesktop.org/autostart/latest/),
[variables/directorios base XDG](https://specifications.freedesktop.org/basedir/latest/).
