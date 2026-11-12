# Diagnóstico TCP de sockets/UID (F03 parcial)

Actualización posterior F03: [LINUX_TCP_HOLDERS.md](LINUX_TCP_HOLDERS.md) añade
consulta separada de un descriptor seleccionado por PID/start_ticks mediante
referencias fijadas/cookie. Es asociación de holder, no tráfico exclusivo de ese
proceso. El sampler diagnóstico descrito aquí conserva su propio alcance.

`pcinsight/linux_tcp.hpp` ofrece un sampler explícito de solo lectura, sin CLI,
daemon, libbpf, SDK de systemd ni cambios de firewall. Usa NETLINK_SOCK_DIAG,
INET_DIAG_INFO y TCP_INFO directamente. No se activa en Monitor ni en su captura
por defecto. No hay elevación ni instalación de dependencias.

```cpp
#include <pcinsight/linux_tcp.hpp>
pcinsight::LinuxTcpSampler tcp;
auto baseline = tcp.sample(); // contadores; tasas todavía ausentes
// En el siguiente tick de la aplicación, no inmediatamente en un busy-loop:
auto next = tcp.sample();
if (pcinsight::metric_is_known(next)) {
    for (const auto& socket : next.value->sockets) {
        // acknowledged_bytes_per_second, received_bytes_per_second
        // ¡No son tráfico total por PID ni throughput Ethernet!
    }
}
```

## Alcance exacto

Consulta IPv4 e IPv6 de los sockets TCP en ESTABLISHED(1) y CLOSE_WAIT(8), dentro
del namespace de red del **hilo llamador**. No incluye otros estados, sockets
desaparecidos entre muestras, UDP, QUIC, Unix sockets ni otras familias/protocolos.
No recoge direcciones, puertos remotos ni contenido del tráfico en la API; solo
identidad diagnóstica, UID, estado y contadores. No es un capturador de paquetes.

acknowledged_bytes y received_bytes conservan los campos nativos acumulados
tcpi_bytes_acked y tcpi_bytes_received, sin convertir colas o segmentos a bytes.
ACKed no es bytes enviados en ese instante: depende de confirmaciones remotas;
los contadores no son bytes de la interfaz física, headers, retransmisiones o
el total de tráfico del equipo. No se suman ambos para inventar un throughput.
Campos ausentes o TCP_INFO más corto se mantienen desconocidos por campo, no0.

La agrupación users usa **socket_uid reportado por el kernel**, no username,
UID real/effective del proceso, sesión logind ni responsabilidad causal de una
aplicación. Un socket puede compartirse entre procesos, heredarse o transferirse.
UIDs no representables pueden mapearse al UID overflow del namespace; no se
promete distinguir las identidades originales. No se publica un PID inferido
desde inode/fd ni se asigna arbitrariamente un socket compartido a un proceso.
Se preservan identidad dev/inode de los namespaces de red **y usuario**.
No combinar estas filas con totales J04 por real UID como si fueran equivalentes.

## Tasas, identidad y cobertura

Cada fila lleva captured_at monotónico al recibir su datagrama. El tracker exige
cookie nativa persistente, familia, inode, socket_uid y namespaces coincidentes.
No usa inode solo como identidad. INET_DIAG_NOCOOKIE deja contadores visibles
pero sin tasa. Primera observación, pérdida de muestra/socket/familia, reset,
cambio de identidad/namespace o tiempo sin avance no produce tasa.
Si disminuye cualquier contador conocido se descarta el intervalo en ambas
direcciones. Ausencia por campo requiere dos observaciones consecutivas válidas
de ese campo, sin impedir la dirección independiente que sí tiene contadores.
No se desborda duration firmado al comparar clocks extremos ni uint64 al sumar.

Tasa=delta/segundos propios del socket, bytes/s enteros redondeados hacia abajo,
Support::estimated. Cero observado es conocido; desconocido no se sustituye por0.
Cookies duplicadas invalidan el lote y limpian referencias para no contar dos
veces. La suma por socket UID conserva conocidas/ausentes por dirección y un
indicador overflow. Sin tasas conocidas no hay suma, aunque haya sockets.
Con parte conocida hay suma parcial estimada, no extrapolación. Overflow deja
el total ausente, no truncado/saturado. Las ventanas de cada socket pueden ser
distintas; no es snapshot atómico ni total global de ese usuario.

ipv4_dump_completed e ipv6_dump_completed distinguen dump completo vacío de
familia no disponible/denegada/incompleta. Si una familia falla se descarta toda
su respuesta parcial y se conserva la otra con diagnóstico explícito. Si fallan
ambas, el Metric exterior no tiene valor; una denegación observada se conserva.
Ningún dump vacío certifica que no haya tráfico de otros estados/protocolos.
Una muestra/familia fallida borra su baseline: no se calcula una tasa que salte
por encima de una interrupción desconocida.

## Protocolo, permisos y límites

Cada consulta abre su socket netlink dedicado y comprueba origen kernel pid0,
puerto local, secuencia, longitudes, alineación, atributos, flags de interrupción
de dump, errores y finalización NLMSG_DONE. Truncamiento, overrun, error, deadline,
identidad inconsistente o datos tras DONE no se publican como enumeración válida.
Namespaces se leen mediante /proc/thread-self/ns/net y /proc/thread-self/ns/user
antes/después; cambio durante consulta invalida toda la muestra.

No exige CAP_NET_ADMIN por diseño para estos campos; disponibilidad real depende
de kernel, módulos/configuración, namespaces y políticas LSM/seccomp. Denegación
se declara, no se evita con sudo. La librería no carga programas eBPF ni llama
insmod/modprobe; el kernel puede resolver sus propios módulos a demanda al
procesar una consulta. Encabezados UAPI Linux con los campos TCP_INFO usados son
necesarios al compilar; un kernel runtime más viejo puede devolver campos cortos.
Windows conserva la API pero sample devuelve ausencia motivada.

Por familia: hasta50000 filas,16MiB de datagramas,100000 mensajes, datagrama256KiB,
8 interrupciones reintentadas y presupuesto2s entre operaciones, poll hasta100ms.
No se promete deadline duro de syscalls del kernel. Resultado combinado100000
filas máximo. La memoria del tracker está acotada por esa cantidad, no es cero
coste: conserva contadores de la muestra previa. Objeto no thread-safe; el host
debe serializar sample/reset y no cambiar namespaces/credenciales concurrentes.
No hay tareas de fondo ni caché global entre instancias.

## Estado del proyecto y pruebas

Esto es una implementación nativa útil pero **NO cierra F03**: faltan atribución
por proceso/app con identidad y sockets compartidos, UDP/otras coberturas y su
proveedor de contabilidad opcional. Tampoco cierra J04 o H04. Los campos legacy
ProcessInfo/UserSession de red siguen ausentes; no se renombra TCP parcial como
tráfico total. No se añade un ID ya resuelto ni crédito fraccionario al tablero.
API adicional por header propio/símbolos, sin cambiar structs existentes ni
formato de historial/ABI virtual; recompilar/relinkar para usar los símbolos nuevos.

Pruebas portables del tracker y fixtures netlink Linux verifican identidad,
clocks, resets, ausencias, cero, overflow, respuestas malformadas/truncadas,
secuencia/puerto/familia/estado, NOCOOKIE, INFO corto/ausente/duplicado, errores
y dump interrumpido. La prueba nativa crea únicamente sockets propios en
127.0.0.1, transfiere8192B y compara el contador/tasa de su cookie; cierra sus
sockets sin modificar otras conexiones. Si el proveedor nativo falta, devuelve
skip77 explícito, que no cuenta como verificación positiva. Los resultados
finales de este entorno se registran también en SEGUIMIENTO.txt.

Verificación2026-10-17: Linux94/94 y Windows110/110 Release aprobadas, sin skip
del test TCP nativo. Consumidores externos instalados recompilados/relinkados/
ejecutados exit0; Linux consulta también la API instalada y Windows comprueba
ausencia motivada. No cambió código de librería tras esa instalación: la última
ampliación solo reforzó pruebas de recepción y decoder IPv6 y se repitieron las
suites completas. WSL real IPv4 loopback: cookie cliente16385, socket_uid1000,
delta reconocido8192B y recibido en servidor8192B; tasas muestreadas42999916B/s,
2sockets/1grupo UID. No es benchmark físico, valor permanente ni tráfico total.
IPv6 positivo del parser es sintético; no se afirma transferencia IPv6 nativa.
Logs work/tcp-diag-{build,verified-build,final-build}-{linux,windows}.log,
work/tcp-diag-{suite,final-suite}-{linux,windows}.log y
work/tcp-diag-final-native-linux.log. Builds sin avisos. Riesgos históricos N03
CPU/historial no se consideran corregidos por pasar estas ejecuciones.

Fuentes primarias: [UAPI inet_diag](https://github.com/torvalds/linux/blob/master/include/uapi/linux/inet_diag.h),
[UID y estructura de diagnóstico](https://github.com/torvalds/linux/blob/master/net/ipv4/inet_diag.c),
[proveedor TCP del kernel](https://github.com/torvalds/linux/blob/master/net/ipv4/tcp_diag.c).
