# Ventanas seleccionadas de cookies observados (internas)

Continuacion revision82: [ventana comun multiactor interna](LINUX_NETWORK_OBSERVED_MULTI.md)
ya agrega registros de varios actores con reset grupal y updates, sin API publica.
La evidencia y contrato individual de revision81 se conservan debajo.

Revision81 conecta el motor de revision80 a LinuxSelectedNetworkSampler mediante
start_discovering(authorized,pidfd,pid,start_ticks) y sample_discovered(). No exige
cookie al llamador. Sigue siendo un proveedor INTERNO para UN actor seleccionado,
no API publica, seleccion general, Monitor/Sampler, multiactor ni historial.
Las APIs publicas de cookies declarados no cambian. F03 sigue abierta.

## Identidad y ciclo de vida

Consentimiento explicito antes de adquirir recursos; PID/start validos y APIs
completas. Duplica el pidfd y conserva su propio descriptor, por lo que el llamador
puede cerrar el original. El join inicial pidfd/procfs proporciona el token kernel
TGID/UID real inicial/inicio del lider, separado de PID/UID del namespace local.
La identidad del productor tambien procede del sondeo autenticado existente.
Misma disciplina de rollback ante ausencia, denegacion y excepciones que el modo
estricto. No elevacion automatica, CLI, descarga, nueva dependencia ni cambio de
namespace/configuracion. Serializar llamadas; contextos inyectados deben vivir mas
que el objeto y no se puede sustituir su tabla de descriptores.

Cada muestra valida el pidfd y contexto antes y despues del barrido integral del
mapa. Exige identidad kernel/boot originales y contexto local igual en los dos
extremos, reloj disponible y bracket ordenado/no solapado de hasta1s. Fork del
recolector rechaza lectura y cierra los descriptores heredados de su copia. Un
proceso terminado deja ausencia, no se reasigna un PID reutilizado. No se atribuyen
filas a un usuario local por coincidencia numerica con UID inicial.

## Tasas y calidad

sample_discovered devuelve contadores brutos por cookie, exclusiones, secuencia,
bracket, contexto local y NetworkAccountingOutput. Coverage interna nueva
selected_observed_namespace_set: cero a16 cookies observados de UN actor, no
inventario de sockets ni total de red. La lectura vacia inicial es conocida y no
inventa cookie0, fila de proceso/usuario ni tasa cero.

El primer encuentro de CADA cookie establece su baseline, sin inferir tasa desde
el inicio de captura o desde una muestra vacia. Una segunda observacion limpia
puede producir cuatro tasas TCPsend/recv UDPsend/recv, incluso cero medido. Tasas
son estimated: delta payload dividido entre finales nominales del barrido, no
wire throughput ni exactitud temporal por evento. Bucket nuevo no invalida los
anteriores; suma UID inicial sigue parcial e informa buckets conocidos/faltantes.
Una fila con solo recepciones excluidas puede tener payload0 conocido; no equivale
a ausencia. Contadores de exclusiones no se convierten en payload.

Error, soporte estimated del origen, bracket invalido, cambio de contexto durante
lectura o excepcion descarta TODOS los baselines. Cambio de contexto entre llamadas
toma nueva baseline. Intervalo mayor60s no genera tasas; discard_window tambien
descarta tasas sin destruir captura. Stop cierra sondas/mapa/pidfd, borra registro y
estado; reiniciar requiere consentimiento y genera epoch nuevo/mapa vacio.

Ademas de las garantias sticky del motor, se verifica que secuencia y acumulados
no retrocedan y que slots observados no desaparezcan/cambien de cookie. Se conserva
el ultimo registro validado aun al descartar baselines: no se acepta encubrir una
regresion como primera muestra tras un error. Metodos clasicos/observados rechazan
el modo equivocado sin convertir resultados ni cambiar silenciosamente modo.

## Verificacion y limites

Fixtures linux-network-selected-sampler: consentimiento/argumentos sin llamadas,
ocho fallos de inicio/excepciones y limpieza, vacio->cookieA->A+B, cuatro tasas y
suma UID parcial, tasa nueva ausente,16cookies/actividad solo excluida, scope vacio
valido y rechazo multiactor/17filas. Veintiun fallos de lectura cubren denegacion,
calidad estimada, deriva entre joins, UID/boot, flags globales/writers, reloj,
excepciones, secuencia/slot/bytes/exclusiones/registro regresivos y prefijo corrupto;
recuperacion conserva contadores pero no cruza ventanas invalidas. Contexto entre
llamadas, discard, intervalo largo, stop/reinicio/cambio de modo y fork simulados.

Manual --authorized-private-observed-window, fuera de CTest, usa dos hijos privados
propios y captura solo uno. Cierra pidfds originales tras setup; vacio, despues
primer cookie sin tasas; trafico adicional de dos hilos concurrentes del mismo
actor da TCP12288/12288 UDP768/768 y24exclusiones acumuladas. Cuatro tasas y suma UID
comparadas con oracle entero; otro hijo/padre excluidos, fork rechazado sin afectar
padre, discard sin tasa, restart vacio/epoch nuevo y proceso terminado rechazado
dos veces. Sin fugaFD. Watchdogs parent60s/child50s; no plazo duro de carga BPF.

SOLO UN namespace real. El hijo inicializa SO_NETNS_COOKIE para oracle independiente,
NO entrega cookie al start. No demuestra cookies sin inicializar, multiples netns
reales, otros kernels/drivers, cobertura completa de todos caminos de socket ni
servicios arbitrarios. Las16cookies son fixtures, no16namespaces fisicos.

Regresion nativa --authorized-private-selected-window positiva; modo estricto no
cambia. Linux126/126 Release sin skips, dos tests20repeticiones, consumidor instalado
reconstruido/enlazado/ejecutado exit0. MSVC2/2 focalizadas de codigo compartido;
no nueva implementacion Windows, ni suite/consumidor Windows generales validados.
Fallo consumidor general revision75 sigue N03. Logs work/network-discovered-window-*.
Sin layouts/API publicos, GUI ni persistencia nuevos. F03/H04/J04 abiertas.

Siguiente: integrar estos registros dinamicos en la coleccion seleccionada
multiactor con bracket comun, joins y reset grupal, antes de API/Monitor/Sampler
observados. No ampliar captura a procesos no autorizados.
