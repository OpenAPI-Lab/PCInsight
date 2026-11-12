# API publica: cookies observados de procesos seleccionados

Revision93: [backend compartido PUBLICO](LINUX_NETWORK_SHARED_PUBLIC.md)
activado en standalone/Monitor/Sampler/owner; nativo17/censo inicial N+14FDs,
updates-retencion-readmision y limpieza positivos. Los comentarios anteriores
de productor no compartido/coste fallido debajo son historicos.64/1024 siguen
fixtures; admision general y totales de app/usuario no se deducen de este cambio.

Revision88: [consulta con compuerta](LINUX_NETWORK_QUERY_SESSION.md) PASA nativo17
completo/tasas/limpieza. Sin productor compartido ni64actores reales certificados.

Revision87 (historica): [capacidad estructural64 y coste real](LINUX_NETWORK_CAPACITY64.md).
El limite16/256 de procesos/filas citado debajo es historico; el limite16 cookies
por actor NO cambia. Nativo17 falla por coste/vida acotada; NO captura escalable
certificada. Se conserva la evidencia original de cada revision.

Revision83 expone observed_namespace_buckets en LinuxProcessNetworkSampler.
Opt-in explicito; constructor predeterminado sigue strict_single_namespace.
No captura global ni enumeracion general de procesos. Continuacion revision84:
[Monitor/Sampler observados](LINUX_PROCESS_NETWORK_OBSERVED_MONITOR.md) ya admiten
setup opt-in de tres argumentos. Se conserva abajo evidencia historica revision83.

## Uso C++20

```cpp
#include <pcinsight/linux_process_network.hpp>
#include <array>

pcinsight::LinuxProcessNetworkSampler red(
    pcinsight::LinuxProcessNetworkSelectionMode::observed_namespace_buckets);

pcinsight::Metric<std::uint64_t> iniciar_red(
    int pidfd, std::uint32_t pid, std::uint64_t inicio_ticks,
    bool consentimiento)
{
    const std::array<pcinsight::LinuxProcessNetworkSelection, 1> seleccion{{
        {pidfd, pid, inicio_ticks, 0} // 0 OBLIGATORIO: no cookie manual
    }};
    return red.start(consentimiento, seleccion);
}

// Desde el worker del consumidor, no bloquear el hilo del GUI:
// auto muestra = red.sample();
// if (pcinsight::metric_is_known(muestra)) { ... }
// red.stop();
```

pid/start son de la misma vista procfs/boot del consumidor; usar el PID y
ProcessInfo.linux_start_ticks conocido de la biblioteca y un pidfd vivo del mismo
proceso. No convertir ticks en inicio kernel ni suponer que PID/UID locales son
IDs iniciales. Posee pidfd duplicado tras setup, original puede cerrarse. No se
ejecuta CLI ni se obliga al consumidor a crear CMake antes de su GUI: incluir
encabezados y enlazar biblioteca actualizada en su IDE tambien sirve.

Seleccion1..16procesos distintos, cookie entrada0 en TODAS las filas. No admite
cookies manuales, duplicados ni cambiar modo en sesion activa. Descubre hasta16
cookies de namespace del SOCKET por actor (256filas total) durante operaciones
positivas instrumentadas. No inventario de sockets inactivos ni proceso general.
Constructor/stop no cargan BPF; start false no autoriza captura. Update exige
consentimiento nuevo y seleccion completa, conserva modo al mover/stop/restart.
Fuera de Linux autorizado da unsupported, no implementacion Windows nueva.

## Resultado y disponibilidad

Snapshot coverage observed_tcp_udp_socket_namespace_buckets diferencia este modo
de strict/declarado. observed_processes contiene1..16asociaciones, en orden de
seleccion: PID/start/contexto local, TGID/UID real inicial/inicio kernel separados,
observed_buckets y completed_updates del actor completo. No cookie ficticio0.

processes sigue siendo vector de BUCKETS, NO una fila por proceso. Cada fila tiene
cookie real/identidades/bytes/exclusiones/tasas. Observed_processes mantiene un actor
sin actividad aun cuando no hay bucket suyo. Seleccion vacia no es valida; registro
de trafico vacio SI: processes/users vacios, observed_processes no vacio y counts0.
Eso no afirma que el proceso no use red por otras rutas/no instrumentadas.

Relacionar por identidad, no indice: accounting/buckets estan ordenados por clave,
asociaciones por seleccion. Para varias cookies del mismo actor completed_updates
se REPITE en sus filas: es secuencia global del actor, NO sumar por cookie. Su
valor unico tambien esta en observed_processes. Exclusiones son por bucket y no
son bytes. No promover ninguna suma a total de proceso/usuario.

Primer encuentro de CADA bucket tiene bytes pero NO tasa ni intervalo, incluso si
antes hubo registros vacios. Llegada tardia no invalida baseline de otros buckets.
Segunda muestra limpia permite tasa cero medida o positiva. Snapshot estimated
por cobertura parcial/barrido sesgado, bytes comprobados available, tasas/sumas UID
estimated. Respetar cada Metric: ausencia no es0. Arrays TCPsend/recv UDPsend/recv.
Usuarios son UID inicial al operar, no cuenta local/sesion/username.

Un solo bracket de barrido<=1s/intervalo nominal entre finales, NO adquisicion
atomica. Actor terminado, calidad sticky/overflow, identidad/contexto incoherente,
read/clock fallido rechazan TODO el grupo y descartan TODOS baselines. Recuperacion
sin tasa, intervalo>60s tampoco genera tasa. Frame inconsistente/excepcion detiene
captura. El productor conserva guard de registro/contadores monotonicamente validos
al descartar ventanas: error no oculta regresion.

Update retiene origen de capturas coincidentes PID/start, retiro cierra recursos y
readd crea registro vacio sin trafico del gap. Todo update exitoso cambia epoch y
resetea TODAS las tasas/brackets. Forma/consentimiento invalidos no mutan captura;
fallo runtime cierra todo. Stop/restart requiere opt-in otra vez/mapas nuevos.
Fork rechaza lectura/update y cierra copia de referencias sin afectar al padre.

## Permisos, calidad y compatibilidad

Kernel/BTF/FEXIT/pidfd/atomics64 y BPF/ptrace/LSM compatibles; ausente/denegado con
nota, nunca fallback al trafico del adaptador. Cookie0 sin inicializar,17o namespace
por actor, overflow o fallo identidad/lectura causan ausencia sticky, no eviccion.
No fuerza inicializacion del cookie en otros namespaces. Payload de operaciones
TCP/UDP instrumentadas, no wire bytes/ACKs/interfaz/propiedad exclusiva ni todos
caminos I/O. Serializar llamadas/misma tablaFD; no manipular FDs propios, unshare
ni cancelar asincronamente. No plazo duro BPF ni privilegio elevado automaticamente.

CAMBIO DE LAYOUT0.x: LinuxProcessNetworkSnapshot agrega observed_processes y
LinuxNetworkObservedProcess es tipo nuevo; tambien cambia el layout de quien
contenga el snapshot (incluido SystemSnapshot). Recompilar biblioteca y TODOS
los consumidores, no mezclar objetos/binarios viejos con headers nuevos. Se agregan
valores enum y constantes de limites; actualizar switches exhaustivos. APIs
anteriores conservan sus politicas/resultados, observed_processes vacio en ellas.
Sin formato de persistencia ni GUI nuevo; historial durable no integrado aqui.

## Verificacion

linux-process-network conserva fixtures anteriores y agrega observado: vacio con
dos asociaciones, mixto actor vacio/activo, primer bucket sin tasa, ordenes distintos,
IDs local/kernel separados, varias cookies/update repetido sin duplicar UID, maximo
16x16=256filas del proyector y23frames invalidos (identidades, duplicados, calidad,
inactividad imposible, cardinalidad, cookie0, bytes/UID/seq/formato ambiguo). Proyector
no publica resultados parciales. Test Monitor confirma rechazo observado sin
llamar proveedor, no falso soporte. Consumer instalado enlaza tipo/modo/limites
nuevos y verifica consentimiento/sin captura. Los256positivos son fixtures.

Manual --authorized-private-public-observed prueba API PUBLICA con dos hijos
privados propios autorizados: exige cookie0/default estricto intacto; copia pidfds,
cierra originales y mueve sampler activo. Metadata conserva actores vacios, primer
bucket/tardio sin tasa; dos hilos concurrentes por actor producen A TCP12288/12288
UDP768/768 peek24, B doble. Cuatro tasas por actor/suma UID contra oracle entero,
padre excluido/fork rechazado/consentimiento update, retiro/readd conserva origen
retenido y nuevo excluye gap/epochs/reset limpios. Salida de B rechaza grupo dos
veces, remover B recupera sin cruzar ventana; stop/restart conserva modo observado,
rechaza cookie declarada y empieza vacio. Ambos hijos terminan, sin fugaFD.

SOLO UN namespace real compartido. Hijo inicializa SO_NETNS_COOKIE para oracle
independiente, NO lo entrega al productor. No prueba multiples netns reales,
cookie sin inicializar,16actores/256buckets reales ni todas distros/kernels.
Watchdogs parent60s/child50s, no cambio persistente/CLI/dependencia/captura global.
Linux126/126 Release sin skips,3tests20repeticiones cada uno; biblioteca sin
warnings/consumidor instalado recompilado-enlazado-ejecutado exit0 con encabezado
y layout nuevos. Regresion nativa publica estricta positiva. MSVC3/3 focalizadas
de codigo compartido sin nuevos C++ warnings (D9025 esperado asserts activos).
No suite/consumidor Windows completos nuevos: fallo consumidor revision75 sigue
abierto N03, no compatibilidad total. Logs: work/network-public-observed-*.

F03 permanece abierta: siguiente integrar modo observado en Monitor/Sampler,
despues politica de seleccion general. H04/J04 siguen abiertas; no rellenar
ProcessInfo/UserSession/AppHistoryEntry con estos datos parciales como totales.
