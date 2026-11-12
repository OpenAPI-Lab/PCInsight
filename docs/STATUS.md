# Estado real de implementación

Para el alcance de la entrega pública, empieza por [SUPPORT_STATUS.md](SUPPORT_STATUS.md).
Los checkpoints siguientes son evidencia histórica; no convierten todos los
campos o proveedores en completos ni representan CI remota.

Este documento evita confundir un campo definido en la API con una métrica ya implementada.

Checkpoint172: [verificador de hardware Linux](LINUX_HARDWARE_EVIDENCE.md)
contra SDK instalado 0.2.0; prueba sintética y WSL 10/10 cada una, regresión
Linux 200/200. N02/G07 siguen bloqueadas por falta de evidencia física.
Oficial 60/75=80%. Espejo ClickUp: 90140278768612.

Checkpoint171: [evidencia firmware](LINUX_FIRMWARE_VIRTUALIZATION_EVIDENCE.md),
B08 E→E. WSL sin rutas OEM/KVM sondeadas; 10/10 focales, sin positivo físico
ni código nuevo. Oficial 60/75=80%. Espejo ClickUp: 90140278765329.

Checkpoint170: [CI Linux](LINUX_CI.md) N01 P→E. Ensayo local de la etapa de
verificación Release: Linux200/200, entrega12/12, marcador1/1 y rechazos
controlados. Windows intacto, sin push ni CI remota. Oficial60/75=80%.
Espejo ClickUp: 90140278762897.

Checkpoint169: [política del SDK](SDK_VERSION_POLICY.md) y manifiesto Linux de
54 archivos verificados; rechazo del digest alterado, marcador1/1, baseline3/3,
Linux200/200. A05 E sin declaración ABI1.0 ni cambios de biblioteca/versionado.
Siguiente revisión CI Linux N01. Oficial 60/75=80%. Espejo ClickUp: 90140278761934.

Checkpoint168: [línea base Linux](LINUX_ABI_BASELINE.md) A05 E→E. Sonda SDK
instalado: 15 firmas/tipos, 28 tamaños/alineaciones, perfil GCC13.3.0/libstdc++
20240904. Tres pruebas/30 recorridos y Linux200/200. Sin cambio de biblioteca
ni declaración de ABI1.0. Oficial 60/75=80%. Espejo ClickUp: 90140278760856.

Checkpoint167: [marcador de headers](SDK_HEADER_COMPATIBILITY.md), A05 P→E.
Linux/MSVC SDK coincidencia exit0 y rechazo de símbolo alternativo tras
compilación; archivo Linux anterior rechazado, g++ directo exit0 y Linux200/200.
SDK 0.2.0; sin cambios de layout/vtable/formatos ni garantía ABI1.0.
Control oficial 60/75=80%. Espejo ClickUp: 90140278759496.

Checkpoint166: [auditoría de entrega Linux](LINUX_DELIVERY_AUDIT.md) R01–R20.
SDK dedicado: 53 archivos, aceptación exacta 0.2.0/rechazo 0.2.1, seis ejemplos
12/12 y enlace directo g++ exit0. Linux200/200. N03 global abierto; siguiente
revisión Linux de compatibilidad A05, no ABI1.0. Oficial 60/75=80%.
Espejo ClickUp: 90140278755570.

Checkpoint165: [Servicios Linux](LINUX_SERVICES_REQUIREMENTS.md) trazados
por proveedor; ejemplo SDK solo lectura con inventario systemd sin fallback
y consulta de detalles. Linux200/200+20 focales, MSVC10 portables, 25 enlaces.
Sin cambio biblioteca/ABI. N03 E, sigue auditoría de entrega Linux. K03 y L05
no cerrados; control SysV pendiente de autorización. Oficial 60/75=80%.
Consumidor SDK: self-test y muestra exit0, 199 filas systemd sin fallback.
Espejo ClickUp: 90140278753524.

Checkpoint164: [Usuarios y sesiones Linux](LINUX_USERS_REQUIREMENTS.md)
trazados con scopes separados CPU/RAM, disco y red parcial; ejemplo SDK
sin acciones ni captura. Linux198/198+20 focales, MSVC10 portables, 20 enlaces.
Sin cambio de biblioteca/ABI. N03 E, sigue Services; oficial 60/75=80%.
Consumidor SDK self-test/muestra exit0. Espejo ClickUp: 90140278752093.

Checkpoint163: [Startup Apps Linux](LINUX_STARTUP_REQUIREMENTS.md) trazado:
configuración, elegibilidad e impacto separados; ejemplo de inventario y
Properties sin mostrar valores crudos ni ejecutar acciones. Linux196/196+20
focales, MSVC10 portables, 29 enlaces. I07 abierto y ABI intacta. N03 E; sigue
Users. Avance oficial 60/75=80%, 15/75 abiertas.
Consumidor SDK: self-test y muestra exit0. Espejo ClickUp: 90140278750348.

Checkpoint162: [App history Linux](LINUX_APP_HISTORY_REQUIREMENTS.md) trazado
con ejemplo SDK de CPU/red/notificaciones separadas y legado sin contadores
medidos. No activa BPF/observador ni escribe historial. Linux194/194+20 focales,
MSVC10 portables y 16 enlaces verificados. N03 E, siguiente Startup. Sin cambios
de biblioteca/ABI/formatos; control oficial 60/75=80%, 15/75 abiertas.
Consumidor del SDK instalado: self-test y muestra exit0. Espejo ClickUp: 90140278748341.

Checkpoint161: [Processes/Details y acciones Linux](LINUX_PROCESS_REQUIREMENTS.md)
trazados con ejemplo SDK solo lectura, salida SELF e informe de red parcial
separado por bucket/lane. Run new task, End task y Efficiency documentados,
no invocados por el ejemplo. Linux192/192+40 focales, MSVC20 portable. N03 sigue
abierto; App history es el siguiente bloque. Sin cambio de ABI/biblioteca ni
autorización SysV inferida. Conteo oficial 60/75=80%, 15/75 abiertas.
Consumidor SDK self-test y muestra SELF exit0. Espejo ClickUp: 90140278746290.

Checkpoint160: [contrato Performance Linux](LINUX_PERFORMANCE_REQUIREMENTS.md)
terminado: campos públicos/unidades/soporte/test/límites CPU/RAM/disco/red/GPU,
ventana60s y diferencias Windows explícitas. Ejemplo linux_performance.cpp solo
SDK, ayuda por defecto, --self-test puro, --sample dos lecturas, sin acciones ni
captura. Desconocido distinto de0/false, NaN/permisos/padres/zram vacío/motores
verificados. Linux190/190+20focales,MSVC10,consumer SDK instalado/relinkado con
selftest/sample0,65 enlaces comprobados/builds sin warnings de código. Logs
work/performance-contract-*. Biblioteca productiva/layouts intactos. N03 E:
faltan trazabilidad otros bloques/acciones, entrega/auditoría y riesgos históricos
NO corregidos. G07/N02/B08 y K03 aceptación real siguen; SysV sin permiso explícito.
L05/I07/Windows conservados; no instalación/elevación/serviciosreales/eBPF/CIclaim.
C60/E10/P3/B2+3X=80%/20%, TXT autoridad, sin IDs/crédito parcial.
Espejo160 confirmado: comentario90140276895356; título remoto60% histórico.

Checkpoint159: [API pública OpenRC](LINUX_OPENRC_CONTROL_PUBLIC.md) instalada:
LinuxOpenrcService open/request/close y Runner wait/terminate_runner/PID diagnóstico.
Selección conserva FD/metadata/directorios/contexto UID-GID; revoca cambios, exige
consentimiento, consume al devolver hijo; sin reabrir otro destino/retry automático.
Salida/ACK NO estado del servicio. Default-off, lector/Monitor/SystemActions intactos.
Linux188/188 +50 OpenRC, MSVC20 portable, SDK3 consumidores relinkados0 (nuevo
consumer SOLO SDK instalado), FD/limpieza/builds finales correctos. D9025 inicial
UNDEBUG esperado, sin warnings de código finales. Modelo privado NO OpenRC real,
sin instalar herramientas/elevación/servicios reales/CIclaim. K03 E: integración
pública terminada; aceptación real pendiente. Sondeo read-only PATH no localiza
runit/s6/OpenRC, no ausencia global; consulta SysV emitida, permiso aún NO concedido.
L05 despliegue aplazado/I07/Windows intactos. C60/E10/P3/B2+3X=80%/20%; TXT autoridad.
Espejo159 confirmado: comentario90140276888517; título remoto60% histórico.

Checkpoint158: [control OpenRC interno](LINUX_OPENRC_CONTROL.md) implementado
opt-in/OFF, solo start/stop/restart explícitos. Runtime ELF fijado, script/rutas
protegidas y revalidadas, entorno/FD cerrados, gating/pidfd, ACK y waitid(P_PIDFD),
salida/timeout y cancelación explícita solo del runner. No equivale a estado vivo
ni stop del servicio. Pruebas con modelo privado, nunca OpenRC/servicios reales.
Linux187/187 +40 recorridos OpenRC; MSVC10 portable, SDK2consumidores relinkados0,
FD/limpieza/builds finales correctos. Primeros fallos compilación/clockskew en logs
preservados, entrega limpia. No API pública/Monitor/SystemActions nuevos ni install
de dependencias/elevación/CIclaim. K03 E: siguiente integración pública con selección
y consentimiento, luego aceptación real según entorno/permisos. L05 despliegue E
aplazado, no canal Unix nuevo; SysV/I07 conservados. TXT autoridad, C60/E10/P3/B2+3X,
80% cerrado/20% abierto, sin IDs ni crédito parcial. Checkpoints anteriores históricos.
Espejo158 confirmado: comentario90140276877977; título remoto60% histórico.

Checkpoint157: [frontera de transporte](LINUX_PRIVILEGED_TRANSPORT.md) probada:
perfil SOLO TESTS en broker privado real limita mensaje32KiB, rechaza string64KiB
sin entrar al handler y sigue atendiendo otro cliente.3intentos/2entradas/2auth/
0Manager. Los límites son del broker, NO por helper; no instalar en bus compartido.
CuotasFD0 iniciales bloquearon auth; fallo preservado y fixture positivo corregido.
Linux186+160L05/30brokers, MSVC10args/SDK2consumer0/FDcleanup/buildsLinuxclean.
Biblioteca de producción sin cambios/APIlayoutformatos públicos/Monitor/SystemActions
intactos. NO transporte productivo nuevo/instalación/elevación/serviciosreales/
polkitreal/aceptación/CIclaim. L05 E: decisión pedida, todavía pendiente, entre
aplazar despliegue L05 y seguir K03/OpenRC (recomendado), o ampliar a canal Unix
privado. No broker runtime extra ni cambio global del host inferidos. Proceso/
cargador/política-consentimiento/aceptación/UAC pendientes, K03/I07 conservados.
OpenRC148solo control explícito intacto. C60/E10/P3/B2,3X:80%/20%, sin IDs/crédito.
Espejo157 confirmado: comentario90140276862936; título remoto60% histórico.

Checkpoint156: [admisión y presión de colas](LINUX_PRIVILEGED_ADMISSION.md) terminadas.
Bucket por conexión receptora +senderbroker, default64/16s y16/4s, tabla64/idle300s,
crédito entero/mutex/TTL>=recargacompleta/noevicciónactiva; colas read/write64
rechazan trabajo antesRPC. Sin garantía porUID/fairness/globalDoS, queuebyte/pre-
recepción/replies no acotados por este guard. Ambos endpoints comparten admisión;
OFF/busy salen antes. Broker/libsystemd REALES privados, Authority/Manager SIM:
modo5requests2auth1Manager1Sub prueba rechazo sinbudget/check/dispatch y nuevo
sender no evade cuota compartida. Regresión56requests32auth18Manager2Sub usa
perfilalto explícito, no default cambiado. Linux185+150L05/20brokers, MSVC20
admissionargs/SDK2consumer0/FDcleanup/buildsLinuxclean. L05 E: contrato transporte/
despliegue y aceptación reales pendientes; sin install/helperactivado/elevación/
serviciosreales/polkitreal/APIlayoutformatos públicos/Monitor/SystemActions/CIclaim.
OpenRC148solo controlexplícito intacto, SysV/K03/I07/UAC conservados.
C60/E10/P3/B2,3X:80%/20%; sin nuevosIDs/crédito parcial. TXT autoridad.
Espejo156 confirmado: comentario90140276857313; título remoto60% histórico.

Checkpoint155: [presupuesto por solicitud](LINUX_PRIVILEGED_BUDGET.md) terminado:
steadyclock/mutex/1ms..30s/1..128RPC instrumentadas (default5s96), misma instancia
en identity/daemon/Authority/Manager/match/Subscribe/dispatch, RAII timeout default
restaurado y explicitcall=minremaining. Owned auth-only también5s96; legacyraw
sin budget conserva contrato previo. Cooperativo NO deadline absoluto ni rate/
queue/byte limit de transporte; timeoutpostattempt UNKNOWN PUDO actuar/no retry.
Broker/libsystemd REALESprivados, Authority/Manager SIM:56req32auth18Manager2Sub,
retenciones antes/después de envío prueban retorno previo a liberarlas; restorenative
2s porrequest. Linux183+130L05/10broker, MSVC20budgetargs/SDK2consumer0/FDcleanup.
Clockskew inicial preservado, finalbuildclean/suitefinal183 sin tocarreloj/mtime.
L05 E siguiente admisión/transporte-colas; despliegue-proceso-cargador/polkitreal/
aceptación/UAC pendientes. Sin APIlayoutformatos públicos/Monitor/SystemActions/
install/helperactivado/elevación/serviciosreales/CIclaim. OpenRC148controlsolo
explícito intacto, SysV/K03/I07 conservados.
C60/E10/P3/B2,3X:80%/20%; sin nuevosIDs/crédito parcial. TXT autoridad.
Espejo155 confirmado: comentario90140276845885; título remoto60% histórico.

Checkpoint154: [seguimiento asíncrono de jobs](LINUX_PRIVILEGED_JOBS.md) terminado.
Match+Subscribe al owner antes del dispatch, caché por conexión/owner y slot
propiedad de conexión; datos compartidos/snapshot thread-safe sin calls al bus.
uoss/body/sender/bus/ID-pathuint32-unit, early64/event4096/pendingwatches64,
duplicates/conflicts/future-result/deadline1ms..30s, pending/completed/failed/
unknown. Timeout/revocación NO cancelan; completed NO estado actual de servicio.
Broker/libsystemd REALES privados, Authority/Manager SIMULADOS:50requests29auth16
Manager2Subscribe, spoof ignorado/suscripciónfallida sin dispatch/FDcleanup.
Linux182+120repeatL05/10broker, MSVC20FSMargs/SDK2consumer0/buildsLinuxclean.
Sin API/layouts/formatos públicos/Monitor/SystemActions/helperinstalado/activación/
elevación/serviciosreales/polkitreal/aceptación/CIclaim. L05 E: siguiente límites/
deadline agregado del receptor; despliegue-proceso-cargador/polkit reales pendientes.
OpenRC148solo controlexplícito intacto, SysV/K03/I07/UAC conservados.
C60/E10/P3/B2,3X:80%/20%; sin nuevosIDs/crédito parcial. TXT autoridad.
Espejo154 confirmado: comentario90140276834738; título remoto60% histórico.

Checkpoint153: [ejecución cerrada interna](LINUX_PRIVILEGED_EXECUTION.md) OFF defecto.
ControlSystemService separado de auth-only, freshauth/context/gate/policy/conn/
Managerowner-UID revalidados, método start-stop-restart+unit/replace fijos al owner
único; un dispatch autostart0-interactive0/5s, sin retry. Receipt cookie/sender/
signature/path exacto=job_queued NO completion. Fallos/cambios tras intento envío
=dispatch_unknown (PUDO actuar), no rollback/retry ni atomicidad de revocación.
Broker/libsystemd REALES privados, Authority y Manager SIMULADOS;41requests20auth/
8Manager porrecorrido, ninguno servicio real. Linux181+110repeatL05/10broker,
MSVC20defaultsargs/SDK2consumer0/FDcleanup. Clockskew inicial preservado, build
incremental limpio/suitefinal181 sin tocar reloj. Sin APIlayoutformatos/acciones
públicas/helperinstalado/elevación/polkitreal/aceptación/CIclaim. L05 E: siguiente
límites y seguimiento jobs, despliegue/cargador/polkit reales pendientes. OpenRC148
solo controlexplícito intacto, SysV/K03/I07/UAC conservados.
C60/E10/P3/B2,3X:80%/20%; sin nuevosIDs/crédito parcial. TXT autoridad.
Espejo153 confirmado: comentario90140276823908; título remoto60% histórico.

Checkpoint152: [ancla y conexión propias](LINUX_PRIVILEGED_CONNECTION.md).
Ancla absoluta sin symlinks/ancestros inseguros, PID/EUID/mount-user namespaces
del hilo; stickyancestorsOFF defecto (ON explícito solo pruebas/tmp). Conexión
nueva endpoint/UIDnodo/peer separados, Hello/Unix-stream-peercred/CLOEXEC/ready,
fds0/anonymous0/interactive0/exitdisconnect0/timeout<=5s; sin default/envbus/CLI.
Generación por conexión y revocación persistente; receptor unido a su bus con
postrevalidación. Broker/libsystemd REALES privados/Authority SIMULADA,22requests/
9checks: cambio de directorio durante Authpositiva => UNKNOWN/conexión revocada.
Linux180/180+100repeatL05 (10broker), MSVC20args/fallback, SDK2consumer0/FDcleanup.
Warnings iniciales indentación corregidos/preservados, verified/finalbuildclean.
No proceso/cargador endurecidos/helper operativo/polkitreal/ejecutor/instalación/
elevación/serviciosreales/APIlayoutformatos/CIclaim. L05 E: siguiente límites/
flujo cerrado/despliegue antes de aceptación; sin autoridad de activar/instalar.
OpenRC148controlsolo explícito intacto, SysV/K03/I07/UAC conservados.
C60/E10/P3/B2,3X:80%/20%; sin nuevosIDs/crédito parcial. TXT autoridad.
Espejo152 confirmado: comentario90140276816812; título remoto60% histórico.

Checkpoint151: [política protegida L05](LINUX_PRIVILEGED_POLICY.md), parser cerrado
96KiB/256reglas, lectura openat desde ancla/UID confiables, permisos/tipos/enlaces/
rutas y revalidación, sin shell. Almacén con generaciones/refresh invalidante sin
fallback; receptor gestionado descarta invalidez durante identidad/autorización.
Broker/libsystemd privados REALES, Authority SIMULADA:21solicitudes/8checks,
incluye respuesta positiva descartada tras invalidación y recuperación.
Linux179/179+90repeatL05 (10broker), MSVC20 parser/args/fallback, SDK2consumer0,
FDclean/cleanupprivado/buildsLinuxlimpios. Sin API/layout/formatos/acciones nuevos.
No bootstrap seguro de proceso/ancla/cargador, watcher, ejecutor, polkitreal,
instalación/elevación/aceptación/CIclaim; L05 E, siguiente bootstrap/conexión.
OpenRC148 solo control explícito intacto; SysV/K03/I07/UAC conservados.
C60/E10/P3/B2,3X:80%/20%; sin nuevosIDs/crédito parcial. TXT autoridad.
Espejo151 confirmado: comentario90140276807772; título remoto60% histórico.

Checkpoint150: [receptor cerrado L05](LINUX_PRIVILEGED_RECEIVER.md) terminado:
método/path/interfaz/firma exactos, versión/operación/intención/unidad/preflight,
sin campos de identidad del cliente. Transporte libsystemd/broker/UID/owner/
seriales REALES en bus privado sin activación; Authority SIMULADA, no polkitd.
16solicitudes/5checks por recorrido, falsificación sender no suplanta al cliente,
11rechazos sin Authority, start/stop/restart observados y deny conservador.
Linux178/178+80repeatL05 (10broker), MSVC10args/fallback, SDK2consumerrelinked0,
FDclean/cleanup privado; build inicial clockskew preservado/final limpio.
No helper operativo/ejecutor/consentimiento humano demostrado/política real/
instalación/elevación/acciones ni API/layout/formatos/CIclaim nuevos.
L05 E: siguiente config protegida/ciclo de vida, luego ejecutor/aceptación.
OpenRC148 control explícito intacto, SysV/K03/I07/UAC conservados.
C60/E10/P3/B2,3X:80%/20%; sin IDs/crédito parcial. TXT autoridad.
Espejo150 confirmado: comentario90140276799432; título remoto60% histórico.

Checkpoint149: [CheckAuthorization nativo](LINUX_PRIVILEGED_NATIVE_AUTHORIZATION.md)
implementado y unido al coordinador con factory congelada. Preflight/subject-owner/
identitybefore-after/sender-cookie/type-signature/parserbounded64-128-4096-16KiB/
duplicates-trailing rechazos, autostart0/interaccionOFF, sin retry/caché/ejecutor.
Linux176/176+60repeatL05, MSVC10args/fallback, SDK2consumerrelinked0/FDclean.
ABIlibsystemd SIMULADA/socketkernel real privado, NO Authority/brokerreal/cliente
autenticadoreal/helper/receiver/dialogos reales/ejecución/instalación/aceptación.
Clockskew inicial preservado; incremental final limpio/suite176 sin tocar reloj.
Sin API/layout/formatos/Monitor/SystemActions/Windowsfeature/N03fix/CIclaim nuevos.
L05 E: siguiente receptor aislado/payloadcerrado/identidadtransporte real, después
bootstrap/config segura/ejecutor/aceptación. Permiso OpenRC148 SOLO control
explícito intacto, SysV sin extensión; K03/I07/UAC conservados.
C60/E10/P3/B2,3X:80%/20%, sin nuevosIDs/crédito parcial.
Espejo149 confirmado: comentario90140276792674; TXT autoridad.

Checkpoint148 (2026-11-02): [autorización OpenRC vigente](LINUX_OPENRC_CONTROL_DECISION.md).
Usuario aprueba scripts/runtime existentes SOLO start/stop/restart explícitos,
opt-in/OFF/pruebas privadas, nunca datos/instalación/elevación/servicios reales.
NO permiso SysV/helper/polkit/autostart. Retira decisión132, no implementa ejecutor,
valida aceptación ni cierra K03; frases anteriores sinexcepción OpenRC son históricas.
L05 sigue operativa, siguiente CheckAuthorization nativo aislado; al retomar K03
no repetir permiso de desarrollo, pero cada acción requerirá consentimiento.
Sin código/ejecución/tests nuevos por este registro. Evidencia147 sigue histórica.
C60/E10/P3/B2,3X:80%/20%, sin nuevosIDs/crédito parcial/cierres.
Espejo148 confirmado: comentario90140276786160; TXT autoridad.

Checkpoint147: [identidad nativa L05](LINUX_PRIVILEGED_IDENTITY.md) desde mensaje
actual/connectionclientready/address/dest/sender, Unixendpoint/SO_PEERCRED-UIDrouter/
FD y consultas daemon ID/UIDcaller/owner-UIDAuthority/revalidación/contexto. No
identidad de payload/rootUID fijo. Código nativo, validación ABI SIMULADA/socket
kernel real privado: NO broker real/authpolkit/receiver/helper/CheckAuthorization.
Linux174/174+40repeat L05, MSVC10args/fallback, SDK2consumerrelinked0/FDclean,
buildLinux inicial/final limpio. No API/layout/formatos/acciones/diálogos/instalación/
activación/elevación/CIclaim/Windowsfeature/N03fix. L05 E: siguiente consulta nativa
CheckAuthorization subjectfijado/correlación/parsingbounded con pruebas aisladas,
sin ejecutor; config/bootstrap/receiver/integración/aceptación/UAC pendientes.
K03/OpenRC132/I07 intactos; C60/E10/P3/B2,3X:80%/20%, sin nuevosIDs/crédito parcial.
Espejo147 confirmado: comentario90140275959685; TXT autoridad.

Checkpoint146: [coordinador L05](LINUX_PRIVILEGED_AUTHORIZATION.md) interno,
solo fixtures: copia/contexto/checkúnico/revalidación/cookies/denegación/challenge/
retención/errores sin cache. UIDAuthority esperado de configuración del mecanismo,
NO hardcoded0; corregido durante revisión y probado noroot/root/mismatch/ausente.
Bool es aceptación conservadora, no raw is_authorized/ticket. NO adaptador nativo,
identidad autenticada real/polkitreal/helper/IPC/ejecutor/elevación/instalación.
Linux172/172+20repeat, MSVC10pure, SDK2consumerrelinked0/buildLinux inicial-final
limpio. No API/layout/formatos/acciones/CIclaim/Windowsfeature/N03fix nuevos.
L05 E: siguiente adaptador nativo identidad/Authority con transporte verificado y
rechazos privados, antes de conectar ejecutor/helper. K03 aplazada, OpenRC132/I07
intactos/UAC aplazado. C60/E10/P3/B2,3X:80%/20%, sin IDs/crédito parcial.
Espejo146 confirmado: comentario90140275958832; TXT autoridad.

Checkpoint145: [auditoría K03](LINUX_K03_AUDIT.md) terminada: E aplazada por
controlscript noautorizado/aceptación real, inventarios/vistas/Monitor probados
según contrato sin cierre universal. PATH WSL sin rutas de4supervisores por
command-v literales; no instalación/ejecución/ausencia universal.
[L05/Linux](LINUX_PRIVILEGED_PREFLIGHT.md) P->E con preflight privado puro de
start-stop-restart systemdSistema sobre allowlist del mecanismo: exacttarget,
intentdeclared/bound256/invalidcfgdeny/ownedcopy, NO autenticación/autorización/
peer/IPC/polkitgrant/helper/ejecución. Sin layouts/formatos/APIpública nuevos.
Linux171/171+10repeat, MSVC10pure, SDK2consumer relinked0/buildLinux inicial-final
limpio. Siguiente frontera identidad/autorización con rejectfixtures privados,
antes de instalar/activar/ejecutar nada. K03/OpenRC132/I07 intactos, UAC aplazado.
C60/E10/P3/B2,3X:80%/20%, sin nuevosIDs/crédito parcial/Windowsfeature/N03fix/CIclaim.
Espejo145 confirmado: comentario90140275957563; TXT autoridad.

Checkpoint144: [selección de servicios Monitor](LINUX_SERVICE_SELECTION.md),
siete rutas/vistas explícitas sin fallback, argumentos usados guardados antes de
I/O, errores/permisos/switch sin filas viejas; knownempty/partial diferenciados.
legacy_default conserva ruta vieja; disabled no query, OpenRC solo vista elegida
sin namejoin/consulta de otra fuente. Sin nuevas acciones ni rawsource en snapshot.
SampleOptions ABI ampliado: RECOMPILAR TODOS clientes0.x; layout ServiceInfo/
SystemSnapshot/formatos intactos. Linux170/170+10repeat, MSVC10puredispatcher
(NO MonitorWindows), SDK2consumer recompiled/relinked0 inclMonitorLinuxnuevo,
FD/sentinel privados OK. Clockskew inicial /mnt/c preservado; incremental final
limpio/suite170 sin tocar reloj/mtime. Sin exec/scripts/aceptación real/CIclaim.
K03 E: siguiente auditoría de cierre porproveedor/evidencia/bloqueos, pasar L05
cuando proceda, sin prolongar hardware no disponible. OpenRC132/I07 intactos.
C60/E9/P4/B2,3X:80%/20%, sin nuevosIDs/crédito parcial/Windowsfeature/N03fix.
Espejo144 confirmado: comentario90140275956990; TXT autoridad.

Checkpoint143: [OpenRC a ServiceInfo](LINUX_OPENRC_SERVICES.md), dos vistas
configured/marked separadas por fuente/índice, SIN identityjoin por nombre.
IDs distintos/scoped, marked-* Metric estimated NO vida/readiness, PID/desc/
grupo/política UNKNOWN. Partial/permisos/fallos independientes, sourceMetric
originales conservados; no supresión de otra fuente completa ni stopped inventado.
Linux169/169+30repeat OpenRC, MSVC10pure/fallback, SDK2consumer0, FD/sentinel OK,
buildLinux inicial/final limpio. Sin librc/CLI/exec/scripts/creación/aceptación real.
Monitor/default/layout/otrosproveedores intactos. K03 E: siguiente selección
explícita del proveedor en Monitor sin fallback silencioso/nuevas acciones;
integración/control/aceptación pendientes, OpenRC132/I07 intactos.
C60/E9/P4/B2,3X:80%/20%, sin IDs/crédito parcial/Windowsfeature/N03fix/CIclaim.
Espejo143 confirmado: comentario90140275956062; TXT autoridad.

Checkpoint142: [s6 a ServiceInfo](LINUX_S6_SERVICES.md), consulta explícita y
source/evidencia/logger/index conservados. Status record-pid-present/zero y
/finishing/paused Metric estimated NO vida/readiness/controlcapability/authpeer;
PIDu64/waitword/ready solo source, PID común/main_pid UNKNOWN, ready+PID0 válido.
Partial/invalid/permisos UNKNOWN, guards logger/shape/IDs scoped, no grupo/desc/
política inferidos. Linux168/168+50repeat s6, MSVC10pure/fallback, SDK2consumer0,
FD/sentinel/controlsinbytes privados OK. Clockskew inicial /mnt/c <1s conservado;
incremental final limpio/suite168 sin tocar reloj/mtime. Sin Monitor/layout/
contratos133-136 cambiados/exec/scripts/aceptación real. K03 E: siguiente OpenRC,
integración/aceptación pendientes; OpenRC132/I07 intactos. C60/E9/P4/B2,3X:
80%/20%, sin IDs/crédito parcial/Windowsfeature/N03fix/CIclaim/persistencia.
Espejo142 confirmado: comentario90140275955591; TXT autoridad.

Checkpoint141: [runit a ServiceInfo](LINUX_RUNIT_SERVICES.md), consulta explícita,
source/evidencia/filas alineadas de la misma llamada. reported-down/run/finish y
/paused con Metric estimated, NO kernelalive/readiness/authpeer; PID crudo solo
source, PID común/main_pid UNKNOWN. Censo partial/record inválido/permisos =>
statusUNKNOWN, desc/grupo/política no inventados, IDs hex scoped noacciones.
Linux167/167+30repeat runit, MSVC10pureprojection/fallback, SDK2consumerexit0,
FD/sentinel/controlsinbytes privados OK, buildLinux inicial/final limpio.
Sin Monitor/systemd/layout/persistencia/contratos128-129 cambiados ni exec/CLI/
scripts/aceptación real. K03 E: siguiente proyección s6, otrosproveedores/
integración/aceptación pendientes; OpenRC132/I07 intactos. C60/E9/P4/B2,3X:
80%/20%, sin IDs/crédito parcial/Windowsfeature/N03fix/CIclaim.
Espejo141 confirmado: comentario90140275955125; TXT autoridad.

Checkpoint140: [SysV a ServiceInfo](LINUX_SYSV_SERVICES.md), consulta explícita,
evidencia139 y filas alineadas de la misma llamada. Descripción LSB Short/Long
con disponibilidad/permisos/partial, IDs hex scoped noacciones; PID/vida/grupo/
enablement UNKNOWN. No Monitor/systemd/layout común cambiados, sin exec/CLI.
Linux166/166+40repeat SysV, MSVC10pureprojection/fallback, SDK2consumerexit0,
FD/sentinel privados OK. Clockskew inicial /mnt/c <1s conservado; incremental
final limpio+166 sin tocar reloj/mtime. No aceptación en init real ni control.
K03 E: siguiente proyección runit, otrosproveedores/integración/control/vida/
aceptación pendientes. OpenRC132/I07 intactos. C60/E9/P4/B2,3X:80%/20%, sin
IDs/crédito parcial/Windowsfeature/N03fix/CIclaim/persistencia nueva.
Espejo140 confirmado: comentario90140275954616; TXT autoridad.

Checkpoint139: [catálogo SysV descrito](LINUX_SYSV_DESCRIBED_CATALOG.md), opt-in
con metadata ligada al candidato fijado/identidad del primer censo; segundo
censo/raices/contexto guardan publicacion. Swaps antespin/despuesread/raiz,
identidadwrong y partial ->UNKNOWN, no join por basename viejo. Presupuesto0..16MiB
(default8), reserva conservadora2prefijos, NOdiskusage. Catalogo137 sinbodyread.
Linux165/165+30repeat SysV, MSVC20portable/fallback, SDK2consumerexit0/FDclean.
Avisos clockskew inicial /mnt/c <1s preservados; incremental final limpio+165
sin tocar reloj/mtime. Sin exec/CLI/control/vida/enablement/aceptacion real.
K03 E: siguiente proyeccion explicita al modelo comun; integracion/control/
vida/aceptacion pendientes. OpenRC132/I07 intactos. C60/E9/P4/B2,3X:80%/20%.
Sin IDs/credito parcial/Windowsfeature/N03fix/CIclaim/persistencia nueva.
Espejo139 confirmado: comentario90140275953534; TXT autoridad.

Checkpoint138: [descripciones LSB](LINUX_SYSV_LSB.md), parser puro y lector de
prefijo64KiB fijado/typechecked, Short/Description declaradas e independientes.
UTF-8/bounds/duplicados/truncado/controles UNKNOWN, sin shell/eval/exec/CLI,
publisher/vida/defaultpolicyclaim o join inseguro con catalogo anterior.
Focal Linux/MSVC fallo por contar sangria en limite1024: corregido trimming y
test optionalchecked, logs conservados. Linux164/164+20repeat SysV/LSB,
MSVC10portable, SDK2consumerexit0/FDcleanprivado. Relinkclockskew /mnt/c <1s
preservado; incremental final limpio+164 sin tocar reloj/mtime. K03E: siguiente
bindingmetadata a candidatofijado; integracion/control/vida/aceptacion pendientes.
OpenRCscripts132 ycontrolscriptsSysV sin excepcion, I07observador intacto.
C60/E9/P4/B2,3X:80%/20%, sin ID/credito/N03fix/Windowsfeature/CIclaim/newformat.
Espejo138 confirmado: comentario90140275952208; TXT autoridad.

Checkpoint137: [catálogo SysV](LINUX_SYSV.md), explicit1init.d+parentrcN.d,
once niveles0..9/S, candidatos incluyendo.sh/metadata xbits (noaccess/launch),
convención S-KNN/sufijo NOidentity, indicepor dev/inode único enscopestable.
Hardlinks/targetexterno/parcial UNKNOWN, runlevelmissing distinto debrokenalias.
Dosscansmetadata/root-context/bounds/partialvisible; no bodyread/exec/CLI/
PID/liveness/status/control/enable-disable ni estadoactual/effectivebootpolicy.
Linux163/163+20repeat, MSVC10purefallback, SDK2consumer0/FDclean/buildcleanfinal,
fixturesprivadas NO realSysVacceptance. K03E; siguiente LSBcommentsdescripcion
acotada sin shell. Control/integración/aceptación real/otros pendientes intactos,
OpenRCscripts132 sinautorizar eI07observador intacto. C60/E9/P4/B2,3X:80%/20%,
sin ID/crédito/N03fix/Windowsfeature/CIclaim/formato persistido.
Espejo137 confirmado: comentario90140275949924; TXT autoridad.

Checkpoint136: [observación s6](LINUX_S6_OBSERVATION.md). read_record() vinculado
al controlfijado yLinuxS6Observation one-shot consent/baseline/poll/deadline.
Condiciones RAW solamente, NO vida/readiness/frescura/causalcompletion; ready
ignorado, restart exigechangedTAIposterior no PID/readystampsolo, finish/gap
excluidos. Countsretenidos/completedUNKNOWN incluso observado. Sinretry/worker/
timer/undo/finalread, latevalueNOconfirma, timeoutNOfailure, reportcongelado.
Linux162/162+40repeat s6, MSVC10portable, SDK2consumerexit0/buildclean/FDclean
PRIVADO, warninginitialindent corregido; NO real s6acceptance. K03E integración/
liveness/aceptación real/otrosinit; siguiente inventario SysVread-only. I07 y
OpenRCscripts132 aplazados intactos. C60/E9/P4/B2,3X:80%/20%, sinID/crédito/
Windowsfeature/N03fix/CIclaim/formato persistido nuevo.
Espejo136 confirmado: comentario90140275948612; TXT autoridad.

Checkpoint135: [inventario s6](LINUX_S6_INVENTORY.md) explicitScanRoot, dirs/aliases
visibles y1loglevel conparentname; no recursión/manager autodetect/installedglobal.
Recordraw yFIFOtype separados, NOlive-active-peer-permission-delivery-actiontoken.
readAtPin reutiliza lector133, servicio/logger mismopadre; bindings/context/root
guardados, caps/partialvisible, brokenlog noes ausencia. NoCLI/exec/script/locks/
writerprobe/writes/mkdir/deps/serviciosreales. Linux161/161+30repeat s6/record-
control-inventory, MSVC10fallback, SDK2consumerexit0/FDclean/buildcleanfinal.
NO aceptación s6 real; K03E por observación/integración/realacceptance/otrosinit.
Próximo observación targetfijado alcontrol. OpenRCscripts132 sinautorizar/aplazado,
I07observador intacto. C60/E9/P4/B2,3X:80%/20%, sinID/crédito/N03fix/Windowsfeature.
Espejo135 confirmado: comentario90140275946721; TXT autoridad.

Checkpoint134: [control FIFO s6](LINUX_S6_CONTROL.md) público open/request con
consent, pins/contexto, u/d/cru. Countsdeentrega NO completion/readiness/liveness
ni autenticación/versionpeer; completedUNKNOWN. Sin CLI/libs6/exec/scripts,
persistenciadown, worker, retrypositivo o espera. Fixtureprivada EPIPE/EAGAIN/
SIGPIPEpendingmask/forkstale/replacement/FDclean. Linux160/160+20repeat s6/control,
MSVC10pure/fallback, SDK2consumerexit0/buildcleanfinal; NO aceptación s6 real.
K03E: siguiente inventario s6; observación/integración/realacceptance/otrosinit
pendientes. OpenRCscripts sigue sin autorizar/aplazado; I07observador intacto.
C60/E9/P4/B2,3X:80%/20%, sin ID/crédito parcial/Windowsfeature/N03fix/CIclaim.
Espejo134 confirmado: comentario90140275943430; TXT autoridad.

Checkpoint133: [lector s6](LINUX_S6.md), API pública decoder/read registro35
raw, no supervisor/proceso vivo/readiness efectiva. Dos TAIraw/PIDu64 sincast/
waitword/flagsreportados, PID0readytrue válido. Pinsopenat2/context/doslecturas/
metadata/type-sizeguards/permission_required, missing/invalid/changed UNKNOWN.
Noexec/CLI/libs6/skalibs/script/lockprobe/FIFO-I/O/deps/autodetect/ServiceInfojoin.
Linux159/159+20repeat, MSVC10pure/fallback, SDK2consumer exit0, FDclean privado;
no aceptación real s6. Próximo handle/controls6 e inventario/integración pendientes.
Anteprosigue seavanzaalternativa s6, sin asumir excepciónscriptsOpenRC132, que
sigue aplazada/abierta. I07observador intacto. K03E; C60/E9/P4/B2,3X:80%/20%,
sin ID/crédito parcial/Windowsfeature/N03fix/CIclaim.
Espejo133 confirmado: comentario90140275941116; TXT autoridad.

Checkpoint132 SOLO [auditoría control OpenRC](LINUX_OPENRC_CONTROL_DECISION.md).
Ruta oficial revisada ejecuta scripts/runtime; librc mark no sustituye control,
FIFOinit global no se abre para servicioindividual. Decisión solicitada: permitir
excepción opt-in SOLOstart/stop/restart explícitos/nunca datos/pruebasprivadas
sininstalar-elevar-serviciosreales, o aplazarcontrolOpenRC yseguir s6/SysV.
Sin respuesta no se implementa/ejecuta excepción; prosigue no basta. Sin código
ni tests nuevos132. Últimos131 Linux158+20/MSVC10/SDK2consumer0, no repetidos.
K03E/criteriosintactos, I07observador intacto, C60/E9/P4/B2,3X:80%/20% sin crédito.
Espejo132 confirmado: comentario90140275939136; TXT autoridad.

Checkpoint131: [catálogo OpenRC](LINUX_OPENRC_CATALOG.md) explícito1init.d+
1runlevels, candidatos regulares/executable_mode bits no permiso de lanzar,
linksservicios por dev/inode único, hardlinkambiguo UNKNOWN; stacksdirectos
porinode sinrecursion/expansion/ciclos infinitos. Noelevacion/CLI/librc/bodyread/
script/mkdir. Dosscansmetadatarootcontext/bounds/partialUNKNOWN, indiceslocales
nosonautoridad. No habilitado-boot/estadoefectivo/livePID/control ni aceptación
real. Linux158/158+20repeat lectoresOpenRC, MSVC10pure/fallback, SDK2consumers
exit0/FDcleanprivado; clockskewrelinks milisegundos /mnt/c preservado, incremental
final limpio+158 sin tocar relojes/mtime. K03E por control/liveness/integración/
realOpenRC-runit/s6/SysV; I07observador aplazado intacto, N03abierto. C60/E9/P4/B2,
3X:80%/20%, sinIDs/crédito parcial. Próxima unidad: revisar control nativo OpenRC.
Espejo131 confirmado: comentario90140275938588; TXT autoridad.

Checkpoint130: [OpenRC marks](LINUX_OPENRC.md) API explícita de observación
filesystem en siete categorías, decoder propio noRC_SERVICE, doblepasada/pins/
contexto/bounds y partialUNKNOWN. Symlinkmetadata sinseguir destinos, no librc/
shell/scripts/CLI/mkdir. started marcado NO running/PID; ausencia NO stopped;
rootempty NOcero servicios instalados. Linux157/157+20repeat, MSVC10pure/fallback,
SDKgeneral+headersconsumer exit0, FDclean privado. Clockskew inicial /mnt/c
milisegundos logpreservado; incremental final limpio+suite157, sin tocar relojes.
K03E por installed/runlevels/control/liveness/realOpenRCacceptance/integración y
runitreal/s6/SysV. NoI07scopechange/Windowsfeature/N03fix/CIclaim. C60/E9/P4/B2,
3X:80%/20%; sin ID/crédito parcial. Próxima unidad: installed/runlevels OpenRC.
Espejo130 confirmado: comentario90140275936748; TXT autoridad.

Checkpoint129: [observación runit](LINUX_RUNIT_TRANSITIONS.md) K03 one-shot con
consent/baselineexacta/poll explícito/deadline steady. restart exige nueva época
TAI, stop excluye down/wantup. targetobserved NOreadiness/causalcompletion;
delivery.completed UNKNOWN siempre, latevalue no confirma ventana, sin retries,
worker/script/undo. Linux156/156+20repeat, MSVC10transition, SDKgeneral+runit
nuevossímbolos exit0; finalbuildclean/FDclean privados. No realrunsv acceptance.
K03E porrealrunsv/OpenRC/s6/SysV/integracion; I07UNKNOWNaplazada/observador sin
iniciador nuevo, N03abierto. C60/E9/P4/B2,3X:80%/20%, sin IDs/crédito parcial.
Espejo129 confirmado: comentario90140275461997; TXT autoridad. El usuario
confirmó explícitamente el 2026-10-30 mantener observación y pasar a K03;
no desarrollar iniciador propio ni cerrar I07/causa no verificable.
Decisión espejada: comentario90140275539280.

Checkpoint128: [runit](LINUX_RUNIT.md) K03 APIexplicitaopen-read-inventory-request,
status20+FIFO/okreader/pins-context/doblelectura, TAIraw/PIDreported noidentity,
description-groupunknown. startu/stopd/restarttcu deliveryonly/completionUNKNOWN,
no CLI/script/elevation/autodetectmanager ni ServiceInfo/layoutprevio cambiado.
NativeLinux PRIVADO protocolIO/alias/unknown/bound/replacedFIFO/EPIPE/pending-
SIGPIPE/mask/FDclean positivo. NO realrunsv acceptance(serviciosrealesintactos),
runsv no encontradoenPATHWSL/solo consulta. Linux155/155+20/SDKgeneral+newheader
consumer0/MSVC10decoderunsupported/buildclean. K03E pendienteconfirmacion-
transiciones/realrunsv/OpenRC/s6/SysV/integracion. Anteprosigue secontinuaalcance
observador recomendado/no nuevolauncher; I07UNKNOWNaplazada/sincierre/criterios
intactos,N03abierto. C60/E9/P4/B2,3X:80%/20%, sinID/credito parcial/Windowsfeature.
Espejo128 confirmado: comentario90140275459137; TXT autoridad.

Checkpoint127 AUDITORIA: [decision causal](STARTUP_EARLY_DECISION.md). Metadata
sesion/GIO/unitnames/loginproximity yhostpropio no prueban causaOSautomatica.
Se solicita decision de ALCANCE distinta del recorder103 ya autorizado: mantener
observacion/aplazarcausaI07 yseguirK03 (recomendado), o desarrollariniciadoropt-in
solo privado/sininstalar-activar-realapps. Inclusohostgestionado no pruebaqueOS
loinicio; preservarUnknown. Sin defaultUIasumido/cierre/retarget antesrespuesta.
No codigo/tests/sondas nuevos127. Ultimatest126 Linux154+30/MSVC20/SDK0 historica.
I07E/N03abierto,C60/E9/P4/B2,3X:80%/20%, sinID/credito parcial.
Espejo127 confirmado: comentario90140275454763; TXT autoridad.

Checkpoint126,2026-10-29: [recorder manual-only](STARTUP_RECORDER_MANUAL.md)
INTERNAL integrado enLinuxStartupRecorder EXISTENTE: ownsource+start/finish/
canonicalarchive/safePCIMANsave, dobleconsent/noGIOaccept/Monitor-scans. Legacy
tablasempty/rawunbound/autooriginUNKNOWN, publicsavelegacyrefused. Context/reloj/
support/capture-entry-PID-outcome/nativoobserver guards, move/abandon sinfinalread/
kill/reap, PIDownership returned unchanged. Publicdeadlinepoll explicito finaliza;
windowresourcesbirthrelative.4privadosREALrecorder positive/failure/liveUNKNOWN/
finish-save-reload-reprojection/0600/FDclean. Linux154/154+30/MSVC20manual-legacy/
SDK0. Initialmakefuturetimestamps preservados, finalrebuild+suite154clean.
I07E: trustedmanagedautostartcontext/otrasrutas-reapers-descendencia,N03abierto,
noactivacion/publicAPI-formato/Windowsfeature nuevos. C60/E9/P4/B2,3X:80%/20%.
Espejo126 confirmado: comentario90140275453225; TXT autoridad.

Checkpoint125,2026-10-29: [PCIMANv1](STARTUP_MANUAL_STORE.md) INTERNAL terminado:
rawmanualreceipt+embeddedPCISTART3births/windowpolicy,16MiB/checksum/strictvalidation.
SoloHISTORICO/no liveownership/FD/auth/capture; link/resources seRECALCULAN, caches
no serializados. Sharedmemorycodec/IOlegacy extraidos sin cambiarPCISTART1/2/3/
apphistory/publicAPI. ManualsaveLinux verifica0600/currentowner ANTESdepayload;
FSmodeignorado rechazadooldfilepreservado, positivo/tmp. WindowsheredaACL/noPOSIXclaim.
4privadosenvelopes0600 roundtrip/same-source-reprojection/UNKNOWN/rawunbound/FDclean.
Linux153/153+30manual-birth-legacy repeats/SDK0, MSVC10manual+2legacyIOcompat.
Initialfixtureoffsettype yprivacyfailuremode corregidos/guardprobado; initial
futuretimestampsmake preservados yrebuild+suite153finalclean sinclockadjust.
I07E porrecorder/managedautostartcontext/otrasrutas-descendencia, noactivacion/
manual!=automaticcause/N03abierto. C60/E9/P4/B2,3X:80%/20%, sinID/credito parcial.
Espejo125 confirmado: comentario90140275450450; TXT autoridad.

Checkpoint124,2026-10-29: [recibo->recursos](STARTUP_DESKTOP_RESOURCES.md) INTERNAL
terminado. One-shot arma birth/final/exec delcreadorANTESlaunch, luego receipt;
finish explicito poll/close/sample/detach, nohiddenwait/signal/finalreadenstop.
Joinexacto boot/kernelTGID-UID-startboot/capture/nsPID/firstfile, validatorfull
READONLY/stagingcopy; NO bitwise timestamps crosssensor ni bindingmutado.
Recursos lowerboundsMANUALES/birthwindow60s/preexec/waitedIO/high-onlyounknown,
no sums/autostart/app/content/currentproof.4privados:CPU22418570ns/disk1155072B
AFTERreap(1MiBfsync+20msCPU),2preexecerrors noatribuidos,livefinish UNKNOWN con
WNOHANG0/callerlibera-wait7/rawunbound/FDprobecleanup. Linux152/152+20/MSVC10pure/
SDK0. Futuretimestamps make iniciales preservados; finalrebuild+suite152clean.
I07E: recorder/durable/rutaautostartgestionada confiable sinactivacion/manual!=
automaticcause/otrasrutas-descendencia/N03abierto. No publicAPI/archivo/formato/
RunNewTask/associate/Windowsfeature nuevos. C60/E9/P4/B2,3X:80%/20%, sincreditoID.
Espejo124 confirmado: comentario90140275447569; TXT autoridad.

Checkpoint123,2026-10-29: [recibo manual controlado](STARTUP_DESKTOP_RECEIPT.md)
INTERNAL terminado. Session une desktoproute+gate+selectedfirstexec, dobleconsent
antesI/O/child/probe, one-shot/sinfallback/sin relaunchtrasstop. ReciboHISTORICO
manual_request fijo/fullkernel-local-boot-ns/request-pin/outcome/exec-quality-exit.
Link true/false SOLOexec conocido+calidadexacta+PIDreleased; absence/EOF/exit/
estimated/flags UNKNOWN, contradiccionesfailclosed. Sin allocpostreleaseenstart.
Nativo4privados:1matchAFTERreap,2preexecerrorsUNKNOWN/ECHILD,1matchvivo; stop
NOkill/reap, callerlibera+wait, copies/replay/oneshot/FD/probecleanup positivos.
Linux151/151+20focales/MSVC10pure-denial-fallback/SDK0. Initialmakefuturetimestamp
warnings preservados; rebuild+suite151 finalesclean sin ajustarclock/mtimes.
I07E: integracionrecibo-recorder-recursos/rutaautostartgestionadaconfiable sin
activacion/manual!=causaautomatica/otrasrutas-descendencia. N03abierto, sin public
API/archive/RunNewTask/associate/Windowsfeatures nuevos. C60/E9/P4/B2,3X:80%/20%.
Espejo123 confirmado: comentario90140275444937; TXT autoridad.

Checkpoint122,2026-10-29: [ruta directa XDG](STARTUP_DESKTOP_LAUNCH.md) INTERNAL
terminada. Wrapper obtienePROPIOsnapshotefectivo/eligible-condition exacttrue,
Execsubset2escapingstages/wholequote/literalargv/Path, no commandalternativocaller.
Terminal/DBus/vendorunknown/fieldcodesunsupported rechazan; sin shellimplicito/PATH.
Relee fields/ID/orden/precedencia/eligibility ANTES y DESPUES callback; stale
cancela/reap stillgatedchild. No transaccionatomica/currentfile/content/appproof.
Linux150/150+20focales/MSVC10puro-denial-noLinux/SDK0. Nativo privado desktop->
gate->selectedfirstexe vs pin AFTERreap positivo/FDcleanup/sonda retirada;
2mutacionesarm canceladas/ECHILD, shadow/disabled/conditionunknown rechazados.
AccionMANUAL !=causaautostart. Sigue recibogestionado/manual-automaticdistincion/
integracion/otrasrutas-descendencia. No publicAPI/archive/RunNewTask/associate nuevo,
no activacionhost/loginGUI/global/Windowsfeatures. N03abierto.
C60/E9/P4/B2,3X:80%/20%, sinIDs/fracciones/horasestimadas.
Espejo122 confirmado: comentario90140275443077; TXT autoridad.

Checkpoint121,2026-10-29: [primer exec seleccionado](STARTUP_SELECTED_EXEC.md)
INTERNAL terminado. Duplica pidfd/imagepin dentroarmcallback; fullkernel/local/
boot/ns/contexto autenticado antesrelease, attachempty/enrollLAST, un TGID+startboot.
No release_task/CPU dependencia, no birth/counters inventados: raw4..14UNUSEDZERO.
Primer exec filesystemtuple vs pin, no content/app/causaproof; reexec no reemplaza,
failedfirststicky/publishing/unknown respetados. Afterreap/ownreplay/caller guards,
stopdetach/FDcleanup, no signal/reap. PublicAPI/RunNewTask/store/associate intactos.
Linux149/149+20focales/SDK0, MSVC20puredecoder-denial-noLinux. Nativo5hijosprivados:
3successmatches(symlink/reexecOTROinode),2preexecerrorsUNKNOWN,5sentinelsignorados;
muestrasafterreap/inmutabilidad/FDmaskNNP/sondasretiradas positivos. Logs selected-exec-*.
I07E: siguiente desktoproute validada/recibo gestionado/causa fiable/otrasrutas-
descendencia. N03abierto/no activacionhost/loginGUI/noWindowsfeatures.
C60/E9/P4/B2,3X:80%/20%, sin IDs/fracciones/horasestimadas.
Espejo121 confirmado: comentario90140275441257; TXT autoridad.

Checkpoint120,2026-10-29: [gate pre-exec interno](STARTUP_GATED_LAUNCH.md) terminado.
ELF regular pinO_PATH/NO_MAGIC, clone3PIDFD atomico/gate/callbackarmamiento,
fullcontext-identidad-cred-stamp recheck antesexecveatobjfijado. Cancel ownheldchild
porpidfd/reap; releasedsuccess/unknowncallerwaitpid, sin hiddenpostexec kill.
NNPchild/rawsyscalls/kernelmaskrestaurada/nolibcatfork, argv-envliteral/sinshell/
PATH/script/elevacion/fallback. GateNOexec-success/app/autostartreceipt ni sandbox.
Linux148/148+20focales/POSITIVEselfELF/symlink/deny-throw-stopcancel/errorFIFO/FDmaskNNP,
manual4kernelpidfdjoinsprivadosANTESrelease/sondasretiradas; SDKconsumer0.
MSVC10denial/noLinuxfallback, NONEWindowsfeatures. C++finalclean/indentwarning
inicialcorregido/D9025asserts intencional. Sin publicAPI/layout/format/host/loginGUI.
I07E: siguiente execEXITOSOseleccionado/filepin+desktoproute fiable, luego receipt/
managedautostart/otrasrutas-descendencia. N03abierto, C60/E9/P4/B2,3X:80%/20%,
sin IDs/credito fraccionario ni horasestimadas.
Espejo120 confirmado: comentario90140275436885; TXT autoridad.

Checkpoint119,2026-10-29: [overlay historico birth](STARTUP_BIRTH_OVERLAY.md)
terminado: API pura apply_startup_birth_impact/defaultconsentfalse/contextoaceptado
captureID-boot-PIDns exacto, validatorv3 compartido/rollback antesnoexceptcommit.
Declara IDs no app-causa probados ni identidadsnapshotACTUAL. Single CPU/disk
estimated high/unknown, multiplegrupos NOsumas/solo highlowerboundounknown;
intervalo legacyunknown. ScopeHISTORICAL/capture-boot-ns/source-calidad/window,
campos actuales/unrelated/source sinmutacion. No OS/file/Monitorautomatico/GUI.
Linux147/147+20focales/MSVC10puros/SDK0/Native13casos23filas desdev3/0600/FD/limpieza
positivos; high custom1ms SOLOprueba/defaultsintactos, asercionesanterioresintactas.
Sin layouts/formatos existentes nuevos. I07 E: siguiente receipt/atribucionapp-causa
fiable/otrasrutas-descendencia, N03abierto/Windowsaplazado. C60/E9/P4/B2,3X:80%/20%,
sin IDs/credito fraccionario ni estimacion horas.
Espejo119 confirmado: comentario90140275428976; TXT autoridad.

Checkpoint118,2026-10-29: [PCISTARTv3 birth-aware](STARTUP_BIRTH_STORE.md) terminado.
Archive.births preserva rawreport completo/keys/metricpresence-support-note/exec/
window/policy4umbrales, pendientes/unbound/empty/failed. Recorder.save v3 supera
rechazo117, sin proyecciones duplicadas/clock coercion/sumas/autoridad acciones.
Lectura v1/v2/v3, escritura sinbirth conserva1/2; antiguo AUSENTE !=vacio/zero;
sin migracion automatica/apphistory change. Rebuild ABI SDK/consumidores realizado.
Linux146/146+20focales/SDK0; Native13casos23filas(18finales/5ausentes) v3save/load/
reprojection/0600/FD/limpieza pasan. MSVC3fixturesfinales positivos fueraaislamiento;
iniciales2 abortaronassert conreplaceerror5/accessdenied. Stage/native-error
diagnostico+logs conservados SIN cambiar asercion/write-flush-close-replace; no
atribuir/fijar N03historico por ese resultado. C++limpios/D9025assert intencional.
I07 E por receipt/app-causa fiable/guardedoverlay/otras rutas/descendencia; N03 abierto,
Windows aplazado/no activacion host/login/GUI. C60/E9/P4/B2,3X:80%/20%, sincredito.
Espejo118 confirmado: comentario90140275426329; TXT autoridad.

Checkpoint117,2026-10-29: [captura de hijos en grabador](STARTUP_RECORDER_BIRTH.md)
terminada: watch_births opt-in/1creador, factory nativa propia/poll cada llamada,
associate_birth exactkey incluso post-close, cleanup/failure/retencion. Vistas
birth_capture/birth_impacts separadas de legacy: ventana por nacimiento, no fase
global/clock coercion/suma/causa automatica. Save rechaza tras attach inclusovacio,
antesfilesystem/replacedfalse: v2 no representa estas identidades; formato nuevo
pendiente, no perdida silenciosa. Report layout ampliado/SDK consumidor recompilados.
Linux145/145+20focales/MSVC10/SDK0/Native13escenarios23filas(18finales/5ausentes)
con oraculos CPU/IO privados, lifecycle/binding/replay/FD/source checks conservados.
Sin activacion host/login/GUI/Windows. I07 E por receipt/app-causa fiable/formato
birth-aware/overlay/otras rutas/descendencia, N03 abierto. C60/E9/P4/B2,3X:80%/20%,
sin IDs/credito fraccionario; stop caller-scheduled NO cobertura global certificada.
Espejo117 confirmado: comentario90140275422485; TXT autoridad.

Checkpoint116,2026-10-29: [impacto de hijos breves](STARTUP_BIRTH_IMPACT.md)
terminado como API pura measure_startup_birth_impact(report,key exacta,binding
previo). Ventana60s DESDE CADA NACIMIENTO/no boot-login-global; deadline kernel
checked, release<=deadline CPU/disk delta checked estimated lowerbounds. Ausente/
tardio -> unknown sin clip/prorrateo; high solo>policy, NUNCA low/medium/cobertura
completa por valores bajos. Zero medido/permission/overflow explicitos, no legacy
clock/PID-start_ticks inventados ni arbol, causa siempreunknown/binding declarado.
Linux144/144 sin skips+20focales/MSVC10/SDK consumidor0; Native13escenarios23filas
post-reap/binding pasan,18finales/5ausentes, high positivo con policyCUSTOM1ms solo
prueba/defaults intactos. Fuente/default independiente/candidatos/replay/FD/limpieza
conservados. Sin formato/activacion/GUI/implementacion Windows. I07 E por receipt/
app-causa fiable/integracion recorder-durable-overlay/otras rutas/descendencia;
N03 sigue abierto. C60/E9/P4/B2,3X:80,0%/20,0%, sin IDs/credito parcial.
Espejo116 confirmado: comentario90140275370635; TXT autoridad.

Checkpoint115,2026-10-29: [validacion misma captura](STARTUP_CAPTURE_VALIDATION.md)
corregida. Repro previo mode8 CPU10441812/10581094ns (139282ns diferente), IO igual;
el harness comparaba DOS sensores independientes como si fueran la misma fuente.
Ahora exact passthrough MISMA captura real/contexto nativo, y segunda instancia
DEFAULT contra reloj privado/mismos margenes CPU, IO/fstat exactos y propio replay
inmutable. Nueva regresion rechaza CPU0->1ns y conserva CPU0/binding. No cambia
formula/probe/umbrales; nota CPU release-observed no post-switch lifetime garantizado.
Linux143/143+20focales/MSVC10/SDK consumidor0; Native13+20escenarios positivos.
Preciso interleaving kernel sigue SIN demostrar, no declarar bug CPU corregido ni
N03 cerrado. I07 E por receipt/app-causa fiable/integracion/otras rutas/descendencia.
C60/E9/P4/B2,3X:80,0%/20,0%, sin IDs/credito fraccionario, Windows aplazado.
Espejo115 confirmado: comentario90140275365822; TXT autoridad.

Checkpoint114,2026-10-29: [candidatos XDG filesystem](STARTUP_EXECUTABLE_MATCHES.md)
terminados, API read-only current-caller catalog/key exacta/boot-PIDns. Exec subset
literal absoluto/string-quotes-codes validado; O_PATH/openat2 NO_MAGICLINKS/stat
rechecks, props pinned/batch/budget16MiB/censos. Unique solo1match+catalogo completo,
siempre estimated; shared/disabled ambiguos/unknown impide unicidad. No binding/
ejecucion/autostart-cause/impacto, no catalogo historico ni target UID garantizados.
Linux143/143+30focales/MSVC matcher10/SDK consumidor exit0. Native13 pasa con6exec
reales unico->compartido ambiguo, sin promover origen/bindings. Primer Native4
fallo resource equality mode10; diagnostico agregado sin relajar,4+13 repeticiones
pasan sin reproducir. Causa NO demostrada/corregida, N03 sigue abierto. Warnings
indent y fixture <map> corregidos/logs preservados, checks Release activos,
Linux final limpio/MSVC D9025 intencional. I07 E por receipt/app-causa fiable/
integracion temporal/otras rutas/descendencia. C60/E9/P4/B2,3X:80,0%/20,0%,
Windows aplazado/no formato persistido/activacion.
Espejo114 confirmado: comentario90140275359283; TXT autoridad.

Checkpoint113,2026-10-29: [primer exec exitoso](STARTUP_FIRST_EXEC.md) terminado.
observe_first_exec OFF, first_executable Metric inode/dev major-minor/UID proceso/
kernel monotonic time historicos. Allowlist solo birth autenticados, TGID+start,
solo task/start para validar hits, sin file/cred reads ajenos. BTF anon MM depth8/
budget256; raw20/CAS first/state-last/bracket/state3fallo/flag64. Reexec no sustituye
primera evidencia ni la vuelve exe actual/path/hash/app/causa. Bindings caller
estimated/origen autostart unknown intactos; CPU-IO/PID independientes.
Native13casos23filas/18finales/5ausentes/6exec contra fstat propio post-reap,
incluye directo/fallo/reexec/no-lider, replay/FDs correctos. Inicial anon-BTF
fallo+fixture procselfexe tras lider salido corregidos/logs preservados. Linux
142/142+30focales/MSVC30 fixtures/SDK instalado-consumidor exit0. Layouts publicos
ampliados requieren recompilar; formato persistido intacto. Build Linux limpio,
D9025 MSVC asserts intencional. I07 E por app-desktop-launch fiable/causa temporal/
integracion/otras rutas/descendencia. C60/E9/P4/B2,3X:80,0%/20,0%;N03 abierto,
Windows aplazado, no activacion/captura global.
Espejo113 confirmado: comentario90140275341609; TXT autoridad.

Checkpoint112,2026-10-29: [API publica/asociacion por hijo](STARTUP_BIRTH_OBSERVER.md)
terminada. LinuxStartupBirthObserver start/poll/associate/close/report, opt-in sin
worker/autoelevacion/launch/instalacion. Key captura+boot+TGID/UID/start kernel,
PID namespace separado, binding caller estimated/idempotente/conflicto rechazado;
no app inferida de creador/nombre/PID. Contexto caller completo (no PID solo),
transaction/replay/final inmutable/flags/modes, error retiene datos aceptados.
close sin lectura final/report copy, no sumas IO-arbol/clasificacion ni ticks o
steady-time sinteticos. Publico usable pero evidencia ejecutable/causa sigue unknown.
Native9casos15filas publicas comparadas con sensor interno/10finales/5ausentes,
binding/replay despues de reap/cierre/FD correctos. Linux142/142+30focales,MSVC10
fixtures/fallback noLinux, SDK instalado/consumidor nuevo header exit0. Inicial
warning indent y fixture optional corregidos/logs preservados, final limpio;
D9025 MSVC NDEBUG->UNDEBUG intencional. Sin formato persistido/integracion automatica.
I07 E por evidencia app-exec-launch/causa autostart/integracion temporal/otras
rutas/descendencia. C60/E9/P4/B2,3X:80,0%/20,0%;N03 abierto,Windows aplazado.
Espejo112 confirmado: comentario90140275314523; TXT sigue autoridad.

Checkpoint111,2026-10-29: [PID local de nacimiento](STARTUP_BIRTH_PID.md) INTERNO
terminado. BTF independiente y busqueda0..level<=32 en upid por inode del namespace
OBSERVADOR autenticado, no PID global ni innermost. local_pid Metric independiente
de kernel/start/UID; flag32 preserva CPU/IO si mapping falla. Campos privados15u64
antes de publicacion birth; schema ausente disabled+nota, namespace/rango/pares
invalidos rechazados. Sin API/formato publico/worker/FD adicional/activacion.
Native9casos15PID post-reap/10finales/5sin-final esperado, init privado selfPID1
con PID distinto para observador; no mounts/cambios globales. Fixture2hijos tuvo
EOF conservado sin causa syscall probada; corregido1init pasa aislado+suite9sin
cambio collector. VM0/32/global88->local12/faults/CPU0 retenido y BTF adversarial.
Linux141/141+30focales/MSVC20/SDK consumidor exit0. D9025 MSVC NDEBUG->UNDEBUG
intencional para asserts activos. N03 no corregido/Windows aplazado. I07 E por
atribucion app/exec/launch e integracion/otras rutas/descendencia/causa fiable.
C60/E9/P4/B2,3X:80,0%/20,0%; TXT autoridad.
Suite9 repetida exit0: selfPID1/observerPID450/kernelTGID1568, namespace4026532221.
Espejo111 confirmado: comentario90140275296541. Sondas temporales retiradas.

Checkpoint110,2026-10-29: [conexion nacimiento->finales](STARTUP_CHILD_FINALS.md)
INTERNA terminada. Opt-in OFF, release_task instalado antes del birth hook; filtro
temprano del mismo creador/reaper autenticado, match hijo TGID+start/UID/live0/nr1,
publicacion atomica/bracket/deltas checked/flags8-16. CPU scheduler task+signal,
IO task+signal incluye hijos esperados, no sumar arboles. No seleccion/censo/pidfd
previo de cada hijo en observador. Native8escenarios9finales/5sin-final esperado,
CPU oraculo privado e IO exacto; auto-reap unknown/known-zero/cupo/thread/sentinel/
fork/replay/FDs correctos. Kernel IDs aun no mapping local/app/causa autostart;
no automatizacion Monitor/recorder/history ni otros reapers/descendencia global.
Linux141/141+30focales/MSVC birth-final10+exit-BTF10/SDK consumidor exit0, recheck
limpio tras clock-skew inicial preservado. Sin API/formato/activacion global nuevos.
C60/E9/P4/B2,3X:80,0%/20,0%; I07 E por mapping/atribucion/integracion/causa fiable.
N03 no corregido. TXT autoridad, ClickUp solo espejo.
Espejo110 confirmado: comentario90140275274389. Nativo8escenarios repetido positivo.

Checkpoint109,2026-10-29: [sensor interno de nacimientos](STARTUP_BIRTH.md)
terminado. FENTRY wake_up_new_task/creador TGID-UID-start autenticado/leader nuevo,
ARRAY1..64 cupo/reserva/publicacion/fallos explicitos. Registros kernel IDs y
CPU/IO AL NACER, no finales/local PID/app/real_parent/autostart. Sin censo o
seleccion userland previa de cada hijo. No integrado aun en recorder/historial.
Nativo privado creador+hilo+2hijos reaped ANTES de primer read PID hijo:cap4
2registros quality0;cap1 1registro quality2/estimated. Sentinela/thread ignorados,
copia fork negada/creador reaped/replay retenido/FDs limpios; 0 contadores de
nacimiento medidos. Linux141/141+30focales/MSVC birth10+exit-BTF10/SDK consumidor
exit0. Warning indentacion corregido/clock-skew inicial preservado/recheck limpio.
Sin API/layout/formato publico nuevo ni activacion/captura global. Fork refs
heredadas deben cerrarse/exec; stop libera refs propias, no se promete magia atfork.
I07 E: faltan finales hijos nuevos/mapping local/atribucion-integracion/mas
descendencia/causa fiable. C60/E9/P4/B2,3X:80,0%/20,0%;N03 no corregido.
Espejo108 atrasado confirmado:90140275243922. TXT sigue autoridad.
Espejo109 confirmado:90140275259142.

Checkpoint108: [CPU final compatible](STARTUP_SCHEDULER_CPU.md) integrada para
objetivos seleccionados/cubiertos. Sample.cpu_counter enum, lector POSIX process
CPUCLOCK_SCHED con pidfd vivo/identidad antes-despues/conversion ns checked;
no ticks disfrazados de scheduler. Recorder opt-in legacy diskflag intenta nueva
baseline; kinds de lector sobreviven limite/attach fallido, final compatible o
fallback lower bound, sin mezcla/reset/unknown->0. Defaults OFF y no BPF para
lector vivo. Partial/gap conserva limites; high observado no prueba autostart.
Wire v1/v2 intacto/notas fuente CPU, exit_disk_* aun son diagnosticos de DISCO.
Nuevo Sample layout exige recompilar. Nativo worker liberado antes de baseline
y7casos BPF privados:6recuperaciones CPU/disco y1uncovered unknown; high soloCPU
1241171380ns con262144B disco. Archivo/metadata/flags/0600/FDs correctos.
Linux140/140+30focales/MSVC recorder10+impact10/SDK consumidor instalado exit0,
build limpio. I07 E por vidas no preseleccionadas/descendientes/causa fiable;
CPU compatible y diagnosticos durables seleccionados ya terminados. Sin activar
login/global capture ni soporte Windows nuevo. C60/E9/P4/B2,3X:80,0%/20,0%.

Checkpoint107: [diagnosticos durables](STARTUP_DIAGNOSTICS.md) terminados.
Archive.diagnostics opcional/configuracion/elapsed/16contadores/origen-presencia,
v2 mismo archivo atomico; v1 sigue legible con metadata AUSENTE, no0 fabricado.
Conteos/duracion/booleans/enums/overflow/truncaciones/checksum validan todo o nada.
released_without_final incluye procesos vivos al cierre, NO prueba exit perdido.
finished_at memoria, elapsed checked entero; close reloj invertido/save intervalo
extremo rechazados. Layout Archive/Report requiere recompilar y lectores previos
rechazan v2; no migracion por carga ni cambios a historial de apps. CLI informa v2.
Linux140/140 sin skips+30focales,MSVC store1+recorder10; host privado tardio/vacio,
seis casos privados BPF/provider/recorder/observer/store reales con diagnosticos/
flags/notas/0600 y FDs limpios. SDK consumidor actualizado. Sin activar login ni
captura global. I07 E por CPU compatible/vidas nunca seleccionadas/causa fiable;
persistencia agregada ya terminada, no implica cobertura global ni cierre I07.
C60/E9/P4/B2,3X:80,0% cerrado/20,0% abierto sin credito parcial; N03 no corregido.
SDK instalado/consumidor recompilado exit0. Espejo107:comentario90140272944220.

Checkpoint106: [recuperacion final de disco](STARTUP_DISK_TAIL.md) integrada en
LinuxStartupRecorder/Observer/Store, opt-in OFF/max concurrente1..32. CPU ultima
procfs queda LOWER BOUND, no mezcla scheduler ns. finish_partial_process fuerza
gap/cierre/incomplete; notas/flags permanecen en archivo existente. Pending tras
procfs ausente espera hasta deadline, no falso0; sin late tail/reset/privilegios
automaticos. Fase incluye setup; start puede cerrar durante scan/attach y CLI
default sin BPF informa estado real. Nuevos campos Options/Report requieren
recompilar consumidores, sin prometer ABI/layout anterior; formato intacto.
Nativo privado censo inyectado (NO prueba autostart/GIO), provider/recorder/
observer/store reales:5recuperaciones tras reaping,1auto-reap UNKNOWN;6roundtrips
0600/flags/nota CPU/cleanup/FDs,4MiB high parcial. Linux140/140+30focales,
MSVC recorder10+observer10 y SDK consumidor exit0. Fixture inicial tick no debido
corregida sin relajar guardia. No activa host ni captura global real; N03 sigue.
I07 E por CPU compatible/vidas no seleccionadas/causa fiable/diagnosticos durables.
C60/E9/P4/B2,3X:80,0% cerrado/20,0% abierto, sin credito parcial.
Espejo106 confirmado: comentario90140272943424; TXT sigue autoridad.

Checkpoint105: [contadores de liberacion directa](STARTUP_FINAL_RESOURCES.md)
INTERNO terminado. FENTRY release_task target(no reaper), BTF/identidad/leader/
live0/nr_threads1/overflow/publicacion/contexto/fork/cleanup. CPU scheduler-runtime
ns separado de procfs; IO group-accounted incluye hijos ya esperados, no IO
exclusivo ni suma automatica de arboles. Nativo5casos privados:4positivos con
CPU-clock/procfs-IO oracle y1auto-reap internal repeat correctamente UNKNOWN.
Antes de reaping pidfd exited no inventa totales; tras ACK padre/procfs ausente
conserva datos/replay. FDs limpios. Linux140/140 sin skips+40focales(4x10),MSVC
puro10veces/consumidor SDK exit0/recheck incremental limpio. Fallos iniciales
de fixtures/clock-skew preservados, sin relajar schema/decoder ni corregir N03.
Sin API/layout/vtable/formato publico nuevos, no instala/activa host/captura global.
I07 E por baseline compatible/integracion/vidas no seleccionadas/causa fiable/
diagnosticos durables. C60/E9/P4/B2,3X:80,0% completo/20,0% abierto sin cambio.
Espejo105 confirmado: comentario90140272941957; TXT conserva autoridad.

Checkpoint104: [sensor de salida seleccionada](STARTUP_SELECTED_EXIT.md) INTERNO
terminado. BTF/FENTRY/taskstats_exit ultimo hilo, pidfd/joins y evento one-shot
retenido tras reaping, sin CPU/disco/causa autostart certificados. Nativo nietos
privados/ECHILD/procfs ausente, uno y varios hilos: lider zombie con worker vivo
no final, ultimo worker si. Sentinela/fork/padre/FD limpios. Regresion F03/
Monitor/Sampler/UID/H04 archivo real pasa con lector BTF compartido extraido.
Linux140/140+30focales/MSVCfixture10/consumidor instalado exit0/recheck limpio.
Logs iniciales indices de fixture/cleanup-write/clock-skew preservados, esquema
no relajado/N03 no corregido. Sin API/layout/vtable/formato publico nuevos ni
host instalado/activado. I07 E por contadores finales/vidas no preseleccionadas/
descendientes/causa fiable/integracion durable.80,0%/20,0% igual.
Espejo104 confirmado: comentario90140272941185; TXT sigue autoridad.

Checkpoint103: [host de recorder opcional](STARTUP_RECORDER.md) terminado.
Autorizado desarrollar segunda opcion, NO instalar/activar. API reutilizable
LinuxStartupRecorder/start-poll-close-report-save y binario propio opt-in OFF
por defecto/sin regla install ni startup. Consentimiento/contexto/euid/fork/
dedup/limites/deadline/gaps/cierres/ventanas tardias; archivo historico explicito,
sin persistir diagnosticos de censo ni certificar autostart por GIO.
Nativo host listo ANTES de lanzar candidato propio, CPU real/partial/archivo0600/
Exec intacto/invalidos-corrupto preservados/vacio honesto; no login real tocado.
Linux139/139+30focales/MSVCcontrolador privado10veces/consumidor instalado exit0;
ejecutable comprobado NO instalado en prefijo SDK privado. Fixture inicial
unknown vs enum corregida, clasificador/asserts de ausencia intactos/log retenido.
API aditiva sin layouts/vtable/formato cambiados. I07 sigue E por causa fiable,
vida corta ajena/descendientes/cobertura temprana/diagnosticos durables.
60/75=80,0%,15/75=20,0% igual, sin nuevos IDs/credito parcial. Continuar I07.
Espejo103 confirmado: comentario90140272940396; TXT sigue autoridad.

Checkpoint102: I07 auditada, sin codigo/captura/suites nuevas. Falta elegir
[host de captura temprana](STARTUP_EARLY_DECISION.md): futuro GUI al login o
grabador minimo opcional propio antes de abrir GUI. No se ha instalado/activado
ningun inicio real ni aceptado arquitectura adicional por silencio. Ninguna
opcion prueba por si sola autostart/vida corta ajena; I07 sigue E.
60/75=80,0%,15/75=20,0% sin cambio; resultados101 debajo son historicos.
Espejo102 confirmado: comentario90140272939947; TXT sigue autoridad.

Checkpoint101: [red por UID Linux](LINUX_USER_NETWORK.md), J04 E->C.
Disco revision23 preservado; nueva red unica por UID del observador con dominio,
lanes/send/receive/total parciales, cobertura/overflow y metadata F03 pareada.
No duplica sesiones ni hace join por UID de bus/username; legacy red de sesiones
ausente, informe nuevo es autoridad. Flag dedicado sin inventario de sesiones.
Nativo2hijos privados UID0/2buckets/oraculo4164+4164=8328B/s, worker AB-B-AB/
historial/FD limpios/H04 real258->516B/replay/almacen intacto. Linux136/136+
40focales/consumidor instalado exit0; MSVCproyeccion pura10veces pasa. Ensayo
MonitorWindows aborto0xC0000409 tras fase pura, logs preservados/causa N03.
Integracion nativa del fixture acotada a Linux, no assertions Linux debilitadas
ni funcionalidad Windows modificada/validada. Layout0.x: recompilar TODOS.
C60/E9/P4/B2,75activas/3X,80,0%/20,0%; siguiente I07, no iniciada.
Espejo101 confirmado: comentario90140272939846; TXT sigue autoridad.

Checkpoint100: [historial de red Linux](LINUX_NETWORK_APP_HISTORY.md), H04 E->C.
F03 observado integrado Monitor/Sampler con exe verificado por pidfd/procfs/
boot/namespaces/PID-start/UID/inode/ruta-stamp antes-despues y entre muestras.
Informe linux_network_app_history independiente de CPU; deltas por bucket,
sin retroactividad/doble cuenta/auto I/O. Limite exec-ABA declarado estimated.
Nativo2hijos privados258->516B,50replays/archivo real/FD limpios/sondas retiradas.
Linux135/135+30focales/consumidor instalado exit0; MSVCfixture nuevo10veces pasa.
Repeticion CPU anterior units/boot changed preservada/N03, final pasa sin cambiar
aserciones. Nuevo layout SystemSnapshot0.x: recompilar TODOS los consumidores.
C59/E10/P4/B2,75activas/3X,78,7%/21,3%; siguiente J04, no iniciada.
Espejo100 confirmado: comentario90140272939021; TXT sigue autoridad.

Checkpoint99: [almacen Linux con disponibilidad](LINUX_HISTORY_STORE.md) listo.
H04 P->E, NO cerrada: F03 aun no alimenta network_bytes por aplicacion.
LinuxHistoryStore guarda filas CPU/red/notificaciones/value-Support-nota en
formato separado; merge cumulativo sin doble cuenta y validacion estricta,
reset no elimina directorios, no I/O automatico/captura/ABI existente nuevos.
Linux134/134+30focales/consumidor instalado exit0/roundtrip CPU REAL sin red0
inventada; MSVC20focales fuera sandbox pasan. Dentro, reemplazo temporal denied5
tambien en almacen anterior; logs preservados, sin codigo Windows nuevo.
Fixtures iniciales corregidas/assercciones activas/N03 no arreglado.
Falta atribucion/ventanas por exe, integracion F03 y persistencia de red REAL.
C58/E11/P4/B2,75activas/3X,77,3%/22,7%; siguiente H04. Espejo99 confirmado: comentario90140272938633.

Checkpoint98: [F03 aceptada/vista real](LINUX_NETWORK_ACCEPTANCE.md), E->C.
Fuente nativa OPCIONAL implementada conforme al criterio previo, no totalidad
universal/wire/async. SELF corregido (contexto completo+2lecturas kernel nuevas),
externos conservan permisos/joins. WSL REAL30/30 strict/0excluidos, tres snapshots
Monitor/red-epoch-metadata, UDP delta97/tasas/FDs limpios. Permiso temporal confirmado,
ensayos<10s acumulados, sondas retiradas/no autorizacion permanente. EINVAL/actor
no vivo iniciales preservados; churn falla-cerrado/requiere refresh autorizado.
Linux133/133+30focales, MSVC10join/consumidor instalado exit0, nativo17 privado
regresion TCP-UDP/IPv4-IPv6/updates/gap/fork PASS. H04/J04/N02/N03 abiertas.
58/75=77,3%,17/75=22,7%,C58/E10/P5/B2. Siguiente H04/leer HISTORY.md.
Recheck build Linux exit0 sin avisos tras clock-skew generado; logs preservados.
Espejo98 confirmado: comentario90140272937903; TXT manda.

Checkpoint97: [capacidad1024/lotes](LINUX_NETWORK_SCALE.md) terminada.
RAW64/lote,16cookies por actor/16384buckets; continuidad de retenidos/altas
frescas sin gap. No truncamiento ni garantia de1Hz; sin cambios de RLIMIT/config.
Recompilar TODOS por constexpr. Fixtures65/129/1024/16384buckets/fallos lotes2y16;
nativo65 SOLOprivados PUBLICO2lotes/87FDs+admision restringida/152FDs/UDP/tasas/
UID/unchangedrefresh/limpieza, regresion17 positiva. Linux133/133+70focales,
MSVC70focales/consumidor instalado exit0. Primer static_assert de consumer viejo
actualizado al limite16384/log preservado, no checks relajados. Sin nuevos
warnings C++/D9025 esperado/N03 abierto. Falta validacion/cobertura vista real,
autorizacion WSL solicitada/no recibida en esta revision; NO captura global.
F03/H04/J04 abiertas,76,0%/24,0%,sin credito parcial. Espejo97 confirmado: comentario90140272937319.

Checkpoint96: [admision/procedencia Monitor/Sampler](LINUX_NETWORK_ADMISSION_MONITOR.md)
integrada. SystemSnapshot metadata/red pareadas, no censo nuevo ni total global.
Consentimiento CADAvez/defaultstrict/modos exclusivos/fallo cierra ambos,
worker continua y flag/restart no rearma. Refresh concurrente/history/move/gap/
origen probados. Layout SystemSnapshot0.x ampliado: recompilar TODOS.
Linux133/133+100focales/consumidor instalado exit0, MSVC40focales; nativo SOLO
2privados+marcador sintetico/enumerador restringido por objeto pasa, no vista
completa autorizada/validada. Primer assertion MSVC preservado/causa no aislada,
rebuild/repeats sin relajar asserts pasan; N03 consumidor75 no resuelto.
Avisos clock-skew temporales WSL registrados; rebuild posterior exit0 sin avisos.
F03/H04/J04 abiertas por escala/
cobertura/validacion autorizada e integraciones siguientes;76,0%/24,0% sin credito parcial.
Espejo96 confirmado: comentario90140272936245; historicos remotos intactos/TXT manda.

Checkpoint95: [politica parcial opt-in/procedencia](LINUX_NETWORK_ADMISSION_POLICY.md)
plan puro/strictdefault/verified_only explicito, exclusiones PID-estado-motivo
junto al sample, no datosmalformed ocultos ni filas/tasas/UID ficticios. >64elegibles
rechazaTODO, no first64; calidad all_encountered_verified NO readiness ni hostglobal.
Snapshot calificado guarda ultimo refresh, no nuevo censo. LayoutAdmissionChange0.x
ampliado: recompilar TODOS. Linux132/132+100focales/consumidor instalado exit0,
MSVC40focales/nativo SOLO2privados+marcador sintetico positivos. Monitor/Sampler,
escala>64 y validacion vista real autorizada pendientes, F03/H04/J04 abiertas.
57/75=76,0%,18/75=24,0%,sin credito parcial.
Espejo95 confirmado: comentario90140272317289; historicos remotos intactos.

Checkpoint94: [admision estricta standalone](LINUX_NETWORK_ADMISSION.md)
inventario-owner-proveedor observado publica; ALLverificados<=64 o rechazaTODO,
consentimiento cadarefresh, conjunto igual conserva epoch/tasas. Sin captura
al construir/polling/rearmado por sample. Nativo SOLO2hijos privados con
enumerador restringido/procfs-pidfd-BPF nativos: gap/origen/unchanged/censos18->16
->26/fork/desconocido/restart/vacio/limpieza pasan. NO vista completa capturada.
Linux131/131+80focales/consumidor nuevoheader-API instalado exit0; MSVC20fixture,
sin nuevos warnings C++/D9025 esperado. F03/H04/J04 abiertas: politica practica
de vistas grandes/inaccesibles/validacion autorizada e integracion admision con
Monitor/Sampler pendientes.57/75=76,0%,18/75=24,0%,sin credito parcial.
Espejo94 confirmado: comentario90140272312863; TXT manda.

Checkpoint93: [backend publico compartido observado](LINUX_NETWORK_SHARED_PUBLIC.md)
activado standalone/Monitor/Sampler/owner. Nativos17 con N+14FDs iniciales,
AB->B->AB con origen retenido/gap excluido/censos16->15->24/tasas-UID/epoch,
move/fork/worker-concurrente/optout sin rearmado/limpieza PASAN. Estricto y
declarado nativos pasan, sin cambio de API/layout/cobertura/consentimiento.
Final93 Linux130/130 sin skips+60focales/consumidor instalado exit0;
MSVC3/3+60focales pasan. Sin nuevos warnings C++/D9025 esperado. Error inicial del asistente
por builds simultaneos mismo directorio/ranlib preservado, reconstruccion serial.
F03/H04/J04 abiertas por admision general y posteriores integraciones, no global.
57/75=76,0%,18/75=24,0%,sin cierre/credito parcial.
Espejo93 confirmado: comentario90140272305719; historicos remotos intactos.

Checkpoint92: [update interno autenticado compartido](LINUX_NETWORK_SHARED_UPDATES.md)
terminado, NO backend publico activado. Retenidos sin reinicio, altas/readmisiones
frescas sin gap, epoch/reset de todas las tasas. Lotes vivos acotados/no slots
reciclados/no3sondas constantes tras churn; no corte atomico entre lotes.
Nativo16->15->16; Linux130/130+40focales/consumidor instalado exit0, MSVC40focales.
Publico/Monitor/Sampler/owner y admision pendientes. F03/H04/J04 abiertas;
57/75=76,0%,18/75=24,0%,sin cierre/credito parcial.
Espejo92 confirmado: comentario90140272299452; historicos remotos intactos.

Checkpoint91: retirada RAW compartida segura terminada, NO update publico.
Nativo17->16->1/origen retenido/8FDs/retirados excluidos aun enviando/limpieza;
fixtures evento suspendido/64->63/regresion/fallos y fork. Linux130/130 sin skips,
MSVC2/2 veinte veces. Altas/reincorporacion/transicion autenticada/publico siguen
pendientes. [Contrato y limites](LINUX_NETWORK_SHARED_RETENTION.md). F03/H04/J04
abiertas;57/75=76,0%,18/75=24,0%. Linux40focales y consumidor instalado exit0.
Espejo91 confirmado: comentario90140272294875; historicos remotos intactos.

Registro interno de referencia: SEGUIMIENTO.txt (no distribuido aquí), revisión95:
57/75 subtareas cerradas (76,0%),11 en curso,5 pendientes,2 bloqueadas;18abiertas(24,0%).
ClickUp es solo espejo. Resumen: [PROGRESS.md](PROGRESS.md). No es porcentaje
de esfuerzo. Las cifras anteriores70% estimado y60,4% histórico quedan retiradas
como indicadores vigentes. Actualizar tras cada unidad con evidencia.
Prioridad revision63 por usuario: [terminar Linux antes de Windows](LINUX_FIRST_PLAN.md).
Siguiente F03 -> H04/J04, una subtarea por vez; implementaciones Windows aplazadas
en checkpoint62. No se elimina alcance ni cambian estados/conteo. Esta revision
solo reordena trabajo: sin codigo/captura/UAC/suites/cierres nuevos. Regresion
Windows solo cuando la compatibilidad compartida lo requiera.
Espejo63 confirmado: comentario90140271872526; titulo/estados historicos no conciliados.
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
work/network-inventory-*. C57/E11/P5/B2,76,0%/24,0%,18abiertas;
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
work/network-selection-*. C57/E11/P5/B2,76,0%/24,0%,18abiertas;
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
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin credito parcial.
Espejo84 confirmado: comentario90140272049411 en la lista.

Unidad anterior revision83: F03 E->E, [proveedor publico observado](LINUX_PROCESS_NETWORK_OBSERVED.md).
Opt-in/default estricto/1..16actores-cookie0/16cookies cada uno/256filas, coverage
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
work/network-public-observed-*. C57/E11/P5/B2,76,0%/24,0%,18abiertas;
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
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin credito parcial.
Espejo82 confirmado: comentario90140272046531 en la lista.

Unidad anterior revision81: F03 E->E, [ventanas seleccionadas observadas](LINUX_NETWORK_DISCOVERED_WINDOWS.md).
Pidfd propio/join por extremos/guard fork/boot-contexto/bracket1s; sin cookie
entrada/vacio conocido/primer bucket sin tasa/A->A+B/4tasas/UID parcial.
Reset errores-contexto-discard sin ocultar regresion registro/contadores,
stop/restart limpio/nuevo epoch/modos separados. Fixtures16cookies/solo exclusiones/
scope0-17-multiactor/8fallos inicio/21lectura/recuperacion; WSL1hijo/pidfd original
cerrado/TCP12288/12288 UDP768/768 peek24/2threads/4tasas-UID oracle/otros excluidos/
fork-padre intacto/discard/restart/exited2veces/FD limpios. SOLO1netns real/oracle
SO inicializa cookie/16cookies fixtures; estricto nativo positivo. Linux126/126
sin skips,2tests20veces/consumidor instalado exit0/sin warnings; MSVC2/2 focalizadas
sin nuevos C++ warnings/D9025 esperado, consumidor75 sigue N03/no total.
Sin API/layout publico/GUI/persistencia/dependencia/CLI/captura global nuevos.
F03 E por multiactor/bracket comun/API-Monitor-Sampler observados/politica general;
H04/J04 abiertas. work/network-discovered-window-*.
Espejo81 confirmado: comentario90140272044981 en la lista.
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision80: F03 E->E, [motor privado cookies observados](LINUX_NETWORK_DISCOVERY.md).
Sin cookie entrada/token kernel actor,16slots/CMPXCHG64/sin eviccion/calidad
sticky global/lectura integral8reintentos/validacion/vacio-detach-stop-reinicio.
Fixtures16cookies/4lanes/peer igual-distinto/capacidad/calidad/rollback y WSL1hijo
sin cookie productor/oracle SO/TCP12288/12288 UDP768/768 peek24/2threads/otros
excluidos/detach/restart vacio luego TCP8192/8192 UDP512/512 peek16/FD limpios.
SOLO1netns real,16cookies/race distinto/capacidad fixtures, oracle inicializa
cookie. Regresion Monitor declarada positiva. Linux126/126 sin skips/2tests20veces/
consumidor instalado exit0/build sin warnings; MSVC2/2 focalizadas/sin C++ warnings
nuevos/D9025 assert esperado, consumidor75 sigue N03/no total. Error macro assert
fixture corregido/log preservado. Sin API/layout publico nuevo/GUI/persistencia/
dependencia/CLI/setns-unshare/captura global. F03 E por join/contexto/ventanas-tasas/
multiactor/proveedor/politica general; H04/J04 abiertas. work/network-discovery-*.
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision79: F03 E->E, [buckets Monitor/Sampler](LINUX_PROCESS_NETWORK_BUCKET_MONITOR.md).
Setup3args modo/consentimiento,2args siempre estricto tras stop; flag/restart no
reautorizan, setup activo no cambia modo. Sampler detenido+flag/update conserva modo.
Fixtures y WSL Monitor/worker4oraculos bucket-UID/retiro-readd/gap/historial/update
concurrente/baselines/default estricto/limpieza positivos. SOLO1netns real/segunda
cookie NO observada, no2netns reales simultaneos ni descubrimiento. Linux126/126
sin skips,3tests20veces/consumidor instalado exit0/build sin warnings; MSVC3/3
focalizadas sin C++ warnings nuevos/D9025 assert esperado, consumidor75 sigue N03/
no total validado. API aditiva/sin layouts ni opciones nuevas/GUI/persistencia/
dependencia/CLI/captura global. F03 E por politica-descubrimiento/buckets dinamicos/
cobertura general, H04/J04 abiertas. work/network-bucket-monitor-*.
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision78: F03 E->E, [buckets declarados](LINUX_PROCESS_NETWORK_BUCKETS.md)
en proveedor individual, constructor opt-in/16filas PID-start-cookie, varias por
proceso/cookies disjuntos, filtro antes de mapa/default estricto intacto/calidad
sticky/dedup-contexto-UID coherentes/coverage nuevo/suma parcial sin duplicado.
Update epoch-reset TODOS baselines/retencion-retiro-readd sin trafico del gap.
Fixtures2cookies positivos4lanes/negativos, WSL1hijo2cookies declarados/SOLO1netns
real/otro cookie no observado:4oraculos/otro0/suma sin duplicado/gap/fork/FDs;
NO2netns reales simultaneos, regresion estricta2hijos positiva. Linux126/126 sin
skips/5tests20veces/consumidor instalado exit0/build sin warnings; MSVC5/5
focalizadas sin warnings C++ nuevos/D9025 assert esperado, fallo consumidor75
sigue N03/no total validado. Error protocolo fixture overflow corregido/log
preservado. API aditiva/enum ampliado/sin layouts nuevos/GUI/persistencia/
dependencia/CLI/setns-unshare/captura global. F03 E por modo buckets Monitor/
Sampler/politica-descubrimiento/buckets dinamicos/cobertura general, H04/J04
abiertas. work/network-buckets-*, C57/E11/P5/B2,76,0%/24,0%,18abiertas.

Unidad anterior revision77: F03 E->E, [Sampler actualizable activo](LINUX_PROCESS_NETWORK_WORKER.md).
Serializa coleccion/commit/update/stop; update pendiente tiene prioridad y wakeup
protegido. Historial conserva epochs, latest no estado de captura. Captura
existente/consentimiento nuevo, update no rearma, fallo cierra captura no worker.
CTest concurrente40updates/start-stop/lecturas,3tests20veces; WSL2hijos privados
AB->B->AB con worker/updater otro hilo/4oraculos actor-UID/gap/origen/baseline/
fallo-restart sin rearmado/FDs limpios. Primer nativo fallo en retirada, eliminada
posible inanicion/repeticion y final positivas/log preservado. Linux126/126 sin
skips/consumidor instalado exit0/build sin warnings, MSVC3/3 focalizadas sin
warnings C++ nuevos. N03 mantiene fallo consumidor75, no compatibilidad total.
API aditiva/sin layouts nuevos/persistencia/GUI/dependencia/captura global.
F03 E por politica-descubrimiento/buckets/cobertura general; H04/J04 abiertas.
work/network-worker-*, C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision76: F03 E->E, [seleccion actualizable](LINUX_PROCESS_NETWORK_UPDATE.md)
en proveedor publico/Monitor. AB->B->AB/reorden, retencion por PID/start/cookie,
retiradas cerradas antes de nuevas, fresh consent/epoch y reset TODOS baselines.
Contadores retenidos conservan origen/nuevos sin trafico del hueco. Runtime
failure cierra TODO; Monitor cualquier fallo autorizado cierra. Fixtures y WSL
2hijos privados4oraculos/gap/fork/salida-recuperacion/sin fugaFD. Linux126/126 sin
skips,2tests20veces/consumidor instalado exit0/build sin warnings. Assert inicial
orden fixture corregido/log preservado; API aditiva/sin layouts nuevos. F03 E por
update worker/politica-descubrimiento/buckets/cobertura general; H04/J04 abiertas.
N03 mantiene fallo consumidor MSVC revision75, no compatibilidad total validada.
MSVC76:2/2 focalizadas pasan, D9025 esperado asserts activos/sin warnings C++ nuevos.
work/network-update-*, C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision75: F03 E->E, [Monitor/Sampler seleccionados](LINUX_PROCESS_NETWORK_MONITOR.md)
integrados Linux, flag false/setup explicito separado, opt-out/worker stop cierran,
restart/flag no reautorizan. WSL2hijos4oraculos/ciclo worker/move/fork/rearmado/
limpieza positivos. Linux126/126 sin skips,18tests red20veces/consumidor instalado
Linux final exit0/build verificado sin warnings. Layout0.x nuevo SystemSnapshot/
SampleOptions: recompilar TODOS. Metric header extraido sin cambio semantico.
MSVC2/2 focalizadas pasa, consumidor general0xC0000409 despues de checks nuevos,
causa NO probada/riesgo N03 pendiente, NO compatibilidad completa validada.
work/network-monitor-*. No captura Windows nueva/GUI/persistencia/dependencia.
F03 E por seleccion dinamica/buckets/cobertura general; H04/J04 abiertas.
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin credito parcial.

Unidad anterior revision74: F03 E->E, [proveedor publico seleccionado](LINUX_PROCESS_NETWORK.md)
terminado. Nuevo LinuxProcessNetworkSampler Pimpl movable con start opt-in/
sample/stop, cobertura tipada/estimated/IDs locales-kernel por clave, contadores/
tasas/calidad/UID inicial parcial/brackets/contexto. Proyeccion pura/34negativos/
baseline0/UID0/overflow/ordenes distintos. WSL API PUBLICA2hijos4oraculos actor-UID/
FDs caller cerrados/move activo/idle0/padre excluido/fork-salida/limpieza.
Linux125/125 sin skips,17tests red20veces, encabezado instalado/consumidor externo
compilado/ejecutado exit0/build final sin warnings. Error fixture corregido/log
preservado. API ADITIVA, sin cambiar layouts/ABI existentes/totales legacy.
No autoMonitor/Windows nuevo/GUI/historial/dependencia, work/network-public-*.
F03 E por integracion opt-in Monitor/politica seleccion-cobertura-buckets;
H04/J04 abiertas. Siguiente flujo general seleccionado sin total falso,
pruebas privadas/no captura global. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo74 confirmado: comentario90140271926244; titulo/estados remotos historicos.

Unidad anterior revision73: F03 E->E, [ventana nominal comun/UID](LINUX_NETWORK_COMMON.md)
terminada. Joins todos antes/despues/barrido<=1s/commit posterior; seleccion1..16/
dedup TGID-start, fin nominal comun/tasas estimadas/UID parcial con availability/
overflow/brackets/asociaciones por clave. Reset todos baselines ante fallo/
excepcion/cambio/modo. Fixtures mismoUID/2UID/12fallos/1s/>60s/overflow/modos/fork,
WSL2hijos privados4oraculos actor-UID/FDs caller cerrados/idle0/padre excluido/
fork/salida/limpieza. Regresiones nativas selected/independent exit0.
Linux124/124 sin skips,16tests red20veces, consumidor instalado exit0/build sin
warnings, mismos CTests ampliados. work/network-common-*. Sin Windows/Monitor/
API/ABI/historial/dependencia nueva. No snapshot atomico/global/cookies dinamicos.
F03 E por contrato cobertura/buckets/proveedor publico; H04/J04 abiertas.
Siguiente cobertura/proveedor opt-in seleccionado, pruebas privadas, no captura
global. C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin credito parcial.
Espejo73 confirmado: comentario90140271923048; titulo/estados remotos historicos.

Unidad anterior revision72: F03 E->E, [coleccion multiactor seleccionada](LINUX_NETWORK_MULTI.md)
terminada. Lista fija1..16/dedup local-kernel/observer-boot comun, captura/dup
pidfd por actor y rollback TODOS los baselines ante fallo/excepcion/cambio.
Intervalos propios, no snapshot simultaneo/suma UID comun. Fixtures2actores
2s/3s/mismo UID-cookie/5fallos inicio/4fallos segunda fila/reset/fork y WSL2hijos
privados autorizados A4096/4096/256/256B B8192/8192/512/512B,4oraculos por actor,
trafico cruzado-padre excluidos/idle0/FDs entrada cerrados/duplicado-fork/salida/
limpieza sin fugaFD. Linux124/124 sin skips,16tests red20repeticiones, consumidor
instalado exit0/build final sin warnings. Warning indentacion inicial corregido,
log preservado. work/network-multi-*. Sin Windows/API/ABI/Monitor/historial/
dependencia nueva. F03 E por ventana comun/agregacion UID/cobertura/buckets/
integracion; H04/J04 abiertas. Siguiente ventana comun acotada/agregacion UID,
pruebas privadas, no captura global. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo72 confirmado: comentario90140271920104; titulo/estados remotos historicos.

Unidad anterior revision71: F03 E->E, [ventanas seleccionadas revalidadas](LINUX_NETWORK_SELECTED_WINDOW.md)
terminadas. Dup pidfd propio, joins frescos extremos/kernel/boot/observer/contexto,
commit reductor solo tras exito y baseline descartado ante fallo/excepcion/cambio.
Tipo cobertura1actor/cookie separado SELF/global, productor distinto actor,
epoca compartida/4tasas estimadas/UID parcial. Fixtures FD0/8fallos inicio/
15fallos muestra/reset/fork y WSL hijo privado autorizado PID402/kernel913 UID0,
oraculo4tasas/UID/caller FD cerrado/idle0/fork/reinicio/salida/sin fugaFD.
Linux123/123 sin skips,15tests red20repeticiones, consumidor instalado exit0.
Primer relink warnings make por timestamps DrvFS, build repetido sin warnings/
suite repetida sin tocar reloj; logs preservados work/network-selected-window-*.
Sin Windows/API/ABI/Monitor/historial/dependencia nueva. F03 E por multiactor/
buckets/cobertura/integracion publica, cookie aun declarado; no atomicidad/ABA/
vida futura/plazo duro. H04/J04 abiertas. Siguiente cobertura/buckets/multiactor,
pruebas privadas, no captura global. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo71 confirmado: comentario90140271917419, titulo/estados remotos historicos.

Unidad anterior revision70: F03 E->E, [join seleccionado pidfd/procfs](LINUX_NETWORK_PIDFD_JOIN.md)
terminado: dup/poll, boot/contexto observador/objetivo estables por extremos,
dos tokens kernel iguales, fdinfo del hilo caller/PID/start/status/ns revalidados.
Dominios separados, UID procfs del observador, no conversion ticks/ns. WSL hijo
privado autorizado PID2513/kernel6353 UID0, parent-read start/join repetido/
negativos/captura4lanes/limpieza sin fugaFD. Tabla FD worker separada de lider
probada sin privilegios: pipe/pidfd con mismo numero correctamente distinguidos.
Linux122/122 sin skips,14tests red20repeticiones cada uno, consumidor instalado
exit0/build sin warnings. work/network-join-*. No atomicidad/ABA/vida futura/
cookie externo automatico/cobertura global. Sin Windows/API/ABI/Monitor/historial/
dependencia nueva. F03 E por ventanas revalidadas/multiactor/buckets/cobertura/
integracion; H04/J04 abiertas. Siguiente ventanas/tasas seleccionadas revalidadas,
pruebas privadas. C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin credito parcial.
Espejo70 confirmado: comentario90140271913864; titulo/estados remotos historicos.

Unidad anterior revision69: F03 E->E, [token desde pidfd](LINUX_NETWORK_PIDFD.md)
terminado. Dup/liveness, consulta FD-1/EBADF con sonda FEXIT de permisos/hilo
caller/modo0x12/ret0, BTF y resultado unico; sin attach ptrace/senales/parada del
objetivo/recorrido global. Mascara caller temporal restaurada, puede retrasar
handlers. WSL autorizado hijo sin reportar kernel token PID554/kernel1640 UID0,
2consultas/negativos/captura TCP4096/4096 UDP256/256/8peeks/padre excluido/detach/
start-UID incorrectos sticky/retiro/mask/sin fugaFD. Linux120/120 sin skips,
12tests red20repeticiones, consumidor instalado exit0, build final sin warnings.
work/network-pidfd-*, __pidfd_fget ausente descartado; warning pair corregido y
target nuevo reconfigurado. Sin Windows/Monitor/API/ABI/historial publico/
dependencia nueva. F03 E por join/revalidacion procfs/contexto/multiactor/buckets/
cobertura/integracion; H04/J04 abiertas. Siguiente join pidfd-procfs seleccionado,
pruebas privadas. C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin IDs/cierres/credito.
Espejo69 confirmado: comentario90140271911142; titulo/estados remotos no conciliados.

Unidad anterior revision68: F03 E->E, [captura de actor seleccionado](LINUX_NETWORK_SELECTED.md)
terminada: token kernel/cookie declarados, no identidad externa autenticada;
tipo explicito separado de SELF,8puntos fallo/excepcion/rollback/FD0/reinicio.
WSL autorizado hijo cooperativo PID2075/kernel2600,UID0,TCP4096/4096B,UDP256/256B,
8peeks/padre excluido/detach/start-UID falsos sticky/pidfd salida/limpieza sin
fugaFD. SELF asociado nativo PASS. Linux119/119 sin skips,11tests red20repeticiones,
consumidor instalado exit0, build sin warnings, work/network-selected-*.
Sin Windows/Monitor/API/ABI/historial publico/dependencia nueva. F03 E por
identidad externa confiable/multiactor/buckets/cobertura/integracion; H04/J04
abiertas. Siguiente F03 descubrimiento/revalidacion confiable de actores, pruebas
privadas/permisos acotados. Preferencia GUI/CMake en LINUX_FIRST_PLAN.md.
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin IDs nuevos/cierres/credito parcial.
Espejo68 confirmado: comentario90140271905836; titulo/estados remotos no conciliados.

Unidad anterior revision67: F03 E->E, [SELF kernel/procfs](LINUX_NETWORK_SELF_JOIN.md)
asociado por caller directo: token local PID/start_ticks/UID/procfs/namespaces,
lector pinned/acotado caller/lider y doble identidad BPF fresca/boot estable.
No conversion ticks/ns ni igualdad de IDs; rechazo/cambio/permiso/contexto/raw-
joined invalida baseline. Linux119/119 sin skips,11tests red20repeticiones,
consumidor instalado exit0. Procfs nativo UID1000/PID950 estable20; WSL autorizado
PID390/kernelTGID4965,UID0/0,TCP4096/4096B,UDP256/256B,8peeks/oraculo4tasas-UID/
793949ns,idle0/fork/reinicio/sin fugaFD. work/network-self-join-*. Warning
indentacion corregido, timestamps WSL preservados, build final sin warnings.
Sin Windows/API/ABI/historial publico/Monitor/dependencia nueva. Solo SELF por
extremos, no snapshot atomico/actores externos/apps. F03 E por multiactor/buckets/
identidad externa/cobertura/integracion; H04/J04 abiertas, siguiente F03 actores
externos con pruebas privadas/permisos acotados.
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin IDs nuevos/cierres/credito parcial.
Espejo67 confirmado: comentario90140271891145; titulo/estados remotos no conciliados.

Unidad anterior revision66: F03 E->E, [ventanas/tasas de captura SELF](LINUX_NETWORK_WINDOW.md)
conectadas. Sampler posee captura/scope/epoch/reductor, dominios iniciales del
kernel tipados, scope1fila/cookie, boot UUID procfs y bracket monotono <=1s;
tasas estimated entre finales/max60s, UID parcial. Ausencia descarta baseline,
reinicio cambia epoch, fork rechaza copia. Linux117/117 Release,9tests red
20repeticiones, consumidor instalado reconstruido/exit0. WSL autorizado TCP4096/
4096B,UDP256/256B,8peeks,oraculo4tasas/UID,646362ns, idle0/fork/reinicio/sin fugaFD.
work/network-window-*. Build inicial clock-skew0.27s, verificado sin warnings.
Sin Windows/Monitor/API/ABI/historial publico/dependencia nueva. F03 E por
multiactor/buckets/join procfs-apps-UID/cobertura/integracion publica; H04/J04
abiertas. Siguiente F03 identidad procfs para el productor.
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin IDs nuevos/cierres/credito parcial.
Espejo66 confirmado: comentario90140271884970; titulo/estados remotos no conciliados.

Unidad anterior revision65: F03 E->E, [namespace del socket](LINUX_NETWORK_NAMESPACE.md)
terminado: BTF opcional skc_net.net->net.net_cookie, restriccion SELF declarada,
mismatch/nulo/lectura fallida invalida de forma sticky, tambien peeks positivos.
Linux116/116 Release,8tests red20repeticiones, consumidor instalado reconstruido/
exit0. WSL autorizado cookie1 corroborado SO_NETNS_COOKIE, TCP6144/6144B,
UDP384/384B,12peeks/hijo excluido/threads/retirada/reinicio sin fugaFD; cookie
distinto rechazado persistentemente. Modo anterior PASS, work/network-netns-*.
Sin setns/unshare/configuracion/dependencia nueva ni Windows/API/ABI/historial
publico/Monitor. No prueba multins real. F03 E por buckets/multiactor/join/
cobertura/ventanas/integracion; H04/J04 abiertas, siguiente continuar F03.
C57/E11/P5/B2,76,0%/24,0%,18abiertas; sin IDs nuevos/cierres/credito parcial.
Espejo65 confirmado: comentario90140271880147; titulo/estados remotos no conciliados.

Unidad anterior revision64: F03 E->E, [sesion privada Linux SELF](LINUX_NETWORK_SESSION.md)
con ciclo de vida terminada: opt-in/identidad exacta/mapa9/carga antes de attach,
rollback en14puntos de fallo/excepcion, FD0, lectura coherente acotada y retiro/
destructor/reinicio. Linux116/116 Release, fixture20repeticiones, consumidor
instalado reconstruido/exit0. Nativo WSL autorizado PASS TCP6144/6144B,
UDP384/384B,12peeks excluidos/hijo excluido/threads/retirada/reinicio/sin fugaFD;
sin bandera77 confirmado. Build inicial aviso clock-skew0.0062s, posterior sin
warnings; wrapper77 corregido por quoting. work/network-session-*. Sin avance
Windows ni API/ABI/historial publico/Monitor nuevos. F03 sigue E por netns/join/
multiactor/cobertura/ventanas/integracion, H04/J04 abiertas; siguiente F03.
C57/E11/P5/B2,76,0%/24,0%,18abiertas, sin nuevos IDs/credito parcial.
Espejo64 confirmado: comentario90140271876915; titulo/estados remotos no conciliados.

Unidad anterior revision62: M02 E->E, [correlacion IRP](WINDOWS_DISK_CORRELATION.md)
terminada: TDH init/completion version3/IRP32y64, identidad al inicio conservada
tras salida/reuso PID/TID. Colisiones/duplicados/huerfanos/epoca/orden/cobertura/
TTL/limites,10000pares y pipeline simulado probados. Windows136/136 ambas,
Linux115/115 Release,20repeticiones decoder/correlador por entorno,3consumidores
instalados reconstruidos/exit0, work/disk-correlation-*. Fixture far macro SDK
corregido, D9025MSVC esperado; sin ETW/UAC/nueva prueba nativa/Monitor ni cambios
API/ABI/persistencia/dependencias publicas. Productor nativo ciclos/inicioI/O/
reconciliacion/drenaje/perdidas/disco/red generales pendientes; cobertura aun
precondicion del caller. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo62 confirmado: comentario90140271870463; titulo/estados remotos no conciliados.

Unidad anterior revision61: M02 E->E, [indice temporal PID/TID](WINDOWS_TRACE_IDENTITY.md)
terminado. Epoca/generaciones/vidas QPC, lookup inicio real/intervalo, no
completion ni IDs actuales. Cobertura completa precondicion del caller, no
certificada. Reemplazo validado sin stale/solapamientos/duplicados, contencion/
limites y100000registros, oraculo20000. Windows135/135 ambas,Linux114/114 Release,
20repeticiones por entorno,3consumidores instalados reconstruidos/exit0,
work/trace-identity-*. Sin ETW/UAC/reintento MSVC cancelado, sin API/ABI/persistencia/
dependencias publicas nuevas. M02 productor nativo ciclos/inicioI/O, correlacion
IRP/drenaje/perdidas/disco/red generales pendientes; no captura completa garantizada.
C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo61 confirmado: comentario90140271604191; titulo/estados remotos no conciliados.

Unidad anterior revision60: M02 E->E, [decoder/conteo privado ETW](WINDOWS_DISK_EVENTS.md)
terminados. TDH nativo por nombres/4bytes/version3, no PID/header/offsets manuales.
Hilo propio vivo/ventana QPC/uint64/cero/overflow, oraculo10000. MinGW nativo PASS
read1048576B/write1142784B,16reads/38writes, errores/perdidas reportadas0,
archivo y sesion retirados, sin ETL. Primer schemaError13 conservado, causa exacta
no diagnosticada. MSVC nativo no ejecutado por elevacion cancelada, no reintentada.
Windows134/134 ambas,Linux113/113 Release,20repeticiones decoder/ciclo por Windows
toolchain,3consumidores instalados reconstruidos/exit0, work/etw-disk-events-*.
tdh nativoWindows exportado en paquete; sin API/ABI/persistencia publica nueva.
M02 general disco/red/identidad temporal/drenaje/perdidas pendientes; no captura
completa garantizada. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo60 confirmado: comentario90140271603279; titulo/estados remotos no conciliados.

Unidad anterior revision59: M02 E->E, [control/ciclo de vida ETW](WINDOWS_DISK_TRACE_LIFECYCLE.md)
terminado tras autorizacion prosigue a solicitud58. Sesion propia sinETL, eventos
todos descartados, sin Monitor/default; propiedad/collision/errores/buffers/flags/
idempotencia/cierre concurrente probados. Elevado MinGW/MSVC normal y watchdog
PASS, STOP0/QUERY4201 ausencia confirmada,64KiB4/max64. Windows133/133 ambas,
Linux112/112 Release,20repeticiones fixtures por Windows toolchain,3consumidores
instalados reconstruidos/exit0. work/etw-lifecycle-*. Fallos iniciales ANSI/wide,
flag hibrido13 y UPDATE87 corregidos, logs conservados y retirada confirmada en
todos los intentos creados. Sin API/ABI/persistencia nueva. No llegada/atribucion
de eventos ni disco/red por PID validados aun; M02 sigue E, C57/E11/P5/B2,76,0%/24,0%.
Espejo59 confirmado: comentario90140271601441; titulo/estados remotos no conciliados.

Punto de control anterior revision58: M02 E->E, [decision ETW disco](WINDOWS_PROCESS_DISK_DECISION.md).
Prueba temporal15s/64MiB/sinETL pendiente de autorizacion: puede recibir eventos
del sistema y requerir elevacion, retener solo datos privados. No implementado
ni ejecutado, sin nuevas suites ni cierre; resultados vigentes revision57.
Espejo58 confirmado: comentario90140271600221; titulo/estados remotos no conciliados.

Unidad anterior implementada revision57 (2026-10-21) M02 E->E: E/S general Windows terminada.
[Contrato](WINDOWS_PROCESS_IO.md). ProcessInfo.windows_io: seis contadores
uint64 GetProcessIoCounters y tasas estimated por identidad/ventana propia,
no disco fisico ni red causal. Campos especificos siguen sin proxy. Baseline,
reuso/errores/reset/limites, cero y64bits probados. Windows132/132 MinGW/MSVC,
Linux112/112 Release,20repeticiones por Windows toolchain; pipe privado1024B
read/write1024B positivos, Monitor baseline/rates/off-on,3consumidores instalados
reconstruidos/exit0. work/process-io-verified-* y install/consumer. Layout publico
ProcessInfo0.x cambia: reconstruir biblioteca y TODOS los consumidores. Sin
vtable/persistencia nuevos. C4312 fixture volumes corregido, D9025MSVC esperado.
M02 disco/red especificos, atribucion/cobertura pendientes; M04 global/hardware
pendiente, M05 aplazada. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo57 confirmado: comentario90140271599726; titulo/estados remotos no conciliados.

Unidad anterior revision56 (2026-10-21) M04 E->E: AMD experimental explicito terminado.
Usuario eligio opt-in separado21oct. [Contrato](WINDOWS_AMD_RUNTIME_MEMORY.md).
Nueva API/tipos/dos cantidades estimated/driverCL; no global ni Monitor/Sampler/
default/historial. ABI size_t[2]/KiB aclarado por primarias; no repetir formato
desconocido como bloqueo: persiste alcance runtime/heaps/caches/budget/reservas.
Interop unica observada/revalidada no identidadfisicaLUID, validaciones/64bits/
cero/overflow. Helperextraido preserva proveedor versionCL. RX7900XT positivo
ambas toolchains, primera muestra21435580416B/bloque21413273600B, Monitor globalAMD
unknown. Windows131/131 ambas,Linux111/111 Release,20repeticiones AMD einterop
por Windows toolchain,3consumidores instalados reconstruidos/exit0. work/amd-runtime-*;
expectativa inicial4cierres corregida a opened==closed en rechazo temprano, logs
conservados. D9025MSVC esperado, sin warningsC finales. API0.2 adicional sin
layouts/vtable/formatos existentes cambiados. M04 globalAMD/Intel/NVIDIAfisico/
matriz pendientes, M05 aplazada. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo56 confirmado: comentario90140271597603; titulo/estados remotos no conciliados.
Unidad anterior revision55 (2026-10-21) M04 E->E: loader NVML System32/legacy terminado.
[Contrato/evidencia](WINDOWS_NVML_LOADER.md): NVSMI por KnownFolder ProgramFilesX64
y sufijo fijo solo ante ausenciaSystem32, no bypass denegacion/badimage ni PATH/
entorno/instalar. Ruta absoluta validada/flags acotados, sin certificar firma/ACL/
reparse/modulos del host. CUDA/KMT y13simbolos/identidad/memoria sin cambios;
diagnostico Win32 en nota y sin status0 ficticio. Windows130/130 ambas,
Linux111/111 Release,20repeticiones loader y memoria por Windows toolchain,
3consumidores instalados reconstruidos/relinkados/exit0. work/nvml-legacy-*,
tipos/narrowing inicial MinGW corregidos, D9025MSVC test esperado. NativeAMD
ambas ubicaciones sin DLL126, sin positivo fisico NVIDIA. Sin API/ABI/vtable/
formatos nuevos, Linux intacto. M04 freeAMD/Intel/cobertura pendientes, M05
aplazada. C57/E11/P5/B2,76,0%/24,0%,18abiertas. Espejo55 comentario90140271596413.
Unidad anterior revision54 (2026-10-21) M04 E->E: proveedor NVML free Windows terminado.
Campo existente memory_available_bytes, bytes framebuffer sin asignar NVMLv1,
no garantia allocatable/presupuesto/restaDXGI-PDH. [Contrato](WINDOWS_NVML_MEMORY.md).
LUID/UUID CUDAv2->NVML root UUID exactos/unicos/relecturas, KMTcountfisico1
antes/despues, limites/errores/no-write/parcial/MIG/linked unknown, cero/64bits
conocidos. Uso/capacidad/shared/engines otros intactos, integrado tras PDH.
System32 DLLs opcionales sin SDK/CLI/instalacion/fallbackNVSMI,13simbolos/caller64,
sin crear contextos/buffers/kernels/workers propios; cuInit puede conservar
estado/hilos internos del driver; NVMLbalanceado yKMT/DLL RAII. Windows129/129
ambas,Linux111/111 Release,20repeticiones por Windows toolchain,3consumidores
instalados reconstruidos/relinkados/exit0. work/nvml-free-* finales final-* y
release-repeat-*; D9025MSVC test esperado, sin nuevos warningsC. HostAMD/probeDLLs
ausentes, NO positivo fisico NVIDIA/WDDM/TCC/multiGPU/MIG/linked/hotplug. Sin
API/ABI/vtable/formatos nuevos, Linux intacto. M04 freeAMD/Intel/cobertura
pendientes, M05 aplazada. C57/E11/P5/B2,76,0%/24,0%,18abiertas. Espejo54 comentario90140271595547.
Unidad anterior revision53 (2026-10-21) M04 E->E: Vulkan MSVC y unicidad terminados.
CMake Windows admite PCINSIGHT_VULKAN_INCLUDE_DIR/VULKAN_SDK, privado biblioteca/
test, headers ya locales sin instalacion/descarga/export ni linkloader. MSVC
antes not-built, ahora RX7900XT1.4.349 por LUID igual MinGW. OFF probado1/1
aun con headers; -U o buildnuevo para cache de comprobacion antiguo. Destinos
de ID repetido/vacio unknown, otros unicos conservados, no valor stale.
[Contrato/evidencia](WINDOWS_GPU_VULKAN.md), Windows128/128 ambas,Linux111/111
Release,20repeticiones Vulkan por Windows toolchain,3consumidores instalados
reconstruidos/relinkados/exit0. work/vulkan-portable-*, sin API/ABI/vtable/
formatos nuevos; D9025MSVC test intencional. Investigacion AMDfree acotada sin
decoder inventado/resta DXGI-PDH. M04 memoria global/cobertura pendiente,
M05 aplazada y demas IDs conservados. C57/E11/P5/B2,76,0%/24,0%,18abiertas.
Espejo53 confirmado: comentario90140271591287; titulo/estados remotos no conciliados.
Unidad anterior revision52 (2026-10-21) M04 E->E: WGL opt-in terminado. Opcion
existente/defaultfalse, GL_VERSION de contexto desktop llamador por LUID EXT
Win32 estable/unica fila DXGI; no maximo/nodo fisico/aceleracion ni fallback
por nombre. Contratos/limites/duplicados/version/cambios/sentinels, System32RAII/
funciones por contexto; biblioteca no crea ventanas/contextos ni draw/glGetError.
[Contrato](WINDOWS_CURRENT_OPENGL.md). Diagnostico privado querynodo AMD1280:
prototipo por nodo descartado, ambito final ADAPTADOR viaLUID documentado sin
consulta ni inferencia de nodo. Native RX7900XT4.6.0 atribuido correctamente en
MinGW/MSVC directo+Monitor, sin error nuevo final ni consumir previo/contexto/DC
cambiados. Test restaura/limpia ventana/contexto/clase privados. Windows128/128
ambas,Linux111/111 Release,20repeticiones finales WGL por Windows toolchain,
3consumidores instalados recompilados/relinkados/exit0. API/layout/vtable/formatos
sin cambios, Windows opcion antes ignorada ahora opt-in, Linux EGL/GLX intacto,
Sampler no hereda contextoGUI. Logs definitivos work/wgl-release-*; cero conocido/
salida ausente o parcial/HighPart negativo probados, warning GCC inicial corregido,
D9025MSVC /UNDEBUG test esperado. M04 resto criterio/memoria global/cobertura
abiertos, M01 abierta/M05 aplazada/N01/A05/
N03 conservados. C57/E11/P5/B2,76,0% cerrado/24,0% abierto. Espejo52 comentario90140271590512.
Unidad anterior revision51 (2026-10-21) M01 E->E: compatibilidad MSVC x64 terminada,
VS18/MSVC19.51/SDK26100. SDK rename faltante corregido por ABI interna documentada
con offsets/static_assert/mismo simbolo y control por handles, sin WDK/fallback.
C++ /utf-8+test literal, tipos/ASCII/variable explicitos; test negativo del reloj
portable usando tick nativo, sin cambiar validador/almacen. Ver
[MSVC_VALIDATION.md](MSVC_VALIDATION.md). MSVC127/127,MinGW127/127,Linux111/111
Release,80repeticiones MSVC/100Linux,3consumidores instalados separados exit0.
Native Startup privado positivo y CPU/firmware host, no mutacion real Startup/
SCM ni hardware multigrupo certificado; manualSCM compilado no ejecutado.
WarningsC finales corregidos, D9025 MSVC de /UNDEBUG test intencional conservado;
MinGW/Linux sin warnings. Logs previos preservados, finales work/msvc-verified-*
y msvc-native-suite/post-clock/consumidores. Sin API/layout/vtable/formato nuevo,
version0.2. M01 sigue compresion estable/hardware multigrupo; N01/A05/N03 sin
cierre, M05 aplazada por confirmacion explicita21oct. C57/E11/P5/B2,76,0%
cerrado/24,0% abierto,18subtareas pendientes, sin credito fraccionario.
Espejo51 confirmado: comentario90140271587468.
Unidad anterior revision50 J05 P->C por capacidad indisponible justificada, la
alternativa del criterio existente; NO desconexion real implementada. Nuevo
query_linux_disconnect_capability por adaptador utmp/login1, sin I/O, false/
available+motivo Linux, enum desconocido/noLinux unknown. Inventario propaga
solo metadata conocida/reconocida; desconocida ahora unknown, no false generico.
Fundamento nativo [documentado](LINUX_DISCONNECT.md), sin deducir capacidad de
ID/Active/Remote/State/token logout ni afirmar imposibilidad de todo transporte
remoto. No señales/bloqueo/logout/elevacion; J02/M06 separados, J03 no duplicado.
Linux110/110,Windows126/126 Release,60repeticiones por plataforma, consumidores
ambos recompilados/relinkados/ejecutados exit0. Hijo POSIX/utmp privados intactos,
login1 real solo lectura1sesion/State/Active. Nuevo casoNUL inicialmente fallido
por expectativa corregido exigiendo excepcion existente, sin cambiar validador;
logs iniciales conservados. work/disconnect-capability-*. Sin layouts/vtable/
historial/dependencias nuevas. C57/E11/P5/B2,76,0% cerrado/24,0% abierto.
M05 aplazada sin consentimiento experimental por 'prosigue'. Espejo50 comentario90140270925508.
Unidad anterior revision49 M05 E->E: investigacion documental, sin codigo ni nuevas
pruebas. StartupTask administra el propio paquete y RequestEnableAsync no anula
desactivacion del usuario; no resuelve control general de otras aplicaciones.
Fuentes oficiales revisadas no aportaron contrato general StartupApproved; no
se afirma inexistencia universal de API. Run6801 rev48 sigue pendiente sin
reintentos/reconfiguracion/causa atribuida. Decision solicitada: aplazar M05 o
aceptar proveedor experimental interno limitado por version. No se cambian
criterios ni se autorizan writes reales/UAC/ACL. Ver
[decision tecnica](WINDOWS_STARTUP_APPROVAL_DECISION.md). C56/E11/P6/B2,74,7%
completo/25,3% abierto. Suites126/110 son evidencia48, NO reejecutadas aqui.
Espejo49 confirmado: comentario90140270919388.
Unidad anterior revision48 M05 E->E: HKLM32/64 lectura/listado terminados, control
transaccional implementado/modelo probado pero native positivo pendiente6801.
Listado por vista separado, read/accion por ID, listado/Monitor viejos soloHKCU.
Software fijado por vista, flags explicitos, backup mismahive-view, permisos/
arquitecturas correctos/sin fallback. Audit Run copia request antes callback.
HKLM real32=1/64=4filas y lectura individual positivas sin writes; native privado
redirigido6801sinwrites. No mutacion HKLM real/ACL elevada/legacy virtualizacion/
MSIX certificada. WINDOWS_MACHINE_RUN_CONTROL.md y work/machine-run-pinned-*.
Linux110/110,Windows126/126 Release,60repeticiones Windows/20Linux, consumidores
ambos recompilados/relinkados/ejecutados exit0, builds finales sin warnings.
API/enum adicionales sin virtual/layouts/formatos, sin Startup/HKLM real tocado.
Comportamiento0.x HKLM all-users requiere consentimiento host, sin UAC/ACL/config.
M05 sigue native positivo Run/StartupApproved/aprobacion/accion generica Windows,
A05/L05/N03 abiertos. C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo48 confirmado: comentario90140270915863.
Unidad anterior revision47 M05 E->E: Common Startup ARCHIVOS terminado, scope por
ID/Known Folder y listado comun separado. User/Monitor siguen solo usuario.
Comportamiento0.x ampliado: impacto todos los usuarios con derechos actuales,
aviso/consentimiento host; no elevacion/ACL/fallback. Checked/audit/rename
conservados, native privado dos raices positivo y conflictos/scopes/root/
aislamiento verificados, error de permisos de enumeracion inyectado. Common
real solo lectura; no ProgramData/UAC/ACL mutantes ni denegacion ACL nativa
probados. WINDOWS_COMMON_STARTUP_FILE_CONTROL.md, logs work/startup-common-*.
Linux110/110,Windows125/125 Release,60repeticiones Windows, consumidores ambos
recompilados/relinkados/ejecutados exit0/builds sin warnings. API libre adicional,
sin nuevos virtual/layouts/formatos ni Startup real tocado/fixtures residuales.
M05 sigue por HKLM/aprobacion/StartupApproved/accion generica Windows; Run6801
no resuelto, A05/L05/N03 abiertos. C56/E11/P6/B2,74,7% completo/25,3% abierto.
Espejo47 confirmado: comentario90140270909066.
Unidad anterior revision46 M05 E->E: accion explicita de archivos Startup integrada
en SystemActions/AuditedActions, operacion distinta/ordinales preservados,
request copiado antes de callback, checked backend sin fallback. Log ID sensible
bounded sin contenidos/identidades. Politicas requested/completed/best-effort y
excepcion conservadas. Native privado auditado retiro/restore y fallos de sink
positivos, portable forwarding/errores/limites/callback mutante. Nuevo virtual0.x
requiere recompilar TODOS los consumidores/backends; sin layouts Snapshot/Options/
AuditEvent ni formatos cambiados. WINDOWS_STARTUP_FILE_AUDIT.md,
work/startup-file-audit-*.log. Linux110/110,Windows125/125 Release,
40repeticiones Windows/20Linux, consumidores ambos recompilados/relinkados/
ejecutados exit0. Builds finales sin warnings, inicial WSL Clock skew registrado.
Run6801 confirmado sin writes; suites anteriores incluian esa prueba aunque
notas 'no repetido' (sin sondeo dedicado). Sin Startup real tocado/fixtures
residuales; M05 sigue HKLM/common/aprobacion/accion generica Windows, A05/N03.
C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo46 confirmado: comentario90140270903626.
Unidad anterior revision45 M05 E->E: informe de respaldos Monitor/Sampler opt-in
terminado, separado de Startup/Run. Off no invoca proveedor, on conserva
empty/error/permisos/nota sin datos viejos; snapshots propios, latest/history/
timed_history sin nueva persistencia/acciones/ventana60s. Native fixture privada
positiva de llenado y Monitor/Sampler reales read-only; gates/errors/copy inyectados.
SI cambia layout0.x Snapshot/Options, recompilar biblioteca y TODOS los consumidores;
tipos proveedor trasladados sin cambio, sin ABI virtual/formatos persistidos nuevos.
WINDOWS_STARTUP_FILE_MONITOR.md, logs work/startup-backup-monitor-*.log. Linux109/109,
Windows124/124 Release,40repeticiones Windows/20Linux, consumidores ambos exit0.
Clock skew WSL en compiler_depend.make registrado/pasada settled adicional,
ultima exit0/sin warnings ni recompilaciones/enlaces; Windows sin warnings,
sin cambiar reloj/configuracion ni atribuir causa. M05 conserva HKLM/common/
aprobacion/audit/acciones genericas pendientes, Run6801 no resuelto/repetido,
A05/N03 abiertos. C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo45 confirmado: comentario90140270896765.
Unidad anterior revision44 M05 E->E: listado explicito de respaldos terminado,
nombre UTF-8 y registration para restaurar sin IDs guardados. Enumeracion por
handle, dos pasadas/identidades/contexto/conflictos, acotada/estimated/no atomica;
no crea respaldo ni escribe, errores/permisos sin lista parcial. Native fixture
privada positiva, multiples archivos/Unicode/tamano0/orden/exclusiones/conflictos;
fallos/records/limites/permisos inyectados y cambios rechazados. Documento
WINDOWS_STARTUP_FILE_CONTROL.md, logs work/startup-backup-listing-*.log.
Linux108/108,Windows123/123 Release,20repeticiones Windows, consumidores ambos
recompilados/relinkados/ejecutados exit0, builds finales sin warnings. Solo API/
tipo adicionales, sin layouts existentes/ABI virtual/historial modificados.
Sin CLI/elevacion/KTM/Startup real modificado; M05 sigue por HKLM/common/
aprobacion/integracion, Run6801 no resuelto ni repetido. No cierre completo.
C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo44 confirmado: comentario90140270891958.
Unidad anterior revision43 M05 E->E: accion explicita nueva de retirar/restaurar
archivos Startup del usuario actual, nativa POSITIVA en fixture privada fuera
del Startup real. Identidad carpeta/archivo comprobada y rename no-replace por
handles/NtCreateFile/NtSetInformationFile, respaldo PCInsight.Disabled.v1,
mismo archivo/bytes/atributos. Read no crea, caller conserva ID para restore,
no listado completo de respaldos integrado aun. Solo drive local; no UNC/common/
HKLM/StartupApproved ni integracion SystemActions/Monitor/audit automatica.
Sin CLI/KTM/elevacion/fallback ni cambios a layouts existentes/ABI virtual/historial. Documento
WINDOWS_STARTUP_FILE_CONTROL.md, logs work/startup-file-control-*.log.
Linux108/108,Windows123/123 Release,20repeticiones Windows; consumidores ambos
recompilados/relinkados/ejecutados exit0, builds finales sin warnings. Readonly
conservado, conflictos/stale/sharing/cambios/limites probados;2fixtures abortadas
por expectativa readonly incorrecta limpiadas por nombre exacto, finales limpias.
Pendientes M05 conservados, Run TxR6801 no solucionado/repetido. No cierre.
C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo43 confirmado: comentario90140270888697.
Unidad anterior revision42 M05 E->E: corregida aceptacion de raiz Startup con colon
de stream por is_absolute (regresion fallo antes/paso despues). Drive/UNC normal/
extendido, rechazo antes de enumerar, cleanup; GLOBALROOT/pipes/UNC incompleto
ya se rechazaban en runtime actual, sin fallo previo atribuido. Documentado en
WINDOWS_STARTUP_FOLDERS.md, evidencia work/startup-root-*.log. Linux108/108,
Windows122/122 Release,20repeticiones Windows; consumidores instalados ambos
relinkados/ejecutados exit0, builds sin warnings. Run TxR sigue6801 sin writes;
sin cambiar KTM/Startup real. Acciones/aprobacion pendientes, no cierre M05.
C56/E11/P6/B2,74,7% completo/25,3% abierto. Espejo42 confirmado: comentario90140270883882.
Unidad anterior revision41 L03 B->C: opcion avanzada limitada aceptada tras
propuesta concreta con advertencia de PID/TID reutilizable, sin checked/EcoQoS/
threads nuevos/restore garantizados. SystemActions generico on/off Linux ahora
no soportado sin tocar destino, availability false/available; checked sigue
no soportado, sin fallback. apply/restore avanzados exigen ack=true explicito
y recibo conservado tras fallos. Cambio de COMPORTAMIENTO0.x documentado en
LINUX_EFFICIENCY.md y LINUX_EFFICIENCY_DECISION.md, sin firmas/layouts/ABI
virtual/historial nuevos; Windows EcoQoS intacto. Nativo hijos privados con
rechazos sin cambios, opt-in2threads, restore completo/mixto denegado con flags
pendientes, caller intacto e hijos recolectados. Linux108/108,Windows122/122,
20repeticiones Linux y consumidores instalados ambos recompilados/relinkados/
ejecutados exit0, builds finales sin warnings. No carrera resuelta/helper/
elevacion/ptrace/cgroup nuevos; L05/N02/N03 conservados.
C56/E11/P6/B2,56/75=74,7%,19/75=25,3%; Espejo41 confirmado: comentario90140270880053, sin IDs/fracciones nuevos.
Unidad anterior revision40 E06 E->C: usuario respondio "prosigue" a propuesta
concreta E06, identificacion reportada/unknown aceptada sin NVMe=>SSD, no medio
exacto universal implementado ni aceptacion L03. RPM permission_required ahora
conservado sin reporte valido con ATA/B1 denegada; dato independiente sobrevive.
Tests capas/VPD ajena, ausencia/permisos/stale SSD/no-RPM0 y alias valor-soporte-
nota. LINUX_DISK_IDENTITY.md. WSL4contextos/0medios (unknown correcto), positivos
tecnologicos fixtures, no fisicos,N02/N03 conservados. Linux108/108,Windows122/122
Release,10repeticiones focalizadas Linux; consumidores ambos recompilados/
relinkados/ejecutados exit0, sin warnings. Sin layouts/API/ABI virtual/historial/
dependencias/acceso /dev/CLI/elevacion/cambios de discos nuevos.
C55/E11/P6/B3,55/75=73,3%,20/75=26,7%; Espejo40 confirmado: comentario90140270876164, sin IDs/fracciones nuevos.
Unidad anterior revision39 C03 E->C: limite estimado aceptado en respuesta
"prosigue" a propuesta concreta C03. Diferencia instalada-utilizable estimada
y etiquetada, unknown sin datos validos; no reserva exclusiva hardware/BIOS.
No aceptacion para E06/L03. Permiso del total utilizable propagado a diferencia/
alias, fixtures ausencia/denegacion/valor contradictorio/unsupported/cero
SIEMPRE estimated, sin dato viejo. LINUX_MEMORY_COMPOSITION.md. Linux108/108,
Windows122/122 Release,10repeticiones focalizadas Linux; ambos consumidores
instalados recompilados/relinkados/ejecutados exit0, builds sin warnings.
Nativo composicion positiva, zram0observados,Zswap/firmware ausentes, no ceros
globales. Positivos compresion/firmware fixtures,N02/N03 conservados.
Sin layouts existentes/ABI virtual/historial/dependencias/elevacion/zram/swap/
presion de RAM nuevos. C54/E12/P6/B3,54/75=72,0%,21/75=28,0%; Espejo39 confirmado: comentario90140270873160.
Unidad anterior revision38 M03 E->C: WMI5/5,2HDD/3SSD SATA/NVMe, sin UAC/cambios
de permisos ni codigo WMI. Fallo previo historico/no causa corregida. FS5/5,
sistema1/pagefile1,read/write/latencia5/5; active valido1/5 en muestra detallada,
diagnostico posterior5/5 (disponibilidad variable, sin clamp/cero).
Red55tasas send/receive conocidas sin generar
trafico. API query_windows_network_identity LUID/GUID/PnP unico/revalidado,
fabricante/descripcion Metric:9fabricantes/8Ethernet, Realtek PCIe2.5GbE positivo.
WINDOWS_NETWORK_IDENTITY.md;20repeticiones identidad y20red. Windows122/122,
Linux108/108 Release, consumidores ambos recompilados/relinkados/ejecutados
exit0. API adicional sin layouts existentes/ABI virtual/historial nuevos.
Cierre proveedores soportados/topologias/ausencia explicitas, no marca fisica
autenticada ni RAID/MPIO/hotplug fisico universal/reparto arbitrario de capas.
M02/M06/F03/N01/N03 conservados. C53/E13/P6/B3,53/75=70,7%,22/75=29,3%.
Espejo38 confirmado: comentario90140270634154; sin IDs/fracciones nuevos.
Unidad anterior revision37 M07 E->C: open_windows_service read-only para GUI,
errores tipados/metadata Metric para vacio conocido vs inaccesible, sin CLI/UAC.
Estado/PID/UTF16/buffers/handles verificados; grupo carga/descripcion SCM raw.
WINDOWS_SERVICE_VIEW.md; positivo8servicios/8descripciones/8grupos. Launch propio
con argv confirmado, token obsoleto rechazado, EcoQoS checked on/off releido y
end checked mismo handle/exit1; hijo privado limpiado.20repeticiones por nueva
prueba(40ejecuciones). SCM positivo revision36 conservado/limpiado, no repetido.
Windows121/121,Linux108/108 Release; consumidores ambos recompilados/relinkados/
ejecutados exit0. API adicional sin layouts existentes/ABI virtual/historial
nuevos. Sin modo combinado/ahorro/restore auto ni texto localizado/grupo svchost
prometidos. L05/M02/M06/N01/N03 abiertos. C52/E14/P6/B3,52/75=69,3%,23/75=30,7%.
Espejo37 confirmado: comentario90140270623147; sin IDs/fracciones nuevos ni cambio de criterio.
Unidad anterior revision36 M07 E->E: control SCM real autorizado/UAC, servicio
exclusivo LocalService manual/sin dependencias. API publica start/stop/restart/
idempotencia e inventario estado/PID/descripcion/grupo positivos. Copia en
ProgramData ACL protegida, no permisos del perfil ni LocalSystem. Watchdog60s,
registro/copia/directorio eliminados/ausencia reconsultada. RESULT PASS, harness
tests/windows_service_native.cpp manual NO CTest; WINDOWS_SERVICES.md. Fallo
inicial Win32=5 desde perfil limpiado/evidencia preservada. Guardas sin opt-in/
privilegios verificadas. Windows119/119,Linux108/108 Release, sin codigo productivo/
API/ABI/layout/historial nuevos, consumidores sin migracion necesaria. M07
pendiente abrir detalles y disponibilidad metadata; UAC productivo en L05.
C51/E15/P6/B3,51/75=68,0%,24/75=32,0%; sin cierre global/IDs nuevos.
Espejo36 confirmado: comentario90140270598882; titulo/estados remotos sin cambios.
Unidad anterior revision35 E07 P->C: consulta read-only ext2/3/4 por region/ID de
disco, sin montar. Sysfs/particiones/objeto completo, expected major:minor y
revalidacion de identidad/layout/diskseq; total solo si TODAS regiones conocidas/
no solapadas, no parcial/aliases/capas. Capacidad declarada incluye metadata,
NO statvfs util/libre ni total fisico universal. LINUX_FILESYSTEM_CAPACITY.md.
Nativo imagenes32MiB reales, FS32MiB en fuente48MiB, WSL8:48/sdd1TiB/checksum y
consulta por disco positivos. Sin discos/mounts reales modificados; SDK sin CLI/
dependencias nuevas (formatter solo harness). Linux108/108,Windows119/119,
consumidores instalados recompilados/relinkados/ejecutados ambos exit0, builds
sin warnings. API adicional sin layouts existentes/ABI virtual/historial nuevos.
Otros formatos/MBR-GPT raw/reparto fisico RAID/LVM/loop desconocidos. N02/N03
abiertos, F03 diferida. C51/E15/P6/B3,51/75=68,0%,24/75=32,0%; Espejo35 confirmado:
comentario90140270531513.
Unidad anterior revision34 H05 P->C: LinuxNotificationObserver opt-in/consentimiento,
politica de privacidad/atribucion/periodos y positivo nativo. Notify/respuesta
owner/sender/serial; llamadas aceptadas/reemplazos, no avisos mostrados/leidos.
Rechazos/sin-acuse/pendientes/expiraciones separados, hint app declarado estimated,
unknown sin hint, no PID/exe/CPU join. Sin contenido retenido/CLI/descargas/
autolaunch/elevacion/persistencia. LINUX_NOTIFICATIONS.md. Nativo privado con
aceptadas3/reemplazos1/rechazadas1/appunknown1/sinacuse1, denegacion real y owner-loss;
10/10 repeticiones/directo exit0 no skip. Linux106/106,Windows118/118 y consumidores
instalados ambos recompilados/relinkados/ejecutados exit0; builds sin warnings.
API adicional sin layouts existentes/ABI virtual/SystemSnapshot/historial nuevos.
No protocolo universal GTK/portal/vendor ni retroactivo, H04/M08/F03/N03 abiertos.
C50/E15/P7/B3,50/75=66,7%,25/75=33,3%; Espejo34 confirmado: comentario90140270530011.
Unidad anterior revision33 I05 E->C: proveedor AppStream/politica de fuentes y
condiciones ausentes, criterio conservado. Editor declarado estimated mediante
match desktop-id explicito/unico; no firmante/propietario del ejecutable.
Consulta publica y Monitor opt-in, libxml2 del sistema opcional sin instalar/
helpers/descargas. Error/ausencia/ambiguedad limpia etiqueta vieja, queda unknown.
LINUX_STARTUP_PUBLISHER.md; nativo XML/Monitor con archivos privados exit0/no skip,
rechazos DTD/XXE/permisos/duplicados/idiomas/limites/lector incompleto probados.
Linux104/104,Windows117/117, consumidores instalados recompilados/relinkados/
ejecutados ambos exit0, sin warnings finales. Nuevo flag SampleOptions0.2:
recompilar TODOS; sin cambio StartupEntry/ABI virtual/historial. No inventario
universal de fuentes/vendor/Flatpak/Snap ni cierre I07/K03/F03/N03.
C49/E15/P8/B3,49/75=65,3%,26/75=34,7%; Espejo33 confirmado: comentario90140270529005.
Unidad anterior revision32 E05 E->C: archivo swap activo REAL privado64MiB,
autorizado en WSL/ext4. /proc/swaps65532KiB/used0 -> sdd -> lector/informe
nativos -> Monitor.contains_page_file=true estimated. swapoff/unlink/rmdir
completados, swap previo intacto, sin fstab/config permanente/herramientas.
LINUX_SWAP_DISKS.md, prueba manual linux_swap_active_native.cpp (NO CTest),
flag obligatorio/guard77. Linux102/102,Windows116/116, consumidores ambos exit0,
sin warnings. Sin API/layout/historial nuevos. Alcance particiones/ext2/3/4,
topologia conservadora; no kernel-inode universal ni XFS/Btrfs/F2FS/RAID/LVM.
C48/E16/P8/B3,48/75=64,0%,27/75=36,0%; Espejo32 confirmado:
comentario90140270527425. Estados/titulo remotos sin cambios. F03 diferida/N03 abiertos.
Unidad anterior revision31 J06 P->C: integracion explicita Manage user
accounts via host GUI. AccountManagementIntegration query/open, capacidad
reconsultada y recibo host_invoked aun con errores/excepciones; sin provider
no ejecuta nada. No gestor nativo incluido/panel universal/CRUD/elevacion ni
comandos adivinados. Host aporta vista/gestor; no sandbox de callbacks.
ACCOUNT_MANAGEMENT.md. Positivo publico con hijo privado, no UI de cuentas real.
Linux102/102,Windows116/116, consumidores instalados ambos recompilados/relinkados/
ejecutados exit0; sin warnings. API adicional sin ABI virtual/layouts existentes/
historial nuevos. C47/E17/P8/B3,47/75=62,7%,28/75=37,3%; solo J06 P->C sin IDs/
fracciones nuevos. F03/J04/N03 abiertos. Espejo31 confirmado: comentario90140270525751.
Unidad anterior trabajada revision30 F03 E->E: identidad y calidad privadas,
TGID/start_boot/UID y guardas activos/secuencia/overflow sticky. Native autorizada
positiva (threads mismo lider, negativos de UID/inicio/overflow, hijo/retirada),
TCP6144/6144,UDP384/384,12peeks. LINUX_NETWORK_QUALITY.md. Linux101/101,Windows115/115,
consumidores ambos exit0, sin warnings. Solapamiento rechazado por fixtures,
no observado en vivo (10lecturas coherentes/0ocupadas). No productor integrado/
netns/join/API publica/ABI/historial. F03 E, J04/H04 abiertos, N03 conservado.
46/75=61,3%,29/75=38,7% sin cambio. Usuario pide no alargar: diferir integracion
F03 y pasar a otro ID abierto. Espejo30 confirmado: comentario90140270524925.
Unidad anterior trabajada revision29 F03 E->E: clasificacion privada4lanes TCP/UDP
IPv4/IPv6 por socket/BTF, control de fallos de lectura y flags positivos
excluidos PEEK/TRUNC/ERRQUEUE. LINUX_NETWORK_CLASSIFIED.md. Native autorizada
send/sendto+sendmsg TCP4096/4096,UDP256/256,0fallos,8peeks excluidos; hijo y
retirada3enlaces comprobados, exit0. Legacy1088/1088 sigue positivo. Linux101/101,
Windows115/115 y consumidores instalados ambos exit0; builds sin warnings.
Root temporal sin pin/config permanente/utilidades instaladas; native manual,
nunca CTest con sondas. No API publica/ABI/historial ni productor completo
UID/start_boot/netns/identidad duradera/saturacion/coherencia ni integracion.
F03 E, J04/H04 abiertos, legacy red ausente y N03 conservado.46/75=61,3%,
29/75=38,7% abiertas sin IDs/fracciones. Continuar F03 identidad/calidad.
Espejo29 confirmado: comentario90140270524004, estados/titulo remotos sin cambios.
Unidad anterior trabajada revision28 F03 E->E: prueba privada eBPF autorizada
positiva __sys_sendto/sock_recvmsg, firmas BTF/slots correctos, TGID del kernel
obtenido con TEST_RUN no adjuntado y filtro previo a contexto/mapa. TCP1024+
UDP64=1088/1088; fases, hijo excluido y retirada comprobados. Root temporal,
RAII, sin pin/config permanente. LINUX_NETWORK_PROBE.md. Linux100/100,
Windows114/114, consumidores instalados ambos exit0. Fallos iniciales registrados
(helper14 en SOCKET_FILTER, sock_sendmsg0/1088, __sock_sendmsg ausente BTF).
No API publica/ABI/historial nuevos. NO productor real4lanes/UID/start_boot/
netns/perdidas/coverage ni integracion reductor/Monitor/legacy red. F03 E,
J04/H04 abiertos y N03 conservado.46/75=61,3%,29/75=38,7% abiertas sin IDs/
fracciones. Continuar F03 productor/cobertura/identidades. Espejo28 confirmado:
comentario90140270523263, estados/titulo remotos sin cambios.
Unidad anterior trabajada revision27 F03 E->E: reductor interno TCP/UDP/UID,
tasas/ventanas/perdidas/identidad/cobertura y overflow sin convertir ausencias
en0. LINUX_NETWORK_ACCOUNTING.md. Fixtures/oraculo10000casos, no captura real.
Linux99/99,Windows113/113 finales, consumidores instalados ambos exit0.
Windows sandbox inicial98/113 (15fallos de entorno/acceso), fuera113/113 sin
debilitar tests; logs conservados. Sin API publica/ABI/historial ni productor/
cargador/sondas nuevos. F03 E por proveedor real/cobertura/integracion/pruebas
positivas pendientes; J04/H04 abiertos y red legacy ausente. Usuario autorizo
explicitamente2026-10-18 prueba temporal WSL solo procesos privados, privilegios
necesarios/limpieza final, sin configuracion permanente ni instalar herramientas.
No carga realizada en esta unidad. N03 sigue abierto.
C46/E17/P9/B3,46/75=61,3%,29/75=38,7% abiertas sin nuevos IDs/fracciones.
Continuar F03 productor/cargador/integracion. Espejo27 confirmado:
comentario90140270522304, sin cambiar titulo/estados remotos.
Unidad anterior trabajada revision26 F03 E->E: preparacion BTF interna read-only,
firmas/tipos/offsets compatibles sin utilidades externas ni constantes de kernel.
LINUX_NETWORK_BTF.md. Linux98/98 (BTF nativo Passed),Windows112/112; consumidores
instalados ambos relinkados/ejecutados exit0. Fixture inicial corregido sin
relajar parser; logs de fallo/finales conservados. No API publica/ABI/historial
nuevos. NO cargador/sondas/mapas/eventos/contabilidad TCP/UDP implementados aqui.
Prueba futura de sondas privadas pendiente de autorizacion explicita; no carga,
attach ni elevacion. F03 E, J04/H04 abiertos, legacy red ausente y N03 sin resolver.
46/75=61,3%,29/75=38,7% abiertas, C46/E17/P9/B3 sin IDs/fracciones nuevos.
Continuar F03 con cargador/contabilidad/pruebas autorizadas. Espejo26 confirmado:
comentario90140270363748, sin modificar estados/titulo remotos.
Unidad anterior trabajada revision25 F03 E->E: consulta TCP de holder por PID/token/
FD, pidfd_getfd externo/self F_DUPFD_CLOEXEC. Comprueba procfs/Pid proyectado de
fdinfo, PID/start_ticks/vigencia pidfd y slot cookie/dev/inode antes/despues.
Sin fallback proc-fd/inode ni elevacion/cambios de datos/flags; dup prolonga vida
del socket brevemente. Holder no es origen exclusivo/causal, cookies compartidas
no son trafico independiente. LINUX_TCP_HOLDERS.md. Linux96/96,Windows111/111 y
ambos consumidores instalados reconstruidos/relinkados exit0. Nativo8192B ACKed/
recibidos y hijo privado pidfd_getfd positivo misma cookie padre; reemplazo FD/
token/UDP/listener/flags/datos verificados. F03 sigue E por atribucion causal/
exclusiva, UDP/otras coberturas e integracion. J04/H04 abiertos, legacy red ausente.
No layouts existentes/ABI virtual/historial nuevos ni IDs/fracciones.46/75=61,3%
y conteo sin cambio. Continuar F03; espejo25 confirmado: comentario90140270363050. Riesgos N03 conservados.
Unidad anterior trabajada revision24 F03 P->E: LinuxTcpSampler opt-in SOCK_DIAG/
TCP_INFO sin CLI/eBPF/elevacion. Contadores y tasas por cookie/familia/inode/
socket_uid/namespaces del hilo; suma parcial estimada por UID DEL SOCKET, no
UID real/PID/app/red total. Solo TCP ESTABLISHED/CLOSE_WAIT; UDP/otras coberturas,
atribucion PID/app/sockets compartidos y contabilidad/integracion pendientes.
LINUX_TCP.md. Linux94/94,Windows110/110 sin skip TCP nativo; consumidores
instalados ambos recompilados/relinkados exit0. Loopback propio8192B reconocidos/
8192B recibidos,2sockets/1UID. No benchmark ni captura ajena ni nuevos layouts
existentes/ABI virtual/historial. J04/H04 siguen abiertos y legacy red ausente.
46/75=61,3% sin IDs nuevos/fracciones. Continuar F03; espejo24 confirmado: comentario90140270361819.
Unidad anterior trabajada revision23 J04 P->E: disco visible por UID numerico con
LinuxUserIoDetails/cobertura, no username como clave de E/S. UID real observado
coincidente alrededor de lecturas y extremos del intervalo D02; ausencia/cambio/
permiso descarta atribucion del intervalo aunque tasa por proceso siga conocida.
Cobertura/suma parcial estimated explicitas; UID repetido comparte total, utmp
sin UID sin I/O. Red F03 pendiente: J04 NO cerrada. LINUX_USER_IO.md.
Linux92/92,Windows109/109 y consumidores instalados recompilados/relinkados exit0.
Nativo UID1000:8779070B/s,4conocidas/2ausentes tras temporal privado, no benchmark;
1informe publico con procesos ocultos. Nuevo layout ProcessInfo/UserSession/
SystemSnapshot0.x: recompilar TODOS.46/75=61,3% sin nuevo ID/fraccion. Siguiente
dependencia F03 iniciada revision24, espejo23 confirmado: comentario90140270360992; limites/riesgos previos conservados.
Ultima unidad cerrada revision22 K04: inventario/abrir detalles/control del manager
systemd del usuario efectivo mediante API separada LinuxServiceTarget/Inventory/
View con scope system/current_user, UID y owner de observacion. Bus local fijo
sin redireccion por entorno; peer UID y owner UID verificados, destino/sender
fijados y owner revalidado; no GUI/shell/elevacion/fallback. Job confirmado para
Start/Stop/Restart con errores/outcome unknown. LINUX_USER_SERVICES.md.
Linux91/91,Windows108/108; consumidores instalados recompilados/relinkados exit0.
WSL solo lectura199servicios sistema/19usuario y abrir detalles ambos; controles
nuevos positivos en loader simulado, no servicios reales modificados. Sin layout
previo/ABI virtual nuevo; no token persistente. K04 P->C:46/75=61,3%. Propuesta
J04 iniciada revision23, espejo22 confirmado: comentario90140270360264. K03/L05/N01/N02 y riesgos previos separados.
Unidad anterior cerrada revision21 G06: auditoria/correcciones DRM/sysfs/NVML.
Lector bounded por campo/FD/bindings, PCI subsystem, version de modulo real
sin nombre-como-version. LinuxGpuSysfsDetails VRAM/GTT/sensores por etiqueta,
margen estimated no libre exacta/shared Windows inventado/presupuesto proceso.
NVML tuple completo coherente por PCI available y alias dedicated_used;
linux_gpu_inventory_source separa opt-out/vacio/ausente/parcial.
LINUX_GPU_TELEMETRY.md. Linux89/89,Windows107/107 y consumidores instalados
recompilados/relinkados/ejecutados exit0. WSL0filas DRM estimated/NVML ausente,
positivos de lector/campos son fixtures; G07/N02 fisicos y M04 separados.
Nuevo layout GpuInfo/SystemSnapshot0.x: recompilar TODOS.45/75=60,0%, sin nuevo
ID/denominador. L03 B/otros limites/riesgos N03 no resueltos implicitamente.
Propuesta K04 realizada en revision22; espejo21 confirmado: comentario90140270359672.
Unidad cerrada revision20 G08: politica de fuentes/ausencia motivada de
fecha GPU Linux, NO fuente de fecha real. Criterio original admite ausencia
motivada: DRM/sysfs/NVML sin fuente verificada de publicacion/instalacion,
driver_date ausente/unsupported con motivo por fuente/merge. No inferir BIOS,
mtime/build/paquetes/kernel/API ni placeholder DRM0. LINUX_GPU_DRIVER_DATE.md.
Linux88/88,Windows106/106 y consumidores instalados reconstruidos/relinkados/
ejecutados exit0; WSL0filas GPU, positivos de politica/merge son fixtures.
Sin API/ABI/layout/formato cambiado ni nuevas consultas GPU/dependencias.
G08 P->C:44/75=58,7%,31abiertas. G06/G07/N02/M04 y L03 B siguen pendientes,
propuesta G06 auditoria no iniciada; espejo20 confirmado: comentario90140270359048.
Unidad cerrada revision19 G05: contexto EGL/GLX por PCI y capacidades de
perfiles GLX declaradas, separadas de GL_VERSION en LinuxOpenGlDetails/Metric.
GLX directo/Mesa con interop ABI1, IDs/aceleracion/sysfs y contexto/display/screen
comprobados; rechazo de cambios/ambiguedad/duplicados. LINUX_CURRENT_GLX.md.
Linux87/87,Windows105/105 y consumidores instalados recompilados/ejecutados exit0;
4tests EGL/GLX con10repeticiones cada uno. Fixture GLX dinamico y sysfs privado,
nativo WSL sin contexto GLX/0atribuciones, no hardware positivo ni headless.
G07/N02 y M04 separados. Nuevo layout GpuInfo0.x: recompilar TODOS.
43/75=57,3%, sin nuevos IDs/denominador. L03 sigue B sin aceptar limite;
propuesta G08 no iniciada. Espejo revision19 confirmado: comentario90140270358672.
Revision18 L03 E->B: decision pendiente de aceptar proveedor limitado actual o
elegir diseno adicional cooperativo/invasivo. LINUX_EFFICIENCY_DECISION.md.
No se introduce helper/tracing/integracion en apps destino sin decision.
Syscalls numericas y listas muestreadas no cumplen checked/alcance universal.
Solo diagnostico/documentacion; ningun codigo ni pruebas nuevas, evidencia de
revision17 conservada. Conteo56,0% sin cambio, espejo18 confirmado: comentario90140270357868.
Unidad revision17 L03 parcial: API Linux opt-in unchecked por threads capturados,
recibo de originales nice/ioprio y flags de writes pendientes. Restore exacto
por campo, conflictos observados, validacion completa del recibo y reintentos;
sin auto-rollback/elevacion ni default0/BE4 como falso restore. Linux85/85,
Windows105/105, consumidores instalados reconstruidos/ejecutados exit0 y20/20
repeticiones Linux. Nativo con hijos privados de2threads: restauracion E/S y
denegacion CPU13 explicita con recibo parcial. LINUX_EFFICIENCY.md.
L03 permanecio E en revision17 (B en revision18) por identidad checked y alcance completo/nuevos threads; stat-check
no elimina reutilizacion entre syscalls. Sin layout/virtual existente cambiado.
Conteo sin cambios42/75=56,0%; espejo revision17 confirmado: comentario90140270357119.
Tarea cerrada K01 en revision16: metadata/estado systemd por Metric, group=Slice o vacio
con proveedor/group_kind separados, no grupo Windows/Unix; MainPID0 y strings
vacios conocidos distinguidos de ausencia/permiso. Unit.Id y owner comprobados,
fuente manager vs fallback parcial, sin metadata efectiva inventada en archivos.
LINUX_SERVICE_METADATA.md. Linux85/85,Windows105/105 y consumidores instalados
recompilados/relinkados exit0;199servicios WSL,117MainPID/117Slice conocidos
(15Slice vacios), sin crear/controlar servicios reales. Nuevo layout ServiceInfo/
SystemSnapshot0.x: recompilar TODOS. K03/K04/L05/N01/N02 siguen pendientes.
Propuesta L03 iniciada en revision17; checked/alcance completo aun pendientes.
Unidad revision15 J01: LinuxSessionDetails con State/Active/Remote/UID y
fuente por campo, metadata/identidad verificadas y cobertura de inventario
explicita para login1/utmp/vacio/ausente. No equivalencia WTS ni total global
de recursos, sumas siguen visibles por username. LINUX_SESSION_STATE.md.
Linux85/85,Windows105/105 y consumidores instalados recompilados exit0; WSL
1sesion/1State/1flag foreground conocidos, utmp privado3registros/vacio/ausente,
sin controlar sesiones reales. Nuevo layout UserSession/SystemSnapshot0.x:
recompilar TODOS. J04/J05/J06 y N02 no cerrados. K01 se cerro luego en revision16.
Unidad revision12 A01: contrato Metric y vista comun AppHistorySample con
disponibilidad/origen, alias legacy explicito e importacion sin promover ceros
ni positivos a mediciones. Ver METRIC_AVAILABILITY.md. No elimina tipos raw
compatibles ni cierra cobertura de backends/auditoria de cada campo/API1.0.
Validacion A01:Linux83/83,Windows103/103; consumidores instalados recompilados exit0. API
aditiva, sin layout existente nuevo ni formato v1/v2 cambiado. C03/E05 abiertas;
N03 P->E: diagnosticos CPU completados por motivo, incluyendo estado/valores
de retroceso, con evaluacion compartida. Integracion imprime nota antes de la
asercion original; formula/guardias sin cambio. Fixtures y100 repeticiones nativas
pasaron; fallo historico no reproducido ni causa demostrada. N03 sigue abierta;
LINUX_CPU_USAGE.md. Suites83/83 y103/103 y consumidores instalados relinkados
exit0 repetidos en revision13.
Revision14 E06 P->E: identificacion SAT/SCSI/NVMe reportada y comprobada con
protocolo/transporte/medio/RPM separados; LINUX_DISK_IDENTITY.md. E06 abierta
por medio exacto restante o aceptacion explicita del limite unknown (sin respuesta
asumida). Linux final84/84,Windows104/104 y consumidores instalados recompilados
exit0. Nuevo layout DiskInfo/SystemSnapshot0.x, recompilar TODOS. WSL4contextos/
0medios; positivos fisicos solo fixtures. Primera suite83/84 fallo historial
ausente con discos desactivados, logs preservados; luego3/3 y84/84. Nota de prueba
anadida, logica/aserciones intactas; causa pendiente N03, no atribuida al CPU.

## Linux implementado

| Área | Estado | Fuente sin subprocess |
|---|---|---|
| CPU estático y dinámico | Implementado con límites; porcentaje validado, cachés/topología online deduplicadas; frecuencias actual/base separadas sin promedios parciales | procfs, cpufreq, sysfs, SMBIOS; ver LINUX_CPU_USAGE.md, LINUX_CPU_CACHE.md, LINUX_CPU_TOPOLOGY.md, LINUX_CPU_FREQUENCY.md y LINUX_COUNTS.md |
| Virtualización Linux | Firmware desconocido explícito; consulta de API KVM separada, sin deducir habilitación BIOS de un archivo | sysfs misc + lstat/open/fstat + KVM_GET_API_VERSION; ver LINUX_VIRTUALIZATION.md |
| Memoria del kernel | Lectura validada; total con disponibilidad, faltantes no0, derivados estimated y sumas comprobadas | `/proc/meminfo`; ver LINUX_MEMORY.md |
| Composicion/compresion/reserva | C03 cerrada con limite aceptado: particion Linux estimada, categorias no aditivas, zram/zswap separados y diferencia instalada-utilizable estimated/unknown; NO reserva exclusiva hardware | meminfo/sysfs/SMBIOS; LINUX_MEMORY_COMPOSITION.md; no equivalencia pools/composicion Windows; validacion fisica N02 separada |
| RAM física | Implementado si firmware accesible | parser SMBIOS propio |
| Procesos CPU/RAM/E/S/usuario/ELF | D01..D04 cerrados con limites: RSS, E/S, detalles y CPU por proceso estimated; red por app pendiente | procfs, NSS y XDG; ver LINUX_PROCESS_CPU.md, LINUX_PROCESS_DETAILS.md, LINUX_PROCESS_DESCRIPTION.md, LINUX_PROCESS_MEMORY.md y LINUX_PROCESS_IO.md |
| Dispositivos de bloques, mounts y tasas | Rendimiento estimated con baselines propias; capacidad visible validada y pista rotational explícita; capacidad montada deduplicada estimated, ausencia no0 | sysfs stat/dev/diskseq/size/rotational, mountinfo, fstatvfs; ver LINUX_DISK_RATES.md / LINUX_DISK_PROPERTIES.md / LINUX_FORMATTED_CAPACITY.md |
| Capacidad ext fuera de mounts | E07 cerrada: consultas explicitas de region/ID de disco, ext2/3/4 declarada incluyendo metadata; sysfs/particiones/identidad esperada y total solo con todas regiones conocidas/no solapadas. Sin total fisico universal ni sustitucion de statvfs | LINUX_FILESYSTEM_CAPACITY.md; opt-in raw readonly, SDK sin CLI; otros formatos/capas no inferidos |
| Identificacion de disco | E06 cerrada con alcance reportado/unknown aceptado: protocolo/dispositivo/transporte/medio separados, fuente/soporte por campo; sin SSD por nombre/rotational/NVMe | SAT89 ATA IDENTIFY, SCSI B1/VPD83,NVMe transport sysfs; LINUX_DISK_IDENTITY.md; no medio exacto universal, N02 fisico separado |
| Red por adaptador | Implementado; tasas estimated con continuidad por nombre/ifindex, reinicios/pausas sin picos falsos | getifaddrs y sysfs; ver LINUX_NETWORK_RATES.md |
| Identidad de red | F02 cerrada: PCI/USB/modelo/vendor con fuentes opcionales, driver/bus/IDs separados y tipo por evidencia; no nombre retail universal ni Ethernet por defecto | sysfs/pci.ids local opcional; LINUX_NETWORK_IDENTITY.md; fisico adicional N02 |
| GPU general | Parcial según driver | DRM, PCI, hwmon |
| Nombres GPU PCI | IDs numéricos DRM y etiquetas locales estimated implementados; no modelo comercial garantizado | pci.ids opcional existente, sin utilidades/descargas; ver LINUX_GPU_NAMES.md |
| Vulkan/OpenCL por GPU | Versiones por PCI único probadas con API/loaders sintéticos; físico Linux pendiente | LINUX_GPU_VULKAN.md / LINUX_GPU_OPENCL.md; no fallback loader/plataforma |
| Sesiones y consumo por usuario | J01 cerrada con limites: estado/foreground/origen/UID por Metric, fuente seleccionada conocida/vacia/ausente; utmp no prueba actividad. Recursos visibles CPU/RAM por username, no por sesion; otros recursos J04 | utmpx/login1 con identidad y owner comprobados; LINUX_SESSION_STATE.md / LINUX_USERS.md / LINUX_LOGOUT.md |
| Startup de escritorio | Flags, precedencia, OnlyShowIn/NotShowIn/TryExec y condiciones por archivos/GSettings/GNOME3-session verificados; editor AppStream opt-in con match explicito/no ambiguo y politica de fuentes/ausencias. Otros mecanismos no se presentan como apps XDG; impacto automatico I07 pendiente | XDG + stat + GIO/sd-bus/libxml2 opcionales; ver XDG_PRECEDENCE.md, XDG_ELIGIBILITY.md, XDG_FILE_CONDITION.md, XDG_GSETTINGS.md, XDG_SESSION_CONDITION.md y LINUX_STARTUP_PUBLISHER.md |
| Servicios | K01/K02/K04 systemd verificados: metadata por Metric/Slice, inventario de sistema/usuario efectivo separados, abrir detalles y control explicito por scope; otros init K03 y helper L05 pendientes | D-Bus sd-bus; LINUX_SERVICE_METADATA.md, LINUX_SYSTEMD.md, LINUX_SERVICE_ACTIONS.md y LINUX_USER_SERVICES.md |
| Servicios/acciones Windows | M07 verificado: SCM inventario/control, open_windows_service para detalles con disponibilidad, launch/end checked/EcoQoS; positivos nativos privados. Descripción raw/grupo de carga, no svchost ni modo combinado; helper UAC L05 pendiente | WINDOWS_SERVICES.md, WINDOWS_SERVICE_VIEW.md, WINDOWS_LAUNCH.md, WINDOWS_TERMINATE.md, WINDOWS_EFFICIENCY.md |
| Historial CPU por app | H01 cerrada: informe linux_app_history observed estimated, continuidad exe/PID y conversion con resto, opt-out/reset; sin persistencia nueva/red/notificaciones dentro de ese informe | procfs, CLK_TCK; ver LINUX_APP_HISTORY.md; H04 abierto, H05 fuente independiente LINUX_NOTIFICATIONS.md, A01 cerrada despues |
| Notificaciones Linux observadas | H05 cerrada: observador explicito con consentimiento, Notify aceptadas/reemplazos/rechazos/sin-acuse y periodos, hint de app declarado/no autenticado, no contador de avisos mostrados ni join CPU | D-Bus/libsystemd opcional, permiso BecomeMonitor; sin contenido retenido ni persistencia, fuente parcial FDO, LINUX_NOTIFICATIONS.md |
| Ventana dinámica 60 s | A03 cerrada: filtro steady por inicio, timestamps inicio/fin y expiracion al leer incluso detenido; capacidad automatica por intervalo, explicita puede recortar | `pcinsight::Sampler`; ver SAMPLER_WINDOW.md; no cadencia exacta ni snapshot atomico |
| Run new task | Validación y errores chdir/exec verificados; PID estructurado, caller recolecta hijos | fork/execve + pipe CLOEXEC; ver LINUX_LAUNCH.md |
| End task | PID+start_ticks, señal por pidfd y confirmación de salida verificadas; límites del token documentados | procfs + pidfd, kernel 5.3+; ver LINUX_TERMINATE.md |
| Efficiency preset Linux | L03 cerrada con alcance avanzado limitado aceptado: apply/restore por threads capturados con ack/recibo; generico y checked no soportados, sin fallback. NO identidad estable, threads futuros, ahorro ni restore garantizados | nice/ioprio nativos; LINUX_EFFICIENCY.md y LINUX_EFFICIENCY_DECISION.md; cambio de comportamiento0.x |
| Enable/disable/reset startup | Edición directa, override personal y reset recuperable verificados; condiciones vendor/impact pendientes | XDG por grupo + publicación/archivo personal; ver XDG_OVERRIDE.md, XDG_RESET.md y LINUX_STARTUP.md |
| Startup Properties | Consulta read-only de archivo efectivo, tamaño/campos/grupos y condiciones verificada; no properties del binario Exec | XDG + lectura nativa; ver XDG_PROPERTIES.md |
| Disconnect user Linux | Capacidad indisponible justificada por adaptador utmp/login1; API estatica+motivo e inventario. NO detach real implementado ni imposibilidad universal | ver LINUX_DISCONNECT.md; J05 cerrada por alternativa del criterio, no SIGHUP/logout implicito; Windows en M06 |
| Cerrar sesión explícito | Proveedor login1 verificado con sesión PAM privada real: identidad, cierre, auditoría y limpieza | ver LINUX_LOGOUT.md; Terminate no reemplaza Disconnect |

## Capacidad D3D12 Windows verificada

`GpuInfo.direct3d12_feature_level`: consulta del adaptador DXGI explícito sin
crear dispositivo, candidatos 11_0..12_2; no versión de DirectX instalada.
Runtime que no reconoce candidatos superiores produce límite inferior estimated.
Ver WINDOWS_GPU_D3D12.md. Windows50/50 y Linux35/35; paquetes/consumidores
instalados verificados. Otros runtimes Windows y temperatura siguen pendientes.

## Parcial o pendiente

A01 cerrada por reemplazo documentado: AppHistorySample/app_history_view y
metric_is_known con cero medido distinto de ausencia, import_legacy_history
con todos los contadores unknown, LegacyAppHistoryEntry alias raw explicito.
Formatos v1/v2/simbolos/layout existentes no cambian; almacenamiento moderno
comun/Linux no anadido. METRIC_AVAILABILITY.md. Linux83/83,Windows103/103 y
consumidores instalados exit0. No cierra A05/N03 ni captura H04/H05/M08.

E05 E->E (LINUX_SWAP_DISKS.md): tamano KiB/bytes y metadatos fd/ruta antes/despues
de resolver, cambios detectados dejan unknown. Nuevo test linux-swap-path fuerza
carreras; pipeline nativo de archivo regular8:48->sdd con inventario SINTETICO,
NO archivo swap activo. WSL real solo /dev/sdc partition. Falta prueba activa
de archivo/topologias admitidas; no se modifica swap para forzar cierre.
Linux82/82,Windows102/102, consumidores relinkados exit0; sin layout publico
nuevo. Conteo de esa unidad39/75=52,0% sin cambios; A01 cerrada despues en revision12.

F02 cerrada (LINUX_NETWORK_IDENTITY.md): LinuxNetworkDetails por adaptador,
ifindex/type/directorios comprobados, fuente PCI/USB con etiquetas y campos
ausentes explicitos. Model/connection_type legacy valor o vacio. No medio fisico,
retail exacto universal o clases virtuales inventadas. Linux final81/81,
Windows101/101, consumidores recompilados exit0. Nativo WSL loopback/vmbus;
PCI/USB/Wi-Fi positivos fixtures. Layout NetworkAdapter0.x nuevo. Fallo CPU
aislado reincidio en primera suite; logs conservados, luego integracion3/3 y
81/81 pasaron sin cambiar aserciones. N03 sigue abierto. Siguiente propuesta E05.

C03 E->E: nuevo MemoryInfo.linux_details, particion MemTotal/MemAvailable/MemFree,
Zswap/Zswapped y zram por dispositivo con ausencias/limites/identidad explicitos.
Reserva legacy proxy ahora tambien bajo installed_minus_usable_bytes. No se
declara reserva exclusiva hardware implementada. Falta proveedor exclusivo o
aceptacion explicita del usuario del limite. Ver LINUX_MEMORY_COMPOSITION.md.
Linux80/80,Windows100/100 y consumidores instalados recompilados exit0. Layout
MemoryInfo/SystemSnapshot0.x nuevo; recompilar TODOS. Nativo WSL sin zram/Zswap/
firmware-diferencia, positivos de esos proveedores solo fixtures, no hardware.
Conteo permanece38/75=50,7%. No es cierre de N02/CI ni del riesgo previo N03.

A03 cerrada (SAMPLER_WINDOW.md): intervalo inclusivo [steady_now-60s,steady_now]
por inicio de coleccion, consultas con expiracion, latest antiguo separado.
SamplerSample/latest_timed/timed_history y capacidad0 auto61a1s/601a100ms.
Nueva corrida limpia grafica, conserva Monitor; explicita limita puntos y puede
recortar ventana. Pruebas sinteticas de60s, nativas y ciclo de vida; Linux79/79,
Windows99/99 y consumidores instalados exit0. API0.x cambia defaults/comportamiento,
sin nuevo layout SystemSnapshot en A03. C03 iniciada despues en revision9.

H01 cerrada (LINUX_APP_HISTORY.md): duracion CPU observada, no total historico ni
porcentaje. Exe/ruta/PID/start/stamp, utime/stime, reloj propio, contexto/boot y
unidades comprobados; conversion exacta con resto, huecos y overflow sin commit
parcial. Legacy app_history vacio, nuevo linux_app_history con Metric por campo.
HistoryStore NO lo serializa; no convertir desconocidos a0. Nuevo layout publico
SystemSnapshot0.x, recompilar TODOS los consumidores. Linux78/78,Windows98/98 y
consumidores instalados exit0. H04/H05/A05 siguen abiertos; A01 cerrada despues. Sigue pendiente
la causa del fallo CPU global aislado anterior en N03. A03 cerrada despues en revision8.

D04 cerrada (LINUX_PROCESS_CPU.md): reloj propio tras stat, utime/stime separados
y CLK_TCK positivo; valida PID/start/contexto global/contador/rango0..100 sin
clamp. Normalizacion por capacidad visible host, no cuota/afinidad ni utilidad
ponderada por frecuencia. Guest no se duplica; cutime/cstime de hijos no se incluyen. No se promete
igualdad de ventanas/sumas con CPU global. Permiso/ausencia/opt-out reinician
referencias. Linux77/77,Windows97/97 y consumidores reinstalados/relinkados exit0.
Sin nuevo layout publico en D04. H01 cerro despues en revision7 y A01 en revision12.

D03 cerrada: stat/status acotados, PID/start antes/despues,4UID estrictos/NSS,
exe regular fijado y ELF validado; ProcessInfo.linux_details expone Metric por
campo y descripcion/source XDG Comment estimated. Asociacion por stamp de archivo,
no nombre; no metadata admitida o ambigua queda desconocida. Subset conservador
de Exec y sin localizacion, no identidad comercial certificada ni todas las apps.
No snapshot atomico ni generacion exec universal. LINUX_PROCESS_DETAILS.md y
LINUX_PROCESS_DESCRIPTION.md detallan fuentes/limites/pruebas. Linux76/76,
Windows96/96, consumidores reinstalados y recompilados exit0. Nuevo layout API0.x,
recompilar libreria y consumidores. No cierra A01 ni CPU/historial de procesos.
La ausencia CPU aislada de una pasada previa sigue registrada en N03, no se
declara arreglada por este cambio. Conteo oficial35/75=46,7%.

E/S por proceso Linux corregida (LINUX_PROCESS_IO.md): read_bytes/write_bytes
separados, validación de cada reinicio, identidad stat/io/stat por directorio
fijado, lecturas acotadas y reloj propio. Ausencia no0; primera muestra y pausas
reinician referencia. Bytes/s estimated, no throughput físico del dispositivo.
Linux74/74 y Windows94/94 Release, positivo nativo propio delta write131072B;
sin nuevo layout público.
Instalación/consumidores no repetidos en esta unidad. CPU/historial/UID/State/ELF
y coherencia completa de fila siguen pendientes. ClickUp bloqueado por cuota.

RSS por proceso Linux corregido (LINUX_PROCESS_MEMORY.md): VmRSS uint64/kB exacto,
producto1024 validado, ausencia no0, duplicados/PID/texto/bounds rechazados y
Support::estimated por contabilidad asíncrona. RSS incluye páginas compartidas,
no es memoria privada/PSS ni idéntico a Windows. Directorio O_PATH fijado,
stat/status/stat acotados por openat; mismo PID/start_ticks y Pid status requerido.
Datos de identidad ausentes/cambiantes no reciben memoria. Linux73/73,
Windows93/93, instalación/consumidores exit0. Nativo propioPID1539/start5173,
RSS4538368B y Monitor5976064B estimated en pasada registrada; token incorrecto
rechazado sin forzar reutilización real de PID. No cambios de API/layout/Windows,
procesos ajenos, permisos o presión de RAM. No cierra PSS/smaps, lectura de UID/
State, CPU/E/S/historial ni coherencia atómica de la fila completa; token limitado
al boot/namespace y resolución de ticks. Permisos reales no se alteraron para pruebas.

Identidad CPU corregida (LINUX_CPU_IDENTITY.md): procfs acotado, registros processor
únicos y resumen textual común/completo por campo, no primera fila arbitraria.
Claves model name/cpu model/cpu y vendor_id; no nombres de placa ni lookup supuesto
de implementer/part. SMBIOS Type4 Manufacturer/Version por string-set e índices,
solo central poblado y común; alternativa estimated cuando no hay nombre proc,
sin ocultar mezcla/incompletitud observada. UTF-8/control/text bounds y reset
probados. Nuevos CpuInfo.reported_vendor/reported_model Metric<string>; legacy
vacío significa desconocido y refleja los Metric. Windows refleja registro CPU0
estimated/representativo, no resumen de todos los paquetes. Linux72/72,
Windows92/92 y consumidores recompilados exit0. WSL32registros AuthenticAMD /
AMD Ryzen 9 7950X 16-Core Processor. ARM/MIPS/PowerPC y SMBIOS positivo probados
con fixtures; hardware nativo no-x86 y firmware positivo real aún pendientes.
No catálogo comercial universal ni atomicidad con CPU online; firmware cacheado
al construir Monitor. API0.x: nuevo layout CpuInfo/SystemSnapshot, recompilar
biblioteca y todos los consumidores. No cierra todo CPU ni el proyecto.

Virtualización Linux corregida (LINUX_VIRTUALIZATION.md): retirado exists(dev/kvm)
=>firmware true. virtualization_enabled queda desconocido; nuevo Metric<unsigned>
CpuInfo.linux_kvm_api_version separado confirma únicamente consulta API12 en fd
de caracteres concordante con registro KVM sysfs antes/después. lstat previo,
open solo lectura/no symlink, fstat del mismo fd; rechazo antes del ioctl de
archivo/directorio/FIFO/dispositivo incorrecto. Errores de permisos explícitos,
sin false/0 inventados ni estado anterior retenido. Linux71/71 y Windows91/91,
instalación y consumidores recompilados exit0. WSL sin registro KVM (ENOENT),
ausencia nativa comprobada; positivo API12 solo clasificación fixture, no
validación positiva en hardware. Sin VM creada, MSR, BIOS, módulos o cambios
de estado. Lectura directa de firmware y otras evidencias/arquitecturas siguen
pendientes; no cierra toda virtualización. API0.x: nuevo layout CpuInfo/SystemSnapshot,
recompilar todos los consumidores; Windows firmware sin cambio y nuevo campo ausente.

Uptime Linux corregido (LINUX_UPTIME.md): lectura procfs acotada256B, exactamente
dos campos con dos decimales y conversión entera comprobada a chrono::seconds.
Sin stod/prefijos/NaN/Inf, redondeos o conversiones fuera de rango; cero válido,
idle acumulado puede superar uptime y nunca se usa como tiempo de arranque.
Faltantes/errores limpian valores anteriores, no generan0. Linux70/70,
Windows90/90 y consumidores instalados con uptime presente/no negativo exit0.
WSL muestra52s dentro de CLOCK_BOOTTIME52..52 en pasada registrada; es boottime
de ese entorno, no encendido físico del host. Sin suspensión/reinicio/cambios
de reloj/namespace forzados, fallback sin procfs no implementado. Sin layout
público ni cambio de backend Windows. Virtualización todavía requiere revisión
propia; esta unidad no cierra toda la sección CPU ni el proyecto.

Frecuencias CPU Linux corregidas (LINUX_CPU_FREQUENCY.md): lector cpufreq acotado,
kHz uint64 positivos, sin stod permisivo ni NaN/Inf/overflow. CPU online completo,
media por procesador lógico de cpuinfo_cur_freq o scaling_cur_freq sin mezclar
fuentes ni ignorar faltantes; nota distingue hardware de valor del controlador.
Base exige valor común para todos y concordante entre dos lecturas; sin CPU0
obligatorio, medias de bases heterogéneas ni SMBIOS Current Speed/max como base.
Linux69/69, Windows89/89 y consumidores instalados exit0. WSL32online no expone
las fuentes: ausencia end-to-end comprobada, sin fabricar frecuencias. Fixtures
verifican valores presentes/policy compartida; positivo físico Linux pendiente.
Estimated, lectura secuencial no atómica, sin garantía de identidad persistente
de policy/driver; ACPI CPPC y frecuencias efectivas por ciclos no implementadas
en esta unidad. Sin layout público ni escrituras a CPU. Uptime y virtualización
todavía requieren revisión propia; no se cierra todo CPU.

Topología CPU Linux corregida (LINUX_CPU_TOPOLOGY.md): sockets/cores/logical desde
sysfs online y grupos package/core. Retirados fallback1socket y cores=logical,
conteo por líneas cpuinfo y dependencia del orden physical/core ID. Grupos
completos/coherentes, package IDs no negativos, particiones sin solapamientos
incluso offline y núcleos contenidos en paquetes. Alias ABI antiguos solo si
camino moderno ausente; errores no ocultan faltantes. Lecturas dobles/bounds,
valores anteriores limpiados y ausencia parcial por métrica. Linux68/68,
Windows88/88 y consumidores instalados exit0. WSL32logical/16core groups/1package,
estimated y limitado a lo reportado por el kernel; sin auditoría física,
hotplug, cambios de afinidad, BIOS, cuotas cgroup ni sockets solo offline.
Sin cambio de layout público. Frecuencias/uptime/virtualización quedan fuera
de esta unidad y requieren su propia revisión, no se cierra todo CPU.

Cachés CPU Linux corregidas (LINUX_CPU_CACHE.md): todos los CPU online, no solo
cpu0; identidad por nivel/tipo/shared_cpu_list, tamaños coherentes y cobertura
completa por nivel. L1 Data/Instruction sumadas, instancias SMT/socket compartidas
deduplicadas. Lecturas dobles coincidentes, límites de texto/filas/IDs y suma
sin overflow; ausencias/incoherencias no producen totales parciales ni cero.
Estimated con alcance sysfs online, no hardware oculto/offline ni afinidad/cgroup.
Linux67/67, Windows87/87 y consumidores instalados exit0. WSL32CPUs/128filas,
L1=1MiB/L2=16MiB/L3=32MiB expuestos, no auditoría física independiente. Sin cambio
de layout público ni hotplug. No cierra topologías heterogéneas/no representables,
BIOS, datos ocultos por VM o el resto del inventario CPU.

Memoria dinámica Linux robustecida (LINUX_MEMORY.md): meminfo acotado con unidades/
números/duplicados validados, ausencia no0, relaciones antes de restar y caché
sin overflow. MemAvailable/uso/porcentaje/caché son estimaciones etiquetadas;
compressed es proxy limitado a Zswap, pools Windows ausentes. Nuevo Metric público
MemoryInfo.reported_total_bytes en Linux/Windows; legacy0 no establece total.
Reservado hardware no se calcula con total ausente. API0.x: recompilar. Linux66/66,
Windows86/86 y consumidores instalados exit0; WSL total16557035520B y uso4,35461%
en pasada registrada. No cierra compresión/composición universales ni cgroups.

Utilización CPU Linux robustecida (LINUX_CPU_USAGE.md): lectura /proc/stat acotada,
estados/esquema/CPU IDs/btime validados, deltas no decrecientes y suma comprobada;
guest sin doble suma, iowait en idle y steal en resto como convención explícita.
Fuente inválida borra baseline; no avance no0 ficticio. Denominador de procesos
usa el mismo intervalo, sin porcentaje ante CPU global inválida. Linux65/65,
Windows85/85 y consumidores instalados exit0; nativo32CPUs, CPU global/propio
test3,52113% en pasada registrada. No cierra cuotas cgroup, hotplug físico ni
identidad/acumulación/conversión completa del historial Linux legacy.

Swap activo Linux corregido (LINUX_SWAP_DISKS.md): incluye camino de archivos
ext2/3/4 y dispositivos, parseo /proc/swaps con escapes/límites, dos configuraciones
coincidentes (Used puede variar), O_PATH/fstat y asociación directa a bloques.
Positivos conocidos estimated; entradas inaccesibles/capas impiden false global.
Linux64/64, Windows84/84 y consumidores instalados exit0; nativo swap existente
1true/27false, referencia regular probada solo por metadatos. Sin swapon/swapoff,
lecturas de contenido ni nuevos swaps. Archivo swap activo nativo end-to-end,
otros filesystems/topologías y token inode de swap del kernel no quedan cerrados.

System disk Linux corregido (LINUX_SYSTEM_DISK.md): fd O_PATH de / + st_dev,
contraste mountinfo y asociación directa a /sys/block. true/false estimated
solo con root identificable y una fila de inventario coincidente; desconocido,
mismatch o ambigüedad no se convierte en false para todos. Sin layouts nuevos.
Linux63/63, Windows83/83 y consumidores instalados exit0; WSL root sdd,1true/27false.
No indica EFI/bootloader, host fuera del namespace ni todos los soportes RAID;
topologías fuera de política pendientes. Swap tiene proveedor posterior separado
en LINUX_SWAP_DISKS.md, con límites propios y sin atribución física universal.

Capacidad montada Linux corregida (LINUX_FORMATTED_CAPACITY.md): no0 ficticio
si no hay asociación, parseo/escape mountinfo acotado, aliases deduplicados con
reintento, conflictos/errores sin suma parcial y producto/suma sin overflow.
Lectura por referencia O_PATH validada con st_dev y fstatvfs del mismo fd; suma
estimated de filesystems montados visibles, NO capacidad formateada total universal.
Lista conservadora de tipos, otros requieren proveedor/topología. Linux62/62,
Windows82/82 y consumidores instalados exit0; nativo1disco medido/27ausentes.
Sin API pública nueva. Formatos desmontados, topologías/cobertura adicional y
semántica system_disk/page file no se cierran por este cambio; proveedores
posteriores separados: LINUX_SYSTEM_DISK.md / LINUX_SWAP_DISKS.md.

Capacidad visible/pista de medio Linux corregidas (LINUX_DISK_PROPERTIES.md):
size decimal completo y conversión512B con rango comprobado; lectura inválida
no produce bytes inventados ni aborta por stoull. DiskInfo.rotational Metric
publica pista del kernel; media_type rotational/non-rotational/unknown en vez de
HDD/SSD inferidos. Posteriormente E06 anade reported_media_type de identificacion
reportada, NO derivada de esa pista (LINUX_DISK_IDENTITY.md); bus no se deduce
por nombre nvme*. API0.x ampliada: recompilar. Linux61/61, Windows81/81, comparación
nativa28filas y consumidores instalados exit0. Clasificación física exacta y
volúmenes/formatted/system disk/page file no se cierran con esta unidad.

Corrección de rendimiento de discos Linux terminada (LINUX_DISK_RATES.md):
intervalo propio, opt-out borra baseline, continuidad por nombre/dev/diskseq
opcional, resets sin restas uint64 inválidas y tasas/sumas sin wrap entero.
Promedio de respuesta sin operaciones queda ausente, no0 artificial. Lecturas
nativas28dispositivos y fixtures; Linux60/60, Windows80/80 y consumidores instalados
exit0. No cierra hotplug físico, tipos exactos, capas/mounts ni E/S por proceso.

Corrección de tasas de red Linux terminada (LINUX_NETWORK_RATES.md): reloj propio
por interfaz, opt-out limpia baseline, desaparición/reuso visible/reset de
contadores no genera diferencias inválidas, overflow de tasa sin cast inseguro.
Prueba nativa UDP loopback y fixtures; Linux59/59, Windows79/79, instalación y
consumidores externos exit0. No añade red por proceso/app ni prueba hotplug físico.

Historial observado de CPU por ejecutable Windows implementado:
include_app_history e informe windows_app_history separado del legacy. Deltas
GetProcessTimes por PID/creación/ruta, grupos de ruta visibles y sumas estimated;
red/notificaciones sin valor. Persistencia separada WindowsHistoryStore terminada,
preserva valor/soporte/notas y rechaza destino corrupto/legacy; merge acumulativo,
retención/reset explícitos, sin I/O automático. WINDOWS_HISTORY_STORE.md:
Windows78/78, Linux58/58 y consumidores instalados exit0. Control de
huecos/reuso/overflow/límites/resets y CPU nativo del propio test verificados:
WINDOWS_APP_HISTORY.md. Windows77/77, Linux57/57 y consumidores instalados exit0.
No es historial completo del SO ni incluye actividad antes de activar captura.

Control propio reversible HKCU Run: proveedor y modelo de transacciones
implementados (WINDOWS_RUN_CONTROL.md), sin tocar StartupApproved ni mapearlo al
estado enabled del inventario. **Pendiente cierre nativo positivo**: host devuelve
6801 ERROR_RM_NOT_ACTIVE; rechazo sin writes verificado. Retirada/restauración,
conflictos/rollback comprobados con API inyectada, no como ejecución nativa exitosa.
Acciones de carpetas/HKLM pendientes. Enumeración de respaldos
ahora implementada aparte, sin modificar ese pendiente de validación nativa.
Regresión global de esta unidad: Windows73/73 y Linux54/54, consumidores instalados
exit0. No confundir pruebas inyectadas/rechazo seguro con enable/disable nativo
validado ni con subtarea funcional cerrada.
Actualización: lector de estados propios HKCU Run/respaldo nativo verificado sin
KTM, con dos observaciones coincidentes y Support::estimated, no snapshot atómico.
Consulta privada y entrada real exitosas sin escrituras. La acción sigue exigiendo
TxR; no interpretar lectura disponible como habilitación de la acción. Cambio
entre lecturas y proveedor sin callbacks KTM probados; ver WINDOWS_RUN_CONTROL.md.
Listado nativo de Run/respaldo HKCU terminado (WINDOWS_RUN_LISTING.md): reconstruye
IDs sin memoria previa del caller, incluye respaldo-only y conflictos sin case,
doble enumeración estimated/no atómica. Fixture privada y lectura real16filas
verificadas. Windows74/74, Linux54/54 y consumidores instalados exit0. No cierra
retirada/restauración ni cambia status StartupApproved.
Integración de ese listado Monitor/Sampler terminada (WINDOWS_RUN_MONITOR.md):
include_windows_managed_run defaultfalse independiente de inventario genérico;
Metric windows_run_registrations, opt-out sin caché retenida, snapshots propios y
latest/history verificados. Windows75/75, Linux55/55, consumidores instalados
exit0; Monitor nativo16filas y Linux unsupported. No construye GUI ni valida la
acción transaccional pendiente. Coste de memoria de retener reportes documentado.
Auditoría del control Run explícito terminada (WINDOWS_RUN_AUDIT.md): nuevo virtual
SystemActions independiente del genérico y operación AuditedActions propia,
solicitud/resultado sin comando/bytes/diagnósticos. Forwarding checked y políticas
de fallo conservados; Windows76/76 y Linux56/56, consumidores instalados exit0.
No equivale a validación positiva nativa de escritura (6801 reconfirmado), actor
autenticado, consentimiento ni almacenamiento inviolable del log.

Properties Windows Run implementado: get_startup_properties consulta el valor
por ID/vista, devuelve comando crudo y campos de registro con doble observación,
sin ejecución/expansión. Tamaño de archivo como Metric ausente para registro,
available para .desktop Linux. Properties del archivo fuente Startup también
implementado: tamaño/atributos/FILETIME, metadatos .lnk/.url del mismo handle y
revalidación de archivo/inventario. Windows72/72, Linux54/54 y consumidores
instalados. Ver WINDOWS_STARTUP_PROPERTIES.md y WINDOWS_STARTUP_FILE_PROPERTIES.md.
No es Properties del ejecutable destino ni implementa enable/disable, aprobación
o impacto.

Presupuestos DXGI del proceso llamador/nodo0 implementados en campos separados
(WINDOWS_GPU_PROCESS_BUDGET.md), Windows55/55 y Linux39/39. Incluyen uso, reserva
y margen presupuestario, NO memoria libre global; ese pendiente sigue abierto.

Temperatura GPU Windows KMT implementada (WINDOWS_GPU_TEMPERATURE.md), por LUID
exacto y solo con count físico1. RX7900XT nativa43°C; Windows54/54 y Linux39/39.
Otros drivers, linked adapters y callers32 bits pendientes; no hotspot/VRAM.

OpenCL Windows por LUID implementado y probado (WINDOWS_GPU_OPENCL.md).
Windows53/53 y Linux39/39. El driver OpenCL local no anuncia cl_khr_device_uuid;
WINDOWS_OPENCL_INTEROP.md añade fallback D3D11 uno-a-uno estimated y obtuvo
OpenCL 2.0 AMD-APP (3679.0) en GPU real. No es prueba de identidad física LUID.

Vulkan Windows por LUID DXGI verificado (WINDOWS_GPU_VULKAN.md), sin copiar
la versión del loader a cada GPU; Windows51/51, Linux35/35 y build sin headers
opcionales comprobado. Corregida correlación Linux por PCI único en
LINUX_GPU_VULKAN.md; Linux37/37, Windows51/51. Validación física Linux pendiente.

| Campo | Razón |
|---|---|
| Red por proceso/usuario/app | No existe en procfs; requiere contabilidad eBPF/cgroup con privilegios y soporte del kernel. |
| Motores GPU 3D/copy/compute | Pipeline opt-in Monitor/Sampler (LINUX_DRM_MONITOR.md), taxonomía exacta AMDGPU/i915/Xe (LINUX_DRM_CLASSES.md), pares busy/total por ciclos (LINUX_DRM_CYCLES.md) y alternativa ciclos/maxfreq (LINUX_DRM_MAXFREQ.md), con sumas separadas por base. Informe linux_drm separado, no carga global GPU. Linux49/49 y Windows62/62, consumidores instalados ejecutados. Otros drivers/clases, GPU de plataforma sin PCI, cobertura hardware y físico pendientes. |
| GPU NVIDIA | Proveedor NVML implementado (modelo/PCI/utilización/memoria/temperatura/driver). Validación física NVIDIA pendiente: el equipo disponible es AMD. No incluye motores 3D/copy/MIG. Ver GPU_NVML.md. |
| GPU Linux DRM/sysfs y memoria | G06 auditada/corregida: LinuxGpuSysfsDetails VRAM/GTT separados, margen estimated, sensores etiquetados, modulo real/version y diagnósticos de inventario (LINUX_GPU_TELEMETRY.md). No shared Windows fabricado, presupuesto como libre global ni tuple mezclado con NVML. G07/N02 físicos siguen pendientes. |
| Driver date GPU | Linux: ausente/unsupported con motivo por DRM/sysfs, NVML o merge, politica G08 probada (LINUX_GPU_DRIVER_DATE.md). DRM date obsoleto/placeholder y versiones API no son fecha verificada de publicacion/instalacion. No fuente positiva Linux; no inferir BIOS/mtime/build. Windows conserva proveedor separado. |
| OpenGL version | Linux: contexto desktop EGL/GLX opt-in por PCI y perfiles maximos GLX declarados separados de GL_VERSION por Metric (LINUX_CURRENT_GLX.md, LINUX_CURRENT_OPENGL.md). No crea contextos ni prueba maximo fisico/headless. Otros vendors sin identidad quedan unknown. G07/N02 fisicos y M04 Windows siguen abiertos. |
| Notificaciones por app | Requiere observar el bus/portal desde que PCInsight empieza; el SO no conserva un contador universal. |
| Historial entre reinicios | `HistoryStore` legacy y `WindowsHistoryStore` para informe Windows con disponibilidad por campo, en archivos separados; el consumidor debe cargar, combinar y guardar con coordinación propia. No hay registro automático en segundo plano. Ver WINDOWS_HISTORY_STORE.md. |
| Startup impact | Núcleo/observador/candidatos + overlay explícito, deadline ante fallos, finales de hijos (STARTUP_CHILD_EXIT.md) y persistencia (STARTUP_STORE.md). Capturador de referencia end-to-end verificado (STARTUP_CAPTURE.md). Causa automática/captura temprana y procesos ajenos breves pendientes. Monitor sin overlay sigue not_measured. |
| Publisher/description amigable | Startup Linux tiene editor declarado AppStream opt-in, estimated, nunca firmante/propietario del ejecutable. Sin coincidencia o con ambiguedad/error queda unknown; no se infiere de username/Exec. LINUX_STARTUP_PUBLISHER.md. XDG/procfs no garantizan metadata para cada app. |
| Estado de servicios oneshot | Resuelto para systemd por ActiveState/SubState: active/exited no se reporta running. Otros init managers pendientes. |
| Control OpenRC/runit/s6 | No comparten una API universal; faltan adaptadores específicos. |
| Sesiones Windows | Inventario WTS opt-in: cuenta/dominio/estado y CPU/RAM visibles estimated por ID de sesión del token + cuenta exacta, incluso con procesos ocultos. Ver WINDOWS_USERS.md y WINDOWS_USER_RESOURCES.md. Disco/red por proceso y acciones pendientes; no se publican ceros inventados. |
| Startup Windows | Inventario Run HKCU/HKLM con vistas WOW64 y archivos de carpetas Startup conocidas; WINDOWS_STARTUP_REGISTRY.md / WINDOWS_STARTUP_FOLDERS.md. Opt-in de metadatos .lnk/.url con presupuestos (WINDOWS_SHORTCUT.md / WINDOWS_INTERNET_SHORTCUT.md), sin resolución del shell/apertura de URL. CompanyName estimated separado de consulta Authenticode opt-in, política local/cache-only sin revocación (WINDOWS_STARTUP_PUBLISHER.md / WINDOWS_STARTUP_SIGNATURE.md). Windows70/70, Linux54/54 y consumidores instalados; explorer.exe nativo LONG0, Microsoft Windows. Registro/archivo no prueba habilitación: status unknown, impacto not_measured. Catálogos/firmas múltiples/revocación vigente, resolución efectiva/tareas/paquetes, aprobación y acciones pendientes. |
| Backend Windows | CPU/RAM/SMBIOS, procesos/servicios/red/discos, sesiones WTS con sumas CPU/RAM visibles, GPU DXGI/PDH/KMT y CPU app history observado con persistencia separada (WINDOWS_APP_HISTORY.md / WINDOWS_HISTORY_STORE.md) implementados con límites documentados. Compresión estable, hardware multigrupo, otros campos GPU, startup/red y disco por proceso/control de sesiones, red/notificaciones del historial Windows pendientes. |

Los elementos pendientes no pueden resolverse de forma correcta con una única lectura “universal”. Se implementarán mediante proveedores opcionales incluidos en PCInsight, seleccionados en runtime y sin exigir programas de línea de comandos.
