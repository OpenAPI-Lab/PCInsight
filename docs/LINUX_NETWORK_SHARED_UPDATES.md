# Cambios autenticados de seleccion compartida Linux

Continuacion revision93: [backend publico observado](LINUX_NETWORK_SHARED_PUBLIC.md)
ya activa esta ruta en LinuxProcessNetworkSampler/Monitor/Sampler/owner, con
pruebas nativas especificas. El pendiente de activacion92 debajo es historico;
admision general e integraciones de historia/app/sesion siguen pendientes.

Revision92, 2026-10-22. Unidad interna F03 terminada:
`LinuxSharedSelectedNetworkSampler::update_selection`. No sustituye aun al
backend PUBLICO, Monitor o Sampler. No hay admision automatica. F03/H04/J04
siguen abiertas hasta completar sus criterios, no se asigna credito parcial.

## Contrato de la transicion

Cada update requiere consentimiento nuevo y1..64 PID/start/pidfd unicos, cookie0.
Errores de consentimiento/forma/capacidad dejan la seleccion y tasas intactas.
Vacio se rechaza (usar stop). Se retienen las copias de pidfd de las filas con
PID/start coincidentes; los nuevos reciben copias propias. Igual que el update
publico anterior, el descriptor propuesto para un retenido no sustituye al que
ya posee el sampler. No se almacenan permisos como resultados vigentes.

Todos los actores propuestos se autentican ANTES y DESPUES del cambio mediante
joins actuales; cada join nativo contiene dos consultas kernel independientes.
Tokens kernel, boot y observador han de ser consistentes y unicos. Los retenidos
deben seguir coincidiendo con su identidad original. Contexto cambiado entre
los extremos, permiso denegado, actor muerto o excepcion en runtime cierran
TODA la sesion, incluida la query y pidfds; no se mantiene una seleccion parcial.
Los pidfds nuevos tienen guardia RAII independiente hasta el commit.

Cada exito, incluso solo reordenar, recibe epoch nuevo y descarta TODAS las
baselines de tasas, contextos y bracket previo. Los bytes/cookies/secuencias de
los actores retenidos conservan su origen de captura. Reincorporar un actor
retirado NO conserva su origen anterior y nunca incluye el intervalo ausente.

## Batches disjuntos, sin slots reciclados

`LinuxSharedNetworkSet` compone productores compartidos por lote de admision.
Empieza con un solo lote. Antes de dar de alta reemplazos, retira del HASH de
cada lote las filas que salen; los registros retirados nunca se borran/reusan.
Despues inicia un NUEVO productor compartido para todas las altas de ese update.
Un lote sin actores activos se cierra por completo. Los retenidos nunca se
desconectan, reinician ni migran a otro mapa.

Esto evita reasignar un slot al que una invocacion antigua todavia podria
escribir. Una invocacion ya admitida solo termina en el registro retirado del
mapa anterior, que no se publica como actor activo. Las nuevas invocaciones
del actor retirado salen antes de leer contexto/contadores. No se promete
revocar eventos que ya estaban en curso. Ver LINUX_NETWORK_SHARED_RETENTION.md.

Coste con N actores activos y G lotes no vacios: N pidfds +6 FDs de query
reutilizable +8G FDs de trafico (2mapas/3programas/3links por lote).
1<=G<=N<=64. Inicio: N+14. Churn puede fragmentar hasta64 lotes (limite582FDs
persistentes al terminar la operacion en este modo nativo); no se promete tres sondas constantes despues de
updates ni una mejora universal frente al fanout anterior. Hay como maximo64
registros originales por lote; memoria/links acotados por lotes vivos, NO por
el numero de updates de toda la vida. No hay presupuesto de64 altas vitalicias.
Durante la preparacion del update pueden coexistir hasta64 copias NUEVAS de
pidfd con las anteriores (hasta646FDs persistentes/provisionales propios antes
del commit, ademas de FDs transitorios de las consultas/BTF). El censo nativo
publicado es despues de cada operacion, no un pico medido dentro del syscall.

## Lectura y calidad

Cada lote mantiene su bracket writer/secuencia y antirregresion anteriores,
incluidos registros retirados para cuadrar su secuencia global. La salida se
reordena por las identidades propuestas, no por indices reciclados.
El barrido de TODOS los lotes tiene orden temporal comprobado y maximo1s.
No es un corte atomico entre lotes: los contadores de un lote pueden cambiar
mientras se lee otro. Tasas nominales y sumas UID permanecen estimated/parciales,
no throughput global, paquetes de enlace, totales de app/usuario ni socket-owner.

Fallo de cualquier lote, overflow de suma de secuencias, contexto/clock/joins o
forma rechaza TODO el frame y descarta todas las baselines del sampler. Los
checkpoints raw ya leidos permanecen antirregresion; no se borran para recuperar
una calidad sticky. El siguiente frame valido vuelve a calentar las tasas.

## Evidencia de esta unidad

- Fixtures de retencion/reordenacion/alta/reincorporacion: bytes conservados,
  gap de1000bytes ausente, nuevos buckets sin tasa inicial, epoch nuevo y reset
  de todos los intervalos. Copias propias y cierre de pidfds retirados.
-80updates alternados conservando un actor:2 lotes vivos/16FDs de trafico,
  contador retenido crece y cada reincorporacion empieza fresca.
- Peor fragmentacion64lotes/64actores/1024buckets: tasas por cuatro lanes,
  capacidad65 rechazada sin mutacion; reduccion a1lote conserva bytes y libera63.
- Fallo y excepcion inyectados en CADA operacion del update, incluido borrado,
  nueva carga/attach y ultimo join: todos los recursos cerrados. UID/kernel
  duplicado/boot/observador/fork/calidad incorrectos rechazados.
- Calidad perdida en un lote rechaza todo/reset tasas. Barridos locales de
  menos1s cuyo conjunto excede1s tambien se rechazan.
- Nativo privado17 inicial y reinicio16:16->15->16 con el actor retirado aun
  enviando. Conservados mantienen bytes TCP/UDP, reincorporado sin bucket viejo
  ni bytes del gap. Nuevo lote añade exactamente8FDs. Ventanas TCP/UDP IPv4/IPv6,
  cuatro tasas/intervalo nominal y suma UID por oracle entero; reordenar no
  reconstruye productores pero resetea tasas/epoch. Owner original liberado,
  copia fork rechazada, actor muerto rechaza todo, hijos/FDs finales limpios.

Logs `work/network-shared-update-*`: Linux Release130/130 sin skips y40ejecuciones
focales, consumidor instalado exit0. MSVC40ejecuciones focales, no certificacion
Windows total ni correccion N03. Build final sin nuevos warnings C++, D9025
esperado. Espejo92 confirmado: comentario90140272299452; TXT siempre manda.
Prueba nativa: solo1namespace real/17hijos privados;64/1024 son fixtures, no
certificacion fisica universal. Sin CLI de captura/dependencias/instalaciones,
configuracion permanente, historial, GUI o layout publico nuevos.

Siguiente paso: activar y verificar este backend en el proveedor observado
publico y sus rutas Monitor/Sampler/owner; no declarar esa integracion hecha
porque la proyeccion privada ya acepta frames internos.
