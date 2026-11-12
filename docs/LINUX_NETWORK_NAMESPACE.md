# Captura SELF acotada al namespace del socket

Revision65, F03 en curso. La sesion privada admite ahora
`start(authorized, expected_socket_netns_cookie)`. El valor no cero declara el
unico namespace de sockets aceptado; no selecciona otro PID ni amplifica el
alcance SELF. Cero conserva el modo anterior sin restriccion de namespace y se
devuelve como campo opcional ausente, no como una identidad valida.

## Identidad y BTF

El resolver BTF comprueba la ruta real del kernel:
`socket.sk -> sock.__sk_common.skc_net.net -> net.net_cookie`.
Valida tipos, anchos unsigned64, puntero a struct net, wrappers/typedefs, limites
de contenedores y offsets acotados a1MiB. El layout es opcional: kernel sin ese
campo/CONFIG_NET_NS, duplicados, puntero incorrecto o wrapper invalido deja
ausente esta capacidad, sin inventar offset0 ni impedir las pruebas antiguas.
El modo con cookie la exige exacta antes de crear mapas/cargar programas.

La identidad procede del **socket de la operacion**, no current->nsproxy,
getpid, inode de /proc/.../ns/net, nombre de interfaz ni netns del observador.
Los sockets pueden sobrevivir a cambios de namespace o pasarse entre procesos;
la pertenencia actual del proceso no certifica la del socket.

El codigo usa lecturas BPF acotadas y offsets del BTF vivo, no direcciones
constantes del kernel ni el helper de netns cookie de otra clase de programa.
No requiere utilidades ni SDK de telemetria. El cookie es opaco, no un puntero.
El campo de salida `expected_socket_network_namespace_cookie` expresa la
restriccion declarada por el caller, **no** un inventario observado de namespaces.
Un mapa nuevo puede contener cero antes de que haya alguna operacion coincidente;
no prueba que exista un socket en el namespace declarado ni su cobertura total.

## Rechazo y coherencia

Despues del filtro temprano TGID y del token inicio/UID, cada retorno positivo
comprueba socket/sk/net/cookie. Un nulo, lectura fallida, cookie0 o distinto fija
un fallo de calidad bajo la misma guarda escritor/secuencia. Es sticky: las
lecturas posteriores quedan ausentes hasta retirar/recrear la captura.
No se suman bytes de namespaces distintos ni se ignora el fallo como actividad0.
Incluso recepciones PEEK/TRUNC/ERRQUEUE positivas pasan primero la comprobacion
de namespace; si coinciden se cuentan como excluidas, nunca como payload util.
Operaciones fallidas o retorno0 siguen sin aportar bytes. Un TGID ajeno sale
antes de leer contexto/socket. No se capturan contenidos ni direcciones de red.

Este modo falla cerrado ante mezcla: **no** implementa aun buckets dinamicos por
cada netns. No resuelve multiples actores, cambios intermedios de UID, join
procfs/apps/usuarios, cobertura de todos los caminos I/O, integracion con
reductor/Monitor/tasas/historial. Ningun campo total de ProcessInfo/UserSession
se rellena con estos acumulados experimentales. H04/J04 siguen abiertos.

Revision78 agrega un modo SEPARADO de [buckets declarados](LINUX_PROCESS_NETWORK_BUCKETS.md)
al proveedor publico: cookies no cero ajenos se excluyen sin escribir en el mapa,
sin cambiar este modo estricto. Varias filas por proceso/cookies disjuntos, no
descubrimiento automatico ni prueba de existencia de namespace declarado.

## Verificacion

Fixtures ampliados existentes, sin nuevo ID de tarea ni dependencia:

- BTF: ruta positiva y10 corrupciones de puntero/anchos/alineacion/duplicados/
  wrappers/ciclos/limites/campo ausente; la capacidad opcional falla cerrada.
- Interprete de bytecode: IPv4/IPv6, TCP/UDP, send/receive, cookie64 con bits
  altos, nulo/0/distinto/lectura fallida, flags excluidos y TGID ajeno.
- Sesion: rechazo de layout ausente/estimated, metadato declarado, fallo sticky
  y stop/reinicio sin conservar la restriccion del mapa anterior.

El modo manual `--authorized-self-namespace-session` usa la autorizacion temporal
WSL privada ya recibida. Obtiene SO_NETNS_COOKIE de un socket UDP propio, ejecuta
la matriz loopback y despues prueba un valor deliberadamente distinto. No
crea otro namespace, ejecuta setns/unshare ni configura interfaces. El negativo
prueba desacuerdo de cookie real contra valor declarado, **no** dos namespaces
reales a la vez. Sin bandera no se carga captura y retorna77.

Prueba nativa autorizada exit0, WSL x86_64: cookie1 corroborado, TCP6144/6144B,
UDP384/384B,12peeks, hijo excluido/threads propios, detach/stop/reinicio sin fuga
de descriptores; cookie distinto invalida lectura de forma persistente. Modo
anterior sin restriccion tambien exit0. Logs `work/network-netns-native-linux.log`
y `work/network-netns-legacy-native-linux.log`. No es certificacion universal
de kernels, memoria/arquitecturas ni contenedores con namespaces distintos.

Regresion Linux Release116/116;8pruebas de red repetidas20veces cada una,
consumidor instalado reconstruido y ejecutado exit0. Build sin warnings.
Logs adicionales `work/network-netns-{build,focused,suite,repeat,install,consumer}-linux.log`.
No se ejecutaron builds/capturas Windows en esta unidad, ni se cambio API publica.

Fuentes primarias revisadas: [struct net/possible_net_t Linux6.6](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/net/net_namespace.h)
y [SO_NETNS_COOKIE Linux6.6](https://raw.githubusercontent.com/torvalds/linux/v6.6/net/core/sock.c).
El layout efectivo se valida contra BTF del kernel ejecutado, no contra offsets
de esas fuentes. [Ciclo de vida](LINUX_NETWORK_SESSION.md) y
[guardas](LINUX_NETWORK_QUALITY.md) conservan las limitaciones previas.
