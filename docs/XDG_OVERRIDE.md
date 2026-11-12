# Startup solo para el usuario actual

Unidad funcional terminada: `SystemActions::set_startup_enabled_for_current_user`
crea o edita un override personal XDG, sin modificar el archivo origen del sistema.
La acción anterior `set_startup_enabled` sigue siendo edición directa del path
seleccionado; no se cambió silenciosamente su contrato. Usar la nueva variante
para el toggle personal del GUI, especialmente cuando el inventario indica system.

```cpp
auto result = actions->set_startup_enabled_for_current_user(entry.id, false);
// Recargar startup_entries: el ID efectivo pasa a ser el path de la copia personal.
// No ejecuta ni detiene procesos; cambia configuración de futuros logins.
```

## Destino y precedencia

La ruta fuente debe ser absoluta, .desktop, sin NUL, y pertenecer directamente a
uno de los directorios autostart del contexto XDG del proceso. Destino:
XDG_CONFIG_HOME/autostart/<mismo filename> o HOME/.config/autostart según el
resolver existente; HOME inválido usa la cuenta efectiva NSS como fallback.
Se rechaza contexto sin directorio personal o que coincida con un directorio
system de XDG_CONFIG_DIRS. Fuentes arbitrarias fuera de esos directorios no crean
un nuevo programa de autostart por accidente.

Si existe destino personal, sus Name/Exec/comentarios/acciones y metadatos
prevalecen: se modifican solo Hidden/GNOME mediante el editor ya verificado.
No se refresca desde system ni se sobrescriben customizaciones, incluso si el
GUI todavía envía un source ID de system. El origen del sistema puede haber
desaparecido: una copia personal existente sigue editable en su propio contexto.

Si falta el destino, se lee el origen de forma acotada, se valida con el lector
existente y se copian sus bytes reescribiendo los flags del grupo principal.
No hay ejecución de Exec/shell/CLI ni elevación de privilegios. Disabled escribe
Hidden=true y GNOME=false; enabled hace lo contrario. Por filename y prioridad
XDG, la copia personal pasa a ser efectiva y oculta copias inferiores.
Enabled no elimina OnlyShowIn/NotShowIn/TryExec ni condiciones vendor: sigue
siendo configuración y puede tener xdg_standard_eligible=false/unknown.

## Publicación y seguridad

Archivo hasta 1 MiB. Source final O_NOFOLLOW/O_NONBLOCK y archivo regular;
se compara identidad/dev/inode/size/mtime/ctime al leer y antes de publicar,
incluida la ruta fuente. Directorio personal final O_NOFOLLOW, propietario UID
efectivo y sin escritura group/world. Si se crea autostart, su modo se fija 0700.
Padres faltantes pueden crearse; un error posterior puede dejar directorios vacíos.
No se cambia el modo de directorios preexistentes para forzar permisos.

Temporales exclusivos O_EXCL (128 colisiones máximo), archivo nuevo modo0600,
owner/GID del caller/FS. No se copian owner, modo, xattrs ni etiquetas del archivo
system a la copia personal: es un archivo del usuario y conserva contexto nativo
de creación. ACLs heredadas quedan limitadas por el modo0600; no se presume una
política SELinux uniforme en todas las distros. Overrides existentes sí conservan
metadata mediante el editor anterior. Symlink/hardlink/owner ajeno del destino
se rechazan, no se siguen para editar otro archivo.

Se sincronizan datos/permisos, se cierra temporal y se publica mediante linkat
sin reemplazo. Destino que aparece concurrentemente -> stale_target; no se
trunca ni sobrescribe. Se elimina solo el temporal propio y se fsync el directorio.
Un fallo después de publicación dice override aplicado/resultado parcial; no
reintentar ciegamente como si nada hubiera cambiado. Filesystem sin hardlink o
permisos suficientes devuelve error explícito, sin un fallback no atómico.
Lock advisory compartido con el editor directo; lectores siguen pudiendo leer.

Se confía en el entorno HOME/XDG, NSS y ancestros del caller; ancestros symlink
pueden existir. No es sandbox ni barrera para un helper root con inputs hostiles,
ni selecciona automáticamente la sesión gráfica de otro usuario. El consumidor
debe coordinar cambios de entorno y escritores externos que ignoran flock.
El chequeo y publicación no son CAS universal frente a reemplazos hostiles de
directorios/temporales por un actor con igual acceso. El modo0600 no oculta datos
de un administrador. Leer system puede actualizar atime; contenido/modo/inodo y
mtime no se escriben. Copias personales no reciben automáticamente actualizaciones
posteriores de un paquete system. El reset separado se documenta en XDG_RESET.md.

## Auditoría y verificación

2026-10-14: Linux Release22/22, Windows Release33/33. Instalación de ambos
paquetes y reconstrucción/ejecución de consumidores externos verificadas.

AuditedActions registra startup_current_user, fuente y enabled, conserva
errores/resultado; no almacena payload Exec del archivo. Destino se indica en
ActionResult de éxito y se obtiene después del inventario. Virtual API/audit enum
0.x ampliados: reconstruir biblioteca y wrappers/consumidores juntos.
Otros backends heredan not_supported, no se imita XDG en Windows.

Tests portables: resolución destino/contexto, invalid paths, conflicto de roles.
Tests Linux nativos con entorno XDG **del proceso de prueba** apuntando a /tmp:
origen de solo lectura intacto, bootstrap/directorio0700/archivo0600, flags y
precedencia observados por Monitor, enable aún excluido de GNOME por OnlyShowIn,
custom Exec preservado, symlink/hardlink rechazados, lock concurrente, temporal
preexistente conservado, ausencia de leftovers, directorio inseguro/contexto
compartido rechazados y fuente malformed sin crear config. Auditoría verificada.
No se editó autostart real ni se lanzó ninguna aplicación; HOME no se modificó.

Referencia: [override personal y Hidden XDG](https://specifications.freedesktop.org/autostart/latest/).
