# Enable/disable XDG startup: edición y estado de flags

Unidad funcional terminada y verificada el 2026-10-14: editar los flags de un
archivo `.desktop` seleccionado mediante `set_startup_enabled(path, enabled)`
y leer su estado sin confundir grupos de Desktop Actions.

La implementación anterior reemplazaba claves de cualquier grupo y agregaba
flags al final del archivo, que podía ser otro grupo. También truncaba un
temporal de nombre fijo. Esas rutas se sustituyeron por un editor por grupos
y un reemplazo con temporal exclusivo.

## Contrato de edición

- Solo modifica Hidden y X-GNOME-Autostart-enabled de `[Desktop Entry]`.
  Enabled=true escribe Hidden=false/GNOME=true; disabled hace lo contrario.
- Inserta las claves faltantes antes del siguiente grupo, no en Desktop Action.
  No modifica Exec, otros grupos, comentarios ni nombres localizados.
  Conserva LF/CRLF y líneas no modificadas; una inserción puede añadir newline.
- Exige Desktop Entry como primer grupo y una única ocurrencia de ese grupo;
  rechaza flags duplicados, NUL, estructura de grupos malformada y límites.
  No es un validador completo de toda la especificación Desktop Entry.
- La acción recibe **el archivo que se editará**. No crea automáticamente un
  override personal de una entrada del sistema ni cambia quién la utiliza.
  La API separada posterior set_startup_enabled_for_current_user implementa
  ese alcance personal; véase XDG_OVERRIDE.md.
  Editar un archivo system-wide puede afectar varios usuarios si el llamador
  tiene esos permisos; el GUI debe seleccionar el destino y autorizarlo.
- No ejecuta Exec ni otras aplicaciones, no usa shell y no eleva privilegios.

## Archivo, metadatos y commit

Límite de 1 MiB por archivo. Se abre el directorio y luego el archivo con
openat/O_NOFOLLOW; se rechazan enlaces simbólicos en el archivo final, hardlinks,
directorios/dispositivos y archivos mayores del límite. Ancestros symlink no
se rechazan: el directorio queda anclado por su descriptor una vez abierto.
No es una barrera de autorización para un helper privilegiado con paths hostiles.

El lock advisory del directorio serializa operaciones de esta implementación;
otra operación concurrente recibe stale_target. Un escritor externo que ignore
ese lock no queda bloqueado. Se compara dispositivo/inodo/tamaño/mtime/ctime
tras leer y antes de renombrar, pero no existe CAS atómico para modificaciones
externas entre la última verificación y rename. Trabajar sobre directorios de
confianza y coordinar escritores; no se promete ausencia de toda carrera.

Temporales con O_EXCL/O_NOFOLLOW en el mismo directorio; una colisión no trunca
ni elimina un temporal preexistente. Se preserva UID/GID, modo y xattrs legibles;
se eliminan atributos heredados adicionales que no tenía el original. Un fallo
al preservar ownership, ACL, etiqueta de seguridad u otro atributo cancela el
commit. Sistemas SELinux/ACL adicionales requieren validar sus permisos de
preservación; la biblioteca no elimina metadatos para forzar éxito.

Se escriben/sincronizan/cierran datos, se revalida el destino y se usa renameat.
Errores detectados antes del rename no reemplazan el original; se retira solo el
temporal propio. Después se sincroniza el directorio. Si esa última operación
falla, el resultado dice explícitamente que **los flags ya fueron aplicados**;
un error no siempre significa ausencia de efectos y no debe reintentarse a ciegas.
Garantías de atomicidad/durabilidad finales dependen del filesystem/dispositivo.
No hay backup automático ni recuperación de datos modificados por otros procesos.

Metadatos bounded: hasta 64 KiB para nombres/valor de cada xattr y hasta 1 MiB
de valores totales. Atributos que cambian durante la lectura invalidan la acción.
No se conservan timestamps anteriores: una edición cambia tiempos/inodo.

## Estado leído

El lector usa únicamente Name/Exec/Hidden/GNOME del grupo principal. Entradas
inaccesibles, duplicadas o malformadas no se presentan como enabled: unknown.
Hidden=true o GNOME=false implica disabled; flags válidos no deshabilitantes
implican enabled como **configuración**, no como aplicación realmente iniciada.
Los valores de booleanos inválidos producen unknown.

No evalúa TryExec, OnlyShowIn/NotShowIn, condiciones del escritorio ni elegibilidad
de sesión. Directorios personalizados y precedencia por nombre se resolvieron
posteriormente en [XDG_PRECEDENCE.md](XDG_PRECEDENCE.md), para el contexto del
proceso llamador. Publisher y startup impact siguen pendientes. Estos cierres
no completan toda la sección startup.

## Evidencia de cierre

Release Linux 13/13 y Windows 25/25. En Windows se verifica el parser portable;
no se afirma que allí se editen entradas XDG.
Instalación y consumidor CMake externo verificados en Linux y Windows después
de los cambios finales.

Pruebas Linux nativas en directorios privados de `/tmp`, como usuario normal:
enable/disable/idempotencia, estado coherente, CRLF/no-newline, Desktop Actions,
comentarios/Exec/localización, duplicados/límites, rechazo de symlink/hardlink,
permisos denegados, lock concurrente y original intacto en errores probados.
Se preservaron modo/UID/GID y user xattr. Una ACL default con usuario adicional
en el directorio de prueba no se introdujo en el archivo reemplazado. El temporal
preexistente de colisión se conservó y no quedaron temporales propios.

No se editaron entradas reales de autostart ni se inició una app de startup.
Esto prueba edición/lectura de flags, no el comportamiento de un login gráfico.

Referencias primarias: [Autostart XDG](https://specifications.freedesktop.org/autostart/latest/),
[formato Desktop Entry](https://specifications.freedesktop.org/desktop-entry/latest/basic-format.html).

Actualizacion2026-10-19: editor declarado AppStream disponible mediante opt-in;
ver [LINUX_STARTUP_PUBLISHER.md](LINUX_STARTUP_PUBLISHER.md). Los parrafos de
estado originales son historicos. No es firmante verificado ni modifica flags.
