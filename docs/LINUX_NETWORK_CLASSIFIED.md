# Cuatro contadores TCP/UDP e IPv4/IPv6 (F03 en curso)

Esta sección describe el modo revisión29; revisión30 añade identidad y guardas
en el modo privado --authorized-quality: [LINUX_NETWORK_QUALITY.md](LINUX_NETWORK_QUALITY.md).
Los modos previos conservan sus limitaciones históricas.

Revisión29 amplía la prueba privada de capacidad, no entrega todavía un colector
de producción. `make_private_classified_network_probe` en linux_network_probe
genera bytecode directo sin herramientas externas. Sigue filtrando exactamente
un TGID del kernel antes de leer contexto/socket/mapa. La identidad se obtiene
mediante TEST_RUN local no adjuntado, nunca suponiendo que coincide con getpid.

## Qué cambió

Envío usa inet_sendmsg e inet6_sendmsg, no el syscall sendto ni sock_sendmsg.
Se valida en BTF su retorno int32 y los argumentos socket*,msghdr*,size_t64.
Recepción mantiene sock_recvmsg, cuya firma ya estaba validada. Todos esos
hooks tienen3argumentos y retorno fexit en slot24. Los hooks inet ausentes se
representan con ID0: no son un hook válido, ni invalidan la lectura del resto
del layout; si existen con firma ambigua/incompatible se rechaza el layout.
La prueba nativa clasificada requiere ambos para afirmar IPv4+IPv6 positivo.

Después de filtrar TGID y descartar retornos int32 no positivos, obtiene el
socket* del contexto seleccionado, lee socket.sk, skc_family y sk_protocol
mediante probe_read_kernel. Offsets y anchura de protocolo1/2B provienen del
BTF validado, no de constantes de otra versión. Scratch de pila inicializado
completamente antes de cada lectura. Punteros nulos/errores se contabilizan;
no se interpreta un fallo de lectura como familia/protocolo0 conocidos.

Solo AF_INET/AF_INET6 y IPPROTO_TCP/IPPROTO_UDP alimentan los cuatro lanes.
Los demás protocolos/familias quedan fuera del alcance, no se reclasifican
por puerto/nombre del proceso. El mapa ARRAY propio u32->u64 tiene6entradas:

| Índice | Significado |
| --- | --- |
| 0 | TCP envío, bytes de retorno positivo |
| 1 | TCP recepción, bytes de retorno positivo no excluido |
| 2 | UDP envío, bytes de retorno positivo |
| 3 | UDP recepción, bytes de retorno positivo no excluido |
| 4 | Número de operaciones positivas con lectura/socket fallidos |
| 5 | Número de recepciones positivas excluidas por flags |

MSG_PEEK, MSG_TRUNC y MSG_ERRQUEUE se excluyen de los bytes recibidos, y se
incrementa entrada5. Es política conservadora de esta prueba: un peek no
consume, un retorno con TRUNC puede no describir bytes copiados y ERRQUEUE no
es recepción de payload normal. No se oculta esa exclusión como contabilidad
completa. Los flags se inspeccionan antes de leer el socket de esa operación.
Otros flags, como DONTWAIT, no se excluyen por sí solos; errores/retorno0 no
aportan bytes ni se cuentan como recepciones positivas excluidas.

## Pruebas y autorización

El ejecutable manual linux-network-probe-native añade --authorized-classified.
Se usa la autorización humana explícita del2026-10-18, solo procesos privados
WSL/root temporal/limpieza final; no hay pinning ni configuración permanente
ni herramientas instaladas. No se registra la prueba privilegiada en CTest.
Sin argumento explícito, ni el modo anterior ni éste cargan sondas.

Fixtures portables en tests/linux_network_classified.cpp: TCP/UDP, familia4/6,
anchura1/2, filtro temprano sin lecturas ajenas, cada fallo de helper/punteros
nulos, familias/protocolos no incluidos, flags, retorno/error/signo, lookup nulo
y layout inválido. Intérprete limitado al bytecode generado, no sustituto del
verificador del kernel. Pruebas BTF incluyen firmas inet opcionales ausentes,
size_t incorrecto, ambigüedad y función extern no adjuntable.

Positivo real WSL6.6.114.1: TCP/UDP IPv4/IPv6 usando tanto send/sendto como
sendmsg,1024B TCP y64B UDP por caso, contenidos/retornos y contadores por fase
comprobados. TCP4096/4096, UDP256/256, fallos de lectura0 y8peeks excluidos sin
doble conteo. El hijo privado repite ambas APIs/familias sin incrementar el
mapa del TGID permitido. Tras cerrar los tres enlaces, repetir todo tampoco
incrementa: retirada comprobada. Prueba anterior no clasificada sigue pasando.
Logs: work/network-classified-sendmsg-native-linux.log y
work/network-classified-legacy-native-linux.log. El primer positivo, antes de
añadir sendmsg, se conserva en network-classified-native-linux.log.

Regresión Release final: Linux101/101 (BTF nativo Passed, no skip), Windows115/115.
Builds finales sin warnings. Consumidores instalados de ambos sistemas
relinkados/ejecutados exit0. Logs network-classified-suite-{linux,windows}.log,
network-classified-final-build-linux.log y network-classified-build-windows.log
bajo work/, además de install/consumer con el mismo prefijo. La prueba viva
clasificada final fue exit0, PID visible1281/TGID kernel3721. Los riesgos N03
históricos siguen abiertos; estas suites no prueban su corrección.

## Límites: no presentar como proveedor terminado

Los cuatro contadores pertenecen solo a la prueba privada actual. No hay mapa
por actor/UID/start_boottime/cookie netns, token de proceso reutilizable,
coherencia de snapshot para producción, detección de saturación uint64 ni
contabilidad de pérdidas global completa. Entrada4 solo cuenta fallos de las
lecturas instrumentadas; no demuestra cobertura completa ni ausencia de otras
pérdidas. Los contadores de prueba pueden desbordar y no deben publicarse sin
el control de saturación futuro. El proceso permitido sigue vivo todo el test.

No se prueba aquí sendmmsg/write/splice/io_uring/async, otros protocolos,
interfaces físicas ni wire bytes/ACKs/retransmisiones. Campos de socket leídos
a la salida no constituyen un token inmutable de su estado al inicio de la
operación. No se deduplica por socket/cookie: son retornos de operaciones del
actor observado, no propiedad exclusiva de un socket compartido.

El mapa aún NO satisface el contrato del reductor de revision27: no se conecta
a LinuxNetworkAccounting, Monitor, ProcessInfo/UserSession ni historial.
Sin API pública/layout existente/ABI virtual/historial nuevos. F03 sigue E,
J04/H04 abiertos. Falta productor contable con identidades, calidad/coherencia,
saturación, cobertura declarada e integración pública; no se inventa un ID
cerrado para subir el porcentaje por esta prueba.

Referencias primarias: [inet_sendmsg Linux6.6](https://raw.githubusercontent.com/torvalds/linux/v6.6/net/ipv4/af_inet.c),
[inet6_sendmsg Linux6.6](https://raw.githubusercontent.com/torvalds/linux/v6.6/net/ipv6/af_inet6.c),
[UAPI helpers BPF](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/uapi/linux/bpf.h).
