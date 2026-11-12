# Capacidad estructural y coste real de captura Linux

Revision97: [selecciones mayores de64](LINUX_NETWORK_SCALE.md) amplia la
capacidad estructural a1024 procesos/16384buckets usando lotes RAW de hasta64.
Los limites64/1024 y los costes de fanout citados debajo son historicos.

Revision93: [backend PUBLICO compartido observado](LINUX_NETWORK_SHARED_PUBLIC.md)
ya supera nativo17 con censo N+14FDs, tasas/UID/limpieza. Monitor/Sampler/owner
y cambios de seleccion tambien positivos;64/1024 siguen fixtures. Los fallos
de coste/fanout y pendientes anteriores debajo son evidencia historica, no la
implementacion actual del modo observado. Estricto/declarado conservan fanout.

Actualizacion revision88: [consulta de sesion con compuerta](LINUX_NETWORK_QUERY_SESSION.md)
PASA nativo17 completo con tasas/UID/limpieza y fases27..29ms en ese ensayo.
Los fallos y el siguiente paso citados debajo corresponden a revision87 y se
preservan como historia;64/1024 siguen siendo fixtures, no cobertura real64.

Revision87, 2026-10-22. F03 sigue EN CURSO: ampliar un limite no termina la
captura general ni demuestra que sea apropiada para refrescar un GUI.

## Contrato actual

`LinuxProcessNetworkSampler::max_processes` pasa de16 a64. Estricto:64 procesos;
declarado:64 filas PID/start/cookie (no64 cookies por proceso); observado:64
procesos distintos, hasta16 cookies por proceso,1024 buckets en total. El owner
y el puente de inventario usan el mismo limite;65 se rechaza, nunca se trunca,
rota ni filtra silenciosamente. El registro individual sigue rechazando una
cookie17; no se ha confundido el limite de procesos con el de namespaces.

Es una ampliacion ESTRUCTURAL del fanout existente: una captura por actor,
no un productor BPF compartido. Todos los controles previos de calidad,
identidad, reset grupal, nuevo bucket sin tasa y suma UID parcial se conservan.
Recompilar biblioteca y consumidores: cambia una constante publica constexpr.
Sin cambio de layout publico existente, GUI, persistencia, dependencias nuevas,
comandos externos para obtener metricas ni configuracion permanente.

## BTF por sesion, no identidades en cache

El sampler seleccionado reutiliza su esquema BTF durante la sesion, tambien
para las dos consultas independientes de identidad de cada join. Antes cada
consulta volvia a leer y analizar el BTF completo del kernel. Stop elimina el
esquema guardado y start obtiene otro. No se guardan resultados de identidad,
permisos o vida del proceso: pidfd, procfs, contexto y consultas BPF actuales se
siguen comprobando antes y despues de leer los contadores. Las fuentes de
prueba antiguas sin callback layout-aware mantienen su ruta anterior.

Esto NO elimina el coste de crear/cargar/adjuntar las consultas BPF de identidad
en cada comprobacion; aun no existe una consulta persistente compartida.

## Evidencia y fallo que NO se convierte en aprobacion

Fixtures:64 actores positivos,64x16=1024 buckets proyectados,65 rechazados,
cookie17 rechazada, UID y cuatro tasas/intervalo comun, reorden/retencion/
retiro-readd sin gap, fallo del ultimo actor y rollback de todos. Callback BTF
por sesion probado con checks de identidad por extremos, permiso, excepcion,
stop/reinicio y fallo del nuevo esquema.

Manual `pcinsight-linux-network-capacity-native --authorized-private-capacity17`
(fuera de CTest):17 hijos privados, sockets UDP loopback64..80B, SO_NETNS_COOKIE
como oracle independiente, watchdog55s hijos/60s padre. Solo actores privados
autorizados; no captura de otros procesos. Dos intentos, logs preservados:

- `work/network-capacity64-native-linux.log`: FALLA con pidfd salido durante query.
- `work/network-capacity64-native2-linux.log`, tras reutilizar BTF: preparado19ms,
  start11212ms, registro vacio35072ms, primera muestra53607ms. Los17 primeros
  contadores y ausencia inicial de tasas pasaron sus aserciones. La segunda
  muestra falla por pidfd salido durante query, al alcanzar la vida acotada de
  los hijos. NO se verificaron tasas nativas de17, suma UID ni limpieza normal
  final del harness. Los destructores cierran captura/FDs y retiran/recolectan
  los hijos al salir por excepcion. No hubo ampliacion de watchdog ni tercer intento.

Las lecturas entre fases tardaron aproximadamente24s y19s en ESTE entorno;
no es benchmark portable ni prueba causal de cuanto tarda cada syscall. Se
demuestra un problema real de coste del fanout/consultas repetidas.64 actores y
1024 buckets son positivos de fixtures, NO captura nativa certificada ni
rendimiento util del GUI. Solo un namespace real compartido en este ensayo.

Regresion Release Linux128/128 sin skips, seis tests focales20 veces cada uno,
consumidor externo instalado reconstruido/ejecutado exit0. MSVC seis focales
6/6; no validacion Windows completa (fallo consumidor revision75 sigue N03).
Logs `work/network-capacity64-final-*`; builds finales sin warnings C++ nuevos.

## Proximo paso concreto

Reducir las cargas/adjuntos BPF repetidos mediante una consulta de identidad
propiedad de sesion, manteniendo el filtro por hilo llamante/target, aislamiento,
permiso real actual, liveness y ambos extremos de cada join. Despues repetir una
prueba privada acotada; solo entonces evaluar un productor multiactor compartido
y admision/retiro automaticos con cobertura explicita. NO aumentar mas limites
ni temporizadores como sustituto de solucionar el coste. F03/H04/J04 abiertas;
el porcentaje oficial sigue57/75=76,0%,18/75=24,0% abiertas.
