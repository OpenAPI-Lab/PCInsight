# API publica de red por procesos Linux seleccionados

Revision97: [capacidad1024](LINUX_NETWORK_SCALE.md), lotes observados de hasta64,
16384buckets estructurales, nativo65 privado. Limites previos debajo historicos;
el numero de cookies por proceso sigue16, no1024. Recompilar TODOS.

Revision88: [consulta con compuerta](LINUX_NETWORK_QUERY_SESSION.md) PASA nativo17
completo/tasas/limpieza. Sin productor compartido ni64actores reales certificados.

Revision87 (historica): [capacidad estructural64 y coste real](LINUX_NETWORK_CAPACITY64.md).
El limite16/256 de procesos/filas citado debajo es historico; el limite16 cookies
por actor NO cambia. Nativo17 falla por coste/vida acotada; NO captura escalable
certificada. Se conserva la evidencia original de cada revision.

Revision83 agrega [modo observado opt-in](LINUX_PROCESS_NETWORK_OBSERVED.md) al
proveedor standalone: sin cookies manuales, metadata de actores aun vacios,
hasta16cookies por proceso. Nuevo layout0.x de snapshot/SystemSnapshot: recompilar
TODOS. [Monitor/Sampler observado](LINUX_PROCESS_NETWORK_OBSERVED_MONITOR.md) ya
integrado en revision84. El contrato estricto y evidencia
historica de revision74 se conservan debajo; constructor default sigue estricto.

Revision74: `pcinsight/linux_process_network.hpp` publica
`pcinsight::LinuxProcessNetworkSampler`. API aditiva separada de Monitor y de
campos legacy; no cambia layouts existentes SystemSnapshot/SampleOptions/
ProcessInfo. Constructor y stop no adjuntan sondas. Fuera de Linux start
autorizado devuelve unsupported, sin implementacion Windows nueva.

## Uso C++20

```cpp
#include <pcinsight/linux_process_network.hpp>
#include <array>

pcinsight::Metric<std::uint64_t> iniciar(
    pcinsight::LinuxProcessNetworkSampler& red,
    int pidfd, std::uint32_t pid, std::uint64_t inicio_ticks,
    std::uint64_t cookie_namespace_socket, bool consentimiento)
{
    const std::array<pcinsight::LinuxProcessNetworkSelection, 1> seleccion{{
        {pidfd, pid, inicio_ticks, cookie_namespace_socket}
    }};
    return red.start(consentimiento, seleccion);
}
```

Conservar el objeto en el worker del consumidor y llamar `red.sample()` cada
ciclo. Comprobar `metric_is_known(resultado)` y cada Metric de tasa; primera
muestra/referencia recuperada tiene contadores pero no tasas. Ausencia se muestra
como no disponible, no cero. Arrays en orden TCP send/receive, UDP send/receive;
LinuxNetworkPayloadLane permite nombrar los indices. stop/destructor retiran
recursos. API sincrona, no promete1Hz/plazo duro; no bloquear el hilo del GUI.

El consumidor puede enlazar include/ y la biblioteca desde su IDE: no obliga a
crear su CMake antes del GUI. El build interno sigue compilando/probando PCInsight.
La instalacion incluye el nuevo encabezado; enlazar la biblioteca actualizada
para sus simbolos. No se crea/modifica un GUI o build de aplicacion en esta unidad.

## Seleccion, permisos y propiedad

start exige opt-in explicito,1..16 procesos distintos con pidfd vivo, PID e
inicio en ticks del mismo contexto procfs y UN cookie de namespace de socket
DECLARADO por proceso. start false no inicia captura nueva ni detiene una sesion
existente. No elevacion automatica, instalaciones, servicios o comandos externos.
Denegacion se expresa permission_required y fuente incompatible unsupported,
con nota. No se intenta inventar datos ni fallback a contadores del adaptador.

ProcessInfo.linux_start_ticks y PID local no son inicio/TGID inicial kernel.
Para un socket TCP seleccionado, read_linux_tcp_socket_holder ya expone
network_namespace_cookie si el sistema/permisos lo permiten. No es discovery
automatico de todos los sockets. No inventar cookie si falta. start no atesta
el cookie declarado: una operacion instrumentada de otro namespace invalida la
captura; inactividad/ceros no prueban que ese cookie exista o sea cobertura total.

Se retienen duplicados propios: el caller puede cerrar sus pidfds despues de
start, no concurrentemente durante esa llamada. Llamadas serializadas, misma
tabla FD, sin unshare/manipulacion asincrona de FDs internos. Objeto movable/no
copiable: move conserva captura; move assignment libera la anterior del destino.
Moved-from sample falla cerrado, stop seguro; start puede reutilizarlo. Fork
rechaza muestreo y cierra solo las referencias heredadas de la copia hija.

Requiere BTF/FEXIT/pidfd/BPF y politica ptrace/LSM compatibles. Se heredan limites
de arquitectura/endian del BTF; positivo real solo WSL x86_64. Descubrimientos
bloquean temporalmente mascara de senales caller y la restauran, pudiendo retrasar
handlers. Sin cancelacion asincrona/plazo duro ni ptrace attach/senal/parada del
objetivo. La contabilidad no lee contenido de trafico.

## Datos y cobertura

Cobertura tipada selected_tcp_udp_socket_operations: payload de retornos positivos
instrumentados de inet_sendmsg/inet6_sendmsg/sock_recvmsg, TCP/UDP IPv4/IPv6.
Recepciones PEEK/TRUNC/ERRQUEUE se excluyen y cuentan aparte. Otras rutas/asincrono/
offload/familias/protocolos no pasan a ser totales. No wire bytes/paquetes/ACKs
ni propiedad exclusiva de sockets compartidos: atribucion al proceso que realiza
la operacion instrumentada, desde start, no historial retroactivo.

processes une POR CLAVE identidad kernel initial_tgid/initial_real_uid/inicio ns/
cookie con identidad procfs local y calidad. observer conserva contexto propio.
UID local se expresa en namespace del OBSERVADOR, no el del objetivo; no comparar
IDs numericamente ni convertir ticks a ns. Cada fila incluye contadores desde
start, cuatro tasas Metric, intervalo nominal y contadores de calidad.

users contiene partial_payload_bytes_per_second por UID INICIAL, no username/
sesion/total. known/missing/overflow preservan disponibilidad; UID0 y cero medido
son validos. Suma tasas redondeadas por actor (floor), sin wrap en overflow.
Campos legacy ProcessInfo/UserSession/AppHistoryEntry no se rellenan con estos
valores parciales. Snapshot global Support::estimated, contadores comprobados
Support::available y tasas/sumas estimadas; no promover lo parcial a total.

La ventana comun es NOMINAL, conserva brackets actual/previo/epoch/boot/productor,
con barrido<=1s, NO adquisicion atomica. Joins extremos no eliminan ABA/vida futura.
Ver [semantica temporal](LINUX_NETWORK_COMMON.md). Fallo ordinario descarta
referencias y puede conservar captura para recuperar; sticky/actor salido puede
requerir stop/reinicio/cambio de seleccion. Excepcion/frame inconsistente detiene
captura. Recuperacion inicial sin tasa. Rendimiento/cadencia no garantizados.

## Verificacion y pendientes

Proyeccion pura comprueba scope/dominios/brackets/cardinalidad/dedup/asociacion
por clave/calidad/bytes/disponibilidad y sumas UID/known/missing/overflow. Fixtures
con ordenes distintos, IDs locales-kernel distintos, baseline0 sin tasa, UID0,
overflow y34frames invalidos. Construccion/move/opt-out/argumentos invalidos sin
BPF en CTest. Primer build encontro nombre de campo equivocado en fixture;
corregido sin relajar pruebas/log inicial preservado.

Manual NO CTest --authorized-private-public-window:2hijos propios sin token
kernel comunicado, cuatro oraculos actor/UID parcial, cobertura/identidades/
brackets, caller FD cerrado, move activo sin perder baseline, idle0/padre excluido,
fork/salida rechazados y sin fugaFD. Log work/network-public-native-linux.log,
intervalo nominal2.307999437s en este WSL. Consumidor instalado incluye/enlaza el
encabezado nuevo, sin habilitar captura. No procesos ajenos/captura global.

Termina proveedor publico SELECCIONADO, no todo F03. Revision75 agrega
[integracion opt-in Monitor/Sampler](LINUX_PROCESS_NETWORK_MONITOR.md), con nuevo
layout0.x y recompilacion de todos los consumidores. Revision76 permite
[actualizar la seleccion explicita](LINUX_PROCESS_NETWORK_UPDATE.md) sin recrear
las capturas retenidas; no descubrimiento automatico. Revision77 agrega
[actualizacion con el worker activo](LINUX_PROCESS_NETWORK_WORKER.md).
Sigue pendiente seleccion automatica/cobertura/buckets dinamicos sin falsear totalidad.
H04/J04 mantienen historial/sesiones pendientes.
Revision78 agrega [buckets declarados por namespace](LINUX_PROCESS_NETWORK_BUCKETS.md)
mediante constructor opt-in; permite varias filas/cookies por proceso en el
proveedor individual. Revision79 permite [elegir modo al iniciar Monitor/Sampler](LINUX_PROCESS_NETWORK_BUCKET_MONITOR.md),
sin cambiar las llamadas anteriores estrictas. No descubrimiento automatico ni
cierre de F03/H04/J04.

Resultados historicos de esta unidad publica inicial:
SEGUIMIENTO.txt revision74, logs work/network-public-*.

Verificado: Linux125/125 Release sin skips,17tests red repetidos20veces cada uno,
build final sin warnings. Encabezado instalado en include/pcinsight/ y consumidor
externo compilado/relinkado/ejecutado exit0 con esa instalacion. Logs final-build,
suite,repeat,install,consumer del mismo prefijo; no prueba de CI/hardware/distros
universales ni ejecucion Windows nueva.
