# Sesion privada Linux de contadores SELF

Revision68 agrega [actor seleccionado](LINUX_NETWORK_SELECTED.md) como metodo
interno separado, con token declarado y tipo explicito; no convierte SELF en
descubrimiento/atestacion de procesos externos. Lo siguiente conserva SELF.

Revision64 extrae la captura clasificada/guardada del ejecutable de prueba a
`src/linux_network_session.{hpp,cpp}`. Es un componente **interno**, no una API
instalada ni un proveedor activado desde Monitor. F03 sigue en curso.

Revision65 incorpora un [modo acotado al cookie del socket](LINUX_NETWORK_NAMESPACE.md),
con BTF opcional y rechazo sticky de mezclas. El contrato revision64 siguiente
describe el modo sin esa restriccion; no constituye un proveedor multiactor.
Revision66 conecta esta captura con [ventanas/tasas SELF](LINUX_NETWORK_WINDOW.md),
sin publicar totales globales ni convertir IDs a procfs.

## Alcance y autorizacion

Sin argumento de autorizacion, `start(false)` no consulta BTF ni llama a BPF.
`start(true)` solo descubre la identidad del proceso llamante: no acepta PID
externo, cambio de UID ni filtro global. Usa BTF del kernel y syscalls BPF,
sin ejecutar utilidades ni cargar una biblioteca de telemetria externa.
Requiere soporte del kernel y permisos de carga/attach; EPERM/EACCES producen
`permission_required`, no un contador cero. No instala paquetes, cambia sysctl,
crea daemon ni fija objetos en bpffs. La autorizacion humana para la prueba
WSL temporal y acotada sigue siendo la recibida el2026-10-18.

El descubrimiento usa un programa RAW_TRACEPOINT no adjuntado y TEST_RUN local,
mapa3 propio. TGID del kernel, UID inicial y start_boottime del lider deben ser
exactos/validos antes de crear la captura. No se convierte getpid/start_ticks
en identidad del kernel ni se acepta un BTF/contador estimado como exacto.

## Propiedad y estados

`stopped -> active -> detached -> stopped`; tambien `active -> stopped`.
La sesion no se copia ni mueve. El API inyectable tiene contexto prestado que
debe sobrevivirla. Sus operaciones de estado/lectura se serializan por el
caller; no se ofrece acceso simultaneo seguro al mismo objeto.

Antes de adjuntar, carga todos los programas clasificados inet_sendmsg,
sock_recvmsg e inet6_sendmsg contra un mapa9 nuevo y la identidad SELF fija.
Fallo o excepcion durante inicio revierte todos los recursos ya adquiridos.
El mapa/programa temporales de identidad se cierran antes de adjuntar. fd0 es
valido y tambien se cierra. Cada descriptor se retira de propiedad antes de
cerrarlo; no se reintenta close ante EINTR ni se cierra dos veces.

`detach()` cierra enlaces en orden inverso y despues programas; retiene el mapa
para inspeccion. No promete drenaje portable de escritores arbitrarios que ya
estuvieran ejecutandose; el lector conserva la guarda de calidad. La prueba
de mapa congelado usa trafico terminado antes de detach, seguido de trafico
nuevo. `stop()` cierra tambien el mapa, borra identidad y es idempotente.
Destructor hace stop. No hay objetos persistentes tras terminar el proceso.
No se reinicia mientras se posee mapa, ni siquiera detached: primero stop.
Despues de fork el hijo debe cerrar su copia con stop; la identidad heredada
sigue siendo del creador, **no** se reetiqueta como trafico SELF del hijo.

## Lectura y limites

Cuatro acumulados uint64 de payload instrumentado: TCP envio/recepcion y UDP
envio/recepcion. No son tasas, bytes de interfaz, ACK ni cobertura total de red.
PEEK/TRUNC/ERRQUEUE positivos quedan excluidos y contados por separado. Se
conservan identidad, calidad sticky, overflow y secuencia del
[productor anterior](LINUX_NETWORK_QUALITY.md). Lectura con fences seq_cst y
doble guarda escritor/secuencia, hasta8intentos; ocupado, movimiento, fallo de
calidad, overflow o lookup denegado devuelven ausencia, no0 inventado. Cero
conocido solo se devuelve de un mapa leido coherentemente. Una lectura ausente
no desmonta la captura: caller/destructor siguen siendo responsables.

El soporte available de una lectura significa coherencia de esos contadores
instrumentados, **no** captura completa de toda la red. Revision64 no resolvia
socket netns cookie; revision65 anade restriccion declarada de un namespace,
no buckets dinamicos. Siguen pendientes join exacto procfs/apps/usuarios, multiactor, sendfile/splice/
io_uring y otros caminos no certificados, integracion con reductor/Monitor ni
ventanas/historial. No publica red total en ProcessInfo/UserSession. No se
inventa cookie a partir del inode del namespace. H04/J04 siguen pendientes.
La coherencia nativa se valida en WSL x86_64, no en todos los kernels/arquitecturas.

## Pruebas

`tests/linux_network_session.cpp` usa API inyectada sin privilegios/captura:
14 puntos de fallo y excepcion, identidad invalida, permiso, fd0, estimados,
lecturas ocupadas/movidas/overflow, orden de cierre, destructor y reinicio.
CTest mantiene asserts activos en Release y timeout20s.

`pcinsight-linux-network-session-native --authorized-self-session` es manual,
**no CTest**. Sin bandera retorna77 sin cargar sondas. El modo autorizado usa
solo loopback propio IPv4/IPv6 TCP/UDP con send/sendto/sendmsg, peeks excluidos,
hijo privado excluido y threads propios; verifica detach/stop/reinicio y cuenta
descriptores antes/despues. Watchdog20s y timeouts de sockets2s acotan el ensayo.
Unavailable/exit77 nunca se cuenta como positivo.

Verificacion2026-10-21: Linux Release116/116, prueba inyectada20/20 repeticiones,
consumidor instalado reconstruido y ejecutado exit0. Prueba nativa autorizada
exit0: TCP6144/6144B,UDP384/384B,12peeks excluidos, hijo excluido, mapa sin cambios
por trafico posterior a detach, stop/reinicio y sin fuga de FD. Sin bandera,
retorno77 confirmado directamente. Windows no se compilo ni ejecuto en esta unidad.
Logs `work/network-session-{suite,repeat,native,no-optin,install,consumer}-linux.log`.
Build inicial aviso clock-skew de0.0062s en dependencia startup-reset ajena al
componente; build verificado posterior sin warnings. El primer envoltorio de
comprobacion de exit77 fallo por quoting de shell, no por activar captura;
ejecucion directa y lectura de LASTEXITCODE confirmaron77. No se oculto el fallo.
