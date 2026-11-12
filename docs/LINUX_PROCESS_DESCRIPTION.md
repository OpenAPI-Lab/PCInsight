# Descripcion y disponibilidad de detalles Linux

`ProcessInfo.linux_details` expone `Metric` por nombre, estado, username,
UID real, ejecutable, arquitectura, descripcion y fuente de descripcion. Cada
campo conserva valor opcional, soporte y nota. Los seis strings legacy reflejan
su metrica, o vacio si falta: nunca son sustitutos de consultar disponibilidad.
En Windows el informe Linux permanece unsupported, sin reinterpretar sus datos.
No cambia la semantica de las metricas CPU/RSS/E/S ni cierra A01 para todo el API.

## Descripcion con procedencia

La fuente implementada es `Comment` no localizado de un archivo `.desktop`,
normalizado como UTF-8 de escalares validos y texto corto sin controles. No se
usa Name, GenericName, nombre kernel, ELF ni basename como descripcion inventada.
No es metadata firmada ni prueba de identidad comercial o publisher.

Se enumera XDG_DATA_HOME/applications (alternativa HOME/.local/share/applications)
y XDG_DATA_DIRS/applications (predeterminado /usr/local/share:/usr/share).
IDs relativos con separadores convertidos a guiones, precedencia del primer
directorio y colisiones ambiguas tratadas como mascaras. Una entrada prioritaria
oculta, invalida o ilegible no reactiva silenciosamente una copia inferior.
No se recorren directorios symlink. No se ejecutan programas ni se instalan datos.

Se asocia el programa `Exec` con el exe fijado por D03 usando dev/inode/size/
mode/mtime/ctime, no semejanza de nombres. Programas absolutos y nombres simples
resueltos en PATH del caller; componentes relativos/vacios anteriores a la
resolucion fallan cerrados. PATH del caller puede no ser el de la sesion original:
la asociacion sigue siendo estimated, NO una reconstruccion de como se lanzo.
No se abre el exe por el nombre muestreado para obtener su arquitectura.

Subconjunto conservador de Exec: programa simple o entre comillas dobles sin
escapes, seguido de cero o un codigo standalone %f/%F/%u/%U. Wrappers, argumentos
literales/opciones, escapes complejos, activacion D-Bus, otros codigos y formatos
no implementados no producen asociaciones. No se intenta adivinar identidad de
scripts, terminales o launchers por sus argumentos. No es un parser de lanzamiento.

Una sola asociacion admitida puede producir descripcion estimated. Dos entradas
distintas del catalogo admitido para el mismo archivo son ambiguas, aunque sus
textos coincidan. Fuente modificada/desaparecida despues del indice => ausencia.
No hay asociacion admitida, Comment ausente o solo localizado => desconocido con
nota, no cadena de relleno ni descripcion tomada de otro ejecutable.
Se descartan archivos individuales ilegibles/no admitidos; el resultado describe
solo ese subconjunto accesible, NO certifica un catalogo completo de todas las apps.

## Coste y limites

`SampleOptions.include_linux_process_descriptions` es true por defecto. False
evita todo el escaneo y devuelve descripcion/source unsupported con nota disabled,
sin borrar los otros detalles. Se construye un indice por muestra cuando se
necesitan procesos (tambien para usuarios/historial); no un escaneo por proceso.
No cache permanente, descargas, comandos CLI ni servicios auxiliares obligatorios.

Max64raices,16384entradas examinadas, profundidad8,4096IDs,64KiB por archivo,
16MiB de contenido total,4096lineas/8192B por linea y1024B de comentario. Variables
de entorno limitadas a32768B y PATH a128componentes para resolver cada programa.
Exceder limite de catalogo o error de enumeracion invalida el indice completo;
un archivo individual no admitido se omite conservando su precedencia.
Archivos de metadata fijados O_PATH, regulares antes de lectura; se leen por fd
propio con verificacion de metadata antes/despues. EINTR max3reintentos.
Lector UTF-8 reutiliza la validacion de cpu_identity_text; el lector autostart no
se reutiliza para inferir comentarios/IDs de applications con otras reglas.

Las verificaciones son secuenciales, no atomicidad de catalogo, proceso o exe.
Cambios A->B->A y cambios no observables en metadata no estan excluidos. Un actor
puede escribir comentarios falsos en sus archivos: el GUI debe conservar soporte
estimated y mostrar la fuente. No usar estas asociaciones para autorizacion.
Limites de bytes no garantizan deadline de filesystem/NSS. El caller no debe
mutar concurrentemente el entorno mientras la biblioteca lo lee.

## Verificacion

Test linux-process-metadata: parser de subset, duplicados, flags, wrappers,
UTF-8/escapes y limites; informe publico conserva permiso/ausencia/UID0, borra
valores legacy viejos y no hereda defaults. Fixtures Linux privados: Comment
positivo para el archivo del propio ejecutable, precedencia Hidden, colisiones
de ID, asociaciones multiples, cambio de fuente, PATH y opt-out de Monitor.
Nativo WSL PID2083; no se modificaron aplicaciones instaladas ni cuentas.
Los metadatos positivos se crean en un catalogo privado, no son certificacion
de inventario comercial real de todas las distribuciones.

Cambio de layout publico en API0.x: reconstruir biblioteca y TODOS los
consumidores con cabeceras nuevas, mismo compilador/runtime. No mezclar objetos
antiguos ni suponer ABI estable. Ver PACKAGING.md y LINUX_PROCESS_DETAILS.md.

Referencias primarias del formato (el subset es una politica de este proveedor):
[Exec](https://specifications.freedesktop.org/desktop-entry/latest/exec-variables.html),
[Comment y Hidden](https://specifications.freedesktop.org/desktop-entry/latest/recognized-keys.html),
[IDs y precedencia](https://specifications.freedesktop.org/desktop-entry/latest/file-naming.html).
