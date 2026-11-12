# Cookies observados: ventana comun interna multiactor

Revision88: [consulta con compuerta](LINUX_NETWORK_QUERY_SESSION.md) PASA nativo17
completo/tasas/limpieza. Sin productor compartido ni64actores reales certificados.

Revision87 (historica): [capacidad estructural64 y coste real](LINUX_NETWORK_CAPACITY64.md).
El limite16/256 de procesos/filas citado debajo es historico; el limite16 cookies
por actor NO cambia. Nativo17 falla por coste/vida acotada; NO captura escalable
certificada. Se conserva la evidencia original de cada revision.

Continuacion revision83: [proveedor publico observado](LINUX_PROCESS_NETWORK_OBSERVED.md)
ya proyecta esta fuente con metadata de actores vacios/tasas/calidad. Monitor/
Sampler observado sigue pendiente. Se conserva debajo contrato/evidencia revision82.

Revision82 extiende LinuxMultiNetworkSampler con una politica interna fija
observed_cookies (quinto argumento del constructor). No cambia API/layout publico,
LinuxProcessNetworkSampler, Monitor/Sampler ni formatos de historial. F03 sigue
abierta: seleccion EXPLICITA de procesos no es descubrimiento general de apps.

## Seleccion y consentimiento

Modo observado exige1..16 filas con pidfd/PID/start validos y socket_cookie=0.
Rechaza cookies no cero, procesos duplicados y combinacion con namespace_buckets
declarado; no ignora silenciosamente cookies ni cambia politica al detener/reiniciar.
start y CADA update_selection requieren consentimiento explicito. Los modos
anteriores conservan cookies declarados/default estricto. sample independiente no
esta disponible en observado: usar sample_common.

Cada actor conserva su pidfd propio y registro BPF de hasta16cookies observados,
con filtros de identidad kernel y calidad sticky existentes. El llamador puede
cerrar sus pidfds. No se enumeran otros procesos/sockets, no se ejecutan utilidades,
no elevacion automatica, paquetes nuevos, captura global o configuracion permanente.
Mismo contrato de serializacion/contextos/tablaFD que los samplers internos previos.

Updates retienen capturas de PID/start coincidentes, cierran retiradas ANTES de
crear nuevas y revalidan todas las asociaciones retenidas/agregadas. Retenido
conserva origen de contadores y registro; re-agregado empieza vacio, sin trafico
del gap. Cada update exitoso cambia epoch y descarta TODOS los baselines/brackets,
incluso al reordenar o repetir seleccion. Error de forma/consentimiento no muta;
fallo runtime detiene todo, no deja una seleccion parcial ni reautoriza al actualizar.

## Barrido y resultado

Join pidfd/procfs de TODOS antes del barrido; exige boot/observer coherentes y
actores kernel distintos sin alias ambiguos. Barre registros completos y validos,
despues join de TODOS y verifica contextos por extremos/guard del proceso creador.
Un solo bracket de contadores, disponible/ordenado/no solapado y hasta1s. NO snapshot
atomico: filas se leen en instantes distintos, con finales nominales comunes.

Coverage interna selected_observed_actor_set permite0..256filas, hasta16actores y
16cookies por actor; no relaja limites de scopes antiguos. UID inicial consistente
por identidad, cookies/filas unicas; reducer rechaza limites/identidades invalidas.
Un actor sin actividad aun conserva entrada en observed con identidad/contexto,
lista vacia y secuencia0. No inventa cookie0, fila de trafico, usuario ni tasa cero.

PrivateCommonNetworkWindow.observed contiene UN registro por proceso seleccionado,
en orden de seleccion, con identidad kernel/contexto local, buckets y secuencia
completed_updates del ACTOR completo. No sumar esa secuencia por cookie. El vector
associations antiguo queda vacio en este modo; modos declarados hacen lo contrario.
Accounting contiene filas ordenadas por clave actor/cookie y sumas UID parciales:
buscar por clave, nunca por posicion entre vectores. Exclusiones permanecen por
bucket, no son payload. El mismo cookie de dos actores NO fusiona sus contadores.

Una nueva fila necesita dos muestras limpias propias antes de dar tasa, aunque
otros actores ya tengan baseline. El registro vacio no es una baseline de cero.
Tasas TCP/UDP send/receive son estimated: delta de payload de operaciones positivas
instrumentadas dividido por intervalo entre finales nominales del grupo. No wire
bytes, interfaz, ACKs, red total de proceso ni propietario exclusivo del socket.
Sumas UID iniciales informan buckets conocidos/faltantes/overflow, no total usuario.

Cualquier fallo de actor/lookup/calidad/join/reloj/contexto/fork/exception descarta
TODOS los baselines y previous_read_bracket, sin devolver un grupo parcial. Registros
de ultima muestra validada se conservan como guard de monotonicidad, aun tras error.
Los nuevos registros solo se confirman despues de TODOS los reads/joins/reduccion,
sin commits de captura del primer actor cuando el ultimo falla. Recuperacion toma
nueva baseline. Stop cierra todos los recursos; fork cierra solo copia heredada.

## Evidencia

linux-network-multi: consentimiento/forma/cookies/modos contradictorios, vacio con
dos identidades, llegada escalonada de buckets A y B, mismo cookie en actores
distintos y cookie adicional, cuatro tasas/suma UID conocida-faltante, modo
independiente rechazado, update/reorden sin recrear capturas, retiro/readd sin
registro previo, epoch/reset, stop/restart y fork simulado. Veinte fallos de lectura
incluyen segundo actor fallando despues del primero, joins/deriva/UID/boot-contexto,
lookup/exception/estimated, writer/flags/overflow, reloj e invariantes de registro/
contadores; recuperacion sin intervalo oculto. Cinco fallos de inicio y fallo de
adicion runtime limpian todo.32buckets de dos actores/UIDs y payload0 por actividad
solo excluida; fixture del reducer acepta256filas(16x16) y rechaza17actores,
17cookies por actor y UID inconsistente. No256buckets reales medidos.

Manual --authorized-private-observed-multi, fuera de CTest: dos hijos privados
propios autorizados, pidfds originales cerrados, SIN cookies de entrada. Vacio,
primer actor/cookie sin tasa, segundo tardio sin tasa mientras primero mide0.
Luego dos hilos concurrentes por actor: primero TCP12288/12288 UDP768/768 peek24
acumulados, segundo el doble. Cuatro tasas por actor y suma UID contra oracle
entero/intervalo comun. Trafico padre excluido; fork rechazado. Retiro/readd
conserva contadores retenidos/borra registro del nuevo y excluye gap; nuevos epochs
y TODOS baselines limpios. Salida de un hijo rechaza grupo dos veces, retirarlo
recupera sin cruzar baseline. Ambos terminan normalmente, sin fugaFD.

SOLO UN namespace real compartido; SO_NETNS_COOKIE inicializa cookies de los hijos
para oracle independiente. No prueba multiples netns reales, cookie sin inicializar,
16procesos reales ni cobertura de todos caminos/kernel/distros. Watchdogs parent60s
y child50s, no plazo duro de carga BPF. Ninguna captura de procesos no seleccionados.

Regresiones nativas common-window estricto y observed-window individual positivas.
Linux126/126 Release sin skips,3tests20repeticiones cada uno, consumidor instalado
reconstruido/enlazado/ejecutado exit0, build sin warnings. MSVC3/3 focalizadas de
codigo compartido, sin nuevos C++ warnings (D9025 esperado asserts activos); no
funcionalidad Windows nueva ni suite/consumidor Windows general validado. Fallo
general revision75 sigue N03. Logs work/network-observed-multi-*.

Siguiente: exponer politica observada opt-in en proveedor publico seleccionado,
con identidades/registros vacios/limites/calidad/brackets bien representados,
despues Monitor/Sampler; aun falta politica de seleccion general. F03/H04/J04
siguen abiertas; sin persistencia/GUI nuevos ni credito parcial de progreso.
