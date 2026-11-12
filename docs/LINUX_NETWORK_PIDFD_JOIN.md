# Asociacion privada pidfd / procfs

Revision70: `read_private_pidfd_network_join(authorized,pidfd,pid,start_ticks)`
asocia el token kernel de un proceso seleccionado con su identidad local en
procfs. Es un componente **interno**, no instalado ni activado en Monitor.
No inicia captura de trafico general, no enumera tareas y no cierra F03/H04/J04.

## Transaccion

Sin autorizacion o con argumentos invalidos no consulta BPF. Conserva un
duplicado CLOEXEC del pidfd durante toda la operacion y comprueba poll antes y
despues. No sustituye el handle por pidfd_open de un numero posiblemente nuevo.
El caller debe serializar close/reutilizacion del descriptor de entrada durante
su duplicacion; no soporta manipulacion reentrante de los descriptores internos.

Secuencia: contexto procfs/boot antes, dos descubrimientos kernel desde el
pidfd, contexto procfs/boot despues, poll final y comparacion estricta.
Cada contexto contiene UUID boot, identidad del observador y del objetivo.
El observador conserva PID/UID/start_ticks, procfs device/inode/process inode,
namespaces PID/user device/inode con las comprobaciones SELF ya existentes.

El lector seleccionado fija `/proc`, verifica procfs y su enlace self, abre el
directorio numerico del observador, su `task/<gettid>/fdinfo` y el directorio
numerico objetivo con NOFOLLOW/CLOEXEC. Consulta `Pid:` del fdinfo **en el mismo
procfs** antes y despues: debe coincidir con el PID esperado. No interpreta
NSpid como TGID inicial. Es importante usar la tabla de FD del hilo llamante:
un hilo puede tener una tabla distinta de la del lider. Los archivos ordinarios
se leen con limite64KiB, max4EINTR y comprobacion procfs; los enlaces de namespace
se siguen intencionadamente y su descriptor debe pertenecer a nsfs.

Comprueba stat PID/start_ticks esperado antes/despues, dos status PID/Tgid/UID
real iguales y namespaces objetivo estables. Los dos contextos completos deben
coincidir y compartir procfs device/inode. Los dos tokens kernel deben coincidir
en TGID inicial, UID inicial y leader_start_boottime_ns; UID0 es valido.
Fallo, permiso denegado, salida, cambio o ambiguedad devuelve ausencia motivada,
no un cero ni una asociacion aproximada. FDs y sondas temporales se retiran.

## Dominios y limites

No exige igualdad de PID/UID entre procfs y kernel ni convierte ticks a ns.
El UID de status se expresa en el user namespace del **observador**, no en el
del proceso objetivo. El token conserva ambos namespaces por separado; los
valores overflow/munged del kernel no se convierten en usernames o IDs iniciales.
La proyeccion pidfd es local al namespace del montaje procfs; no un mapa universal.
Fuentes Linux6.6 revisadas:
[pidfd fdinfo/poll](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/fork.c),
[UID en proc status](https://github.com/torvalds/linux/blob/v6.6/fs/proc/array.c),
[tabla fdinfo por tarea](https://raw.githubusercontent.com/torvalds/linux/v6.6/fs/proc/fd.c).

Es una observacion por extremos, no una transaccion atomica contra cambios y
retornos ABA de credenciales/namespaces/montajes. Los inodes no son certificados
globales y no quedan retenidos al retornar. No garantiza vida futura, conserva
handles de namespaces, detecta cada cambio intermedio ni impide mutaciones del
objetivo. Solo enlaza el proceso seleccionado observado; no identifica cada
thread, aplicacion comercial, historial o trafico total. Revision71 integra
revalidacion en [ventanas seleccionadas internas](LINUX_NETWORK_SELECTED_WINDOW.md).
No descubre cookies de sockets externos ni multiactor.

Hereda requisitos/limites del [descubrimiento pidfd](LINUX_NETWORK_PIDFD.md):
BTF/ABI de kernel compatible, BPF/FEXIT y permisos ptrace/LSM. La mascara de
senales del caller se bloquea temporalmente en cada descubrimiento y se restaura;
puede retrasar handlers, no tiene plazo duro ni cancelacion asincrona soportada.
No hay ptrace attach, senal o parada del objetivo, instalacion ni configuracion
permanente. La autorizacion de pruebas sigue limitada a hijos privados, no al
sistema entero. Linux es la prioridad; sin implementacion Windows nueva.

## Pruebas

Fixture puro: dominios local/kernel deliberadamente distintos, namespace del
objetivo diferente al observador, UID0, 17cambios de contexto, tres cambios kernel,
nueve casos de contexto/token invalido y argumentos/opt-in rechazados sin captura.
No prueba que el kernel real cambie credenciales/namespaces; simula esos extremos.

El modo manual autorizado `--authorized-private-pidfd-session` crea un hijo
privado que no comunica identidad kernel. El padre lee start_ticks en procfs,
resuelve/repite el join y comprueba contra el oraculo local del harness. Rechaza
start obsoleto, PID del padre, pipe y pidfd salido, sin fugas. Conserva prueba de
captura TCP4096/4096B y UDP256/256B,8peeks, padre excluido, detach congelado e
identidad incorrecta sticky. No usa estos bytes para afirmar cobertura total.

CTest nativo no privilegiado `linux-network-selected-proc-native`: un worker
hace unshare(CLONE_FILES) y reemplaza un FD pipe por pidfd conservando el mismo
numero que sigue siendo pipe en el lider. La lectura debe resolver la tabla del
worker; start obsoleto/pipe se rechazan y no hay fugas. Si el entorno deniega
unshare/pidfd_open retorna77, no inventa cobertura. No cambia namespaces ni la
tabla del lider; la tabla privada desaparece al terminar el hilo.

Evidencia final y conteos de suites: control oficial SEGUIMIENTO.txt revision70,
logs `work/network-join-*`. Positivos reales limitados a este WSL x86_64;
sin verificacion de hardware/distribuciones o CI remota universal.
