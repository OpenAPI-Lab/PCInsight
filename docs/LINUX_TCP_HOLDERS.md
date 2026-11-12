# Referencia TCP seleccionada por proceso/descriptor (F03 parcial)

La API adicional de `pcinsight/linux_tcp.hpp` consulta un descriptor explícito
de un proceso, sin inferir propiedad exclusiva desde inode/UID/username. No
enumera todos los FDs ni atribuye causalmente tráfico a una aplicación.

```cpp
pcinsight::LinuxTcpSocketHolder target{pid, process_start_ticks, descriptor};
auto observation = pcinsight::read_linux_tcp_socket_holder(target);
pcinsight::LinuxTcpSocketHolderSampler sampler;
auto baseline = sampler.sample(target);
// En el siguiente tick:
auto next = sampler.sample(target);
```

process_start_ticks procede de ProcessInfo::linux_start_ticks del mismo
host/boot/vista PID. Se exige token positivo, PID dentro de pid_t positivo y
descriptor>=0. El descriptor es un número observado, no un comando/path ni un
token persistente de socket. El primer resultado descubre la cookie actual.
Cambiar el objeto de ese FD entre ticks reinicia la tasa, no reutiliza baseline.

## Fijación y comprobaciones

Para el propio proceso se usa F_DUPFD_CLOEXEC. Para otro proceso se abre pidfd y
se duplica su descriptor con pidfd_getfd, **sin** fallback a /proc/PID/fd,
readlink/inode matching, ptrace attach, sudo ni helper. Ambas duplicaciones
pertenecen a la librería y se cierran al terminar la consulta.

Se comprueba procfs nativo y que /proc/self corresponde al PID del llamador.
Para un target externo se exige además Pid de /proc/self/fdinfo/<pidfd>
coincidente con el PID solicitado: ese campo proyecta el proceso fijado en el
namespace de ese montaje procfs. Si no existe, es0/-1, está malformado/duplicado
o difiere, se rechaza antes de consultar el descriptor. No basta una coincidencia
casual de números self para inferir equivalencia de namespaces para otros PIDs.
La lectura fdinfo se fija, verifica procfs y limita a64KiB/10000líneas/4EINTR.

PID/start_ticks se verifican antes y después. En el target externo el pidfd debe
seguir sin salida antes y después: no se sustituye el proceso al reutilizar PID.
El FD fijado debe ser socket TCP stream IPv4/IPv6 en ESTABLISHED o CLOSE_WAIT.
Se leen SO_COOKIE y TCP_INFO en esa referencia. Después se duplica **otra vez**
el número FD del target y se exige la misma cookie/dev/inode, para no publicar
una referencia si el slot cambió de socket durante la observación.

Esto prueba asociación observada en los extremos, no posesión continua ni
causalidad: un FD puede cambiar A->B->A entre comprobaciones, compartirse,
heredarse o transferirse. start_ticks tiene resolución de ticks y no identifica
exec/boot globalmente. Una lectura no es transacción ni token para otras acciones.
No se altera el estado, flags, opciones, offset ni datos del socket; solo se
consulta. Duplicar una referencia **puede prolongar brevemente la vida del
socket**, incluso si el target cierra su copia. El sampler no conserva FDs entre
ticks: solo metadatos/contadores. No se promete impacto absolutamente nulo sobre
la temporización del cierre de una conexión por ese breve pin.

## Resultado y tasas

LinuxTcpHeldSocketSample conserva target, cookie, inode completo, ip_version,
estado TCP, captured_at monotónico y contadores nativos ACKed/received. No informa
username/UID propietario ni inventa un PID causal. SO_NETNS_COOKIE opcional
identifica el namespace **del socket**, que puede diferir del hilo actual; no es
el mismo tipo de ID que el inode de namespace del sampler diagnóstico.
Su ausencia queda motivada. SO_COOKIE es obligatorio para esta consulta.

El sampler exige target (PID/token/FD), cookie, inode, familia y metadatos de
namespace compatibles, reloj creciente y contadores no decrecientes. La tasa
por dirección usa su delta/segundos y Support::estimated; no es tráfico total
de proceso ni throughput físico. Primera muestra, error/denegación, reset,
cambio de FD/proceso/socket o namespace observado invalida baseline. Un campo
ausente no borra la otra dirección válida; recuperarlo exige dos observaciones.
Si disminuye cualquier contador conocido se descarta el intervalo de ambos.
Cero medido es conocido; overflow/ausencia no se convierte en cero.

Dos holders con la misma cookie observan **el mismo socket**. No sumar sus tasas
como si fueran tráfico independiente por PID. El mismo proceso también puede
tener varios FDs duplicados de una cookie. Esta API expone las identidades para
que un GUI muestre la compartición, pero no ofrece un agregado por proceso que
arbitrariamente adjudique el tráfico compartido. Incluso una cookie observada
en un único target no demuestra que no existan otros holders ocultos.

## Permisos y portabilidad

pidfd_getfd está sujeto a la comprobación ptrace PTRACE_MODE_ATTACH_REALCREDS,
LSM/Yama/seccomp y derechos vigentes del llamador. EPERM/EACCES permanece
permission_required. No se cambian dumpability, políticas ni privilegios para
evitar la denegación. Kernel/header sin pidfd o SO_COOKIE requerido -> ausencia
motivada; Windows conserva la API pero no publica observaciones Linux.
Opciones o identidades denegadas se preservan como denegadas, no se convierten
en un resultado satisfactorio vacío. Un token no concede autorización.

Consulta una sola referencia por llamada: sin escaneos/procesos de fondo. No hay
deadline global para syscalls del kernel. El host debe serializar sample/reset
y no manipular concurrentemente FDs internos que no le pertenecen, namespaces,
credenciales o montaje procfs. Las excepciones del sampler limpian su baseline.

## Verificación y alcance pendiente

Fixtures portables de identidad/cookie/FD/inode/namespaces/reloj/counter reset,
ausencia con valor residual/permisos, recuperación y cero. Parser de fdinfo
rechaza Pid ausente/duplicado/-1/0/fuera de rango/oversize/junk.
La prueba nativa crea dos sockets TCP privados loopback y un hijo privado con
salida acotada. Verifica referencias duplicadas/compartidas, reemplazo de FD,
token obsoleto, FD inválido, UDP/listener no soportados y correlación de cookie
con el diagnóstico netlink si está disponible. Comprueba que las consultas no
consumen datos pendientes ni cambian F_GETFL. La consulta externa solo usa ese
hijo; jamás se abren descriptores de procesos ajenos para probarlo. Si el kernel
deniega la duplicación se registra como limitación, no positivo de ese proveedor.

F03 permanece en curso: esta asociación de holder no es contabilidad exclusiva
por app. Siguen pendientes UDP/otras coberturas, sockets compartidos/vida corta y
su atribución/integración. No se rellenan ProcessInfo/UserSession.network como
si estos números TCP fueran red total. J04/H04 y N02/CI no se cierran aquí.
API nueva por símbolos/tipos, sin cambiar layouts existentes, ABI virtual ni
formato de historial. Recompilar/relinkar para utilizarla.

Verificación2026-10-17: Linux96/96 y Windows111/111 Release aprobadas, sin skip
de la prueba nativa holder. Paquetes reinstalados y consumidores externos de
ambos sistemas recompilados/relinkados/ejecutados exit0 contra la biblioteca
final. WSL: self PID1799/start_ticks5284/FD4/cookie12289, delta ACKed8192B y
recibidos8192B; tasa muestreada86347011B/s, no benchmark físico. Consulta del
hijo privado por pidfd_getfd **positiva**, misma cookie12289 que el padre;
fdinfo/proyección PID verificados. Alias FD mismo socket, reemplazo FD reinicia,
token inválido/UDP/listener rechazados; datos pendientes y F_GETFL preservados.
No se afirma validación de todos los procesos/derechos/distros/arquitecturas.

Logs work/tcp-holder-{build,verified-build,final-build,clean-build}-{linux,windows}.log,
work/tcp-holder-{suite,final-suite,clean-suite}-{linux,windows}.log y
work/tcp-holder-clean-native-linux.log. Los builds iniciales registraron avisos
de devolución de optional/maybe-uninitialized en GCC y formato misleading-
indentation; se reestructuró la devolución de ausencia para construir un Metric
vacío explícito y se separaron las sentencias del parser. Sin desactivar avisos
ni relajar aserciones; builds finales sin avisos. Riesgos históricos N03 CPU/
historial no se consideran resueltos por estas ejecuciones. F03 E->E,
46/75=61,3% sin crédito fraccionario ni IDs nuevos.

Fuentes primarias: [pidfd_getfd](https://kernel.googlesource.com/pub/scm/docs/man-pages/man-pages/+/master/man/man2/pidfd_getfd.2),
[proyección Pid de pidfd fdinfo](https://github.com/torvalds/linux/blob/master/fs/pidfs.c),
[prueba kernel SO_NETNS_COOKIE](https://kernel.googlesource.com/pub/scm/linux/kernel/git/stable/linux-stable-rc.git/+/refs/tags/v5.16.14/tools/testing/selftests/net/so_netns_cookie.c).
