# Captura privada de un actor seleccionado

Revision69 incorpora [identidad desde pidfd](LINUX_NETWORK_PIDFD.md), con una
prueba privada en la que el hijo no comunica su token del kernel. El contrato
declarado revision68 siguiente no se convierte en discovery/join global.

Revision68 amplia el componente interno de sesion, sin API instalada ni Monitor.
`start_selected(authorized, kernel_identity, socket_netns_cookie)` carga las
tres sondas existentes para **un** TGID/generacion/UID del dominio inicial y un
cookie de namespace de socket. No captura todos los procesos ni enumera actores.

## Contrato y separacion de SELF

El token es declarado por el caller: TGID positivo <=INT32_MAX, start_boottime
no cero, UID uint32 exacto (0 es valido), cookie no cero. Ausencia de autorizacion
o formato invalido no hace llamadas de captura. BTF/hook/namespace ausente o
estimated impide cargar. El kernel sigue aplicando el filtro TGID temprano y
las guardas de generacion/UID/cookie por operacion positiva.

**Aceptar el token no autentica su procedencia, ni verifica que un PID de procfs
se corresponda con ese TGID, ni que el actor exista/este vivo.** Un mapa fresco
coherente con0 significa0 operaciones contabilizadas en el scope declarado,
no existencia/liveness del proceso. El proveedor que use esta via debe obtener
y revalidar un token confiable y comprobar la vida del actor por separado.
Un PID local/NStgid/start_ticks no se convierte automaticamente a este token.
No hay busqueda de otro PID mediante TEST_RUN ni uid_map inferido.

`PrivateNetworkSessionRead.target_kind` distingue `self` de `selected_actor`.
El metodo SELF existente conserva descubrimiento local; selected no ejecuta
ese descubrimiento como sustituto del objetivo. El sampler SELF comprueba el
tipo y nunca publica un selected como si fuera su creador. read_caller_identity
en la sesion exige SELF activo. La funcion interna extraida de descubrimiento
unattached siempre lee al **caller**, nunca al actor seleccionado.

Comparte el mismo ciclo de vida: carga completa antes de attach, rollback,
destructor, detach conservando mapa para lectura guardada y stop idempotente.
Reinicio exige stop, borra el tipo/identidad anteriores; ningun contador viejo
se reasigna al nuevo objetivo. No hay pinning, daemon, configuracion, shell ni
dependencia de aplicaciones externas. Una sesion seleccionada no se detiene
automaticamente al morir su objetivo; su propietario debe retirarla. La prueba
lo hace despues de comprobar salida. No se presenta su mapa como proceso vivo.

## Prueba real: hijo privado cooperativo

Modo manual `--authorized-private-child-session` del ejecutable de sesion,
**no CTest**. Usa la autorizacion temporal WSL ya recibida, filtrada a procesos
privados de prueba. Padre crea hijo y pipes propios, antes de cualquier attach.
El hijo ejecuta solo descubrimiento SELF no adjuntado, obtiene su token procfs
y SO_NETNS_COOKIE y comunica esos datos al padre. Espera comandos antes de
generar trafico loopback IPv4/IPv6 TCP/UDP mediante send/sendto/sendmsg.

El padre verifica que el PID local reportado es su hijo conocido por fork y
mantiene un pidfd con poll de liveness. Esto **no** demuestra que pidfd autentique
el TGID/start reportado ni implementa un protocolo seguro de atestacion IPC:
el hijo es codigo privado conocido y cooperativo. No se consulta ni instrumenta
un proceso ajeno al ensayo. No hay intercambio de contenido de aplicaciones.

Verificado WSL exit0: hijo PID local2075/TGID kernel2600/UID inicial0,
TCP4096/4096B,UDP256/256B y8peeks excluidos. Trafico identico del padre no cambia
el mapa del hijo. Tras detach, nuevo trafico del hijo no cambia los contadores.
Start token o UID deliberadamente distintos producen calidad sticky/ausencia,
no0 fingido. Se observa salida con pidfd, se retira sesion y se comprueba el
conteo de FD inicial/final. En fallo, guard de hijo privado limpia/recolecta;
watchdog padre20s/hijo15s y sockets2s. Politica SIGPIPE/SIGCHLD solo del ejecutable
de prueba, no de la biblioteca ni del sistema. Sin archivo/namespace creado.
Evidencia `work/network-selected-native-linux.log`.

Fixtures ampliados: opt-in/formato sin llamadas,8puntos de fallo/excepcion con
rollback, FD0 del mapa, metadatos/tipo, rechazo de masquerade SELF y reinicio al
modo anterior. La prueba real SELF asociada tambien se repite exit0: cuatro
tasas/UID contra oraculo, reposo/fork/reinicio/sin fugaFD.

F03 sigue E: falta descubrir/revalidar actores arbitrarios, join confiable con
procfs/apps/usuarios, buckets multiactor/netns dinamicos, cobertura adicional y
exposicion publica. No integra selected en el reductor SELF ni en historial,
no rellena red total en ProcessInfo/UserSession, y no cierra H04/J04.
Windows sigue aplazado.

Regresion Release Linux119/119 sin skips;11tests de red20repeticiones cada uno,
consumidor instalado reconstruido y ejecutado exit0. Build sin warnings.
Logs `work/network-selected-{build,focused,suite,repeat,install,consumer}-linux.log`
y `work/network-selected-self-regression-linux.log`. No se ejecutaron builds ni
capturas Windows. Un primer patch no aplico por contexto, sin cambios parciales;
se reaplico con contexto reducido antes de compilar.
