# Progreso de PCInsight

Este documento conserva historia de desarrollo; sus porcentajes y checkpoints
no son el estado vigente de esta entrega. Consulta [el alcance actual](SUPPORT_STATUS.md).

Registro interno de referencia: SEGUIMIENTO.txt (no distribuido aquí), revisión99,
auditada el2026-10-15 contra los requisitos originales, código/API, documentación
y registros de pruebas. ClickUp es únicamente un espejo secundario.

Checkpoint99: [LinuxHistoryStore](LINUX_HISTORY_STORE.md) terminado, H04 P->E.
Formato separado/value-Support-notas, cero medido distinto de ausencia, fusion
cumulativa reemplaza/no suma, periodos unicos normalizados ms, rechazo descenso/
perdida conocida/corrupcion/bounds/formatos ajenos. Retencion por periodo entero,
reset unlink sin directorios, no I/O automatico ni captura nueva eBPF.
Linux134/134+30focales/consumidor instalado exit0; CPU REAL roundtrip mantuvo red/
notificaciones ausentes. MSVC20focales fuera sandbox pasan; dentro ambos almacenes
fallaban reemplazo AccessDenied5, logs preservados/no codigo Windows cambiado.
Fixtures iniciales corregidas (archivo corrupto reutilizado, chrono sub-ms portable),
asserts activos/sin nuevos warnings C++/D9025 esperado/N03 abierto.
H04 no cerrada: integrar F03 con exe/ventanas/positivo nativo y persistir esa red
real. C58/E11/P4/B2,77,3%/22,7%,sin credito parcial.
Espejo99 confirmado: comentario90140272938633; TXT manda.

Checkpoint98: [aceptacion F03/vista real](LINUX_NETWORK_ACCEPTANCE.md), F03 E->C.
SELF corregido con identidad local COMPLETA y dos lecturas kernel actuales,
sin inferencia de IDs ni bypass externo. REAL /proc WSL30/30 strict/0excluidos,
tres snapshots Monitor/red-metadata-epoch, UDP delta97/tasas/limpieza PASS.
Permiso temporal confirmado/usado<10s acumulados/sondas retiradas, no permanente.
Linux133/133+30focales, MSVC10join/consumidor instalado exit0, privado17 regresion
TCP-UDP/IPv4-IPv6/updates/gap/fork pasa. Fallos EINVAL/actor no vivo conservados;
churn sigue falla-cerrado, no total universal. N02/N03/H04/J04 no cerradas.
C58/E10/P5/B2,58/75=77,3%,17/75=22,7%; siguiente H04, leer HISTORY.md.
Recheck build Linux exit0 sin avisos tras clock-skew generado; logs iniciales preservados.
Espejo98 confirmado: comentario90140272937903; TXT manda.

Checkpoint97: [capacidad1024 y lotes](LINUX_NETWORK_SCALE.md) terminada.
RAW64/lote,16384buckets estructurales; altas en lotes nuevos, retenidos sin
reinicio/gap. Limite comun owner/inventario/politica/controlador/proyeccion/
Monitor/Sampler, rechazo>1024 sin truncar. Recompilar TODOS por constexpr.
Fixtures65/129/1024/16384buckets/fallos lotes2y16, nativo65 PRIVADO PUBLICO+
admision restringida/87y152FDs/tasas-UID/unchangedrefresh/limpieza y regresion17
pasan. Linux133/133+70focales, MSVC70focales, consumidor instalado exit0.
Primer assert de consumer esperaba viejo limite1024buckets, actualizado16384
sin relajar checks/log preservado; sin nuevos warnings C++/D9025 esperado.
F03 E por validacion/cobertura vista real; autorizacion WSL solicitada, NO
recibida ni captura global realizada en esta revision. H04/J04 despues.
57/75=76,0%,18/75=24,0%,sin credito parcial. Espejo97 confirmado: comentario90140272937319.

Checkpoint96: [admision integrada Monitor/Sampler](LINUX_NETWORK_ADMISSION_MONITOR.md)
terminada: SystemSnapshot conserva ultimo refresh/red/epoch/exclusiones juntos;
consentimiento CADAvez, strict_all default, modos exclusivos. Fallos cierran ambos
informes, worker continua; flag/restart no rearma. Refresh concurrente/history,
retencion/readmision sin gap y metadata parcial probados SOLO2hijos privados+
marcador sintetico, no captura global. Layout SystemSnapshot0.x: recompilar TODOS.
Linux133/133 sin skips+100focales/consumidor instalado exit0; MSVC40focales.
Primer fallo assertion MSVC preservado/causa no aislada, reconstruccion y repeats
pasan sin relajar asserts; N03 consumidor75 sigue abierto. Clock-skew WSL temporal
registrado; recompilacion de comprobacion exit0 sin avisos. F03 sigue E por escala>64/cobertura/validacion real autorizada;
H04/J04 abiertas.57/75=76,0%,18/75=24,0%,sin credito parcial.
Espejo96 confirmado: comentario90140272936245; historicos remotos intactos/TXT manda.

Checkpoint95: [politica parcial opt-in/procedencia](LINUX_NETWORK_ADMISSION_POLICY.md)
terminada: strict_all default CADAvez, verified_only explicito excluye filas
noverificadas con PID/estado/motivo; no oculta corrupcion/metadata incompleta.
Plan puro con readiness separado de calidad; >64elegibles rechazaTODO/no first64.
Sobre sample_with_admission une ultimo refresh/red, no censo actual/global;
conjunto igual conserva epoch/tasas aun cambiando politica/motivos. Layout0.x
AdmissionChange ampliado, RECOMPILAR biblioteca/consumidores. Linux132/132 sin
skips+100focales/consumidor instalado exit0; MSVC40focales, nativo SOLO2privados+
marcador sintetico/sobre sin fila inventada/epoch-baseline26FDs/strictcierraTODO
limpieza pasa. Pendiente Monitor/Sampler/escala>64/validacion real autorizada.
F03/H04/J04 abiertas,57/75=76,0%,18/75=24,0%,sin credito parcial.
Espejo95 confirmado: comentario90140272317289; historicos remotos intactos.

Checkpoint94: [admision estricta standalone](LINUX_NETWORK_ADMISSION.md)
terminada: inventario-owner-proveedor publico con consentimiento CADArefresh;
ALLverificados<=64 o rechazaTODO, sin filtro/truncamiento oculto. Conjunto igual
conserva tasas/epoch sin update/sample oculto; cambios/PIDreuse/gap/origen seguros.
Nativo SOLO2hijos privados con enumerador restringido/procfs-pidfd-BPF nativos,
AB->unchangedAB->B->AB, censos18->16->26FDs/fork/desconocido cierraTODO/restart/
vacio/limpieza. NO captura completa de /proc probada ni autorizacion global.
Linux131/131 sin skips+80focales/consumidor nuevoheader-API instalado exit0;
MSVCfixture20veces pasa; build final sin nuevos warnings C++/D9025 esperado.
F03/H04/J04 abiertas por politica practica vistas grandes/inaccesibles/validacion
real autorizada e integracion admision Monitor/Sampler;57/75=76,0%,18/75=24,0%.
Sin credito parcial. Espejo94 confirmado: comentario90140272312863; TXT manda.

Checkpoint93: [backend PUBLICO compartido observado](LINUX_NETWORK_SHARED_PUBLIC.md)
activado en standalone/Monitor/Sampler/owner. Estricto/declarado sin cambios.
Nativos17/censo N+14 y AB->B->AB/censos16->15->24FDs propios/tasas-UID/gap/origen/
epoch/fork/worker-concurrent-update-history/optout-no-rearmado/limpieza PASAN.
Estricto PUBLICO y declarado nativos pasan. Final93 Linux130/130 sin skips+
60focales/consumidor instalado exit0; MSVC3/3+60focales pasan, no Windows total.
Sin nuevos warnings C++/D9025 esperado. Builds iniciales colisionaron por error del asistente
(mismo directorio/ranlib); logs preservados, reconstruccion serializada.
Admision general e integraciones H04/J04 pendientes, no captura global.
F03/H04/J04 abiertas,57/75=76,0%,18/75=24,0%,sin credito parcial.
Espejo93 confirmado: comentario90140272305719; historicos remotos intactos.

Checkpoint92: [update compartido autenticado](LINUX_NETWORK_SHARED_UPDATES.md)
terminado: retencion sin reinicio/altas-readmisiones frescas/gap excluido,
epoch/reset de TODAS las tasas. Lotes disjuntos acotados, no3sondas constantes
tras churn; coherencia porlote/barrido total<=1s, no corte atomico entre lotes.
Nativo16->15->16, fixtures80updates/64lotes-1024buckets/fallos-excepciones cada
operacion; Linux130/130+40focales/consumidor instalado exit0; MSVC40focales pasan.
Pendiente activacion PUBLICA/Monitor/Sampler/owner y admision. F03/H04/J04
abiertas,57/75=76,0%,18/75=24,0%,sin credito parcial.
Espejo92 confirmado: comentario90140272299452; historicos remotos intactos.

Checkpoint91: [retirada RAW compartida segura](LINUX_NETWORK_SHARED_RETENTION.md)
terminada sin reiniciar retenidos/reutilizar slots. Nativo17->16->1 mantiene
origen/8FDs y excluye retirados aun enviando. Fixtures evento tardio/64->63/
regresion/fallos1y2; Linux130/130 sin skips, MSVC2/2 veinte veces cada una.
Altas/readmision/transicion autenticada/publico pendientes; F03/H04/J04 abiertas.
57/75=76,0% cerrado,18/75=24,0% abierto; ningun credito parcial/cierre nuevo.
Final Linux130/130+40focales/consumidor instalado exit0; MSVC40focales.
Espejo91 confirmado: comentario90140272294875; historicos remotos intactos.

Prioridad vigente por peticion del usuario: [Linux primero](LINUX_FIRST_PLAN.md).
Windows queda aplazado en revision62 hasta terminar el alcance Linux, no retirado
del proyecto. Siguiente F03; despues H04/J04 y resto Linux segun dependencias,
una subtarea a la vez. Revision63 solo reordena ejecucion: no hay codigo/suites/
cierres nuevos ni cambio del porcentaje. Regresion Windows solo si compatibilidad
de un cambio compartido lo requiere, no avance de funciones Windows.
Espejo63 confirmado: comentario90140271872526; titulo/estados historicos no conciliados.

**Porcentaje oficial:57/75 subtareas cerradas =76,0%.**
Abiertas:18/75 (24,0%):11 en curso,5 pendientes,2 bloqueadas.
Tres filas archivadas [X] no cuentan. No es porcentaje de horas ni garantía de
cobertura de todos los equipos. Cada subtarea pesa igual; trabajo parcial no
cuenta como cerrado.

La auditoría reabrió A01/A03/J01/K01 por problemas concretos de disponibilidad,
ventana temporal, estado de sesión y grupo de servicio. Excluyó C04/D05 por
mejoras no solicitadas y absorbió J03 en J05: un rechazo seguro no es detach.
El cambio de50,0% a45,3% corrige el control; no representa pérdida de código.

El anterior≈70% subjetivo queda retirado como indicador vigente. El60,4%
histórico de ClickUp (32/53) tampoco representa el estado actual. No mezclar
esos denominadores con el inventario oficial de75.

Después de cada unidad: actualizar ID/estado/criterio/evidencia/bloqueo, anotar
el movimiento y recalcular C/(C+E+P+B) en SEGUIMIENTO.txt. Actualizar este resumen
y STATUS.md con los mismos números. No aumentar por prompt ni crear tareas
resueltas para inflar el resultado. Cambios de alcance requieren historial.

Espejo75 confirmado: comentario90140272037855 en la lista.

Espejo76 confirmado: comentario90140272039515 en la lista.

Espejo77 confirmado: comentario90140272040593 en la lista.

Espejo78 confirmado: comentario90140272041626 en la lista.

Espejo79 confirmado: comentario90140272042530 en la lista.
Espejo80 confirmado: comentario90140272044050 en la lista.

Ultima unidad Revision90: F03 E->E, ventanas COMPARTIDAS autenticadas de seleccion FIJA terminadas.
LinuxSharedSelectedNetworkSampler1..64/PID-start-pidfd/cookie0, dup propios/
rollback completo, unicidad kernel/boot-observer y joins antes-despues del start.
CADA ventana: join de TODOS antes Y despues del barrido, cada join nativo DOS
consultas kernel actuales con permisos/liveness/procfs; UNA query de sesion
gated para TODO el conjunto. Dominios locales/kernel separados, no cache permiso.
Coherencia del productor compartido/bracket<=1s, epoch/metadata vacia/bucket
nuevo sin tasa/reductor cuatro lanes/suma UID parcial. Fallo del ultimo AFTER
join rechaza TODO/reset tasas; discard rebaseline, NO borra antirregresion raw.
Fixtures64/1024/dos UID/four lanes20B-s,512known/UID/lane,10240B-s/UID/lane/
proyeccion validada/old join-layout/errores-excepciones cada start/duplicados/
contexto-clock-identity-fork/calidad/ultimo AFTER/reset-recovery.
WSL PRIVADO17 PASA: owner liberado,31FD (17pidfds+8capture+6query), tres links
trafico compartidos+uno identidad gated, cuatro tasas/UID integer oracle con
TCP/UDP IPv4/IPv6, metadata vacia/primer bucket sin tasa/main-worker-main/padre
excluido/fork padre intacto; actor salido rechaza TODO dos veces. Stop/start16
epoch-vacio-contadores NUEVOS/30FD/discard-recovery; todos hijos salida normal/
FD final limpio. Start420ms/medido517ms/stop2262ms incluyen joins-IPC-restart,
NO benchmark. SOLO1netns real/64x16 fixtures,no64reales certificados.
Linux130/130 sin skips/tres tests20veces/consumidor instalado exit0; MSVC3/3
focales+nuevo test20veces. Fixture MSVC SegFault inicial: range-for en resultado
temporal destruido, corregido reteniendo muestra; log final-targeted-msvc
fallido preservado/final2-targeted-msvc y repeat-msvc positivos. Sin nuevos C++
warnings/D9025 esperado; NO corrige consumidor75/N03/ni compatibilidad total.
Sin API-layout publico/GUI/persistencia/dependencias/CLI/config nuevos.
docs/LINUX_NETWORK_SHARED_WINDOWS.md, work/network-shared-window-*.
PENDIENTE updates compartidos conservando captura/origen de retenidos y sin
huecos/slots inseguros; despues activar backend publico/Monitor/Sampler y
admision automatica. Proyeccion comprobada NO es backend publico activado.
F03/H04/J04 abiertas; C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas.
Sin cierre/ID/credito parcial nuevo. Espejo90 confirmado: comentario90140272288882;
lista leida/titulo-estados historicos no conciliados; TXT manda.

Unidad anterior Revision89: F03 E->E, productor COMPARTIDO INTERNO de seleccion fija terminado.
LinuxSharedNetworkSession/1..64tokens kernel declarados, no PID attestation.
HASH TGID->slot+1 y ARRAY global+UID-inicio+105valores por actor; tres programas/
links de trafico para TODO el conjunto. No seleccionado sale antes de contexto/
contadores; inicio deshabilitado hasta adjuntar TODOS. No eviccion/cookie17 ni
fallo de identidad/lectura/overflow como0: calidad sticky rechaza TODO.
Writer/sequence GLOBAL y por actor, tabla/metadata y suma secuencias validadas,
8reintentos/bracket<=1s; referencia previa preservada contra regresiones.
Fixtures64x16=1024buckets/aislamiento/cuatro lanes/IPv4-IPv6/TCP-UDP/tabla/
ultimo actor lleno/overflow; fallos-excepciones cada inicio rollback/clock/
writers/fork/regresion-recuperacion. Solo3cargas-links para64 en fixture.
WSL privado17 PASA: tokens asociados pidfd/procfs ANTES, exactamente8FD
(2mapas/3programas/3links) tras liberar owner/query. UDP64..80 IPv4x2/IPv6x1,
TCP IPv4/IPv6 4x64..80 por direccion/cuatro lanes/cookies/secuencias por actor
y global; vacio/padre excluido/fork padre intacto/restart fresco/FD limpios/
todos hijos salida normal. Start532ms/medido548ms/stop tras restart1765ms;
incluye joins-IPC-workload, NO benchmark. SOLO1netns real/64x16 fixtures.
Regresion PUBLICA17 anterior PASA cuatro tasas/UID y limpieza, NO backend nuevo.
Linux129/129 sin skips/tres tests20veces/consumidor instalado exit0; MSVC3/3
focales/sin nuevos C++ warnings/D9025 esperado; consumidor75 N03/no total.
Sin API-layout publico/GUI/persistencia/dependencias/CLI/config nuevos.
docs/LINUX_NETWORK_SHARED_PRODUCER.md, work/network-shared-*.
PENDIENTE conectar productor a joins CONTINUOS pidfd/ventanas/tasas/updates con
origen retenido, despues publico/Monitor/Sampler y admision automatica. No red
general ni totales apps/usuarios; solo pruebas privadas autorizadas.
F03/H04/J04 abiertas; C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas.
Sin cierre/ID/credito parcial nuevo. Espejo89 confirmado: comentario90140272276062;
lista leida/titulo-estados historicos no conciliados; TXT manda.

Unidad anterior Revision88: F03 E->E, consulta pidfd de sesion con compuerta terminada.
Recursos cargados reutilizados/slot5 caller_pid_tgid exacto/0desarmado; sin
identidad ni permiso cacheados. Caller/thread nuevos, borra resultados, DOS
consultas por join y joins antes/despues; permisos/liveness/procfs intactos.
Fallo de desarmado retira link/programa y rechaza; fork cierra copia SIN escribir
mapas del padre. Fuentes inyectadas anteriores/ruta standalone conservadas.
Fixture40transacciones/tres cargas/un adjunto/cambio hilo sin reload; gate0/
caller distinto no lee contexto, stale/sin trigger/fallos-excepciones/fork/
desarmado-recovery cubiertos. Prototipo link por consulta FALLA nativo17,
log network-query-reuse-native17-linux preservado; NO cuenta aprobado.
Diseno final gated PASA nativo17 completo: start1593ms, vacio1620ms,
primero1647ms, medido1676ms, stop2986ms; fases27..29ms incluyen IPC/workload,
no benchmark/latencia universal. Cuatro tasas/UID parcial/bytes17/padre excluido/
owner clear/FD limpios/todos hijos salida normal; watchdogs55/60sin cambio.
WSL worker2actores main-worker-updater/AB-B-AB/historial/epochs/gap/fallo update
y estricto TCP-UDP/tasas/UID/fork padre intacto/restart/mask standalone positivos.
Linux128/128 sin skips/seis focales20veces/consumidor instalado exit0; MSVC6/6
focales/sin warnings C++ nuevos/D9025 esperado, consumidor75 sigue N03/no total.
Sin API-layout publico/GUI/persistencia/dependencias/CLI/config permanente nuevos.
docs/LINUX_NETWORK_QUERY_SESSION.md, work/network-query-gated-* y prototipo
work/network-query-reuse-*. SOLO1netns real,17reales;64/1024 son fixtures.
Siguiente productor multiactor compartido/cobertura-capacidad explicitas antes
de admision automatica; solo actores privados autorizados, no captura general.
F03/H04/J04 abiertas; C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas.
Sin cierre/ID/credito parcial nuevo. Espejo88 confirmado: comentario90140272264999;
lista leida/titulo-estados historicos no conciliados; TXT manda.

Unidad anterior Revision87: F03 E->E, capacidad ESTRUCTURAL64/BTF por sesion y coste medido.
Estricto64procesos/declarado64filas/observado64x16=1024buckets, owner/puente
max64/65rechaza sin truncar/rotar; cookie17 por actor sigue rechazada.
Fixtures64/1024/ultimo actor falla-reset TODO/retencion-reorden-readd/calidad.
BTF reutilizado por sesion; NO cache de identidades ni supresion de checks:
permiso/liveness/pidfd/procfs y ambos extremos siguen actuales.
Prueba nativa17 FALLA DOS veces, no aprobacion: tras ajuste start11,2s,
vacio35,1s/primera53,6s desde inicio; primeros17contadores UDP correctos,
segunda muestra pidfd salido al alcanzar vida55s hijos. NO tasas17/UID
ni limpieza NORMAL final verificados; RAII cierra captura y recolecta hijos.
No tercer intento ni mas watchdog. Regresion nativa estricta positiva:
cuatro tasas/UID, identidad, padre excluido, fork/restart/FD limpios.
Linux128/128 sin skips/seis focales20veces/consumidor instalado exit0;
MSVC6/6 focales, sin warnings C++ nuevos/D9025 esperado, fallo consumidor75
sigue N03/no compatibilidad total. Constante publica cambia: recompilar TODOS,
sin layout publico existente/GUI/persistencia/dependencias/CLI nuevos.
docs/LINUX_NETWORK_CAPACITY64.md, work/network-capacity64-*. Limite64 NO es
garantia de rendimiento ni productor compartido; F03/H04/J04 abiertas.
Siguiente reducir cargas/adjuntos BPF de identidad repetidos preservando TODOS
los controles, antes de productor compartido/admision automatica.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas; sin cierre/credito parcial.
Espejo87 confirmado: comentario90140272256311 en la lista; estados remotos no conciliados.

Unidad anterior revision86: F03 E->E, [inventario visible/calidad por PID](LINUX_NETWORK_INVENTORY.md).
PUBLICO readonly/procfs fijado-EOF/max100000/contexto-boot-observer/bracket5s
cooperativo; TODOS PID encontrados verified-denied-disappeared-unavailable con
Metric/motivos/UID observador/no0inventado. Fallo global sin parcial/no censo host.
Puente exige TODOS verificados/max16, sin filtrar/truncar/rotar. Fixtures UID0-
vacio/16-17/mixtos/3estados fallo/21fallos fuente/15frames alterados. WSL readonly
/proc real/2hijos IPC verificados/al owner como subconjunto PRIVADO explicito/
puente TODOS o ausencia/FD limpios; denegado-salido positivos fixtures/no hidepid-
exit race real. Sin BPF/root/trafico capturado. Linux128/128 sin skips/2tests20veces/
consumidor instalado exit0/build final sin warnings/1warning inicial corregido-
log/notas stat aclaradas. MSVC2/2 focalizadas/sin nuevos C++ warnings/D9025 esperado,
consumidor75 sigue N03/no total. Header/tipos/funciones/test aditivos sin layout
existente/GUI/persistencia/dependencia/CLI/captura global nuevos. F03 E por
admision automatica/captura escalable MAS DE16; H04/J04 abiertas.
work/network-inventory-*. C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas;
sin credito parcial. Espejo86 confirmado: comentario90140272246025 en la lista.

Unidad anterior revision85: F03 E->E, [propiedad de pidfds por PID/inicio](LINUX_NETWORK_SELECTION_OWNER.md).
PUBLICO readonly0..16candidatos declarados completos/consentimiento/shape-capacidad17
rechaza sin fuente/truncar/rotar. Adquiere-retiene-retira-reemplaza/reorden sin
generation nueva/TODOS por extremos/contexto/poll-fdinfo-stat/RAII/fork-move/vacio.
Runtime fallo limpia propios; NO captura/enumeracion/auto-update. Prestamos cookie0,
downstream duplica; clear/fallo owner NO detiene captura independiente: consumidor
stop/update. Fixtures16positivos/17rechazo/13fallos runtime/calidad-contextos/
excepciones/retencion-cambios/FD limpios; WSL readonly2hijos/move-fork/stale-start/
exit-vacio y autorizado owner->PUBLICO BPF2hijos/fd originales cerrados/clear owner
captura sigue/4tasas-UID oracle/A TCP12288/12288 UDP768/768 peek24 y B doble/
2threads por actor/gap-exit-restart/FD limpios. SOLO1netns real/oracle SO inicializa
cookies/16positivos fixtures/no general. Linux127/127 sin skips/2tests20veces/
consumidor instalado exit0/build final sin warnings/3warnings indentacion inicial
corregidos-log. MSVC2/2 focalizadas/sin nuevos C++ warnings/D9025 esperado,
consumidor75 sigue N03/no total. Header/tipos/clase aditivos sin layout existente/
GUI/persistencia/dependencia/CLI/captura global nuevos. F03 E por inventario con
cobertura/admision automatica/escalabilidad MAS DE16; H04/J04 abiertas.
work/network-selection-*. C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas;
sin credito parcial. Espejo85 confirmado: comentario90140272050845 en la lista.

Unidad anterior revision84: F03 E->E, [observado Monitor/Sampler](LINUX_PROCESS_NETWORK_OBSERVED_MONITOR.md).
Setup3args opt-in/2args siempre estricto/callback viejo rechazado; activo-running
no cambia modo/Sampler detenido+flag. Update vivo conserva modo/metadata/historial-
epochs/retained origin/reset TODOS/nuevo sin gap. Optout-stop cierran/flag-restart
no rearman/fallo update captura cerrada worker sigue. Fixtures ambos modos/calidad/
excepciones/callback viejo/metadata vacia/default/flag/running/consumer. WSL Monitor
PUBLICO2hijos/pidfds cerrados/move/vacio-tardio/4tasas-UID oracle/A TCP12288/12288
UDP768/768 peek24 y B doble/2threads por actor/padre excluido/fork/update-retiro-
readd-gap/exited2veces/stop-restart/default/optout/FD limpios; worker2actores/cookie0/
vacios->buckets/4tasas-UID historial/AB->B->AB/updater concurrente-historial/epochs-
reset/retained origin/nuevo sin gap/fallo update worker sigue/default/restart
desarmado/FD limpios. SOLO1netns real/oracle SO inicializa cookies/no general.
Regresion Monitor declarado positiva. Linux126/126 sin skips/3tests20veces/
consumidor instalado exit0/sin warnings; MSVC3/3 focalizadas/sin nuevos C++ warnings/
D9025 esperado, consumidor75 sigue N03/no total. Sin API/layout nuevo sobre83/GUI/
persistencia/dependencia/CLI/captura global nuevos. F03 E por politica seleccion-
cobertura general; H04/J04 abiertas. work/network-observed-monitor-*.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas; sin credito parcial.
Espejo84 confirmado: comentario90140272049411 en la lista.

Unidad anterior revision83: F03 E->E, [proveedor publico observado](LINUX_PROCESS_NETWORK_OBSERVED.md).
Opt-in/default estricto/1..16actores-cookie0/16cookies cada uno/256filas; coverage
nuevo/observed_processes identidades aun vacios/primer bucket sin tasa/seq actor
repetido no sumar. Proyeccion integral sin parcial. Layout0.x snapshot/SystemSnapshot
nuevo: recompilar TODOS. Monitor/Sampler aun rechazado sin proveedor/siguiente.
Fixtures vacio-mixto/multicookie/23invalidos/256proyeccion/consumer tipos nuevos.
WSL PUBLICO2hijos/pidfds cerrados/move/vacio-tardio/4tasas-UID oracle/A TCP12288/12288
UDP768/768 peek24 y B doble/2threads por actor/padre excluido/fork/update/retiro-
readd-gap/epochs/exited2veces/remover recupera/stop-restart modo-vacio/FD limpios.
SOLO1netns real/oracle SO inicializa cookies,256fixtures. Estricto nativo positivo.
Linux126/126 sin skips/3tests20veces/consumidor instalado exit0/sin warnings;
MSVC3/3 focalizadas/sin nuevos C++ warnings/D9025 esperado, consumidor75 sigue N03/
no total Windows. Sin GUI/persistencia/dependencia/CLI/captura global nuevos.
F03 E por Monitor/Sampler observados/politica general; H04/J04 abiertas.
work/network-public-observed-*. C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas;
sin credito parcial. Espejo83 confirmado: comentario90140272047500 en la lista.

Unidad anterior revision82: F03 E->E, [ventana comun multiactor observada](LINUX_NETWORK_OBSERVED_MULTI.md).
INTERNA1..16actores/16cookies cada uno/scope0..256filas, cookie entrada0/modo fijo,
joins TODOS por extremos/bracket1s/fork; registro con identidad aun vacio/primer
cookie sin tasa/4tasas-UID parcial. Fallo cualquiera reset TODOS/no parcial/commit
registro final. Updates retienen origen/retiro-readd vacio sin gap/epoch/reset
grupal/fallo runtime cierra todo. Fixtures32buckets/2UID/256filas reducer/limites/
20fallos lectura/5inicio/adicion fallida. WSL2hijos/pidfds originales cerrados/
no cookies entrada/vacio-tardio/4tasas-UID oracle/A TCP12288/12288 UDP768/768 peek24
y B doble/2threads por actor/padre excluido/fork/retiro-readd-gap/exit rechaza
grupo2veces/remover recupera/FD limpios. SOLO1netns real compartido/oracle SO
inicializa cookies/32-256fixtures/no16actores reales. Regresiones comun estricta/
individual observada positivas. Linux126/126 sin skips/3tests20veces/consumidor
instalado exit0/sin warnings; MSVC3/3 focalizadas/sin nuevos C++ warnings/D9025
esperado, consumidor75 sigue N03/no total. Sin API/layout publico/GUI/persistencia/
dependencia/CLI/captura global nuevos. F03 E por proveedor publico/Monitor-Sampler
observados/politica general; H04/J04 abiertas. work/network-observed-multi-*.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas; sin credito parcial.
Espejo82 confirmado: comentario90140272046531 en la lista.

Unidad anterior revision81: F03 E->E, [ventanas seleccionadas observadas](LINUX_NETWORK_DISCOVERED_WINDOWS.md).
Pidfd propio/join por extremos/guard fork/boot-contexto/bracket1s, sin cookie
entrada; vacio conocido/primer cookie sin tasa/A->A+B,4tasas/UID parcial.
Reset baselines por error/contexto/discard, no regresion registro/contadores,
stop/restart limpio/epoch nuevo/modos separados. Fixtures16cookies/exclusiones/
scope0-17-multiactor/8fallos inicio/21lectura/recuperacion. WSL1hijo/pidfd original
cerrado/TCP12288/12288 UDP768/768 peek24/2threads/4tasas-UID oracle/otros excluidos/
fork-padre intacto/discard/restart/exited2veces/FD limpios. SOLO1netns real/oracle
SO inicializa cookie,16cookies fixtures. Estricto nativo positivo. Linux126/126
sin skips,2tests20veces/consumidor instalado exit0/sin warnings; MSVC2/2 focalizadas
sin nuevos C++ warnings/D9025 esperado, consumidor75 sigue N03/no total.
Sin API/layout publico/GUI/persistencia/dependencia/CLI/captura global nuevos.
F03 E por multiactor/bracket comun/API-Monitor-Sampler observados/politica general;
H04/J04 abiertas. work/network-discovered-window-*.
Espejo81 confirmado: comentario90140272044981 en la lista.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision80: F03 E->E, [motor privado observado](LINUX_NETWORK_DISCOVERY.md).
Sin cookie entrada/token kernel actor, ARRAY105/16slots/CMPXCHG64 sin eviccion,
17o/fallos/zero/overflow sticky global; writer-seq/lectura integral8reintentos/
validador/vacio inicial/detach-stop-reinicio. Fixtures16cookies/4lanes/peer igual-
distinto/capacidad/calidad/rollback y WSL1hijo/no cookie productor/oracle SO,
TCP12288/12288 UDP768/768 peek24 con2threads/otros excluidos/detach/restart vacio
luego TCP8192/8192 UDP512/512 peek16/FD limpios. SOLO1netns real;16cookies/race
distinto/capacidad son fixtures, oracle inicializa cookie. Regresion declarada
Monitor positiva. Linux126/126 sin skips,2tests20veces/consumidor instalado exit0/
build sin warnings, MSVC2/2 focalizadas/sin C++ warnings nuevos/D9025 assert
esperado, consumidor75 sigue N03/no total. Error macro assert fixture corregido/
log preservado. Sin API/layout publico nuevo/GUI/persistencia/dependencia/CLI/
setns-unshare/captura global. F03 E por join/contexto/ventanas-tasas/multiactor/
proveedor publico/politica general; H04/J04 abiertas. work/network-discovery-*.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision79: F03 E->E, [buckets en Monitor/Sampler](LINUX_PROCESS_NETWORK_BUCKET_MONITOR.md).
Setup3args modo/consentimiento,2args siempre estricto tras stop, no reautoriza
flag/restart, activo no cambia modo/Sampler detenido+flag/update conserva modo.
Fixtures callback/enum/fallos/opt-out/default y WSL Monitor/worker4oraculos bucket-
UID/retiro-readd/gap/historial/update concurrente/baselines/default estricto/
limpieza positivos. SOLO1netns real/segunda cookie NO observada, no2netns reales
simultaneos ni descubrimiento. Linux126/126 sin skips,3tests20veces/consumidor
instalado exit0/build sin warnings; MSVC3/3 focalizadas sin warnings C++ nuevos/
D9025 esperado asserts, fallo consumidor75 sigue N03/no total validado. API
aditiva/sin layouts ni opciones nuevas/GUI/persistencia/dependencia/CLI/captura
global. F03 E por politica-descubrimiento/buckets dinamicos/cobertura general;
H04/J04 abiertas. work/network-bucket-monitor-*.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision78: F03 E->E, [buckets declarados por namespace](LINUX_PROCESS_NETWORK_BUCKETS.md).
Proveedor publico constructor opt-in/16filas PID-start-cookie, varias por proceso;
cookie ajeno no cero excluido antes de mapa, calidad/errores sticky, default
estricto intacto. Dedup/contextos/UID coherentes/coverage nuevo/filas de bucket,
suma UID parcial sin duplicado. Update retiene/remueve/readd con epoch y reset
TODOS baselines/sin gap retroactivo. Fixtures2cookies positivos4lanes y WSL1hijo
seleccionado2cookies, SOLO1namespace real/segundo cookie no observado,4oraculos/
otro0/suma sin duplicado/retirada-readd/gap/fork/otros excluidos/FD limpios.
NO prueba2netns reales simultaneos; regresion estricta2hijos positiva.
Linux126/126 sin skips,5tests20veces/consumidor instalado exit0/build sin warnings;
MSVC5/5 focalizadas sin warnings C++ nuevos/D9025 esperado asserts, fallo
consumidor75 sigue N03/no total validado. Error protocolo fixture overflow
corregido/log preservado. API aditiva/enum ampliado/sin layouts nuevos/GUI/
persistencia/dependencia/CLI/setns-unshare/captura global. work/network-buckets-*.
F03 E por modo buckets Monitor/Sampler/politica-descubrimiento/buckets dinamicos/
cobertura general; H04/J04 abiertas. Siguiente integracion opt-in de buckets.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision77: F03 E->E, [update con worker activo](LINUX_PROCESS_NETWORK_WORKER.md).
Sampler serializa coleccion/commit/update/stop con prioridad para update pendiente
y wakeup protegido. Captura existente/consentimiento nuevo, no rearmado automatico;
fallo cierra captura/worker sigue. Historial/epochs/otras metricas preservados,
latest puede seguir historico hasta proxima muestra, no conectar epochs.
CTest start-stop/40updates/lecturas,3tests20veces; WSL2hijos privados AB->B->AB con
worker/updater otro hilo/4oraculos actor-UID/gap/baseline/origen/limpieza positivos.
Primer nativo fallo en retirada, eliminada posible inanicion; repeticion/final
positivas/log preservado. Linux126/126 sin skips/consumidor instalado exit0/build
sin warnings, MSVC3/3 focalizadas/sin warnings C++ nuevos. Fallo consumidor75
sigue N03, no compatibilidad total. API aditiva/sin layouts nuevos/GUI/historial
persistido/dependencia/captura global. F03 E por politica-descubrimiento/buckets/
cobertura general; H04/J04 abiertas. work/network-worker-*.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision76: F03 E->E, [seleccion actualizable](LINUX_PROCESS_NETWORK_UPDATE.md)
en proveedor publico y Monitor. AB->B->AB/reorden, retiene capturas coincidentes,
cierra retiradas, consentimiento nuevo/epoch y reset de TODOS baselines. Runtime
failure cierra TODO; Monitor tambien ante lista autorizada invalida. Contadores
retenidos conservan origen, nuevos no capturan retroactivamente. Fixtures y WSL
2hijos privados4oraculos actor-UID/gap/fork/salida-recuperacion/sin fugaFD.
Linux126/126 sin skips,2tests20veces, consumidor instalado exit0/build sin warnings.
Primer assert de orden fixture corregido/log preservado; API aditiva/sin layouts
nuevos. No captura global/GUI/historial/dependencia. F03 E por update worker/
politica-descubrimiento/buckets/cobertura general; H04/J04 abiertas.
Fallo consumidor MSVC revision75 sigue N03, no compatibilidad total validada.
MSVC76:2/2 focalizadas pasan, D9025 esperado asserts activos/sin warnings C++ nuevos.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision75: F03 E->E, [Monitor/Sampler seleccionados](LINUX_PROCESS_NETWORK_MONITOR.md)
integrados en Linux. Informe separado/flag false/setup autorizado aparte; opt-out
cierra, worker stop/fin/error cierra y restart/flag solos no reautorizan. Fixtures
lifecycle/sentinelas legacy y WSL2hijos4oraculos/FDs caller cerrados/move/fork/
worker positivo/rearmado epoch-baseline limpio/sin fugaFD. Linux126/126 sin skips,
18tests red20veces/consumidor instalado final exit0/build verificado sin warnings.
CAMBIO LAYOUT0.x SystemSnapshot/SampleOptions, recompilar TODOS. Metric header
extraido sin cambio semantico, shadow BTF corregido; warnings DrvFS preservados
y rebuild limpio. MSVC2/2 focalizadas pasa; consumidor general reproduce0xC0000409
despues de checks nuevos, causa no probada/N03 pendiente, NO compatibilidad total.
work/network-monitor-*. Sin captura Windows nueva/GUI/persistencia/dependencia.
F03 E por seleccion dinamica/buckets namespace/cobertura general; H04/J04 abiertas.
Siguiente alcance manual fijo/namespaces, pruebas privadas/no captura global.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision74: F03 E->E, [proveedor publico seleccionado](LINUX_PROCESS_NETWORK.md)
terminado. LinuxProcessNetworkSampler Pimpl movable/start opt-in/sample/stop,
encabezado instalado, cobertura tipada/estimated/identidades locales-kernel por
clave/contadores-tasas-UID inicial parcial/availability/brackets/boot-epoch-observer.
Proyeccion pura/34negativos/baseline0/UID0/overflow/IDs-ordenes distintos. WSL API
PUBLICA2hijos autorizados4oraculos actor-UID intervalo2.307999437s/FDs caller cerrados/
move activo/idle0/padre excluido/fork-salida/sin fugaFD. Linux125/125 sin skips,
17tests red20veces, consumidor instalado nuevo encabezado compilado/ejecutado
exit0/build sin warnings; error fixture nombre campo corregido/log preservado.
API ADITIVA/layouts-ABI existentes intactos, no autoMonitor/totales legacy.
Sin Windows nuevo/GUI/historial/dependencia. work/network-public-*.
F03 E por integracion opt-in Monitor/politica seleccion-cobertura-buckets;
H04/J04 abiertas. Siguiente informe seleccionado en flujo general, pruebas
privadas/no captura global. C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas.
Espejo74 confirmado: comentario90140271926244; titulo/estados remotos historicos.

Unidad anterior revision73: F03 E->E, [ventana nominal comun/UID](LINUX_NETWORK_COMMON.md)
terminada. Joins todos antes/despues, barrido contadores<=1s/ordenado/no superpuesto,
commit posterior, selected_actor_set1..16/dedup TGID-start, fin nominal comun/
tasas estimadas/UID inicial parcial con known/missing/overflow y brackets actual-
previo/asociaciones por clave. Fallo/excepcion/cambio/modo descarta TODOS baselines.
Fixtures mismoUID/2UID/4oraculos/12fallos/recovery/1s/>60s/overflow/modos/fork.
WSL2hijos privados autorizados A4096/4096/256/256B B8192/8192/512/512B,4oraculos
actor-UID/brackets/idle0/padre excluido/FDs caller cerrados/fork/salida/limpieza.
Regresiones nativas selected e independiente exit0. Linux124/124 sin skips,
16tests red20veces/consumidor instalado exit0/build sin warnings. CTests existentes
ampliados, sin nuevo conteo. work/network-common-*. Sin Windows/Monitor/API/ABI/
historial/dependencia nueva. No snapshot atomico/global; cookies declarados y
limites heredados. F03 E por contrato cobertura/buckets/proveedor publico;
H04/J04 abiertas. Siguiente cobertura/proveedor opt-in seleccionado, pruebas
privadas, no captura global. C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas.
Espejo73 confirmado: comentario90140271923048; titulo/estados remotos historicos.

Unidad anterior revision72: F03 E->E, [coleccion multiactor](LINUX_NETWORK_MULTI.md)
seleccionada terminada. Lista fija1..16, un cookie por proceso, dedup local/kernel,
contexto boot/observer comun y rollback de TODOS los baselines ante cualquier
fallo/excepcion/cambio; primera recuperacion sin tasas. Intervalos independientes,
NO foto simultanea/suma UID comun. Fixtures2actores mismo UID/cookie intervalos
2s/3s,5fallos inicio/4fallos segunda fila/reset/fork. WSL2hijos privados autorizados
A4096/4096/256/256B B8192/8192/512/512B,4oraculos por actor/trafico cruzado-padre
excluidos/idle0/FDs caller cerrados/duplicado-fork/salida/limpieza sin fugaFD.
Linux124/124 sin skips,16tests red20repeticiones, consumidor instalado exit0,
build final sin warnings; warning indentacion inicial corregido/log preservado.
work/network-multi-*. Sin Windows/Monitor/API/ABI/historial/dependencia nueva.
F03 E por ventana comun/agregacion UID/cobertura/buckets/integracion; H04/J04
abiertas. Siguiente ventana comun acotada/agregacion UID, pruebas privadas,
no captura global ni cookies automaticos. C57/E11/P5/B2,75activas/3X,
76,0%/24,0%,18abiertas; sin IDs/cierres/credito parcial.
Espejo72 confirmado: comentario90140271920104; titulo/estados remotos historicos.

Unidad anterior revision71: F03 E->E, [ventanas seleccionadas](LINUX_NETWORK_SELECTED_WINDOW.md)
revalidadas terminadas. Dup pidfd propio, joins frescos antes/despues, validacion
kernel/boot/observador/contexto y commit reductor solo tras exito. Fallo/excepcion/
cambio elimina baseline y recuperacion sin tasa sobre huecos. Tipo1actor/cookie
separado SELF/global, productor separado actor y epoca compartida,4tasas estimadas/
UID parcial. Fixtures FD0/8fallos inicio/15fallos muestra/reset/contexto/intervalo/
fork, WSL privado autorizado PID402/kernel913 UID0,4tasas/UID oraculo intervalo
1.104113519s/caller FD cerrado/padre excluido/idle0/fork/reinicio/salida/sin fugaFD.
Linux123/123 sin skips,15tests red20repeticiones, consumidor instalado exit0.
Primer relink warnings make por timestamps DrvFS preservados; build verificado
sin warnings/suite repetida, sin tocar reloj. work/network-selected-window-*.
Sin Windows/Monitor/API/ABI/historial/dependencia nueva. F03 E por multiactor/
buckets/cobertura/integracion, cookie aun declarado, extremos no atomicos/ABA/
vida futura/plazo duro no garantizados. H04/J04 abiertas. Siguiente cobertura/
buckets/actores multiples, pruebas privadas, no ampliar captura global.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas; sin IDs/cierres/credito parcial.
Espejo71 confirmado: comentario90140271917419, titulo/estados remotos historicos.

Unidad anterior revision70: F03 E->E, [asociacion pidfd/procfs](LINUX_NETWORK_PIDFD_JOIN.md)
seleccionada terminada. Duplicado/poll, contexto boot/observador/objetivo por
extremos, dos tokens kernel iguales, proyeccion Pid en fdinfo del hilo caller,
PID/start_ticks/status/ns revalidados; dominios PID/UID/ticks separados, UID
procfs del observador. Fixtures de cambios/ausencias y WSL privado autorizado
PID2513/kernel6353 UID0, parent-read start/join repetido/stale-PID-pipe-exit
rechazados,4lanes de trafico/padre excluido/detach/negativos/mascara/FD sin fuga.
Tabla FD caller distinta al lider detectada/corregida/probada con worker privado
CLONE_FILES, sin privilegios ni namespaces nuevos. Linux122/122 sin skips,
14tests red20repeticiones cada uno, consumidor instalado exit0/build sin warnings.
work/network-join-*. Sin Windows/Monitor/API/ABI/historial/dependencia nueva.
No atomicidad/ABA/vida futura/cookie externo automatico/cobertura global. F03 E
por ventanas revalidadas/multiactor/buckets/cobertura/integracion; H04/J04 abiertas.
Siguiente ventanas/tasas seleccionadas con revalidacion/reset de baseline.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,18abiertas; sin IDs/cierres/credito parcial.
Espejo70 confirmado: comentario90140271913864, titulo/estados remotos no conciliados.

Unidad anterior revision69: F03 E->E, [identidad desde pidfd](LINUX_NETWORK_PIDFD.md)
terminada/probada. Consulta interna opt-in, dup/liveness, lookup FD-1/EBADF y
sonda FEXIT de permisos/hilo inicial caller/modo0x12/ret0, BTF validado y resultado
unico. No ptrace attach/senales/parada objetivo ni recorrido global. Mascara
caller temporal restaurada; puede retrasar handlers, no plazo duro. WSL privado
autorizado hijo sin token kernel reportado PID554/kernel1640 UID0,2consultas/
handle invalido/exited rechazados,TCP4096/4096 UDP256/256/8peeks,padre excluido/
detach/start-UID erroneos sticky/sin fugaFD. Linux120/120 sin skips,12tests red
20repeticiones, consumidor instalado exit0, build final sin warnings.
work/network-pidfd-*, candidato __pidfd_fget ausente descartado; warning pair
corregido y target nuevo reconfigurado. Sin Windows/Monitor/API/ABI/historial
publico/dependencia nueva. F03 E por join/revalidacion pidfd-procfs/contexto/
multiactor/buckets/cobertura/integracion; H04/J04 abiertas. Siguiente ese join
seleccionado, pruebas privadas/permisos acotados. C57/E11/P5/B2,76,0%/24,0%,
18abiertas; sin IDs/cierres/credito parcial.
Espejo69 confirmado: comentario90140271911142; titulo/estados remotos no conciliados.

Unidad anterior revision68: F03 E->E, [actor privado seleccionado](LINUX_NETWORK_SELECTED.md)
capturado con token kernel/cookie declarados, tipo separado de SELF. Validacion,
8puntos de fallo/excepcion/rollback/FD0/reinicio; no autentica identidad externa.
WSL autorizado hijo cooperativo PID2075/kernel2600,UID0,TCP4096/4096B,UDP256/256B,
8peeks/padre excluido/detach/start-UID erroneos sticky/pidfd salida/limpieza sin
fugaFD. SELF asociado nativo tambien PASS. Linux119/119 sin skips,11tests red
20repeticiones, consumidor instalado exit0, build sin warnings. work/network-
selected-*. Sin Windows/Monitor/API/ABI/historial publico/dependencia nueva.
F03 E por identidad externa confiable/multiactor/buckets/cobertura/integracion;
H04/J04 abiertas. Siguiente F03 descubrimiento/revalidacion confiable de actores,
no ampliar la autorizacion privada a global. Preferencia GUI/CMake conservada
en LINUX_FIRST_PLAN.md. C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin IDs/cierres/credito.
Espejo68 confirmado: comentario90140271905836; titulo/estados remotos no conciliados.

Unidad anterior revision67: F03 E->E, [asociacion directa SELF kernel/procfs](LINUX_NETWORK_SELF_JOIN.md)
terminada. Token local PID/start_ticks/UID/procfs/namespaces, lector acotado/pinned
caller/lider, dos identidades BPF frescas no adjuntadas y boot/contexto estables.
Sin convertir ticks/ns ni inferir IDs iguales entre dominios; ausencia/cambio/
permiso/excepcion/contexto entre muestras/raw-joint descarta baseline.
Linux119/119 sin skips,11tests red20repeticiones, consumidor instalado exit0.
Procfs nativo UID1000/PID950 estable20/sin fugaFD; WSL autorizado PID390 frente
kernelTGID4965,UID0/0,TCP4096/4096B,UDP256/256B,8peeks/oraculo4tasas-UID/793949ns,
idle0/fork/reinicio/sin fugaFD. work/network-self-join-*. Warning indentacion
corregido, timestamps transitorios WSL, build final sin warnings. Sin Windows/
API/ABI/historial publico/Monitor/dependencia nueva. Solo SELF/bracketing,
no identidad atomica/general/apps/actores externos. F03 E por multiactor/buckets/
identidad externa/cobertura/integracion; H04/J04 abiertas. Siguiente continuar F03
captura/identidad de actores externos con pruebas privadas/permisos acotados.
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin IDs nuevos/cierres/credito parcial.
Espejo67 confirmado: comentario90140271891145; titulo/estados remotos no conciliados.

Unidad anterior revision66: F03 E->E, [captura SELF conectada a ventanas/tasas](LINUX_NETWORK_WINDOW.md).
Sampler posee captura/scope/epoch/reductor, dominios kernel_initial tipados,
scope de1fila/cookie declarado, boot UUID procfs acotado y bracket monotono <=1s.
Primera lectura sin tasas; intervalos <=60s, tasas estimated por finales de
lectura y UID parcial, ausencia descarta baseline, reinicio/fork protegidos.
Linux117/117 Release,9tests red20repeticiones, consumidor instalado reconstruido/
exit0. WSL nativo autorizado TCP4096/4096B,UDP256/256B,8peeks,4tasas/UID contra
oraculo, intervalo646362ns, idle0/fork rechazado/reinicio epoch limpio/sin fugaFD.
work/network-window-*. Build inicial clock-skew0.27s, verificado sin warnings.
Sin Windows/Monitor/API/ABI/historial publico/dependencias nuevas. No total
global/procfs join: F03 E por multiactor/buckets/join/cobertura/integracion publica,
H04/J04 abiertas. Siguiente F03 identidad procfs para el productor.
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin IDs nuevos/cierres/credito parcial.
Espejo66 confirmado: comentario90140271884970; titulo/estados remotos no conciliados.

Unidad anterior revision65: F03 E->E, [control de namespace del socket](LINUX_NETWORK_NAMESPACE.md)
terminado. BTF opcional skc_net.net->net.net_cookie, cookie declarado SELF y
rechazo sticky de mismatch/nulos/lecturas fallidas, tambien peeks positivos.
Linux116/116 Release,8tests red20repeticiones cada uno, consumidor instalado
reconstruido/exit0. Nativo WSL autorizado cookie1 corroborado SO_NETNS_COOKIE,
TCP6144/6144B,UDP384/384B,12peeks/hijo excluido/threads/retiro/reinicio sin fugaFD;
cookie distinto rechazado persistentemente. Modo anterior tambien PASS.
work/network-netns-*. Sin setns/unshare/configuracion/dependencia nueva ni
Windows/API/ABI/historial publico/Monitor. No prueba multins real ni captura
general: F03 sigue E por buckets/multiactor/join/cobertura/ventanas/integracion;
H04/J04 abiertas. Siguiente continuar F03 identidad/ambitos del productor.
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin IDs nuevos/cierres/credito parcial.
Espejo65 confirmado: comentario90140271880147; titulo/estados remotos no conciliados.

Unidad anterior revision64: F03 E->E, [sesion Linux SELF](LINUX_NETWORK_SESSION.md)
con ciclo de vida terminada. Inicio autorizado, identidad exacta, mapa privado9,
carga antes de attach, rollback en14puntos de fallo/excepcion, FD0, lectura
coherente acotada, detach/stop/destructor/reinicio y rechazo de estimados.
Linux116/116 Release, fixture20repeticiones y consumidor instalado reconstruido/
exit0. Nativo WSL autorizado PASS TCP6144/6144B,UDP384/384B,12peeks excluidos,
hijo excluido/threads y retirada/reinicio sin fugaFD; sin bandera77. Primer
build aviso clock-skew0.0062s, posterior sin warnings; wrapper77 corregido por
quoting, llamada directa confirma77. work/network-session-*. No trabajo Windows,
API/ABI/historial publico/Monitor nuevos. F03 sigue E por netns/join/multiactor/
cobertura/ventanas/integracion; H04/J04 abiertas. Siguiente continuar F03.
Sin nuevos IDs/cierres/credito parcial: C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo64 confirmado: comentario90140271876915; titulo/estados remotos no conciliados.

Unidad anterior revision62: M02 E->E, [correlacion IRP](WINDOWS_DISK_CORRELATION.md)
terminada. Decoder TDH init/completion version3/IRP32y64, conserva emisor del
inicio tras salida/reuso PID/TID. Colisiones/duplicados/huerfanos/epoca/orden/
cobertura/TTL/limites,10000pares y pipeline simulado probados. Windows136/136
ambas,Linux115/115 Release,20repeticiones decoder/correlador por entorno,
3consumidores instalados reconstruidos/exit0, work/disk-correlation-*.
Build inicial far macro SDK corregido en fixture; D9025MSVC esperado.
Sin ETW/UAC ni nueva prueba nativa/Monitor/API/ABI/persistencia/dependencia publica.
Productor nativo ciclos/inicioI/O/reconciliacion/drenaje/perdidas/disco/red generales
pendientes, cobertura precondicion no certificada. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo62 confirmado: comentario90140271870463; titulo/estados remotos no conciliados.

Unidad anterior revision61: M02 E->E, [indice temporal PID/TID](WINDOWS_TRACE_IDENTITY.md)
terminado. Epoca/generaciones/vidas QPC, lookup al inicio real/intervalo, no
completion ni IDs actuales como sustitutos. Cobertura completa precondicion
del caller, no certificada; reemplazo validado sin respuestas stale, limites/
solapamientos/duplicados/contencion y100000registros. Oraculo20000. Windows135/135
ambas,Linux114/114 Release,20repeticiones en cada entorno,3consumidores instalados
reconstruidos/exit0, work/trace-identity-*. Sin ETW/UAC/reintento MSVC cancelado
ni API/ABI/persistencia/dependencias publicas nuevas. M02 productor nativo ciclos/
inicioI/O, correlacion IRP/drenaje/perdidas/disco/red generales pendientes.
C57/E11/P5/B2,76,0%/24,0%,18abiertas; no captura completa garantizada.
Espejo61 confirmado: comentario90140271604191; titulo/estados remotos no conciliados.

Unidad anterior revision60: M02 E->E, [decoder ETW y prueba privada](WINDOWS_DISK_EVENTS.md)
terminados. TDH por nombres/4bytes/version3, no PID/header/offsets; conteo uint64
del hilo propio vivo en ventana QPC, cero valido, overflow y oraculo10000.
MinGW nativo PASS read1048576B/write1142784B,16reads/38writes, errores/perdidas
reportadas0; archivo y sesion retirados, sin ETL. Primer schemaError13 conservado,
causa exacta no diagnosticada. MSVC nativo no ejecutado por elevacion cancelada,
no reintentada ni contada como positivo. Windows134/134 ambas,Linux113/113 Release,
20repeticiones decoder/ciclo por Windows toolchain,3consumidores instalados
reconstruidos/exit0. tdh nativoWindows en paquete, sin API/ABI/persistencia
publica nueva. M02 general disco/red/identidad temporal/drenaje/perdidas abierto;
no captura completa garantizada. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo60 confirmado: comentario90140271603279; titulo/estados remotos no conciliados.

Unidad anterior revision59: M02 E->E, [ciclo de vida ETW](WINDOWS_DISK_TRACE_LIFECYCLE.md)
terminado. Usuario autorizo prosigue tras solicitud58. Sesion propia, sinETL,
eventos todos descartados, no Monitor/default; colisiones/errores/buffers/flags/
idempotencia/cierre concurrente probados. Elevado MinGW/MSVC cierre normal y
watchdog positivos: STOP0/QUERY4201, retirada confirmada, buffer64KiB4/max64.
Windows133/133 ambas,Linux112/112 Release,20repeticiones fixtures por Windows
toolchain,3consumidores instalados reconstruidos/exit0, work/etw-lifecycle-*.
Fallos ANSI/wide/flag hibrido13/UPDATE87 corregidos con logs y retirada confirmada.
Sin API/ABI/persistencia nueva. No llegada/atribucion de eventos ni disco/red
por PID validado todavia; M02 permanece abierta. C57/E11/P5/B2,76,0%/24,0%.
Espejo59 confirmado: comentario90140271601441; titulo/estados remotos no conciliados.

Punto de control anterior revision58: M02 E->E, [decision ETW de disco](WINDOWS_PROCESS_DISK_DECISION.md).
Pendiente de autorizacion temporal15s/64MiB/sinETL; puede recibir eventos del
sistema y requerir elevacion, reteniendo solo los del proceso privado de prueba.
No ETW implementado/ejecutado ni nuevas pruebas/cierres; resultados de revision57.
Espejo58 confirmado: comentario90140271600221; titulo/estados remotos no conciliados.

Unidad anterior implementada revision57 (2026-10-21):M02 E->E, E/S general Windows terminada.
[Contrato/evidencia](WINDOWS_PROCESS_IO.md): seis contadores uint64 nativos y
tasas estimated por identidad/ventana propia en ProcessInfo.windows_io. No son
disco/red especificos y no rellenan esos campos. Errores/reuso/reset/limites y
cero/64bits probados. Windows132/132 MinGW/MSVC,Linux112/112 Release,
20repeticiones por compilador Windows; pipe privado1024B positivo y Monitor
baseline/rates/off-on. Tres consumidores instalados reconstruidos/exit0.
Layout publico ProcessInfo0.x cambiado: reconstruir biblioteca y TODOS los
consumidores. Sin vtable/persistencia nuevos. work/process-io-verified-* y logs
install/consumer; C4312 fixture volumes corregido, D9025MSVC test esperado.
M02 permanece abierta por disco/red especificos, atribucion/cobertura; M04
global/hardware pendiente, M05 aplazada. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo57 confirmado: comentario90140271599726; titulo/estados remotos no conciliados.

Unidad anterior revision56 (2026-10-21):M04 E->E, AMD experimental explicito terminado.
Eleccion humana21oct, [contrato/evidencia](WINDOWS_AMD_RUNTIME_MEMORY.md).
Nueva llamada/tipos separados, dos cantidades runtime estimated/driverCL, no
global/Monitor/Sampler/default/historial. Evidencia primaria aclara size_t[2]/KiB
y formato (ya no desconocido); alcance/caches/budget/reservas/heaps sigue limitado.
Interop unico observado/revalidado no identidadfisicaLUID, validaciones/64bits/
cero/overflow, helperextraido conserva versionCL anterior. RX7900XT positivo
MinGW/MSVC21435580416B/bloque21413273600B en primera muestra, Monitor globalAMD
unknown. Windows131/131 ambas,Linux111/111 Release,20repeticiones AMD einterop
por Windows toolchain,3consumidores instalados reconstruidos/relinkados/exit0.
work/amd-runtime-*; test inicial esperaba4cierres aun en rechazo temprano,
corregido abierto==cerrado, logs conservados; D9025MSVC test esperado. API0.2
adicional sin layouts/vtable/formatos existentes cambiados. M04 globalAMD/Intel/
NVIDIAfisico/matriz pendientes, M05 aplazada. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo56 confirmado: comentario90140271597603; titulo/estados remotos no conciliados.

Unidad anterior revision55 (2026-10-21):M04 E->E, loader NVML System32/legacy terminado.
[Contrato/evidencia](WINDOWS_NVML_LOADER.md). Ubicacion fija NVSMI por KnownFolder
ProgramFilesX64, fallback solo ausencia, sin bypass acceso/badimage ni entorno/
PATH/instalacion. Ruta absoluta validada/flags acotados, no certifica firma/ACL/
reparse/modulos previos. CUDA/KMT y13simbolos/identidad/memoria igual, Win32 loader
en nota y sin status0 ficticio. Windows130/130 ambas,Linux111/111 Release,
20repeticiones de loader y memoria por Windows toolchain,3consumidores instalados
reconstruidos/relinkados/exit0, work/nvml-legacy-*. Error inicial tipos/narrowing
MinGW corregido, D9025MSVC test esperado. NativeAMD legacy tambien ausente126,
sin positivo fisico NVIDIA. M04 freeAMD/Intel/cobertura pendientes, M05 aplazada;
sin API/ABI/vtable/formatos nuevos, Linux intacto. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo55 confirmado: comentario90140271596413; titulo/estados remotos no conciliados.

Unidad anterior revision54 (2026-10-21):M04 E->E, NVML free Windows terminado.
[Contrato/evidencia](WINDOWS_NVML_MEMORY.md): campo existente recibe bytes
framebuffer sin asignar reportados NVMLv1, no garantia allocatable/presupuesto
ni resta DXGI-PDH. Identidad exacta CUDA LUID/UUIDv2->NVMLroot UUID y KMTcount1,
relecturas/unicidad/limites/topologia/64bits/cero; otros proveedores intactos.
System32 drivers opcionales sin SDK/CLI/descarga ni contextos/buffers/kernels
propios; cuInit puede conservar estado/hilos internos del driver, NVML balanceado
y KMT/DLL RAII. Native64, sin legacyNVSMI fallback. Windows129/129 ambas,
Linux111/111 Release,20repeticiones NVML por Windows toolchain,3consumidores
instalados reconstruidos/relinkados/exit0. work/nvml-free-* finales final-* y
release-repeat-*. HostAMD confirma ausencia/probeDLLs faltantes, NO positivo
fisico NVIDIA/WDDM/TCC/multiGPU/MIG/linked/hotplug; pruebas inyectadas no certifican
hardware. Sin API/ABI/vtable/formatos nuevos, Linux intacto. M04 sigue free
AMD/Intel/cobertura, M05 aplazada. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo54 confirmado: comentario90140271595547; titulo/estados remotos no conciliados.

Unidad anterior revision53 (2026-10-21):M04 E->E, Vulkan MSVC y unicidad terminados.
Deteccion CMake Windows acepta ruta privada a headers ya presentes y entorno
VULKAN_SDK, sin descarga/instalacion/export al consumidor. MSVC antes stub,
ahora positivo nativo RX7900XT Vulkan1.4.349 por LUID igual MinGW. OFF explicito
respetado/probado1/1 con headers presentes; cache negativo se limpia con -U.
IDs destino duplicados/vacios unknown, otra fila unica intacta. [Evidencia](WINDOWS_GPU_VULKAN.md).
Windows128/128 ambas,Linux111/111 Release,20repeticiones Vulkan por Windows
toolchain,3consumidores instalados reconstruidos/relinkados/exit0. Logs
work/vulkan-portable-*. Sin API/ABI/vtable/formatos nuevos. AMDfree investigado
acotadamente, sin suponer formato ni calcular libre DXGI-PDH. M04 sigue memoria
global/cobertura; M05 aplazada. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo53 confirmado: comentario90140271591287; titulo/estados remotos no conciliados.

Unidad anterior revision52 (2026-10-21):M04 E->E, bloque WGL/OpenGL opt-in terminado.
Opcion existente/defaultfalse; GL_VERSION de contexto desktop actual del hilo
solo por LUID EXT Win32 estable y fila DXGI unica. No maximo/capacidad fisica/
nodo/aceleracion, no fallback por nombre. System32 RAII/funciones por contexto,
limites/contratos/version/duplicados/cambios, no creacion de contexto/ventana ni
draw/glGetError en biblioteca. Diagnostico privado AMD querynodo1280; prototipo
por nodo descartado, contrato final ADAPTADOR viaLUID documentado, sin query ni
inferencia de nodos. [Contrato/evidencia](WINDOWS_CURRENT_OPENGL.md): positivo
nativo RX7900XT4.6.0 en MinGW/MSVC directo+Monitor, error previo intacto y sin
error nuevo final, contexto/DC conservados/restaurados y ventana/clase privadas
eliminadas. Windows128/128 ambas,Linux111/111 Release,20repeticiones finales WGL
por Windows toolchain,3consumidores instalados recompilados/relinkados/exit0.
No nuevos layouts/API/vtable/formatos, comportamiento Windows opt-in version0.2;
LinuxEGL/GLX sin cambios, Sampler no hereda contexto GUI. Definitivos work/wgl-release-*;
LUID cero conocido/no-write/parcial/HighPart negativo probados. Warning GCC inicial corregido, D9025 MSVC
esperado /UNDEBUG tests conservado. M04 no cerrada: resto criterio/memoria global/
cobertura adaptadores pendiente; M01 abierta/M05 aplazada y demas IDs conservados.
C57/E11/P5/B2,76,0% cerrado/24,0% abierto,18subtareas. Espejo52 comentario90140271590512.

Unidad anterior revision51 (2026-10-21):M01 E->E, compatibilidad MSVC x64 terminada.
VS18/MSVC19.51/SDK26100, fallo real SDK rename corregido con ABI documentada
interna sin WDK/cambio a rutas; mismo contrato handles/identidad/no overwrite.
/utf-8 soloC+++test literal, advertencias tipos/ASCII/variable corregidas,
test tiempo negativo portable por tick nativo (rechazo/almacen intactos).
[Evidencia MSVC](MSVC_VALIDATION.md):MSVC127/127,MinGW127/127,Linux111/111 Release,
80repeticiones MSVC/100Linux,3consumidores instalados separados recompilados/
relinkados/exit0. Native Startup privado positivo+CPU/firmware host, no Startup/
SCM real tocado ni hardware multigrupo certificado; manualSCM solo compilado.
Final MSVC sin warnings C, conserva D9025 intencional por /UNDEBUG tests;
MinGW/Linux sin warnings. Logs iniciales interrumpidos/errores preservados en
work/msvc-*, finalMSVC verified/native-suite, regresion post-clock. Sin API/
layout/vtable/formatos nuevos, version0.2. M01 sigue compresion estable/hardware
multigrupo, CI N01/A05/N03 sin cierre; M05 aplazada por confirmacion explicita
del usuario21oct. C57/E11/P5/B2,76,0% cerrado/24,0% abierto,18subtareas abiertas.
Espejo51 confirmado: comentario90140271587468, sin credito fraccionario ni nuevos IDs.

Unidad anterior revision50:J05 P->C por alternativa de capacidad indisponible
justificada YA prevista, no por desconexion real. API estatica publica por
adaptador utmp/login1: false/available+motivo enLinux, enum desconocido/noLinux
unknown; inventario propaga solo con metadata reconocida/conocida. Sin metadata
ahora unknown, no false generico. No ID/Remote/Active/State/token logout como
capacidad; no señales/bloqueo/logout/elevacion. Contratos nativos explican por
que utmp/login1 no ofrecen detach de transporte; no imposibilidad para cualquier
servidor remoto. [Alcance y evidencia](LINUX_DISCONNECT.md). J03 no se cuenta de
nuevo; WTS M06/logout J02 separados. Linux110/110,Windows126/126 Release,
60repeticiones por plataforma, consumidores ambos recompilados/relinkados/
ejecutados exit0. Hijo POSIX privado preservado/utmp privado limpio; login1 real
solo lectura1sesion. Primer nuevo casoNUL fallo por expectativa, corregida para
exigir invalid_argument existente sin cambiar validador/logs conservados.
work/disconnect-capability-*. Nuevo header/enum/funcion sin layouts/vtable/
historial/dependencias nuevas. C57/E11/P5/B2,76,0% cerrado/24,0% abierto.
M05 aplazada, no opt-in experimental por 'prosigue'. Espejo50 comentario90140270925508.

Unidad anterior revision49:M05 E->E, investigacion documental sin codigo/pruebas
nuevas. StartupTask del propio paquete no es control general; RequestEnableAsync
no anula desactivacion del usuario. No se encontro contrato general en fuentes
oficiales revisadas, sin afirmar inexistencia universal. Evidencia48 Run6801
revisada, sin sondeos/configuracion nuevos ni causa atribuida. Decision pedida:
aplazar M05 o aceptar proveedor experimental interno limitado por version; no
autoriza cambios reales/UAC/ACL en pruebas. Ver
[decision tecnica](WINDOWS_STARTUP_APPROVAL_DECISION.md). Criterio conservado,
sin cierre/credito parcial; C56/E11/P6/B2,74,7% completo/25,3% abierto. Suites
126/110 de48 NO reejecutadas. Espejo49 confirmado: comentario90140270919388.

Unidad anterior revision48:M05 E->E. Lectura/listado HKLM32/64 terminados, control
transaccional implementado/modelo completo, native positivo PENDIENTE6801.
API por vista separada, IDs seleccionan hive/view; listado/Monitor viejos HKCU.
Software fijado/flags explicitos/backup misma hive-view, sin fallback, permisos/
arquitectura conservados. Audit Run copia request antes del sink. HKLM real
32=1/64=4filas y consultas individuales positivas sin writes; native privado
redirigido6801sinwrites, no mutacion HKLM/ACL elevada/legacy virtualizacion/MSIX
certificados. WINDOWS_MACHINE_RUN_CONTROL.md y work/machine-run-pinned-*.
Linux110/110,Windows126/126 Release,60repeticiones Windows/20Linux, consumidores
ambos recompilados/relinkados/ejecutados exit0, builds finales sin warnings.
API/enum adicionales sin virtual/layouts/formatos nuevos. Cambio comportamiento0.x
HKLM all-users exige consentimiento host, no UAC/config/ACL. M05 sigue native
positivo Run/StartupApproved/aprobacion/accion generica Windows, A05/L05/N03.
C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo48 confirmado: comentario90140270915863.

Unidad anterior revision47:M05 E->E. Archivos Common Startup terminados: ID user/
common selecciona Known Folder, listado common separado. Listado user/Monitor
existentes no se amplian, checks/audit/rename no-replace conservados. Cambio
comportamiento0.x: common puede afectar a todos los usuarios con derechos ya
existentes, exige aviso/consentimiento host, sin elevacion/ACL/fallback.
Native privado dos raices: retiro/listado/restore auditados, conflictos/scopes/
raices/aislamiento, permisos de enumeracion inyectados. Common real solo lectura;
no ProgramData/UAC/ACL mutantes ni denegacion ACL nativa probados. Documento
WINDOWS_COMMON_STARTUP_FILE_CONTROL.md, work/startup-common-*.log. Linux110/110,
Windows125/125 Release,60repeticiones Windows, consumidores ambos recompilados/
relinkados/ejecutados exit0, builds sin warnings. API libre adicional sin nuevos
virtual/layouts/formatos ni Startup real tocado. M05 sigue por HKLM/aprobacion/
StartupApproved/accion generica Windows, Run6801 no resuelto; A05/L05/N03 abiertos.
C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo47 confirmado: comentario90140270909066.

Unidad anterior revision46:M05 E->E. Accion explicita de archivos en SystemActions/
AuditedActions terminada. Operacion audit propia/ordinales conservados, request
copiado antes de callback, parametro completo al checked backend sin fallback;
log ID sensible bounded/no contenidos/identidades. Fail-closed requested,
best-effort explicito, completion fallido conserva resultado/excepcion propagada.
Native privado auditado retiro/restore y fallos de sink positivos; portable
forwarding/errores/politicas/limites/callback mutante. Nuevo virtual0.x requiere
recompilar biblioteca y TODOS los consumidores/backends, sin layouts Snapshot/
Options/AuditEvent/formatos nuevos. WINDOWS_STARTUP_FILE_AUDIT.md y
work/startup-file-audit-*.log. Linux110/110,Windows125/125,40repeticiones Windows/
20Linux, consumidores instalados ambos recompilados/relinkados/ejecutados exit0.
Builds finales sin warnings; inicial WSL Clock skew registrado sin configurar reloj.
Run6801 confirmado sin writes; suites previas tambien incluian esa prueba aunque
notas dijeran no repetido (sin sondeo dedicado). Sin Startup real tocado/fixtures
residuales. M05 sigue por HKLM/common/aprobacion/accion generica; A05/N03 abiertos.
C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo46 confirmado: comentario90140270903626.

Unidad anterior revision45:M05 E->E. Respaldos en Monitor/Sampler terminados,
opcion explicita false por defecto, Metric separado sin mezclar Startup/Run.
Off no invoca proveedor; on conserva empty/permisos/errores/valor/nota y limpia
datos viejos. Ownership y Sampler latest/history/timed_history sin persistencia
nueva/acciones/ventana cambiada. Native fixture privada positiva para llenado,
Monitor/Sampler reales read-only y gates/errors/copy inyectados. Cambio layout0.x
Snapshot/Options: biblioteca y TODOS los consumidores recompilados/relinkados.
Tipos proveedor movidos sin cambiar layout; sin metodos virtuales/formatos nuevos.
WINDOWS_STARTUP_FILE_MONITOR.md, work/startup-backup-monitor-*.log. Linux109/109,
Windows124/124 Release,40repeticiones Windows/20Linux, consumidores ambos exit0.
Clock skew WSL en dependencias generadas registrado, pasada settled adicional,
sin cambios de reloj/configuracion ni causa atribuida.
Ultima settled exit0/sin warnings ni recompilaciones/enlaces, binarios de suites
conservados; Windows sin warnings. M05 abierta por HKLM/
common/aprobacion/audit/acciones genericas; Run6801 no resuelto/repetido; A05/N03
conservados. C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo45 confirmado: comentario90140270896765.

Unidad anterior revision44:M05 E->E. Listado de respaldos terminado, API explicita
list_windows_startup_file_backups y tipo adicional nombre UTF-8/registration:
descubre y restaura sin IDs guardados tras reiniciar consumidor. Dos pasadas por
handle/identidades/conflictos/contexto, acotadas; estimated/no atomico, sin crear
directorios ni escrituras, permisos/fallos sin lista parcial. Native fixture
privada positiva, varios archivos/Unicode/tamano0/orden/exclusiones/conflicto;
errores/records/limites/permisos inyectados y cambio entre pasadas rechazados.
WINDOWS_STARTUP_FILE_CONTROL.md, work/startup-backup-listing-*.log. Linux108/108,
Windows123/123 Release,20repeticiones Windows; consumidores ambos recompilados/
relinkados/ejecutados exit0, builds finales sin warnings. Sin cambios a layouts existentes/
ABI virtual/historial, ni CLI/elevacion/KTM nuevos ni Startup real modificado.
M05 abierta por HKLM/common/aprobacion/integracion, Run6801 no resuelto ni repetido.
C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo44 confirmado: comentario90140270891958.

Unidad anterior revision43:M05 E->E. Accion explicita nueva para retirar/restaurar
archivos Startup current-user, nativa POSITIVA con fixture privada fuera del
Startup real. NtCreateFile relativo/handles y NtSetInformationFile rename
sin reemplazo, identidades comprobadas; mismo archivo/bytes/atributos conservados,
respaldo PCInsight.Disabled.v1. Read no crea; se conserva ID para restore, sin
listado completo de respaldos integrado aun. Solo unidad local; no UNC/common/
HKLM/StartupApproved, CLI/KTM/elevacion ni fallback. API instalada adicional,
sin layouts/ABI virtual/historial cambiados. WINDOWS_STARTUP_FILE_CONTROL.md,
work/startup-file-control-*.log. Linux108/108,Windows123/123 Release,
20repeticiones Windows; consumidores ambos recompilados/relinkados/ejecutados
exit0, builds finales sin warnings. Readonly preservado, conflictos/stale/sharing/
cambios/bounds probados;2fixtures abortadas por expectativa readonly incorrecta
limpiadas por nombre exacto, finales limpias. No cierre M05: aprobacion efectiva,
HKLM/common/listado/integracion pendientes, Run TxR6801 no resuelto ni repetido.
C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo43 confirmado: comentario90140270888697.

Unidad anterior revision42:M05 E->E. Corregida raiz Startup que aceptaba colon
de stream (C:\Fixture:stream) por is_absolute; regresion fallo antes/paso despues.
Validador drive/UNC normal/extendido, rechazo antes de enumerar y cleanup probado.
GLOBALROOT/pipes/UNC incompleto ya rechazados en runtime actual, sin fallo previo
atribuido. WINDOWS_STARTUP_FOLDERS.md y work/startup-root-*.log. Linux108/108,
Windows122/122 Release,20repeticiones Windows, consumidores instalados ambos
relinkados/ejecutados exit0, builds sin warnings. Run TxR sigue6801/no escrituras;
sin reparacion KTM ni Startup real modificado. No implementa acciones/aprobacion:
M05 sigue abierta, C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo42 confirmado: comentario90140270883882.

Unidad anterior revision41:L03 B->C. Usuario respondio "prosigue" a propuesta
concreta de opcion avanzada limitada/desactivada por defecto con riesgo PID/TID,
sin checked/EcoQoS/threads nuevos/restore garantizados. Accion generica Linux
on/off desactivada sin tocar destino, availability false/available; checked
sigue no soportado sin fallback. apply/restore avanzados requieren ack=true
explicito y conservar recibo tras fallos. Cambio de COMPORTAMIENTO0.x documentado
en LINUX_EFFICIENCY.md y LINUX_EFFICIENCY_DECISION.md, sin firmas/layouts/ABI
virtual/historial nuevos; Windows EcoQoS intacto. Nativo solo hijos privados,
rechazos sin cambios, opt-in2threads, restore completo/mixto denegado con flags
pendientes, caller intacto e hijos recolectados. Linux108/108,Windows122/122
Release,20repeticiones Linux; consumidores ambos recompilados/relinkados/
ejecutados exit0, builds finales sin warnings. No carrera resuelta ni nuevos
helpers/elevacion/ptrace/cgroups; L05/N02/N03 conservados.
C56/E11/P6/B2,56/75=74,7%,19/75=25,3%; Espejo41 confirmado: comentario90140270880053, sin IDs/fracciones nuevos.

Unidad anterior revision40:E06 E->C. Usuario respondio "prosigue" a propuesta
concreta E06: transporte/medio reportados y unknown sin evidencia, no NVMe=>SSD.
Alternativa prevista/aceptada solo E06,no L03 ni medio exacto universal nuevo.
RPM permission_required conservado sin reporte valido con ATA/B1 denegada;
reporte independiente valido sobrevive. Tests capas/VPD ajena, stale SSD,
permisos/unknown/no-RPM0/alias valor-soporte-nota. LINUX_DISK_IDENTITY.md.
WSL4contextos/0medios, positivos de tecnologias fixtures, no fisicos,N02/N03
conservados. Linux108/108,Windows122/122 Release,10repeticiones focalizadas Linux,
consumidores instalados ambos recompilados/relinkados/ejecutados exit0, sin
warnings. Sin layouts/API/ABI virtual/historial/dependencias/acceso /dev/CLI/
elevacion/cambios de discos nuevos. C55/E11/P6/B3,55/75=73,3%,20/75=26,7%.
Espejo40 confirmado: comentario90140270876164; sin IDs/fracciones nuevos.

Unidad anterior revision39:C03 E->C. Usuario respondio "prosigue" a propuesta
concreta de aceptar diferencia instalada-utilizable estimada/etiquetada y
unknown sin datos validos. Alcance solo C03, no E06/L03. Alternativa prevista,
no reserva exclusiva hardware/BIOS universal implementada. Se conserva permiso
del total utilizable en diferencia/alias; nuevos fixtures de ausencia/denegado/
valor contradictorio/unsupported y cero SIEMPRE estimated, sin datos viejos.
LINUX_MEMORY_COMPOSITION.md. Linux108/108,Windows122/122 Release,10repeticiones
focalizadas Linux; ambos consumidores instalados recompilados/relinkados/
ejecutados exit0, builds sin warnings. Nativo WSL composicion positiva,0zram
observados,Zswap/firmware ausentes (no ceros globales). Positivos compresion/
firmware fixtures,N02/N03 conservados. Sin layouts existentes/ABI virtual/
historial/dependencias/elevacion/zram/swap/presion de RAM nuevos.
C54/E12/P6/B3,54/75=72,0%,21/75=28,0%; Espejo39 confirmado: comentario90140270873160, sin IDs/fracciones nuevos.

Unidad anterior revision38:M03 E->C. WMI5/5discos,2HDD/3SSD,SATA/NVMe sin UAC/
cambios de permisos ni codigo WMI. Fallo previo historico, no causa corregida.
FS5/5,sistema1/pagefile1,read/write/latencia5/5,active valido1/5 en muestra
detallada y5/5 en diagnostico posterior; disponibilidad variable, sin clamp/cero.
Red55tasas send/receive conocidas,
sin generar trafico. query_windows_network_identity con LUID/GUID/PnP unico/
revalidado; fabricante/descripcion por Metric,9fabricantes/8Ethernet y Realtek
PCIe2.5GbE positivo. WINDOWS_NETWORK_IDENTITY.md.20repeticiones identidad y20red.
Windows122/122,Linux108/108 Release; consumidores ambos recompilados/relinkados/
ejecutados exit0. API adicional sin layouts existentes/ABI virtual/historial
nuevos. Cierre proveedores soportados con limites, no RAID/MPIO/hotplug fisico
universal ni marca autenticada/reparto arbitrario. M02/M06/F03/N01/N03 conservados.
C53/E13/P6/B3,53/75=70,7%,22/75=29,3%. Espejo38 confirmado: comentario90140270634154; sin IDs/fracciones nuevos.

Unidad anterior revision37:M07 E->C. API open_windows_service read-only para GUI
sin shell/editor/elevacion; metadata por Metric y error tipado, vacio conocido
distinto de inaccesible. Grupo carga/descripcion SCM raw/estados/PID explicitos.
WINDOWS_SERVICE_VIEW.md. Positivo8servicios/8descripciones/8grupos; launch propio
con argv confirmado, stale-token rechazado, EcoQoS checked on/off releido y
end-task checked mismo handle/exit1; hijo privado limpiado.20repeticiones por
prueba(40ejecuciones). SCM positivo/limpiado revision36 conservado, no repetido.
Windows121/121,Linux108/108 Release; consumidores instalados ambos recompilados/
relinkados/ejecutados exit0. API adicional sin layouts existentes/ABI virtual/
SystemSnapshot/historial nuevos. Cierra proveedores/acciones soportados, no
modo combinado/ahorro/restauracion auto ni descripcion localizada/grupo svchost.
L05/M02/M06/N01/N03 conservados. C52/E14/P6/B3,52/75=69,3%,23/75=30,7%.
Espejo37 confirmado: comentario90140270623147; sin IDs/fracciones nuevos ni cambio de criterio.

Unidad anterior revision36:M07 E->E, validacion SCM nativa autorizada/UAC con
servicio privado LocalService. API publica start/stop/restart/idempotencia e
inventario real estado/PID/descripcion/grupo positivos. Copia estatica en
ProgramData con ACL protegida, sin abrir perfil ni LocalSystem; watchdog60s,
registro/copia/directorio eliminados y ausencia reconsultada. RESULT PASS,
tests/windows_service_native.cpp manual fuera de CTest; WINDOWS_SERVICES.md.
Primer arranque desde perfil Win32=5 limpiado/evidencia preservada. Sin opt-in
exit2, sin privilegios Win32=5 antes de crear. Windows119/119,Linux108/108 Release.
Sin codigo productivo/API/ABI/layout/historial nuevos, no migracion de consumidores.
Pendientes M07: abrir detalles y disponibilidad metadata; helper productivo L05.
C51/E15/P6/B3,51/75=68,0%,24/75=32,0%, sin cierre global ni IDs nuevos.
Espejo36 confirmado: comentario90140270598882; titulo/estados remotos sin cambios.

Unidad anterior revision35:E07 P->C, capacidad declarada ext2/3/4 sin montar,
consulta por region y DiskInfo.id. Sysfs enumera particiones u objeto completo,
identidad esperada/revalidacion y total solo si TODAS regiones conocidas/no
solapadas. No parcial/aliases/capas; declaracion logica con metadata, NO statvfs
util/libre ni total fisico universal. LINUX_FILESYSTEM_CAPACITY.md. Nativo
imagenes reales32MiB, FS32MiB en fuente48MiB, WSL8:48/sdd1TiB/checksum valido,
individual/por disco positivos exit0. Sin discos/mounts reales modificados;
SDK sin CLI/dependencias nuevas, formatter existente solo harness privado.
Linux108/108,Windows119/119, consumidores instalados recompilados/relinkados/
ejecutados ambos exit0, sin warnings. API adicional sin layouts existentes/
ABI virtual/SystemSnapshot/historial nuevos. Otros formatos/MBR-GPT raw y
distribucion fisica RAID/LVM/loop unknown. C51/E15/P6/B3,51/75=68,0%,24/75=32,0%.
F03 diferida/N02/N03 abiertos. Espejo35 confirmado: comentario90140270531513;
titulo/estados remotos sin cambios.

Unidad anterior revision34:H05 P->C, LinuxNotificationObserver opt-in con
consentimiento, politica de atribucion/privacidad/periodos y captura nativa.
Notify/respuesta por owner/sender/serial: llamadas aceptadas con reemplazos,
NO avisos mostrados/leidos. Rechazos/sin-acuse/pendientes/expirados separados.
Hint de app declarado estimated, unknown sin hint, sin prueba PID/exe/join CPU.
Sin contenido retenido/CLI/autolaunch/elevacion/persistencia. Owner-loss/errores
invalidan periodo, stop conserva ultimo finito/restart otro. LINUX_NOTIFICATIONS.md.
Nativo privado aceptadas3/reemplazos1/rechazadas1/appunknown1/sinacuse1, denegacion
real y owner-loss;10/10 repeticiones/directo exit0, no skip. Linux106/106,
Windows118/118, consumidores ambos recompilados/relinkados/ejecutados exit0,
builds finales sin warnings. API adicional sin layouts existentes/ABI virtual/
SystemSnapshot/historial nuevos. No universalidad GTK/portal/vendor ni retroactivo.
C50/E15/P7/B3,50/75=66,7%,25/75=33,3%; H04/M08/F03/N03 abiertos.
Espejo34 confirmado: comentario90140270530011, titulo/estados remotos sin cambios.

Unidad anterior revision33:I05 E->C, proveedor AppStream moderno/legacy y politica
de fuentes/condiciones ausentes (criterio conservado). Consulta publica/Monitor
opt-in, match desktop-id explicito/unico. Editor declarado estimated, no firmante
ni propietario del ejecutable. libxml2 del sistema opcional, sin helpers/
descargas/instalar; ausencia/error/ambiguedad deja unknown, no etiqueta vieja.
LINUX_STARTUP_PUBLISHER.md. Positivo nativo XML/Monitor con archivos privados,
exit0 no skip; rechazos de DTD/XXE/idiomas/duplicados/permisos/limites/lector
incompleto. Linux104/104,Windows117/117; consumidores instalados ambos recompilados/
relinkados/ejecutados exit0, sin warnings finales. Nuevo flag SampleOptions0.2:
recompilar TODOS; sin cambio StartupEntry/ABI virtual/historial. No universalidad
vendor/Flatpak/Snap ni cierre I07/K03/F03/N03. C49/E15/P8/B3,49/75=65,3%,26/75=34,7%.
Espejo33 confirmado: comentario90140270529005; titulo/estados remotos sin cambios.

Unidad anterior revision32:E05 E->C, positivo archivo swap REAL activo autorizado
64MiB WSL/ext4. /proc/swaps65532KiB/used0 -> sdd -> lector/informe nativos ->
Monitor true estimated. swapoff/unlink/rmdir exitosos, swap previo intacto,
sin fstab/config permanente/utilidades externas. LINUX_SWAP_DISKS.md y prueba
manual linux_swap_active_native.cpp fuera de CTest, flag obligatorio/guard77.
Sin cambio API/layout/historial; no soporte universal XFS/Btrfs/F2FS/RAID/LVM ni
identidad kernel-inode. Linux102/102,Windows116/116, consumidores ambos exit0,
sin warnings. C48/E16/P8/B3,48/75=64,0%,27/75=36,0%. Espejo32 confirmado:
comentario90140270527425, estados/titulo remotos sin cambios.
F03 integracion diferida y riesgos N03 sin cambios.

Unidad anterior revision31:J06 P->C, integracion Manage user accounts
para el GUI consumidor. AccountManagementIntegration query/open explicitos,
capacidad reconsultada y recibo host_invoked ante errores/excepciones. Sin
provider no ejecuta nada; no gestor nativo/panel universal/CRUD/elevacion ni
autodeteccion de comandos. Host aporta vista/gestor, codigo de confianza.
ACCOUNT_MANAGEMENT.md. Positivo publico hijo privado ambos sistemas, no UI de
cuentas real. Linux102/102,Windows116/116; consumidores instalados recompilados/
relinkados/ejecutados exit0; sin warnings. API adicional sin ABI virtual/layout
existente/historial nuevos. C47/E17/P8/B3,47/75=62,7%,28/75=37,3%; solo J06 P->C,
sin IDs/fracciones nuevos. F03/J04/N03 abiertos. Espejo31 confirmado:
comentario90140270525751, estados/titulo remotos sin cambios.

Unidad anterior trabajada revision30:F03 E->E, identidad TGID/start_boot/UID y
guardas privadas de escritores/secuencia/overflow sticky. Native autorizada
positiva con threads mismo lider, identidad erronea/overflow inyectado rechazados,
hijo excluido/retirada. TCP6144/6144,UDP384/384,12peeks;101/101 Linux,115/115 Windows,
consumidores ambos exit0, sin warnings. LINUX_NETWORK_QUALITY.md. Rechazo por
solapamiento probado en fixtures, no observado en vivo (10coherentes/0ocupadas).
Sin productor integrado/netns/join/API publica/ABI/historial; F03 E y J04/H04
abiertos, N03 conservado.46/75=61,3%,29/75=38,7% sin cambio. Usuario pide no
alargar etapas: diferir integracion F03 y pasar a otro ID. Espejo30 confirmado:
comentario90140270524925, estados/titulo remotos sin cambios.

Unidad anterior trabajada revision29:F03 E->E, clasificacion privada de4lanes
TCP/UDP por socket/familia/protocolo BTF (inet{,6}_sendmsg/sock_recvmsg), no
puerto/nombre. Fallos de lectura y recepciones PEEK/TRUNC/ERRQUEUE aparte.
Native autorizada send/sendto+sendmsg IPv4/IPv6:TCP4096/4096,UDP256/256,0fallos,
8peeks excluidos. Hijo excluido y retirada3enlaces comprobados, exit0; legacy
1088/1088 sigue positivo. LINUX_NETWORK_CLASSIFIED.md. Linux101/101,Windows115/115,
consumidores instalados ambos exit0; builds sin warnings. Root temporal sin pin/
config permanente/utilidades instaladas. Prueba manual, nunca CTest con sondas.
No API publica/ABI/historial ni productor completo UID/start_boot/netns/calidad/
saturacion/coherencia/integracion. F03 E, J04/H04 abiertos, legacy red ausente,
N03 sigue.46/75=61,3%,29/75=38,7% abiertas sin IDs/fracciones nuevos.
Continuar F03 identidad/calidad/integracion. Espejo29 confirmado:
comentario90140270524004, estados/titulo remotos sin cambios.

Unidad anterior trabajada revision28:F03 E->E, prueba privada eBPF autorizada
positiva. Generador18insns, TGID del kernel verificado por TEST_RUN no adjuntado,
filtro antes de leer contexto/mapa; fexit __sys_sendto/sock_recvmsg firmas BTF.
TCP1024+UDP64 send/receive=1088/1088, fases comprobadas; hijo excluido no suma,
trafico tras cerrar enlaces no suma. Root temporal sin pin/config permanente,
FDs RAII y flag explicito; no prueba nativa en CTest. LINUX_NETWORK_PROBE.md.
Linux100/100,Windows114/114; consumidores instalados ambos exit0. Fallos iniciales
conservados y corregidos: helper14 no permitido en SOCKET_FILTER, sock_sendmsg
no ve send/sendto y __sock_sendmsg ausente BTF. Ahora hook sendto6args/slot48.
NO productor de produccion (faltan4lanes/UID/start_boot/netns/perdidas/coverage/
saturacion y otros caminos), ni integracion con reductor/Monitor/legacy/historial.
F03 E, J04/H04 abiertos y N03 sigue.46/75=61,3%,29/75=38,7% abiertas sin IDs/
fracciones. Continuar F03 productor/cobertura/identidades. Espejo28 confirmado:
comentario90140270523263, estados/titulo remotos sin cambios.

Unidad anterior trabajada revision27:F03 E->E, reductor interno de contadores a
tasas TCP/UDP/sumas UID con cobertura. Identidad actor/ambito, ventanas, perdidas,
reset/overflow/ausencia conservadores y aritmetica entera exacta.10000casos de
oraculo portable, no prueba de captura. LINUX_NETWORK_ACCOUNTING.md.
Linux99/99,Windows113/113 finales; consumidores instalados ambos exit0.
Windows inicial sandbox98/113 (15fallos de entorno/acceso) y repeticion fuera
113/113 sin debilitar pruebas; logs inicial/final conservados.
Sin API publica/ABI/historial nuevos ni productor/cargador/sondas implementados.
No root/attach/cambios de privilegios en esta unidad. Autorizacion explicita
recibida2026-10-18 para futura prueba temporal WSL solo procesos privados,
privilegios necesarios, limpieza final, sin configuracion permanente ni instalar
herramientas. F03 E, J04/H04 abiertos, legacy red ausente y N03 sigue.
C46/E17/P9/B3,46/75=61,3%,29/75=38,7% abiertas sin IDs/fracciones nuevos.
Continuar F03 productor/cargador/integracion/pruebas autorizadas. Espejo27
confirmado: comentario90140270522304, sin cambiar titulo/estados remotos.

Unidad anterior trabajada revision26:F03 E->E, preparacion BTF interna read-only.
Valida firmas/offsets del kernel sin constantes ni utilidades externas; no hay
cargador/sondas/mapas/eventos ni contabilidad TCP/UDP nuevos. LINUX_NETWORK_BTF.md.
Linux98/98 (BTF nativo Passed),Windows112/112; consumidores instalados ambos
relinkados/ejecutados exit0. Fixture inicial corregido sin debilitar parser;
logs fallidos y finales conservados. Sin API publica/ABI/historial nuevos.
Autorizacion para futura prueba de sondas privadas solicitada, aun sin respuesta;
ninguna elevacion/carga/attach realizada. F03 E, J04/H04 abiertos y legacy red
ausente. C46/E17/P9/B3 y46/75=61,3% sin IDs/fracciones;29/75=38,7% abiertas.
Continuar F03 cargador/contabilidad/pruebas autorizadas. Espejo26 confirmado:
comentario90140270363748, sin modificar estados/titulo remotos.
Riesgos previos N03 conservados; suites verdes no demuestran su correccion.

Unidad anterior trabajada revision25:F03 E->E, consulta TCP de holder seleccionado
por PID/start_ticks/FD. pidfd_getfd externo/self F_DUPFD_CLOEXEC, valida procfs/
proyeccion Pid de fdinfo, identidad/vigencia y cookie/dev/inode del slot en ambos
extremos. Sampler reinicia por cambio/ausencia/reset; no FDs retenidos entre ticks.
No fallback proc-fd/inode, elevacion ni cambios de datos/flags; dup prolonga
brevemente vida del socket. Holder NO prueba origen exclusivo/causal; alias o
PIDs con misma cookie no son sumandos independientes. LINUX_TCP_HOLDERS.md.
Linux96/96,Windows111/111 y ambos consumidores instalados recompilados/relinkados
exit0. Nativo8192B ACKed/recibidos, hijo privado pidfd_getfd positivo misma cookie
padre, FD reemplazado/token invalido/UDP/listener y datos/F_GETFL verificados.
Sin layout existente/ABI virtual/historial nuevos. F03 sigue E por atribucion
causal/exclusiva, UDP/otras coberturas e integracion; J04/H04 abiertos y legacy
red ausente.46/75=61,3% sin nuevo ID/fracciones, conteo igual C46/E17/P9/B3.
Continuar F03; espejo25 confirmado: comentario90140270363050. Riesgos previos N03 conservados.

Unidad anterior trabajada revision24:F03 P->E, LinuxTcpSampler opt-in nativo
SOCK_DIAG/TCP_INFO directo. TCP ESTABLISHED/CLOSE_WAIT, contadores/tasas con
cookie/inode/familia/socket UID y namespaces del hilo; sumas parciales estimated
por UID DEL SOCKET con cobertura/overflow. No UID real/PID/app/red total ni
UDP/QUIC; no integracion legacy de red ni cierre J04/H04. LINUX_TCP.md.
Linux94/94,Windows110/110 sin skip TCP nativo, consumidores instalados ambos
recompilados/relinkados exit0. Loopback privado8192B reconocidos/8192B recibidos,
2sockets/1UID; no benchmark, modificacion de conexiones ajenas ni captura de
contenido. Sin CLI/eBPF/elevacion ni layout existente/ABI virtual/historial nuevos.
F03 sigue E por atribucion PID/app/sockets compartidos, UDP/otras coberturas y
proveedor/integracion de ventanas.46/75=61,3% sin IDs nuevos/fracciones. Proxima
unidad continuar F03; espejo24 confirmado: comentario90140270361819. Riesgos previos N03 conservados.

Unidad anterior trabajada revision23:J04 P->E, disco/identidad/cobertura por UID
numerico, no username. LinuxUserIoDetails separa alcance CPU/RAM y tipa cobertura
known/missing/permisos/identidad/overflow/entradas sin UID. D02 conserva tasa por
proceso; disk_rate_real_uid exige UID coincidente alrededor de lecturas y entre
extremos del intervalo, sin inventar propiedad causal exacta. Alias UID comparten
total y UID distinto no se mezcla aunque coincida username. utmp sin UID sin I/O.
LINUX_USER_IO.md. Red F03 pendiente: J04 NO cerrada. Linux92/92,Windows109/109 y
consumidores instalados recompilados/relinkados exit0. Nativo UID1000:8779070B/s,
4conocidas/2ausentes tras archivo privado;1informe publico con procesos ocultos.
Nuevo layout ProcessInfo/UserSession/SystemSnapshot0.x: recompilar TODOS. Sin
ABI virtual/historial cambiados.46/75=61,3% se mantiene, sin IDs nuevos/fracciones.
Dependencia F03 iniciada revision24; espejo23 confirmado: comentario90140270360992. Limites y riesgos
previos conservados, sin aceptar implicitamente reducciones de alcance.

Ultima unidad cerrada revision22:K04, inventario/abrir detalles/control explicitos
del manager systemd del usuario efectivo. API LinuxServiceTarget/Inventory/View
con ambito system/current_user, UID y owner de observacion. Bus fijo del euid,
peer UID/owner UID verificados, destino/sender fijados y owner revalidado, sin
GUI/shell/elevacion/fallback a sistema. Abrir no carga/inicia unidades; control
Start/Stop/Restart usa corredor K02 y job confirmado con outcome unknown cuando
corresponde. LINUX_USER_SERVICES.md. Linux91/91,Windows108/108 y ambos consumidores
instalados recompilados/relinkados/ejecutados exit0. WSL solo lectura199sistema/
19usuario y detalles ambos; controles nuevos positivos simulados, no servicios
reales modificados. Sin layout existente/ABI virtual cambiado.45/75 a46/75=61,3%,
29abiertas=38,7%. K03/L05/N01/N02 y limites/riesgos previos separados. Propuesta
J04 iniciada en revision23. Espejo22 confirmado: comentario90140270360264; sin nuevo ID ni fracciones.

Unidad anterior cerrada revision21:G06, auditoria/correcciones DRM/sysfs/NVML.
Lector acotado por campo, identidad/bindings, modulo real/version no inventada;
VRAM/margen estimated y GTT separados en LinuxGpuSysfsDetails, sensores con
etiqueta, no shared Windows fabricado ni presupuesto como libre global.
Fusion NVML con PCI available/tuple completo coherente, aliases usados juntos,
linux_gpu_inventory_source distingue vacio/ausencia/parcial/opt-out.
LINUX_GPU_TELEMETRY.md. Linux89/89,Windows107/107 y consumidores instalados
recompilados/relinkados/ejecutados exit0. Primer test de temperatura corregido
para exigir -0.001C al leer -1miligrado; bytes/porcentaje siguen rechazados,
logs preservados. WSL0filas y DRM estimated/NVML ausente; positivos fixtures,
no validacion fisica. G07/N02 y M04 separados. Nuevo layout GpuInfo/
SystemSnapshot0.x: recompilar TODOS. G06 E->C mueve44/75 a45/75=60,0%.
Sin nuevo ID/denominador ni fracciones. L03 B y C03/E05/E06/riesgos N03 conservados.
Propuesta K04 realizada en revision22; espejo21 confirmado: comentario90140270359672.

Unidad cerrada revision20:G08, politica de fuentes/ausencia motivada de
fecha de driver Linux. NO se obtuvo fecha real; criterio original admite
indisponibilidad motivada. driver_date ausente/unsupported con motivo DRM/sysfs,
NVML o fusion por PCI, motivo antes perdido al fusionar. No BIOS/timestamps/
paquetes/build/kernel/API/DRMplaceholder0 como fecha de publicacion/instalacion.
LINUX_GPU_DRIVER_DATE.md. Linux88/88,Windows106/106 y consumidores instalados
reconstruidos/relinkados/ejecutados exit0; WSL0filas GPU, fixtures no positivos
fisicos. Sin cambio API/ABI/layout/formato ni nuevas consultas GPU/dependencias.
G08 P->C mueve43/75 a44/75=58,7%, sin nuevo ID/denominador/credito parcial.
G06/G07/N02/M04 y L03 B conservan pendientes; C03/E05/E06 y riesgos N03 abiertos.
Propuesta siguiente G06 auditoria de campos GPU no iniciada; espejo20 confirmado: comentario90140270359048.

Unidad cerrada revision19:G05, EGL/GLX actual por PCI y sondeo de perfiles
declarados. LinuxOpenGlDetails separa version de contexto de maximos core/
compatibility y flags por Metric. GLX directo/Mesa exige interop ABI1, IDs/
aceleracion/sysfs, extensiones y contexto/display/screen antes/despues; no
atribucion por modelo/orden ni software/ambiguo. LINUX_CURRENT_GLX.md.
Linux87/87,Windows105/105; consumidores instalados recompilados/relinkados/
ejecutados exit0.4pruebas especificas EGL/GLX con10repeticiones cada una(40).
Fixture GLX dinamico y lector PCI privado positivos/rechazos; WSL contexto GLX
ausente,0atribuciones. No hardware positivo ni sondeo headless/creacion de
contextos afirmados; G07/N02 fisicos y M04 Windows conservan alcances.
Nuevo layout GpuInfo0.x: recompilar TODOS. G05 E->C mueve42/75 a43/75,57,3%.
Sin nuevo ID/denominador ni credito parcial; L03 sigue B y C03/E05/E06/riesgos
N03 sin aceptaciones/cierres asumidos. Propuesta siguiente G08 no iniciada.
Espejo revision19 confirmado: comentario90140270358672.

En revision18:L03 E->B por decision de alcance/arquitectura pendiente. Ver
[LINUX_EFFICIENCY_DECISION.md](LINUX_EFFICIENCY_DECISION.md): no sustitucion
directa documentada de writes numericos por pidfd; repetir listas no prueba
cobertura universal. Pendiente aceptar explicitamente proveedor limitado o
elegir diseno adicional cooperativo/invasivo; no se introduce sin decision.
Revision exclusivamente documental, sin codigo ni pruebas nuevas. C42/E17/P13/B3,
42/75=56,0%, sin nuevos IDs/credito fraccionario. Espejo18 confirmado: comentario90140270357868.

Unidad verificada en revision17:L03 parcial, API opt-in unchecked para threads
capturados y recibo de restauracion exacta de nice/ioprio. Flags pendientes
conservados tras errores, conflictos observados no sobrescritos, sin rollback
automatico ni elevacion. LINUX_EFFICIENCY.md. Linux85/85,Windows105/105 y ambos
consumidores instalados reconstruidos/ejecutados exit0;20/20 repeticiones Linux.
Nativo solo hijos privados de2threads: E/S original restaurada y CPU-denied13
explicito con recibo pendiente. Caller intacto e hijos recolectados.
En revision17 L03 seguia E (pasa B en revision18): no identidad pidfd-safe ni seguimiento de threads futuros; no
cerrar el criterio checked con comparaciones antes/despues. API aditiva sin
layout/virtual existente cambiado. Conteo permanece42/75=56,0%, sin nuevo ID
ni credito fraccionario. Espejo revision17 confirmado: comentario90140270357119.

Unidad cerrada:K01 en revision16, inventario/estado/metadatos de servicios systemd.
LinuxServiceDetails separa disponibilidad por campo, proveedor y agrupacion Slice;
group refleja Slice o vacio, no etiqueta systemd ni grupo Windows/Unix. MainPID0
conocido distinto de permiso/ausencia, descripcion/Slice vacias conocidas. Unit.Id
antes/despues de propiedades y owner unico verificados; inventario de manager
distinto de fallback parcial de archivos, sin metadata efectiva inventada.
LINUX_SERVICE_METADATA.md; Linux85/85,Windows105/105 y consumidores instalados
recompilados/relinkados/ejecutados exit0. Nativo199servicios,117MainPID conocidos
incluidos0,117Slice conocidos(15vacios), sin crear/controlar servicios reales.
Nuevo layout ServiceInfo/SystemSnapshot0.x: recompilar TODOS. No garantia de
snapshot atomico/PID persistente ni grupos Windows; K03/K04/L05/N01/N02 mantienen
alcances. K01 E->C mueve41/75 a42/75, sin nuevo ID ni cambio del denominador.
C03/E05/E06 y riesgos CPU/historial N03 no cerrados ni decisiones asumidas.
Propuesta L03 de revision16 iniciada en revision17; cierre checked/alcance
completo sigue pendiente, sin reducir el criterio ni cambiar el denominador.

Unidad cerrada en revision15:J01, inventario/estado de sesiones y agregados visibles.
LinuxSessionDetails separa State/Active/Remote/UID con Metric; no equivalencia
WTS connected/disconnected ni actividad inventada de utmp. Id/Name/User y timestamp
antes/despues verificados, owner final comprobado, campos unknown/sin token si
identidad no verificable. Fuente publica distingue enumeracion vacia conocida de
fuente ausente/desactivada, no certifica cobertura total del host.
LINUX_SESSION_STATE.md; Linux85/85,Windows105/105 y consumidores instalados
recompilados/ejecutados exit0. WSL1sesion/1State/1flag foreground conocidos,
utmp privado3registros/vacio/ausente, sin acciones sobre sesiones reales.
Nuevo layout UserSession/SystemSnapshot0.x: recompilar TODOS. CPU/RAM siguen
sumas estimadas visibles por username repetidas entre sesiones, no totales unicos
del host. J04/J05/J06 y N02 mantienen alcances. J01 E->C mueve40/75 a41/75,
sin nuevos IDs ni credito fraccionario. C03/E05/E06 no cerradas ni consentimientos
asumidos; riesgos CPU/historial N03 siguen pendientes. Propuesta K01 de esa
revision fue implementada/cerrada posteriormente en revision16.

Unidad cerrada en revision12:A01, modelo Metric y reemplazo documentado del historial
ambiguo. AppHistorySample/app_history_view con soporte/notas/origen y unidades,
metric_is_known distingue cero/false medido de ausencia. Importacion legacy
conservadora: todos los contadores quedan unknown, ni positivos prueban soporte.
LegacyAppHistoryEntry es alias compatible del payload; v1/v2 no cambian, no
conversion implicita de mediciones a legacy. Ver METRIC_AVAILABILITY.md.
Validacion de A01 en revision12:Linux83/83,Windows103/103 aprobadas;
paquetes reinstalados y consumidores recompilados/ejecutados exit0. API aditiva,
sin nuevo layout de estructuras existentes. A01 no cierra proveedores faltantes,
auditoria de cada campo o API1.0; A05/H04/H05/M08/N03 mantienen sus pendientes.
Causa del fallo CPU global previo sigue N03: ahora notas precisas de rechazo
compartidas por calculo/guardia y salida de diagnostico antes de la asercion
original. Ninguna formula/asercion debilitada. Fixtures de cada motivo y100/100
repeticiones nativas aprobadas, sin reproducir el fallo ni demostrar su causa.
N03 P->E parcial, NO cerrada; ver LINUX_CPU_USAGE.md. Suites83/83 y103/103 y
consumidores instalados relinkados exit0 comprobados nuevamente en revision13.
E05 sigue E por archivo swap activo real pendiente,
sin activar swap para forzar cierre. Ver LINUX_SWAP_DISKS.md.
C03 permanece E, no cerrada: falta proveedor de reserva exclusiva del hardware
o aceptacion explicita de la estimacion; prosigue no se interpreta como aceptacion.
El cierre A01 movio39/75 a40/75 en revision12. Revisiones13/14 mantienen40/75;
sin nuevas tareas ni cambio del denominador ni credito fraccionario por N03/E06.
Unidad de revision14 E06 P->E: LinuxDiskDetails separa protocolo/transporte/medio/RPM,
SAT89/B1/VPD83/NVMe transport con identidad/limites/comparacion de metadatos.
Fuentes reportadas, no inferencias de nombres o rotational. Ver LINUX_DISK_IDENTITY.md.
Release final revision14:Linux84/84,Windows104/104, consumidores instalados
recompilados/ejecutados exit0. NUEVO layout DiskInfo/SystemSnapshot0.x; recompilar
TODOS. WSL4contextos fijados/0medios; positivos SATA/SSD/HDD/SAS/NVMe son fixtures.
E06 sigue E: medio exacto NVMe/SCSI non-rotating/backing oculto o aceptacion
explicita del limite reportado/unknown pendiente. Pregunta enviada, sin respuesta
asumida. CPU preparado para capturar reincidencia, no declarado resuelto.
Primera suite Linux83/84 fallo por informe de historial ausente, con discos
desactivados. Logs preservados; nota anadida antes de asercion, luego3/3 y84/84
sin cambiar logica/aserciones. Causa pendiente N03, no atribuida al CPU/E06.
E05 requiere entorno con archivo swap activo para cerrar su verificacion.

El TXT incluye una matriz R01..R20 que vincula el pedido con tareas activas;
los requisitos R no son tareas extra. Conserva límites de firmware/kernel y
distingue pruebas sintéticas de hardware. La auditoría del control no certifica
ausencia de bugs ni cierra N01/N02/G07/N03.

Cuando ClickUp esté disponible, sincronizar desde el TXT preservando cambios
ajenos y resolviendo conflictos. Un error o cuota remota nunca bloquea la
actualización del control oficial.

Espejo revision12 confirmado2026-10-16, comentario90140269990865,40/75=53,3%.
Espejo revision13 confirmado2026-10-16, comentario90140269993090, mismos40/75.
Espejo revision14 confirmado2026-10-16, comentario90140270001412, mismos40/75.
Espejo revision15 confirmado2026-10-16, comentario90140270005439,41/75=54,7%.
Espejo revision16 confirmado2026-10-17, comentario90140270356361,42/75=56,0%.
No se alteraron estados remotos ni se dio por conciliada la lista antigua.
Su titulo60% sigue siendo historico, no oficial.
