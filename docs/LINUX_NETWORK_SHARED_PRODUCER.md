# Productor de red compartido: seleccion fija, contadores internos

Revision93: [backend publico observado](LINUX_NETWORK_SHARED_PUBLIC.md) ya
activa captura/updates compartidos en standalone/Monitor/Sampler/owner.
Pendientes de activacion89..92 siguientes conservados solo como historia.

Revision92: [updates autenticados por lotes](LINUX_NETWORK_SHARED_UPDATES.md)
ya conservan retenidos y admiten/reincorporan con captura fresca. Activacion
publica pendiente. Los pendientes de revisiones89..91 debajo son historicos.

Revision91: [retirada segura](LINUX_NETWORK_SHARED_RETENTION.md) ya admite
subconjuntos sin reiniciar retenidos ni reciclar slots. Altas/reincorporaciones
y transicion autenticada completa siguen pendientes; descripcion89 debajo
es historica (HASH ahora admite borrados, nunca reasignaciones).

Continuacion revision90: [ventanas autenticadas compartidas](LINUX_NETWORK_SHARED_WINDOWS.md)
ya integran pidfds propios, joins por extremos, tasas/UID y proyeccion validada
en una seleccion FIJA interna. Siguen pendientes updates con origen retenido
y activacion del backend publico. El estado revision89 se conserva debajo.

Revision89, 2026-10-22. Unidad interna F03 terminada. LinuxSharedNetworkSession
NO es todavia el backend de LinuxProcessNetworkSampler/Monitor/Sampler: faltan
sus joins continuos por pidfd, ventanas, tasas y cambios de seleccion reteniendo
origen de contadores. F03/H04/J04 siguen abiertas.

## Un conjunto explicito, tres sondas de trafico

Start requiere consentimiento y1..64 tokens kernel distintos (TGID/UID real/
leader start_boottime). Son tokens declarados, NO una autenticacion publica de
PID/procfs ni garantia de vida futura. No enumera procesos ni admite otros
automaticamente. Se mantienen exactamente tres programas/links de trafico:
inet_sendmsg, inet6_sendmsg y sock_recvmsg. No hay un programa/registro por CPU
ni un grupo de sondas por cada proceso.

Los TGID no seleccionados salen por lookup HASH ausente, ANTES de leer contexto
de red o contadores. Los seleccionados comprueban inicio/UID actuales antes de
los bytes. El namespace es el del socket, no el namespace actual de la tarea.
Cada actor conserva hasta16 cookies observadas con CAS, cuatro lanes TCP/UDP
send/receive y exclusiones PEEK/TRUNC/ERRQUEUE. No se inspecciona contenido.
No se evictan cookies: lleno, identidad incorrecta, lectura fallida o overflow
dejan calidad sticky y la lectura completa se rechaza, sin parcial ni falso0.

| Mapa propio | Disposicion |
|---|---|
| HASH u32->u64 | TGID kernel -> slot+1; inmutable durante la sesion, capacidad del conjunto exacto |
| ARRAY u32->u64 | Prefijo9 global y N registros: UID/inicio + ARRAY105 anterior por actor |

El HASH no es LRU: no usa desalojo automatico. El tipo y su limite max_entries
estan documentados por el [kernel Linux](https://docs.kernel.org/bpf/map_hash.html).
Se crean y actualizan por syscall BPF directo, sin libbpf, SDK, herramientas
auxiliares ni procesos de recoleccion. El ARRAY maximo es6857 valores u64
(54856 bytes de valores; no incluye overhead del kernel ni el HASH).

## Inicio, coherencia y recursos

La captura permanece deshabilitada hasta inicializar ambas tablas y adjuntar
las tres sondas. Fallo en cualquier paso retira todos los recursos propios.
La seleccion es fija: reemplazarla exige stop/start, NO conserva contadores.
Todavia no sustituye update_selection del proveedor publico.

Un writer/sequence GLOBAL encierra todas las actualizaciones de actor/cookie,
ademas de las guardas individuales. Errores del indice tambien usan la guarda
global y un flag sticky. Read barre todo el conjunto y revalida tabla, metadatos,
registros y suma de secuencias de actores contra la secuencia global. Reintenta
hasta8 veces si hay writer/sequence concurrentes; nunca entrega medio conjunto.
El reloj debe ser positivo/monotonic y el bracket no exceder1s; no es un deadline
duro para syscalls. Ultima lectura valida preservada para rechazar regresiones
de secuencias, bytes, exclusiones, desaparicion o sustitucion de slots/cookies.
Un fallo no borra esa referencia ni permite disimular una regresion posterior.

Start/read requieren la misma tabla de FDs, llamadas serializadas y contexto de
source vivo. Fork rechaza read y cierra solo referencias heredadas, sin escribir
mapas del padre. Stop/destructor cierra links, programas y mapas; sin pinning,
configuracion permanente, privilegios adquiridos automaticamente ni daemon.
Fuera de Linux se conserva ausencia motivada; las fixtures siguen ejecutables.

## Evidencia

- tests/linux_network_shared.cpp: interprete del subconjunto de bytecode,
  64 actores/16cookies cada uno (1024 buckets), aislamiento por actor/UID/inicio,
  IPv4/IPv6/TCP/UDP/cuatro lanes, no seleccionado sin contexto/contadores,
  gate inicial, tabla invalida, errores de lectura/identidad/overflow y cookie17
  del ultimo actor rechaza TODO. Exactamente3 cargas/links para64 actores.
- Inicio: denegaciones/excepciones en cada operacion con rollback completo;
  lectura: calidad/cambios metadata/tabla/clock/writer/errores/fork/reservados,
  regresion de bytes/cookies/sequence/registro y recuperacion sin lavar referencia.
- WSL manual privado --authorized-private-shared-capacity17 en el ejecutable
  pcinsight-linux-network-capacity-native (fuera de CTest):17 hijos obtenidos por
  owner, tokens asociados por join pidfd/procfs ANTES del start. Captura posee
  exactamente8 FDs (2mapas/3programas/3links) despues de liberar owner/query.
  UDP64..80B IPv4 dos veces/IPv6 una, TCP IPv4/IPv6 cuatro veces64..80B por
  direccion, oracles de cuatro lanes/cookie/secuencias por actor/globales;
  registros vacios, trafico TCP/UDP padre excluido, fork rechazado/padre intacto,
  stop/restart vacio/fresco, todos hijos salida normal, censo FD limpio.
  Preparado19ms, start532ms, medido548ms, stop tras restart1765ms desde inicio;
  fases incluyen IPC/trafico/joins, NO benchmark ni garantia universal.
  SOLO1namespace real compartido, SO_NETNS_COOKIE inicializado por oracle.
  64/1024 son fixtures, no64actores reales certificados.
- Regresion nativa proveedor PUBLICO anterior17actores: PASA cuatro tasas/UID
  parcial y limpieza; NO se conecta al productor compartido en esta revision.
- Linux129/129 Release sin skips, tres tests20veces=60 ejecuciones focales,
  consumidor externo instalado reconstruido/ejecutado exit0. MSVC3/3 focales,
  sin warnings C++ nuevos/D9025 esperado; N03 consumidor Windows75 sigue abierto.
  Logs work/network-shared-*. Native y regresion privados, sin capturar otros PIDs.

## Pendiente inmediato

Integrar este productor con una coleccion de selecciones autenticadas: pidfds
propios, joins antes/despues, calidad, baselines y cuatro tasas/UID parciales,
sin perder el contrato de update_selection/origen retenido. Despues proveedor
publico/Monitor/Sampler y admision/retiro automaticos con cobertura explicita.
No publicar estos contadores como red total de ProcessInfo/UserSession/apps.
No cubre todo I/O asincrono, actores kernel, bytes de enlace, retransmisiones,
todos los kernels/controladores ni multiples namespaces reales validados.

Sin cambio de API/layout publico/GUI/persistencia/dependencias/CLI nuevos.
Control oficial sin credito parcial:57/75=76,0% cerrado,18/75=24,0% abierto.
