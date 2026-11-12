# D03: estado e identidad de procesos Linux

D03 cerrada2026-10-16 con status/UID/NSS/nombre/ELF, disponibilidad publica por
campo y descripcion estimated de Comment XDG con procedencia. Fuentes/cobertura
de descripcion en LINUX_PROCESS_DESCRIPTION.md; no toda app dispone de metadata.

## Implementado

- stat inicial max64KiB, no lectura ilimitada seguida de parser acotado.
- Directorio de proceso fijado con O_PATH. stat/status/stat mediante openat,
  O_NOFOLLOW/CLOEXEC/NONBLOCK; lecturas por bloques8KiB, status max1MiB,
  max10000lineas/64KiB por linea, EINTR hasta3 reintentos.
- PID positivo uint32 coincidente con la fila esperada. Mismo PID/start_ticks
  antes/despues; identidad distinta o ausente no recibe esos detalles.
- Uid exige exactamente cuatro uint32 completos: real/effective/saved/fs.
  Se usa el REAL como antes, no se atribuye automaticamente al efectivo.
  UID0 valido no se confunde con falta de UID; no se inventa root.
- Campos seleccionados duplicados, signos, overflow, texto sobrante, NUL y
  limites invalidos se rechazan. Campos faltantes individuales permanecen sin
  valor, no invalidan un PID correcto ni inventan datos.
- Name y State son datos observados del kernel, no etiqueta comercial ni prueba
  de que el proceso seguira running despues. Se conservan estados futuros con
  codigo alfabetico; no se convierte un estado desconocido a running.
- Bytes de control/noASCII se representan como escapes ASCII para visualizacion.
  El parser elimina whitespace periferico del valor: no garantiza nombre byte
  exacto ni identidad univoca. Las representaciones no sustituyen UID/PID/tokens.
- getpwuid_r sustituye getpwuid compartido: buffer1024B creciente hasta1MiB,
  ausencias y errores sin fallback root. NSS puede bloquear e incluso consultar
  servicios de identidad; no hay deadline global. No se ejecutan utilidades.
- Quitado description=name: ahora solo refleja Comment XDG asociado y estimated,
  o queda vacia/desconocida. Ver la tercera unidad para el nuevo layout publico;
  las dos primeras unidades no lo cambiaron. Backend Windows sin cambios de fuente.

## Evidencia inicial (primera unidad, antes del informe publico)

Nuevo test linux-process-details, asserts activos en Release: UID0/faltante,
exactamente4UID/rango/signos, PID incorrecto, duplicados, campos parciales,
caracteres escapados, limites de bytes/lineas, token ausente/cambiante y tick0.
Permisos probados mediante propagacion sintetica; no se alteran permisos reales.
Archivos privados Linux: fuentes faltantes/oversized/symlink final rechazadas.
Propio proceso WSL PID1399,UID1000,NSS wiguelito,State R(running),Name
pcinsight-linux; Monitor coincide con cuenta y deja descripcion sin inventar.
No se fuerzan reutilizacion PID, exec races ni cambios de cuentas/procesos ajenos.
Regresion Release: Linux75/75 y Windows95/95. Fixtures portables en Windows,
lecturas nativas Linux solo verificadas en WSL. Instalacion/consumidores no
repetidos: no hay cambios publicos de layout.

## Cierre de descripcion y disponibilidad (tercera unidad D03)

ProcessInfo.linux_details ahora expone Metric por campo y UID real; los strings
legacy reflejan valor o vacio. Descripcion con fuente XDG Comment estimated,
sin suponer que comm/binario es descripcion certificada; ausencia/ambiguedad
explicitas. Esto resuelve disponibilidad de D03, no A01 para otros modelos
legacy como AppHistoryEntry. Nuevo layout publico0.x: recompilar libreria y
todos los consumidores. Regresion final Linux76/76,Windows96/96 y consumidores
instalados recompilados/ejecutados exit0. La fila completa no es snapshot atomico.
PID/start_ticks tiene alcance boot/namespace y resolucion de ticks; no detecta
exec dentro del mismo proceso ni cambios de credenciales posteriores al status.
Escapes de nombres pueden impedir asociacion con utmp/NSS sin normalizacion
equivalente; no dan identidad persistente para agregacion por usuario.

## Ejecutable y arquitectura (segunda unidad D03, 2026-10-16)

El colector ya no combina read_symlink(exe) con una apertura posterior por esa
ruta. Ahora fija el directorio proc y verifica stat antes/despues. Abre exe con
O_PATH/CLOEXEC (sigue el magic link deliberadamente), exige archivo regular por
fstat antes de cualquier lectura y conserva ese descriptor. Abre solo su propio
/proc/self/fd/N para leer hasta64B mediante pread; no ejecuta el archivo.
Si no puede reabrir el descriptor no cae a la ruta de nombre muestreada.

Comprueba dev/inode/size/mode/mtime/ctime, incluyendo nanosegundos, antes y
despues de leer. Repite el enlace exe y abre nuevamente su destino con O_PATH
para detectar cambios observables de archivo. Cambios de link, metadata o
PID/start descartan la observacion; no se mezclan dos ejecutables detectados.
La cadena del enlace tiene limite64KiB y no se elimina el sufijo (deleted):
no es una ruta reutilizable ni debe reabrirse asumiendo que identifica el archivo.

La arquitectura proviene de cabecera ELF32/ELF64 completa de52/64B: magic,
class/data, versiones, ET_EXEC/ET_DYN, e_ehsize y machine compatibles. Se prueban
little/big-endian y varias arquitecturas; no se hereda la arquitectura del host.
ELF truncado/no admitido deja arquitectura desconocida aunque pueda conservarse
la ruta verificada. No es validacion integral del ELF, sus segmentos o firmas.
Un proceso interpretado identifica el ejecutable del interprete, no la
arquitectura o descripcion del script ni del codigo JIT.

Las lecturas son secuenciales: no certifican generacion exec, cambios A->B->A
entre observaciones ni modificaciones de contenido invisibles en metadata.
No hay bloqueo del proceso, ptrace, hashes de todo el binario ni snapshot
atomico con UID/State/RSS. El limite es de bytes; syscalls/filesystems pueden
bloquear, sin deadline universal. Permisos de proc y del archivo son distintos;
una ruta disponible no garantiza arquitectura accesible. Metricas internas
conservan soporte/nota; los strings legacy vacios siguen significando desconocido
El informe publico agregado posteriormente conserva ahora esas notas por campo;
no confundirlo con estabilizacion de toda la API A01.

Pruebas extendidas linux-process-details: todas las longitudes truncadas de
ELF64, campos invalidos, ELF32, endian/clase/maquina, continuidad de stamps/path/
PID/start y fixture de nombre con sufijo literal(deleted). Este ultimo NO es
prueba de un ejecutable real borrado en ejecucion. Fixtures privados Linux
rechazan symlinks hacia directorio/FIFO/dev-null sin abrirlos para I/O; un
archivo regular noELF conserva ruta y no inventa arquitectura. Nativo WSL
PID1354 devolvio x64 y la ruta del propio binario; Monitor coincidio. Ningun
exec/reuso PID real forzado ni archivo de programa ajeno modificado.
Suites finales Release Linux75/75 y Windows95/95, mismo target ampliado, sin
nuevas subtareas ni cambio de layout publico. No se repitieron consumidores.
Una primera regresion Linux fallo en linux-integration por ausencia de CPU
global en la segunda muestra. No se guardaron los contadores de esa muestra:
causa no determinada. La integracion paso3repeticiones consecutivas y luego
la suite completa75/75, sin cambiar el assert ni codigo de CPU. Riesgo registrado
para N03; no se declara un problema CPU resuelto por el cambio de ELF.

Fuentes primarias de la semantica usada:
[proc_pid_exe](https://man7.org/linux/man-pages/man5/proc_pid_exe.5.html),
[O_PATH y reapertura por fd](https://man7.org/linux/man-pages/man2/open.2.html),
[readlinkat y truncamiento](https://man7.org/linux/man-pages/man2/readlinkat.2.html).
