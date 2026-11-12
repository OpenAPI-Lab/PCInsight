# Condiciones de startup por existencia de archivo

Unidad verificada 2026-10-14: StartupEntry::autostart_condition_satisfied evalúa
AutostartCondition del grupo principal del `.desktop` efectivo. Soporta if-exists
y unless-exists sin programas externos ni dependencias nuevas; resultado separado
de status y xdg_standard_eligible. No key -> true (sin este requisito), no garantía
de lanzamiento. GConf no implementado -> unknown. GSettings y GNOME3-session
tienen proveedores opcionales posteriores, documentados en XDG_GSETTINGS.md y
XDG_SESSION_CONDITION.md.

Rutas absolutas se consultan directamente, relativas bajo config directory del
caller (resolver XDG existente), no autostart ni cwd; semántica contrastada con
[GNOME](https://mail.gnome.org/archives/commits-list/2020-June/msg03823.html).
Evaluar un predicado no certifica que el escritorio lo reconozca, que todas las
versiones GNOME lo evalúen igual o que el caller sea la sesión gráfica relevante.

Parser: tipos completos, comparación ASCII sin distinguir mayúsculas; no prefijos
abreviados. Clave duplicada, entrada malformada/ilegible, escapes inválidos,
operando vacío/control inválido/excesivo -> unknown. Solo grupo principal, no
Desktop Action. Decodifica escapes string; path literal, sin shell/expansión de
variables/tilde/globs/comillas. Espacios forman parte del nombre; `..` se resuelve
por el filesystem, no se normaliza alterando semántica de symlinks.

Linux usa stat, nunca open del candidato: directorios/FIFO también existen,
symlinks se siguen y dangling -> false. ENOENT/ENOTDIR -> false; EACCES/EPERM ->
unknown/permission_required; ELOOP y otros fallos -> unknown. unless-exists invierte
solo valores conocidos, nunca errores. Sin config directory para relativo o sin
proveedor -> unknown. No se modifica ni ejecuta ningún archivo.

Cada Monitor.sample vuelve a consultar: enabled puede coexistir con condición
false/unknown, incluso si xdg_standard_eligible es true. No instala watchers ni
inicia/detiene apps cuando cambia la condición. No cubre otras políticas vendor.
Stat puede esperar un filesystem remoto/automount, sin timeout universal ni
snapshot atómico. Límites: entrada 1 MiB, string/path resuelto 32 KiB; metadata
del inventario 64 MiB cuenta la nota nueva. Otros backends dejan unsupported.

Verificación Release: Linux24/24, Windows35/35, paquetes instalados y consumidores
externos reconstruidos/ejecutados. Windows valida parser con proveedor inyectado,
no backend XDG. Linux UID1000 usa /tmp y contexto XDG privado sin cambiar HOME:
Monitor observa aparición/desaparición, espacios, directorios, symlinks válidos/
rotos, FIFO sin bloqueo, ELOOP y permiso denegado (no confundido con inexistencia).
Parser prueba grupos/duplicados/tipos desconocidos/límites/escapes/paths literales
y contexto/proveedor ausente. No se ejecutó ninguna app ni tocó startup real.

Se regeneró un objeto Linux de formato inválido mediante clean-first. Los tests
Windows SCM/PDH fallaron por acceso dentro del sandbox restringido y pasaron en
la ejecución aprobada fuera del sandbox; no se ocultaron errores ni se mutaron
servicios. Campo público 0.x nuevo: reconstruir biblioteca/consumidores juntos.
