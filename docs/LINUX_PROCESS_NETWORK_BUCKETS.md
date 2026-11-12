# Buckets declarados de namespace por proceso

Revision88: [consulta con compuerta](LINUX_NETWORK_QUERY_SESSION.md) PASA nativo17
completo/tasas/limpieza. Sin productor compartido ni64actores reales certificados.

Revision87 (historica): [capacidad estructural64 y coste real](LINUX_NETWORK_CAPACITY64.md).
El limite16/256 de procesos/filas citado debajo es historico; el limite16 cookies
por actor NO cambia. Nativo17 falla por coste/vida acotada; NO captura escalable
certificada. Se conserva la evidencia original de cada revision.

Revision78 agrega modo opt-in al proveedor publico LinuxProcessNetworkSampler.
Permite repetir PID/start en la seleccion cuando cada fila tiene un cookie de
namespace de sockets DISTINTO. Limite total16 filas, no16 cookies por proceso.
No descubre cookies ni procesos automaticamente. No entra en otros namespaces,
ni requiere paquetes/CLI/SDK nuevos ni eleva permisos automaticamente.

```cpp
#include <pcinsight/linux_process_network.hpp>

pcinsight::LinuxProcessNetworkSampler red(
    pcinsight::LinuxProcessNetworkSelectionMode::declared_namespace_buckets);
std::array<pcinsight::LinuxProcessNetworkSelection, 2> seleccion{{
    {pidfd, pid, start_ticks, cookie_a},
    {pidfd, pid, start_ticks, cookie_b}
}};
auto inicio = red.start(consentimiento, seleccion);
// Usar sample/update_selection/stop como en el proveedor seleccionado anterior.
// Verificar support/coverage. Primera muestra por epoch no tiene tasas.
```

El constructor por defecto conserva strict_single_namespace: un proceso por
fila, un cookie declarado, otro cookie detectado invalida la captura. No se
cambia silenciosamente el modo anterior. Constructor con enum invalido no inicia
captura. Stop/start/update mantienen modo; move transfiere configuracion. Un
objeto movido sin Pimpl que se reutiliza mediante start reconstruye modo estricto.
Modo no cambia una captura activa. API aditiva/enum ampliado, sin layouts publicos
nuevos; enlazar biblioteca actual para constructor nuevo. Recompilar todos si
se viene de layouts anteriores a revision75. Consumidor puede usar IDE sin CMake.

## Filtro y calidad

Cada bucket posee mapa/links/pidfd propios y programa filtrado por TGID inicial,
inicio del lider/UID y cookie del SOCKET de la operacion. Reutiliza BTF vivo y
lecturas/guardas escritor-secuencia-overflow ya verificadas. No usa namespace
actual del proceso como sustituto del socket. Sockets heredados/transferidos no
se atribuyen por simple posesion; sigue contabilizando operaciones del actor.

En modo buckets, otro cookie NO CERO sale ANTES de escribir en mapa, incluso
para recepcion excluida PEEK/TRUNC/ERRQUEUE. Por tanto dos buckets disjuntos no
cuentan la misma operacion. Cookie0, punteros nulos, errores de lectura/identidad,
overflow o calidad incoherente siguen fallando cerrado; no se convierten en0.
No mezclar estos buckets filtrados con captura estricta/sin filtro del mismo
actor en una suma. Cada fila tiene un cookie, no un rango ni comodin.

Cookie no observado entrega contador0 en SU filtro, no prueba de que exista ese
namespace ni de que su trafico total sea0. El valor declarado tiene que proceder
de una fuente correcta del consumidor; aqui no se certifica existencia ni se
enumeran sockets/netns del objetivo. Netns sin cookie inicializado/layout
compatible puede quedar ausente. No clasifica contenido, direcciones o paquetes.

## Identidad, ventanas y reporte

Se rechaza duplicar PID/cookie, usar start_ticks distintos para un PID, repetir
bucket kernel, asociar mismo kernel con identidades procfs distintas, o dar UID/
contextos distintos al mismo proceso. Ambitos locales e iniciales siguen
separados. Se revalidan extremos de cada actor; no snapshot atomico ni garantia
de continuidad entre ellos. Cualquier fallo invalida TODOS baselines comunes.

coverage=declared_tcp_udp_socket_namespace_buckets distingue el reporte nuevo.
processes son FILAS DE BUCKET, no una lista deduplicada de aplicaciones: mismo
process puede aparecer varias veces con kernel.socket_network_namespace_cookie
distintos. La asociacion se resuelve por clave, no por orden de vectores. UID
observed_buckets cuenta buckets, no procesos ni sesiones; suma tasas parciales
con known/missing/overflow sin presentar total global/de app/de usuario. No se
rellenan ProcessInfo/UserSession/app_history con esos valores parciales.

Actualizacion explicita completa retiene PID/start/cookie coincidentes, cierra
retiradas, inicia nuevas, crea otra epoch y borra TODOS los baselines/brackets.
Los contadores retenidos conservan origen y los nuevos no recuperan trafico del
hueco. Denegacion/lista mal formada conserva seleccion; fallo runtime cierra
TODO. No sumar diferencias ni unir graficas entre epochs. Lista vacia usa stop.

Revision79 agrega [setup explicito de buckets en Monitor/Sampler](LINUX_PROCESS_NETWORK_BUCKET_MONITOR.md).
Llamadas anteriores siguen estrictas, no heredan buckets entre sesiones. Siguen
pendientes politica/descubrimiento automatico, buckets dinamicos sin cookies
manuales y cobertura general. F03/H04/J04 no se cierran.
Revision80 agrega [captura privada de cookies observados](LINUX_NETWORK_DISCOVERY.md),
sin entrada de cookie, pero no la conecta todavia a este proveedor publico ni a
sus ventanas/join/multiactor. Las APIs de esta pagina siguen declaradas.

## Evidencia

Interprete bytecode: IPv4/IPv6, TCP/UDP, send/receive, dos cookies64 del MISMO
actor positivos por separado, tercero excluido sin escrituras/doble cuenta,
recepciones excluidas, cookie0/null/errores/UID-start/overflow sticky y TGID ajeno.
Sesiones prueban metadato de filtro y reinicio estricto sin flag retenido.
Coleccion/reductor/proyeccion: dos buckets mismo proceso/cookies distintas,
ventana comun/4tasas/suma UID, reorder sin recrear captura, retiro/rollback,
duplicados/identidades o UID incoherentes/limite16 rechazados. Primer test de
overflow nuevo inicializaba otro protocolo que el lane probado: corregido
fixture, sin relajar productor; logs iniciales preservados.

Manual NO CTest --authorized-private-namespace-buckets: dos hijos privados
autorizados, captura de UNO con cookie real y otro deliberadamente no observado.
Cuatro oraculos positivos reales/otro bucket0/suma UID sin duplicado, trafico de
cookie no seleccionado excluido, update retiro/readd con epoch-baseline/gap
limpios, otro hijo/padre excluidos, fork rechazado y sin fugaFD. SOLO UN namespace
real ejercitado: no prueba de dos namespaces reales simultaneos, ni descubrimiento
o existencia del segundo. No setns/unshare/interfaces nuevas. Watchdogs privados
parent60s/child50s, sin plazo duro BPF. Regresion --authorized-private-public-window
tambien positiva con dos actores/modo estricto y sus cuatro oraculos actor-UID.
work/network-buckets-native-linux.log y strict-native-linux.log.

Mismos CTests, no nueva tarea ni dependencia. Logs del prefijo network-buckets
conservan build/suite/repeat/install/consumer y compatibilidad comun MSVC. Sin
captura Windows nueva; fallo del consumidor general MSVC revision75 sigue N03,
no se afirma compatibilidad completa Windows ni todos kernels/distros/hardware.

Resultado: Linux126/126 Release sin skips,5tests focalizados repetidos20veces cada
uno, build sin warnings y consumidor instalado reconstruido/enlazado/ejecutado
exit0. MSVC biblioteca y5/5 focalizadas pasan, sin warnings C++ nuevos (D9025
esperado mantiene asserts). Control oficial revision78:57/75=76,0% cerrado,
18/75=24,0% abierto; sin IDs nuevos ni credito parcial.
