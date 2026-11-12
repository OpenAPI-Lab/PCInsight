# Cookies observados sin lista manual: motor interno

Continuacion revision81: [ventanas seleccionadas internas](LINUX_NETWORK_DISCOVERED_WINDOWS.md)
ya poseen pidfd y verifican join/contexto en ambos extremos con tasas por cookie.
El contrato del motor descrito aqui sigue siendo token-only; no usarlo directamente
como seleccion publica. La descripcion siguiente conserva evidencia de revision80.

Revision80 agrega start_discovering_selected/read_discovered a la sesion PRIVADA
LinuxPrivateNetworkSession. Recibe solo token kernel del actor seleccionado, no
cookie ni inventario de sockets. NO es aun API publica/proveedor de tasas/join
procfs: el token tiene que venir de una fuente autenticada del nivel superior.
Las APIs publicas de revisions78-79 siguen usando cookies declarados. F03 abierta.

## Registro y bytecode

Se reutilizan BTF vivo, filtros TGID/UID inicial/inicio del lider y los hooks
inet{,6}_sendmsg/sock_recvmsg. Se lee el namespace del SOCKET, no el actual del
proceso. Cada cookie NO CERO observado en una operacion positiva instrumentada
se registra por primera vez en una de16ranuras. Cada ranura almacena cookie,
cuatro acumulados payload TCPsend/recv UDPsend/recv y exclusiones de recepcion.
Recepciones positivas excluidas tambien pueden registrar cookie, sin payload.
No descubrimiento de sockets inactivos ni de procesos ajenos. No wire bytes,
paquetes, contenido, direcciones, interfaz ni cobertura de todos caminos I/O.

Mapa ARRAY105 celdas u64: prefijo9 con fallos/overflow/writers/secuencia en4/6/7/8
y16ranuras de6celdas. Cookie se reclama con CMPXCHG64: compara0, reclama si libre,
o usa el valor anterior para elegir esa ranura si otro hilo reclamo el mismo
cookie. Si era otro cookie continua al siguiente slot. Escaneo acotado/desenrollado,
sin bucle no acotado, sin eviccion/borrado/reutilizacion mientras vive captura.
La semantica de R0/compare-and-exchange se comprobo en la
[ISA oficial BPF](https://kernel.org/doc/html/latest/bpf/standardization/instruction-set.html#atomic-operations).
Se requiere que kernel/JIT/verificador soporten las instrucciones usadas; fallo
de carga deja ausencia motivada, no fallback a una captura sin filtro.

Un cookie que ya existe sigue actualizando su ranura aunque16esten ocupadas.
Un17o distinto marca fallo GLOBAL sticky: no entrega16filas como si no hubiera
perdida ni recicla una para esconderla. Cookie0/null/lectura/identidad/fallo de
actualizacion y overflow tambien fallan cerrado. No genera cookies ni inventa
valores: net.net_cookie debe estar inicializado y validado por BTF. Este motor
no fuerza su inicializacion en otro namespace; cero queda indisponible.

## Lectura y coherencia

Guard global de escritores/secuencia engloba reclamacion, acumulado, exclusiones,
errores y overflow. Bytes/exclusiones/secuencia tienen suma atomica con overflow
sticky. Cookie inmutable tras reclamar. Lectura obtiene writer/seq antes, mapa
completo, seq/writer despues; hasta8reintentos ante solapamiento. Rechaza fallo
global/overflow/cambio de seq/writer/prefijo incompatible, ranura sin cookie con
contadores, huecos/duplicados, cookie sin actividad completada y mas cookies
que updates. No mezcla dos barridos para completar una muestra parcial.

Antes de actividad devuelve lista VACIA conocida, no cookie0 ni filas ficticias.
Una ranura o contador0 no representa inventario ni total global. read clasico
rechaza este modo; read_discovered rechaza el otro. detach congela mapas y sigue
permitiendo lectura etiquetada detached; stop cierra todo y borra modo, reiniciar
crea otro mapa vacio. No reset in-place de flags para esconder una perdida.

Este nivel privado no posee pidfd ni revalida procfs/liveness/observer por lectura;
no es seleccion publica de PIDs ni contiene guard de fork del recolector. No
usar token arbitrario ni publicar estos datos como apps/usuarios actuales. Faltan
unir al nivel seleccionado con pidfd propio/contexto por extremos, ventanas/
tasas/epoch/reset y multiactor, despues API/Monitor/Sampler y politica general.
No graficas/historial persistido ni totales legacy cambiados en esta revision.

## Evidencia

Mismos tests linux-network-classified/session:16cookies, retorno a conocido,
17o falla sticky, cookie64 bits altos, TCP/UDP IPv4/IPv6 send/recv, exclusiones,
simulacion de peer reclamando igual/distinto cookie justo antes de CMPXCHG,
sin duplicado ni payload doble, errores/UID/start/zero/null/overflow, registry
corrupto/coherencia, denegacion/estimated/rollback de8adquisiciones-excepciones,
detach/stop/reinicio/modos separados. Checks activos Release.

Manual NO CTest --authorized-private-observed-cookies: dos hijos privados propios,
captura de UNO. Token kernel obtenido por join pidfd/procfs antes de iniciar;
NO cookie entregado al productor. Hijo inicializa el cookie via SO_NETNS_COOKIE
solo como oracle independiente. Vacio antes de actividad; cookie observado
coincide con oracle, TCP12288/12288 UDP768/768 peek24 incluyendo dos threads
concurrentes del mismo actor, otro hijo/padre excluidos. Detach congela, stop sin
fugaFD, restart vacio luego TCP8192/8192 UDP512/512 peek16 concurrente, ambos hijos
terminan normalmente. SOLO UN namespace real;16cookies/capacidad/race distinto
son fixtures, no prueba2netns reales o cookie no inicializado. Watchdogs
parent60s/child50s, no plazo duro BPF ni elevacion automatica.

work/network-discovery-native-linux.log conserva positivo y declared-native-linux
regresion positiva del Monitor de buckets declarados. Primer build fallo por
parentesis de macro assert en fixture (coma de template); corregido sin relajar
productor/log preservado. Inicializacion de stack explicita en caminos de fallo
para verificador. Sin nueva dependencia/CLI/SDK/setns/unshare/captura global.

Resultado: Linux126/126 Release sin skips,2tests focalizados20repeticiones cada
uno, build sin warnings/consumidor instalado reconstruido-enlazado-ejecutado
exit0. MSVC biblioteca y2/2 focalizadas pasan/sin warnings C++ nuevos (D9025
esperado asserts activos). No cambios API/layouts publicos, ni suite/consumidor
Windows completos nuevos; fallo consumidor75 sigue N03. No certificacion universal
de hardware/kernel/distros. F03/H04/J04 abiertas; control revision80 mantiene
57/75=76,0% cerrado y18/75=24,0% abierto, sin nuevos IDs ni credito parcial.
