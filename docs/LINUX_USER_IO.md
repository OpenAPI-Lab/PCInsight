# Disco por UID y cobertura de E/S de usuario (J04)

Revision101: la red ya se publica en el informe unico por UID del observador
`linux_user_network`; ver [LINUX_USER_NETWORK.md](LINUX_USER_NETWORK.md). No se
asocia a sesiones por nombre/UID aislado ni se duplica entre sesiones. El disco
de revision23 descrito aqui conserva su API/semantica. J04 se verifica como
disco por UID reportado + red independiente con dominio explicito, no como un
total global por sesion. La red legacy de UserSession permanece ausente con
una nota que dirige al nuevo informe; no es la API autoritativa de red.

Actualización F03 anterior: [LINUX_TCP.md](LINUX_TCP.md) implementa un sampler
TCP parcial por socket_uid, separado de este informe por UID real. No integra
esa métrica parcial como red total de proceso/usuario ni completa J04/H04.

La unidad J04 exige disco **y red** con identidad/cobertura explícitas. Esta
implementación original corrigio disco y publico cobertura; la parte de red
se completa desde revision101 con la proyeccion F03 citada arriba. No se
convierten ausencias en cero ni se usan sockets/contadores de interfaces como
si demostraran pertenencia exclusiva a una cuenta.

## API de lectura

`UserSession::linux_io_details` es un `Metric<LinuxUserIoDetails>` independiente
del alcance CPU/RAM por username de J01. Contiene:

- scope=`linux-visible-processes-by-reported-real-uid` y real_uid con soporte/nota.
- input_processes: filas de procesos recibidas por la agregación, no todos los
  candidatos /proc ni todos los procesos del host.
- processes_without_uid: filas de entrada sin UID real conocido. Son globales
  a esa entrada y no se atribuyen a ningún usuario por su nombre.
- matching_processes: cantidad de filas con UID real igual al UID reportado de
  la sesión. Ausente si falta un UID numérico de sesión; cero conocido si se
  pudo comparar y no hubo coincidencias. Ninguno significa disco medido en cero.
- disk_coverage: known/missing particionan las filas coincidentes;
  permission_denied e identity_unverified son subconjuntos de missing y pueden
  solaparse. overflow señala suma fuera de uint64, nunca saturación/wrap.
- disk_bytes_per_second: suma estimada de tasas conocidas y atribuidas a ese
  UID. Si hay lecturas ausentes conserva suma parcial y cobertura explícita,
  sin extrapolar. Ninguna conocida -> ausencia (permiso si se observó denegación).
- network_bytes_per_second: ausente; remite a linux_user_network, sin join de
  dominio de sesion no verificado. No se agregan
  valores de red sin un contrato de atribución del proveedor, incluso si un
  ProcessInfo construido por un llamador contiene un número en ese campo.

Los campos legacy de disco/red de UserSession reflejan los de este informe.
Ya no publican una suma de disco por username como si fuese propiedad numérica.
CPU/RAM conservan el alcance anterior descrito por resource_scope; no interpretar
ese campo como el alcance de linux_io_details. Los backends no Linux dejan el
nuevo informe ausente, no un informe Linux vacío.

## Identidad y tiempo del disco

El collector conserva los deltas y reloj propio de D02, sin cambiar su tasa por
proceso. Añade una lectura de status después de los contadores, además de la
anterior. Cada lectura de status y contadores comprueba PID/start_ticks según
el contrato existente. Solo un UID real conocido coincidente antes/después
califica esa observación para atribución de usuario. El tracker exige el mismo
UID en las dos observaciones del intervalo y que exista una tasa D02 válida.

`ProcessInfo::linux_details.disk_rate_real_uid` publica esa atribución estimada.
Primera muestra, cambio/ausencia/denegación de UID, identidad cambiada, contadores
reiniciados, reloj inválido o tasa inválida no producen un UID atribuible. Tras
una ausencia hacen falta dos observaciones consecutivas válidas. Cambiar UID no
invalida necesariamente la tasa por proceso, pero sí impide sumarla al nuevo
usuario durante el intervalo de transición. Reset/opt-out/errores de lote limpian
las referencias de atribución junto con las de D02.

La agregación exige UID real actual conocido, token start_ticks conocido y UID
de tasa coincidente. Nunca usa username para compensar esos campos ausentes.
UID0 es conocido y válido; no se confunde con nullopt. Nombres distintos con el
mismo UID comparten total; un mismo nombre con UIDs diferentes no mezcla E/S.
Una sesión utmp sin UID numérico queda sin E/S atribuida: no se inventa UID con
NSS, PID del registro ni nombre posiblemente truncado. login1 usa el UID reportado
por ListSessions, no prueba identidad actual/continua de una sesión viva.

Varias sesiones del mismo UID repiten el total del usuario, **no son sumandos**.
Deduplicar por el UID y scope dentro del mismo entorno antes de dibujar totales.
No combinar informes de muestras, hosts, boots ni namespaces distintos.

## Límites que siguen vigentes

Es atribución **muestreada** de procesos visibles, no contabilidad causal exacta.
UID antes/después no excluye un cambio A->B->A entre lecturas, ni identifica quién
causó E/S diferida/compartida o de hijos esperados. UID real no es necesariamente
euid/fsuid ni el propietario de sockets. Tokens de ticks no son globales ni
inmutables. No se certifica compatibilidad de espacios de UID entre un bus de
host externo y procfs de un contenedor: los números solo se interpretan en la
vista local declarada, sin garantía global ni traducción automática.

Las tasas por proceso pueden cubrir intervalos distintos; la suma no representa
un intervalo atómico sincronizado. Es bytes/s de read_bytes+write_bytes, no active
time ni throughput físico de un dispositivo. Permisos/hidepid, procesos omitidos,
terminados y vida corta pueden dejar recursos fuera aunque toda fila coincidente
sea conocida. input_processes y processes_without_uid no cuantifican lo oculto.
Ver [LINUX_PROCESS_IO.md](LINUX_PROCESS_IO.md).

Se mantienen límites de agregación10000sesiones,100000procesos y16MiB de
identidades. PID0/duplicado y sesión duplicada contradictoria se rechazan. El
collector puede omitir procesos desaparecidos/inaccesibles; no se declara total
de todo el equipo. include_processes=false sigue recopilando recursos cuando
include_user_sessions=true y oculta solo la lista de procesos entregada.

Para red: /proc/PID/net informa sobre el namespace de red, no
sobre tráfico exclusivo de ese PID. No repartir contadores del adaptador entre
usuarios, no estimar tráfico a partir de cantidad de sockets ni usar rchar/wchar
como bytes de red. La fuente nativa opcional F03 define unidades, identidad,
ventanas, cobertura y privilegios; su proyeccion por usuario tiene un informe
independiente, no se enlaza a esta fila de sesion sin comprobar el dominio.

API0.x amplía LinuxProcessDetails/ProcessInfo y UserSession/SystemSnapshot:
recompilar biblioteca y TODOS los consumidores. No cambia ABI virtual de acciones
ni almacenamiento de historial. No añade utilidades, paquetes ni acciones de
control sobre cuentas/sesiones/procesos ajenos.

## Verificación

Pruebas en tests/linux_user_io.cpp, tests/linux_process_io.cpp y tests/linux_users.cpp:
UID0/cero medido, aliases/repetición por UID, mismo username con UID distinto,
sin UID, token/UID de tasa ausente o incompatible, permisos, suma parcial,
overflow, UID cambiado entre lecturas/intervalos, recuperación y reset. Verifican
que red sin proveedor no se convierte en una suma positiva ni en cero.
La integración Linux escribe/fsync solo un archivo temporal propio, comprueba
atribución del proceso de prueba, agrega con etiquetas de sesión sintéticas y
verifica el recorrido público con procesos ocultos. No crea ni modifica sesiones.

2026-10-17: Release Linux92/92 y Windows109/109 aprobadas. Ambos consumidores
externos instalados recompilados/relinkados/ejecutados exit0, con lectura de los
nuevos campos y del informe público con procesos ocultos en Linux. Windows
comprueba que sus procesos/sesiones dejan los campos Linux ausentes.
Nativo WSL: proceso de prueba UID1000, tasa8779070B/s tras escribir/fsync131072B
en archivo propio; suma visible del UID8779070B/s con4lecturas conocidas/2ausentes.
No es benchmark físico ni tasa permanente. Etiquetas de sesión sintéticas para
esa agregación; recorrido público real1informe de sesión con procesos ocultos.
No se cambiaron usuarios, credenciales, sesiones ni servicios del host.

Logs: work/user-io-build-{linux,windows}.log,
work/user-io-verified-build-{linux,windows}.log,
work/user-io-focused-{linux,windows}.log, work/user-io-suite-{linux,windows}.log y
work/user-io-native-linux.log. Builds iniciales registran clock skew transitorio
MSFS en Linux y aviso NOMINMAX ya existente en test Windows; verificación
incremental posterior sin avisos, sin alterar reloj ni ocultar logs iniciales.
Riesgos históricos N03 no se consideran corregidos por estas ejecuciones.
J04 P->E: disco/identidad/cobertura implementados, red pendiente F03. Sin cierre,
IDs nuevos ni fracción de crédito:46/75=61,3% se conserva.

Referencias primarias: [procfs: Uid y contabilidad I/O](https://www.kernel.org/doc/html/latest/filesystems/proc.html),
[proc_pid_net: ámbito de namespace](https://kernel.googlesource.com/pub/scm/docs/man-pages/man-pages/+/refs/tags/man-pages-6.11/man/man5/proc_pid_net.5).
