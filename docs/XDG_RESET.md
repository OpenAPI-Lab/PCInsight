# Restablecer startup del sistema

Unidad terminada 2026-10-14: `reset_startup_for_current_user(entry.id)` retira
exclusivamente una copia personal seleccionada y conserva un respaldo recuperable.
Es una acción distinta de enable: **retira las personalizaciones Name/Exec/flags
de futuros logins**. No ejecuta aplicaciones ni detiene procesos.

```cpp
auto result = actions->reset_startup_for_current_user(personal_entry.id);
// Recargar inventario después de éxito o de un error que diga reset aplicado.
// Guardar result.startup_backup_path para ofrecer recuperación manual al usuario.
```

Solo acepta el path personal absoluto `.desktop` del contexto XDG del caller,
no un ID system ni un path externo. Rechaza contexto personal/system compartido.
Exige contraparte system: busca el primer archivo existente por prioridad de
XDG_CONFIG_DIRS, sin saltar archivos malformados/inaccesibles hacia otra copia.
Archivo system regular, sin symlink final, lectura máxima 1 MiB; validación de
Desktop Entry y flags no ambiguos, identidad/dev/inode/size/mtime/ctime revalidados.
Sin contraparte devuelve not_found: **no elimina una app exclusivamente personal**.
El estado resultante es el configurado por system; no fuerza enabled ni elimina
restricciones de escritorio/TryExec. Recargar inventario para conocerlo.

Directorio personal abierto O_NOFOLLOW, UID efectivo y sin escritura group/world;
flock no bloqueante compartido con los otros editores. Archivo personal abierto
sin seguir symlink final, regular, propio y nlink=1; datos fsync y revalidación
de descriptor/ruta antes de mover. No exige que el contenido personal sea válido:
puede respaldarse una personalización rota para volver a una contraparte válida.

`renameat2(RENAME_NOREPLACE)` mueve atómicamente el archivo al mismo directorio:
`.pcinsight-reset-<PID>-<contador>.backup`. No termina en .desktop y el inventario
no lo trata como autostart. Hasta 128 colisiones; ningún respaldo existente se
sobrescribe, sigue ni purga. Conserva el mismo inode, contenido, ownership, modo
y xattrs; ctime puede cambiar por rename. No crea directorios ni modifica system.
Sin soporte kernel/filesystem devuelve not_supported, no hace link/unlink como
fallback. No se utiliza CLI, servicio externo ni elevación.

Tras mover, fsync del directorio. Si falla, devuelve system_error con mensaje
**reset aplicado** y startup_backup_path poblado; no promete rollback ni ausencia
de efectos. Recuperación explícita por el consumidor: inspeccionar el respaldo
y moverlo a su nombre original solo si ese destino sigue libre. Esta unidad no
incluye API automática de restore ni purge, y no toca archivos reales en los tests.

Se confía en entorno, ancestros y coordinación con escritores externos: flock
es advisory. Chequeo y rename no constituyen CAS contra actores con igual acceso
que ignoren lock; tampoco hay snapshot global de todos los directorios system.
El consumidor debe coordinar cambios de entorno/configuración y recargar ante
cambios. No usar como barrera de seguridad de un helper privilegiado. La lectura
system puede cambiar atime. Respaldo retiene permisos originales, no se promete
cifrado ni ocultación al administrador. Otros backends devuelven not_supported.

AuditedActions registra startup_reset_current_user, target y resultado sin Exec
ni contenido del respaldo; conserva startup_backup_path en ActionResult. Enum
append-only; API 0.x/estructura resultado ampliadas: reconstruir biblioteca,
wrappers y consumidores juntos.

Verificación Release: Linux 23/23, Windows 34/34 y consumidores externos instalados
en ambos. Fixture Linux /tmp con XDG_CONFIG_HOME/DIRS privados, sin modificar HOME:
reset nativo, contraparte readonly intacta, inode/modo/owner del respaldo intactos,
decoy backup conservado, restauración manual comprobada, precedencia observada por
Monitor, auditoría; rechazos de path system/relativo/orphan, symlink/hardlink,
directorio inseguro, lock, contexto compartido y contraparte malformada/symlink
sin fallthrough. Contraparte lower-priority disabled permanece disabled, incluso
al respaldar una copia personal rota. Windows comprueba unsupported y auditoría,
no un proveedor XDG.
