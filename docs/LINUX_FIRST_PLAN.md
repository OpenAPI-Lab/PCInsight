# Prioridad de ejecucion: Linux primero

Revision63, 2026-10-21. Cambio solicitado explicitamente por el usuario: terminar
el alcance Linux antes de retomar implementaciones Windows. No es eliminacion
de requisitos ni cierre de tareas. SEGUIMIENTO.txt conserva autoridad permanente.

## Regla de trabajo

Unidad172: [verificador N02/G07](LINUX_HARDWARE_EVIDENCE.md) probado en WSL;
no sustituye evidencia física. Sigue revisar aceptación de servicios Linux K03
sin tocar servicios reales ni extender permisos a SysV.

Unidad171: B08 conserva desconocido hasta mapeo OEM y positivo físico. Sigue
revisión de cobertura verificable N02/G07; no cambiar BIOS ni cargar módulos.

Unidad170: etapa CI Linux ensayada en local, N01 en curso. No hubo push ni
ejecución remota. Sigue evidencia de virtualización de firmware B08, sin
convertir indicios en estado de firmware ni cambiar el BIOS.

Unidad169: política de versiones y puertas 1.0 definidas, manifiesto local
verificado. A05 permanece abierto por matriz ABI/aceptación. Sigue revisión
local del flujo CI Linux N01; no se ha ejecutado CI remota.

Unidad168: línea base de compatibilidad del SDK Linux en A05. Sigue política
de versionado y revisión para congelar ABI; A05 y N03 siguen abiertos.

Unidad167: A05 en curso. Marcador opcional de conjunto de headers del SDK
sin declarar ABI1.0; SDK_HEADER_COMPATIBILITY.md. Sigue revisión de contratos
de compatibilidad. N03 global y las autorizaciones previas no cambian.

Unidad166: auditoría de entrega Linux de N03 verificada, sin cierre global.
Sigue revisión Linux de compatibilidad A05 sin declarar ABI1.0; no se infiere
permiso SysV ni se altera el alcance observador I07.

Unidad165: contrato Servicios Linux y ejemplo SDK de solo lectura terminados.
Siguiente auditoría de entrega Linux de N03, sin cerrar automáticamente K03,
L05 ni otras validaciones pendientes. SysV sin permiso nuevo.

Unidad164: contrato Usuarios/sesiones Linux y ejemplo SDK de solo lectura
terminados. Siguiente Services Linux. N03 abierto; SysV sin permiso nuevo.

Unidad163: contrato Startup Linux y ejemplo SDK de solo lectura terminados.
Siguiente Users Linux. N03 e I07 abiertos; sin autorización SysV nueva.

Unidad162: contrato App history Linux terminado, tres fuentes y persistencia
separadas. Ver LINUX_APP_HISTORY_REQUIREMENTS.md. Sigue Startup Linux; N03 E
por otros bloques, auditoría y riesgos históricos. SysV sin permiso nuevo.

Unidad161: contrato Processes/Details y acciones de proceso Linux terminado.
Ver LINUX_PROCESS_REQUIREMENTS.md y ejemplo SDK sin acciones ni captura. Siguiente:
App history Linux; N03 queda E por otros bloques, auditoría y riesgos históricos.
SysV pendiente de permiso explícito y aceptación K03 real sin cambios.

Unidad160: avanzar N03/Linux mientras SysV sigue pendiente de autorización y
aceptación init real carece de entorno. Contrato Performance terminado:
LINUX_PERFORMANCE_REQUIREMENTS.md, ejemplo solo SDK/consumer instalado con
unknown/0/false/calidad/padres/unidades y límites explícitos. Linux190+20focales,
MSVC10,consumer selftest/sample0,65 enlaces/builds correctos; no biblioteca/layout
nuevos. Siguiente trazabilidad procesos/detalles y acciones, una unidad a la vez.
N03 E otros bloques/auditoría/fallos históricos; G07/N02/B08 abiertos, K03 SysV
NO autorizado por este prosigue. L05/I07/Windows intactos. Sin servicios reales/
elevación/instalar/captura/eBPF/CIclaim, C60/75=80%,20% abierto sin crédito parcial.

Unidad159: integración pública OpenRC terminada, linux_openrc_control.hpp con
Service.open/request/close y Runner independiente. Selección original fijada,
credenciales/directorios/metadata revalidados, stale permanente, consentimiento
independiente, one-dispatch, default-off y salida NO estado vivo. Linux188+50OpenRC/
MSVC20/SDK3consumidoresrelinkados0/FDlimpieza/builds finales correctos. Modelo
privado, NO aceptación real; sin instalar/elevación/servicios reales/CIclaim ni
Monitor/SystemActions. LINUX_OPENRC_CONTROL_PUBLIC.md/work/openrc-public-*.
Siguiente control SysV requiere respuesta a consulta explícita emitida; permiso148
no se extiende. Aceptación aislada de runtimes reales pendiente; sondeo PATH actual
sin rutas no demuestra ausencia global. No repetir integración pública como pendiente.
K03 E, L05 despliegue aplazado/I07/Windows intactos. C60/75=80%,20% abierto.

Unidad158: retomado K03 por la ruta recomendada, sin nuevo transporte ni ampliar
L05. Ejecutor OpenRC interno opt-in con permiso148, validación de destinos/FD,
entorno cerrado, hijo gated/pidfd, ACK/espera/resultados sin inventar estado vivo.
Modelo privado, no runtime OpenRC real. Linux187+40 OpenRC/MSVC10/SDK2consumidores
relinkados0/FDlimpieza/builds finales limpios. LINUX_OPENRC_CONTROL.md y logs
work/openrc-control-*-delivery*. K03 E: siguiente integración pública de selección
protegida/consentimiento y aceptación aislada real cuando exista entorno autorizado.
L05 despliegue E aplazado; sin instalar/activar/elevación/servicios reales ni
excepción SysV. I07 conservado, Windows aplazado. C60/75=80%,20% abierto;
sin IDs ni crédito parcial. Unidades anteriores son históricas.

Unidad157: LINUX_PRIVILEGED_TRANSPORT.md y fixture privado XML. Prueba32KiB broker
rechaza string64KiB antes del handler, cliente2 sigue válido;3intentos2entradas2auth
0Manager. Brokerwide NO porhelper, no instalar en buscompartido/añadir broker como
dependencia runtime. Linux186+160L05/30brokers/rolesSIM, MSVC10args/SDK2consumer0/
FDcleanup/buildsLinuxclean; FD0 fallóauth inicialmente, preservado/corregido fixture.
L05 E: unidad prueba/contrato terminada; siguiente dirección pendiente del usuario:
aplazar despliegue L05 yseguir K03/OpenRC (recomendado) o ampliar canal Unix privado.
No elegir por silencio ni instalar/activar/elevar/tocar servicios reales. Producción
sin cambios; despliegue/proceso-cargador/polkitreal/aceptación/UAC pendientes.
OpenRC148controlsoloexplícito intacto, SysV/K03/I07 conservados; C60/75=80%,20%
abierto, sin IDs/crédito parcial.

Unidad156: LINUX_PRIVILEGED_ADMISSION.md. Admisión antesRPC enambos endpoints,
tokenbucket por receptor+senderbroker/tabla acotada/TTLseguro/mutex/clock y
umbral de colas read-write locales. No perUID/fairness ni hardqueue-bytes/pre-
recepción ni límite replies; OFF/busy baratos salen antes. Caso nativo5req2auth
1Manager1Sub confirma cuota compartida/no evasión sendernuevo/rechazo sinqueries.
Linux185+150L05/20brokersprivados-rolesSIM, MSVC20admissionargs/SDK2consumer0/
FDcleanup/buildLinuxclean. Regresión56req32auth18Manager2Sub enperfilalto explícito.
L05 E: contrato transporte/despliegue-proceso-cargador/polkitreal/aceptación/UAC
pendientes. Sin install/activación/elevación/serviciosreales/APIlayoutformatos
públicos/Monitor/SystemActions/CIclaim; OpenRC148controlsoloexplícito intacto,
SysV/K03/I07 conservados. C60/75=80%,20% abierto; sin IDs/crédito parcial.

Unidad155: LINUX_PRIVILEGED_BUDGET.md. Presupuesto compartido en handler paraRPCs
identity/daemon/Authority/Manager/match/Subscribe/dispatch, steadyclock/mutex y
1ms..30s/1..128 (default5s96); lease defaulttimeout restored ycalls explicitcap
remaining. Ownedauthonly5s96, legacyraw sin budget intacto. COOPERATIVO, nohard
deadline de proceso/transporte/cleanup, no cancel/retry, aftersendUNKNOWN PUDO
actuar. Linux183+130L05/10brokerreal privado-Authority/ManagerSIM,56req32auth18
Manager2Sub/retencionesguardadas/restorenative2s. MSVC20budgetargs/SDK2consumer0/
FDcleanup, clockskewinicial preservado/finalbuildclean/suitefinal183 sintocarreloj.
L05 E siguiente admisión/transporte-colas; despliegue-proceso-cargador/polkitreal/
aceptación/UAC pendientes. Sin install/activación/elevación/serviciosreales/
APIlayoutformatos públicos/CIclaim; OpenRC148controlsoloexplícito intacto,
SysV/K03/I07 conservados. C60/75=80%,20% abierto; sin IDs/crédito parcial.

Unidad154: LINUX_PRIVILEGED_JOBS.md. Match/Subscribe pinnedowner antes de dispatch,
correlación uoss ID-pathuint32-unit/early64/events4096/64pendingwatches/deadline,
datos asíncronos thread-safe sin nativecalls desde consumidor. Propiedad de slots
de conexión/cleanup y completed histórico; timeout/revocación NO cancelan, completed
NO servicio actual. Linux182+120L05/10brokerreal privado-Authority/Manager SIM,
50requests29auth16Manager2Subscribe; MSVC20FSMargs/SDK2consumer0/FDcleanup/
buildsLinuxclean. L05 E siguiente límites/deadline agregado del receptor, luego
despliegue/cargador/polkitreal/aceptación/UAC. No install/activación/elevación/
serviciosreales/APIlayoutformatos públicos/CIclaim; OpenRC148controlsoloexplícito
intacto, SysV/K03/I07 conservados. C60/75=80%,20% abierto; sin IDs/crédito parcial.

Unidad153: LINUX_PRIVILEGED_EXECUTION.md. Ejecutor interno ControlSystemService,
OFF defecto/UIDManager explícito/busy, no auth-report reusable ni cambio endpoint
auth-only. Freshauth/context/Managerowner/policy/conn before, Start-Stop-Restart
unit+replace al owner único/autostart0-interactive0/5s un intento, receipt=queued
NO completion; fallo trasenvío UNKNOWN/PUDO actuar, sin retry/rollback/atomicidad.
Linux181+110L05/10brokerreal privado-Authority/Manager SIMULADOS,41requests20auth8
Manager porrecorrido. MSVC20defaultsargs/SDK2consumer0/FDcleanup; clockskew inicial
preservado/incrementalfinalclean/suitefinal181. L05 E siguiente límites/seguimiento
jobs; despliegue-cargador/polkitreal/aceptación pendientes. Sin install/activación/
elevación/serviciosreales/APIlayoutformatos/CIclaim; OpenRC148controlsoloexplicito
intacto, SysV/K03/I07/UAC conservados. C60/75=80%,20% abierto; sin IDs/crédito.

Unidad152: LINUX_PRIVILEGED_CONNECTION.md. Ancla absoluta validada/openat-noFollow/
UIDmodo/namespaceHILO/PIDEUID; stickyancestorsOFF defecto (pruebas/tmpONexplícito).
Conexión propia nueva con endpointcerrado/UIDnodo-peer/Unixpeercred/CLOEXEC,
fds0-anonymous0-interactive0-exitdisconnect0/timeout<=5s/generación/revocación.
Receptor unido a buspropio/postrevalidación,22requests9checks con Authpositiva
descartada tras cambio de ancla. Linux180+100L05/10brokerreal privado-Authoritysim,
MSVC20argsfallback/SDK2consumer0/FDcleanup; warningsindentación iniciales corregidos,
verified/finalbuildclean. L05 E: límites/flujoejecución/despliegue endurecido antes
de aceptación real; sin install/activación/elevación/serviciosreales/polkitreal/
APIlayoutformatos/CIclaim. OpenRC148solo controlexplícito intacto, SysV/K03/I07/UAC
conservados. C60/75=80%,20% abierto; sin IDs/crédito parcial.

Unidad151: LINUX_PRIVILEGED_POLICY.md. Parser cerrado96KiB/256 +lector nativo
openat/FD ancla+UIDconfiables/permissions/types/symlink-hardlink/rutas/revalidación.
Refresh invalida generación sin fallback; receptor gestionado descarta política
invalidada durante CheckAuthorization positiva. Linux179+90repeatL05/10brokerreal
privado-Authoritysimulada, MSVC20parserargsfallback, SDK2consumer0/FDcleanup/
buildsLinuxclean. L05 E: siguiente bootstrap/conexión; entorno/cargador/ancla
fiables, watcher/deadlines/ejecutor/polkitreal/aceptación siguen pendientes.
Sin instalación/elevación/serviciosreales/APIlayoutformatos/CIclaim; OpenRC148
solo control explícito intacto, SysV/K03/I07/UAC conservados. C60/75=80%,20%
abierto; sin IDs ni crédito parcial.

Unidad150: LINUX_PRIVILEGED_RECEIVER.md. Receptor authorize-only cerrado uusb,
sin identidad/argv del payload; identidad/subject del transporte. Prueba con
libsystemd/broker privados REALES, Authority simulada, sin polkitd/ejecutor/
instalación/elevación/servicios reales. Linux178+80repeatL05 (10broker)/MSVC10args/
SDK2consumerrelinked0/FDclean, cleanupprivado y buildfinalclean; clockskewinicial
preservado sin tocar reloj. L05 E: siguiente configprotegida/ciclo de vida;
ejecutor/política/consentimiento reales/aceptación/UAC pendientes. OpenRC148 solo
control explícito intacto, SysV no ampliado/K03/I07 conservados. C60/75=80%,20%
abierto, sin IDs ni crédito parcial.

Unidad149: LINUX_PRIVILEGED_NATIVE_AUTHORIZATION.md, CheckAuthorization nativo+
coordinadorfactoryfrozen/request-scope/subject-owner/identity/cookie/parserbounded,
sin autostart/interaccion defecto/caché/retry/ejecutor. Linux176+60/MSVC10argsfallback/
SDK2consumerrelinked0/FDclean; ABI SIMULADA/socketkernel privado NO polkitreal/
receiver/helper/dialogos/privilegios/instalación/aceptación. Clockskew inicial
preservado/finalbuildclean sin tocar reloj. L05 E: siguiente receptor aislado
payloadcerrado/identidadtransporte real, luego bootstrap/config/ejecutor/aceptación;
permisoOpenRC148 controlsolo explícito intacto, SysV sin ampliar/K03/I07/UAC
conservados. C60/75=80%,20% abierto, sin IDs/crédito parcial.

Unidad148 DECISIÓN vigente 2026-11-02: usuario aprueba excepción OpenRC132
solo scripts/runtime existentes para start/stop/restart explícitos, opt-in/OFF y
pruebas privadas; NO datos/install/elevación/servicios reales ni extensión SysV/
helper/polkit/autostart. LINUX_OPENRC_CONTROL_DECISION.md registra permiso.
K03 E, retirado bloqueo de decisión no implementación/aceptación/cierre; notas
previas sinexcepción son históricas OpenRC. L05/Linux sigue operativa, siguiente
CheckAuthorization nativo aislado; al retomar OpenRC no pedir otra vez permiso
de desarrollo. Cada acción futura requiere consentimiento explícito de ejecución.
Sin código/tests nuevos, C60/75=80%,20% abierto, sin IDs/crédito parcial.

Unidad147: LINUX_PRIVILEGED_IDENTITY.md, observador nativo desde mensajeactual/
conexion/Unixendpoint-peercredUID/ID-UIDcaller-ownerAuthorityUID y revalidación,
config mecanismo no payload/rootUID fijo. Prueba ABI SIMULADA/socketkernel real
privado, NO brokerreal/authpolkit/receiver/helper/CheckAuthorization. Linux174+40/
MSVC10argsfallback/SDK2consumerrelinked0/FDclean/buildclean. L05 E: siguiente
CheckAuthorization nativo subjectfijado/correlación/parsingbounded con fixtures
aisladas, sin diálogos/helper/ejecutor. Config/bootstrap/receiver/integración/
aceptación/UAC pendientes, K03/OpenRC132/I07 intactos. C60/75=80%,20% abierto,
sin nuevosIDs/crédito parcial/API-layout-formato/acciones/instalación.

Unidad146: LINUX_PRIVILEGED_AUTHORIZATION.md, coordinador interno con fixtures
snapshot/contexto-before-after/onecheck/sendercookie/rejectchallenge-retained-error,
UIDAuthority configurado por mecanismo (corregido supuesto0), NO nativeadapter/
authreal/polkitreal/helper/ejecutor. Linux172+20/MSVC10pure/SDK2consumerrelinked0/
buildclean, sin API/layout/formatos/acciones nuevos. L05 E: siguiente adaptador
nativo identidad/Authority sobre transporte verificado y rechazos privados,
sin instalar/activar/helper/ejecutar. K03 aplazada, OpenRC132/I07 intactos/UAC
aplazado. C60/75=80%,20% abierto, sin IDs/crédito parcial.

Unidad145: LINUX_K03_AUDIT.md cierra auditoría, NO K03: E aplazada por control
OpenRC/SysV sin excepción y aceptación real no disponible en PATHWSL comprobado.
Inventarios/vistas/Monitor listos segúncontrato; no más proyecciones por bloqueo.
Pasar L05/Linux: LINUX_PRIVILEGED_PREFLIGHT.md, preflight privado puro de
operaciones cerradas/allowlistmecanismo/intent/bounds/invaliddeny/ownedcopy.
Linux171+10/MSVC10pure/SDK2consumerrelinked0/buildclean. L05 P->E, NO auth/peer/
polkitgrant/daemon/ejecutor/root/install/activación/UAC. Siguiente identidad IPC/
autorización con rechazo/fixtures privados; OpenRC132/I07 intactos.
C60/E10/P3/B2+3X excluidas=60/75=80%,20% abierto, sin IDs/crédito parcial.

Unidad144: LINUX_SERVICE_SELECTION.md, Monitor Linux selecciona7rutas/vistas
explícitas sin fallback, legacy_default intacto/disabled noquery, guards/switch/
quality, OpenRCuna-vista sinjoin. SampleOptionsABI ampliado RECOMPILAR TODOS
clientes0.x; SDK2consumer recompiled0 inclMonitorLinuxnuevo. Linux170+10/MSVC10pure/
FDsentinel OK, clockskew inicial preservado/finalbuildclean sin tocar reloj.
K03 E: siguiente auditar cierre porproveedor/evidencia/bloqueos y continuar L05
cuando proceda; sin prolongar hardware no disponible ni inventar aceptacion.
Sin acciones/scripts/formatos/ServiceInfo-SystemSnapshot-layout cambiados,
OpenRC132/I07 intactos. C60/75=80%,20% abierto, sin nuevosIDs/crédito parcial.

Unidad143: LINUX_OPENRC_SERVICES.md, OpenRC explícito a ServiceInfo doslistas
configured/marked con fuentes Metric/índices separados; NO identidad por nombre.
marked-* estimated no vida/readiness, PID/desc/grupo/política UNKNOWN, partial/
permisos independientes. Linux169+30/MSVC10/SDK2consumer0/FDsentinel/buildclean,
sin librc/CLI/exec/scripts/creación. K03E: siguiente selección explícita del
proveedor en Monitor, default intacto, sin fallback silencioso/nuevas acciones;
integración/control/aceptación pendientes. OpenRC132/I07 intactos.
C60/75=80%,20% abierto, sin IDs/crédito parcial.

Unidad142: LINUX_S6_SERVICES.md, s6 explícito a ServiceInfo/source/index/logger,
record-* Metric estimated NO vida/readiness/controlcapability, PIDu64/waitword/
ready solo source y PID comúnUNKNOWN; guards partial/invalid/permisos/shapes.
Linux168+50/MSVC10/SDK2consumer0/FDsentinel/controlsinbytes; clockskew inicial <1s
conservado y finalbuildclean sin tocar reloj. K03E: siguiente OpenRC sin scripts;
integración/aceptación real pendientes. Monitor/layout/contratos133-136 intactos,
sin exec/excepción inferida; OpenRC132/I07 intactos. C60/75=80%,20% abierto,
sin IDs/crédito parcial.

Unidad141: LINUX_RUNIT_SERVICES.md, runit explícito a ServiceInfo/evidencia por
índice/IDs scoped, reported-* Metric estimated NO vida/hostPID/authpeer. PID solo
source y comúnUNKNOWN, partial/record/permisos UNKNOWN, no desc/grupo/política
inventados. Linux167+30/MSVC10/SDK2consumer0/FDsentinel/controlsinbytes/buildclean.
K03E: siguiente proyección s6; otrosproveedores/integración/aceptación pendientes.
Sin Monitor/layout/contratos128-129 cambiados/exec/scripts ni excepción inferida,
OpenRC132/I07 intactos. C60/75=80%,20% abierto; sin IDs/crédito parcial.

Unidad140: LINUX_SYSV_SERVICES.md, consulta SysV explícita a ServiceInfo con
evidencia139 conservada, Short/Long fallback/permisos/partial, IDs scoped y
PID/vida/grupo/enablement UNKNOWN. Sin Monitor/systemd/layout/exec cambiados.
Linux166+40/MSVC10/SDK2consumer0/FDsentinel OK, finalbuildclean; clockskew inicial
<1s conservado sin tocar reloj/mtime. K03E: siguiente proyección explícita runit,
después otros proveedores/integración/control/vida/aceptación real. Sin excepción
scriptsSysV/OpenRC132, I07observador intacto. C60/75=80%,20% abierto, sin nuevosIDs.

Unidad139: LINUX_SYSV_DESCRIBED_CATALOG.md, catalogo con LSB opt-in ligado a
candidato fijado/identidad inicial, doscensos/raices/contexto y swaps UNKNOWN,
sin join de snapshots viejos por nombre. Budget0..16MiB/reserva nodiskusage,
Linux165+30/MSVC20/SDK2consumer0/FDclean, buildfinal limpio, clockskew inicial
<1s preservado. K03E: siguiente proyeccion explicita al modelo comun de servicios;
integracion/control/vida/aceptacion real abiertos. Sin exec/CLI/scripts ni
excepcion inferida, OpenRC132/I07observador intactos; C60/75=80%,20% abierto.

Unidad138: LINUX_SYSV_LSB.md, lectura LSB declarada con parser/prefix64KiB/
type-context-pins/UTF8/bounds, sinexec/eval/CLI. Linux164+20/MSVC10/SDK2consumer0/
FDclean, fallo inicial de sangria en limite1024 corregido/logs preservados,
finalbuildclean. K03E: siguiente binding de metadata a candidato fijado sin
basenamejoin; despues integracion/control/vida/aceptacion real. Sin excepcion
inferida para scripts, OpenRC132 eI07observador intactos.

Unidad137: LINUX_SYSV.md, catálogoreadonly SysV initd+rcparent/once niveles,
nameconvention S/KNN, inodeassociation única ytype-only fields, contexto/two-
pass/bounds/unknownpartial. Linux163+20/MSVC10/SDK2consumer0/FDcleanprivado,
sinexec-bodyread-CLI-control ni aceptación real. K03E: siguiente LSBcomments
para descripción sin shell; control/vida/integración/aceptación pendientes.
OpenRCscripts132 noautoriza scriptsSysV; amboscontrolscript sin excepción,
I07observador aplazado intacto.

Unidad136: LINUX_S6_OBSERVATION.md, rawconditions ligadas acontrolpin, one-shot
consent/baseline/poll/deadline, NO vida/readiness/causacompletion; bytesretenidos/
completedUNKNOWN. Linux162+40/MSVC10/SDK2consumer0/FDcleanprivado/buildclean,
sinaceptacion s6 real/CLI/exec/scripts. K03E; siguiente inventario SysV read-only,
sin inferir autorización para ejecutar scripts. Integración/aceptación real y
otros pendientes intactos; OpenRCscripts132 aplazado eI07observador intacto.

Unidad135: LINUX_S6_INVENTORY.md, explicitScanInventory dirs-alias+1logger,
parentpin/readAtPin/context-rootguards/bounds/partialvisible. Rawrecord/FIFOtype
NO alive/peer/active/actiontoken; brokenlogUNKNOWN no ausencia. Linux161+30/
MSVC10/SDK2consumer0/FDcleanprivado, sinCLI-exec-locks-writes-serviciosreales.
K03E: siguiente observación s6 ligada acontrolfijado, luego integración/aceptación
real/otrosinit. OpenRCscripts132 sigue sin autorización; I07observador intacto.

Unidad134: LINUX_S6_CONTROL.md, control s6 FIFO nativo consent/pins-context/
u-d-cru, countedbytes NOcompletion; EPIPE/EAGAIN/SIGPIPE-mask-pending/forkstale/
replacement/FDclean privados. Linux160+20/MSVC10/SDK2consumer0/buildcleanfinal,
sinexec/scripts/CLI/deps/serviciosreales/aceptación real. K03E: siguiente inventario
s6, luego observación/integración/aceptación ySysV/otros pendientes. OpenRCscripts
continúa sin autorización y aplazado; I07observador intacto.

Unidad133: LINUX_S6.md, decoder/lector registro35 nativo, historialbinario NO
liveness/readiness efectiva. Linux159+20/MSVC10/SDK2consumer0/FDcleanprivado;
sin exec/locks/libs6/CLI/deps/aceptación real. Anteprosigue sinrespuesta seavanza
alternativa s6, NO excepción OpenRCscripts. K03E: siguiente handle/control s6
porprotocolo con consentimiento, luego inventario/integración/aceptación ySysV.
I07observador aplazado intacto; controlOpenRC sigueabierto sin autorización.

Unidad132: LINUX_OPENRC_CONTROL_DECISION.md, auditoría sin código/tests nuevos.
Decisión pendiente sobre scripts/runtime SOLOcontrol OpenRC explícito opt-in,
nunca recopilación, pruebasprivadas/sin instalar-elevar/tocarserviciosreales;
alternativa aplazar ese control yseguir s6/SysV. Sin respuesta no habilitar
ejecutor ni interpretar prosigue como excepción. K03E/I07observador intactos.

Unidad131: LINUX_OPENRC_CATALOG.md, catálogo explícito1init.d+1runlevels,
metadata/links por dev-inode, stacks directos con ciclos sin expansión. NoCLI/
librc/bodyread/exec/mkdir/active-bootenabledclaim. Linux158+20/MSVC10/SDK2consumer0,
privado/FDclean, sin aceptación OpenRC real. K03E: próxima unidad revisar control
nativo OpenRC, sin sustituir start/stop por escribir marcas. Liveness/integración,
OpenRC-runit real/s6/SysV siguen pendientes; I07observador aplazado intacto.

Unidad130: LINUX_OPENRC.md, lector nativo de marcas runtime OpenRC, sin librc/
CLI/shell/scripts/mkdir. Marcas NO liveness/APIstate; ausencia NO stopped.
Linux157+20/MSVC10/SDK2consumer0, fixture privada/FDclean; sin aceptación real.
K03E: siguiente unidad installed/runlevels OpenRC; control/liveness/aceptación
OpenRC, runit real/s6/SysV/integración pendientes. No instalación para forzar prueba.

Decisión explícita 2026-10-30: el usuario elige mantener observación y pasar a
K03. No desarrollar iniciador propio. I07 queda abierta/aplazada cuando no hay
causa verificable; conserva sus criterios. Confirma el curso conservador128.

Unidad129: LINUX_RUNIT_TRANSITIONS.md, observación posterior explícita de una
solicitud runit; nueva época TAI para restart, polling/deadline sin reintentos ni
ready/causal completion. Linux156+20/MSVC10/SDK2consumer0, FIFOs privados, no
realrunsv acceptance. K03 continúa abierta; siguiente trabajo: aceptación real
cuando exista supervisor y adaptadores restantes OpenRC/s6/SysV/integración.
No instalar runit ni ampliar autostart/I07 para conseguir esa validación.

Actualizacion128: "prosigue" tras la decision127 se toma conservadoramente como
continuar el alcance observador y avanzar K03 recomendado, NO autorizacion de
un nuevo iniciador de autostart. I07/criterio/causaUNKNOWN quedan aplazados y
abiertos; no se reduce su alcance ni se marca cierre. Orden operativo K03->L05
y validaciones Linux, retomando I07 cuando haya fuente/decision adecuada.
Unidad128: LINUX_RUNIT.md, protocolo explicito runit open/read/inventory/request;
Linux155+20/MSVC10/SDK2consumer0, privadoFIFO solamente. Aceptacion runs v real,
confirmaciontransiciones/OpenRC/s6/SysV/integracion siguen enK03, no cierre falso.

Flujo del consumidor confirmado por el usuario: desarrollar primero el GUI y
dejar su CMake para el final; puede enlazar la biblioteca desde su IDE durante
el desarrollo. El CMake interno de PCInsight se mantiene para compilar/probar
la biblioteca, independiente de ese flujo. No iniciar un GUI ni su build por
esta preferencia sin solicitud del usuario.

Una unidad funcional de una subtarea a la vez: implementar, verificar en Linux,
registrar evidencia/limites y actualizar el TXT antes de pasar a otra. No abrir
trabajo Windows nuevo ni repartir actividad entre varios frentes. Compatibilidad
Windows solo se comprueba si un cambio compartido lo hace necesario; eso no
autoriza avanzar funciones Windows ni reiterar pruebas UAC canceladas.

Se detiene M02 en el checkpoint revision62: decoder/correlador/indice probados,
sin productor ETW nativo completo ni integracion publica. En revision63 solo se
reviso documentacion/codigo para ordenar pendientes: no se agrego codigo Windows
de ciclos de vida ni se inicio captura/elevacion. M05 sigue ademas aplazada por
la decision anterior del usuario. M01/M04/M06/M08 esperan la fase Windows.

## Orden Linux sobre los IDs existentes

1. F03: CERRADA revision98 como proveedor nativo opcional de contabilidad de red,
   atribucion/cobertura/ventanas explicitas por proceso. Evidencia y limites:
   LINUX_NETWORK_ACCEPTANCE.md. Proxima prioridad de implementacion: H04.
   Un socket compartido o TCP parcial no demuestra red causal/exclusiva total.
   Pedir autorizacion cuando una prueba real requiera ampliar privilegios o
   alcance; las autorizaciones previas eran temporales y acotadas, no globales.
2. H04: CERRADA revision100, fuente F03 integrada en linux_network_app_history,
   ejecutable verificado/deltas por cookie/periodos propios/persistencia real sin
   doble cuenta. Evidencia LINUX_NETWORK_APP_HISTORY.md. Proxima prioridad J04.
   Criterio original conservado: integrar la fuente F03 en historial por app con identidad, disponibilidad
   y persistencia sin doble cuenta; leer HISTORY.md antes de cambiar semantica.
   Revision99 termina LinuxHistoryStore (disponibilidad/merge/retencion), no la
   atribucion de red por si solo. Revision100 verifica F03/exe antes-despues,
   no PID/ruta aislados; positivo nativo y roundtrip real sin doble cuenta/gaps.
3. J04: CERRADA revision101, red agregada unica por UID del observador F03,
   namespace/coverage/ventanas explicitos y sin join de sesion no verificado;
   disco anterior conservado. LINUX_USER_NETWORK.md. Sin convertir datos
   ausentes en0 ni sumar sesiones repetidas del mismo UID dos veces. Siguiente I07.
4. I07: cerrar captura temprana/atribucion de startup impact, procesos breves,
   permisos y evidencia end-to-end; no presentar asociaciones manuales como
   correlacion automatica verificada.
   Revision103 completa host opcional autorizado para desarrollo SIN activar:
   STARTUP_RECORDER.md. No cerrar I07 por ese host; continuar causa fiable,
   ciclo de vida ajeno/descendientes y cobertura/diagnosticos de captura temprana.
   Revision104: sensor INTERNO de salida seleccionada positivo tras reaping en
   uno/multihilo; STARTUP_SELECTED_EXIT.md. Faltan contadores/causa/cobertura e
   integracion, no activar recorder ni cerrar I07 por evento de ciclo de vida.
   I07 revision105: contadores internos de liberacion directa terminados;
   STARTUP_FINAL_RESOURCES.md. Cobertura parcial/CPU separado/IO incluye hijos
   esperados,5casos privados verificados. I07 aun requiere baseline/integracion/
   causa fiable/cobertura; no cambiar de prioridad ni activar host por este componente.
   Continuacion I07 revision106 anterior a K03: disco final seleccionado ya
   integrado en recorder/observer/archivo, opt-in OFF. STARTUP_DISK_TAIL.md.
   CPU lower bound separada; faltan CPU compatible/vidas no seleccionadas/
   causa fiable/diagnosticos durables. Seguir I07, no activar host por este avance.
   Revision107 completa diagnosticos agregados durables v2 (v1 sigue unknown),
   STARTUP_DIAGNOSTICS.md. Host/casos BPF privados con roundtrip/limites; no
   cerrar I07 por persistencia: seguir CPU compatible/vidas no seleccionadas/
   causa fiable. No activar host ni retomar Windows por este avance.
   Revision108 completa CPU inicial/final compatible seleccionada/integrada,
   STARTUP_SCHEDULER_CPU.md. POSIX clock+pidfd/kindguard/fallback sin mezclar ticks;
   nativo CPU-only high/7casos privados. I07 sigue por vidas nunca seleccionadas/
   descendientes y causa autostart fiable; no retomar Windows ni activar host.
   Revision109 completa sensor INTERNO de creacion por TGID seleccionado antes
   de primera ejecucion, positivo tras reaping antes de primer read PID hijo.
   STARTUP_BIRTH.md: flags de cupo/lectura explicitos y solo contadores AL NACER.
   Seguir I07 con finales de hijos/mapping/integracion/descendencia/causa fiable,
   sin activar host ni captura global por este componente.
   Revision110 completa conexion INTERNA a finales CPU/disco del mismo creador/
   reaper, sin seleccionar cada PID hijo antes de desaparecer. STARTUP_CHILD_FINALS.md:
   native8escenarios/9finales/5sin-final esperado; CPU group scheduler e IO incluye
   hijos esperados, sin sumar arboles ni zero fabricado. Opt-in OFF, sin API/formato
   publico ni automatizacion. Seguir mapping local/atribucion/integracion segura,
   otras rutas reaper/descendencia/causa fiable. I07 E, no activar host por esto.
   Revision111 completa TGID local al nacer cualificado por namespace OBSERVADOR,
   STARTUP_BIRTH_PID.md. BTF upid/ns/level bounded0..32, no heuristica PID global;
   flag32/unknown preserva CPU-IO. Native9casos15PID post-reap/init privadoPID1
   traducido. I07 E: seguir atribucion app/exec/launch e integracion segura, otras
   rutas reaper/descendencia/causa fiable, sin activar host ni retomar Windows.
   Revision112 completa API PUBLICA LinuxStartupBirthObserver/asociacion explicita
   por key exacta de cada hijo, STARTUP_BIRTH_OBSERVER.md. No app heredada del
   padre, no sumas IO-arbol ni relojes/start_ticks inventados. Native9casos15filas
   publicas/10finales/5sin-final. I07 E: evidencia fiable app-exec-launch/causa,
   integracion temporal con recorder/classifier sin mezclar dominios/duplicar IO,
   otras rutas/descendencia. No activar host ni retomar Windows.
   Revision113 completa primera evidencia filesystem en exec EXITOSO, opt-in OFF,
   STARTUP_FIRST_EXEC.md. Allowlist birth/TGID+start y anon MM validado, primer
   registro inmutable/flag64. Native13casos6exec post-reap/fallo/reexec/no-lider.
   No ejecutable actual/desktop/cause automaticos. Seguir app-desktop-launch fiable,
   causa/integracion temporal/otras rutas/descendencia. I07 E, no activar host.
   Revision114 completa candidatos XDG por filesystem current-caller, no binding
   automatico, STARTUP_EXECUTABLE_MATCHES.md. Unique solo1+completo estimated,
   shared/disabled ambiguos/unknown impide unicidad; parser literal absoluto/batch.
   Native13/6exec reales positivos tras un resource mismatch inicial no reproducido
   (N03 causa pendiente). Seguir receipt/app-causa fiable/integracion temporal/
   otras rutas/descendencia; I07 E, no activar host ni retomar Windows.
   Revision115 corrige oraculo exacto entre capturas independientes, no cambia
   probe/formula. STARTUP_CAPTURE_VALIDATION.md: misma captura exacta + segunda
   DEFAULT con CPU mismas cotas/IO-fstat exactos/OWN replay; rewriteCPU1ns rechazado.
   Native13+20positivos, Linux143+20/MSVC10/SDK0. CPU release no post-switch total
   garantizado; kernel interleaving no demostrado/N03 abierto. Seguir I07 receipt/
   causa fiable e integracion temporal, no Windows ni activacion host.
   Revision116 completa puente puro de impacto para hijos retenidos/binding
   explicito, STARTUP_BIRTH_IMPACT.md.60s desde CADA nacimiento/no boot-login,
   deadline kernel/release dentroventana, high-only lowerbound o unknown; no
   coercion legacy/arbol/causa/activacion. Linux144+20/MSVC10/SDK0/Native13con23filas.
   I07 E: seguir receipt/app-causa fiable e integracion recorder-durable-overlay,
   otras rutas/descendencia, no Windows ni host activado; N03 abierto.
   Revision117 completa captura propia de hijos en recorder watch_births/associate_birth,
   STARTUP_RECORDER_BIRTH.md.1creador opt-in/poll cada llamada/vistas separadas/cleanup.
   Save rechaza antesfilesystem: falta formato birth-aware, no coaccion legacy ni
   perdida silenciosa. Linux145+20/MSVC10/SDK0/Native13con23filas/FDpositivos.
   I07 E: siguiente formato birth-aware/overlay y receipt/app-causa fiable, otras
   rutas/descendencia; no Windows ni host activado. N03 abierto.
   Revision118 completa PCISTARTv3 birth-aware/recorder.save, STARTUP_BIRTH_STORE.md.
   Rawreport/window/policy/availability sin clocks legacy/sumas/autoautoridad;
   v1/v2 compatibles, rebuild SDK0/Linux146+20/Native13con23filas roundtrippositivos.
   MSVC3finales fueraaislamiento trasreplaceerror5 inicial, logs guardados/N03 abierto.
   I07 E: siguiente guardedoverlay/receipt app-causa fiable/otras rutas-descendencia;
   no Windows features ni host activado, sincredito parcial.
   Revision119 completa overlay HISTORICO birth conconsent/contexto/validatorv3,
   STARTUP_BIRTH_OVERLAY.md. No causa app probada/currentusage/suma/clockslegacy.
   Linux147+20/MSVC10/SDK0/Native13con23filas desdev3/FDpositivos.
   I07 E: SIGUIENTE receipt/atribucion app-causa fiable y otrasrutas/descendencia;
   no ampliar framework de proyeccion por ese criterio. N03 abierto/no Windows
   features ni activacion host/GUI, sincredito parcial.
   Revision120 termina INTERNALgatepreexec/filepin/clone3pidfd/armcallback/NNP/
   safecancel, STARTUP_GATED_LAUNCH.md. Linux148+20/POSITIVEELF+4kerneljoinsprevios/
   SDK0/MSVC10fallback. NOreceiptapp/autostartprovado; siguiente observar exec
   EXITOSOdelhijoautenticado/compararfilepin ydesktoproute fiable. Luego receipt/
   managedautostart/otrasrutas-descendencia, no masproyeccion comocriteriocausal.
   I07E/N03abierto/noWindowsfeatures/hostloginGUIactivacion/sincreditoparcial.
   Revision121 termina INTERNAL selectedsuccessful-exec: pidfd+pin autenticados
   antesrelease, attachallowlistempty/enrollLAST/TGID-startboot, firstexe tuple
   vspin/no app-content-causeproof. UNUSEDZERO no nacimiento/CPU inventados;
   afterreap/reexecinmutable/ausenciaUNKNOWN.5privados3success+2failure y5sentinels
   ignorados/FDlimpios, Linux149+20/MSVC20puros/SDK0. STARTUP_SELECTED_EXEC.md.
   I07E: SIGUIENTE desktoproute validada y recibo gestionado/app-causa fiable,
   otrasrutas/descendencia; no ampliar proyeccion comoprueba causal. No activacion
   host/loginGUI/capturaglobal/Windowsfeatures, N03abierto/sincreditoparcial.
   Revision122 termina INTERNAL desktoproute: snapshot XDG PROPIO efectivo,
   Execdirectosubset/argvliteral/Path, callbackgate con recheckfields/precedencia/
   eligibleconditionexacttrue antes/despues; stale cancel/reapstillgatedchild.
   Desktop->firstexecpinAFTERreap privado positivo/2mutacionescanceladas/FDlimpios,
   Linux150+20/MSVC10puros/SDK0. STARTUP_DESKTOP_LAUNCH.md. NO causaautomatica por
   lanzamiento MANUAL ni currentfile/app/contentproof. SIGUIENTE recibo gestionado
   con causa manual vs automatica honesta e integracion/otrasrutas-descendencia.
   I07E/N03abierto/no publicAPI-format nuevo/hostloginGUI/global/Windowsfeatures.
   Revision123 termina INTERNAL LinuxDesktopLaunchSession/reciboMANUAL compuesto:
   dobleconsent/oneshot/nofallback/route-fullkey-pin-outcome/execquality/UNKNOWN,
   sin allocpostrelease.4privados matchafterreap+2failureunknown+livechildstopNO
   kill/reap(callerlibera+wait), replay/FDlimpios. Linux151+20/MSVC10puros/SDK0.
   STARTUP_DESKTOP_RECEIPT.md. SIGUIENTE integrarrecibo-recorder/recursos y ruta
   autostartgestionadaconfiable sinactivarla; nunca promocionar manual a automatic
   cause. I07E/otrasrutas-descendencia/N03abierto/noAPI-format/Windowsfeature nuevos.
   Revision124 termina INTERNAL manualreceipt->birthresources: creatorhookANTES
   launch/exactkernelkey-boot-capture/ns-firstfile/stagingcopy/validatorreadonly,
   finishpoll-close-detachsinwait.4privados CPU22418570ns/disk1155072B AFTERreap,
   2preexecerrorsnoatribuidos/livefinishUNKNOWN/callerwait/rawunbound/FDlimpios.
   Linux152+20/MSVC10pure/SDK0, STARTUP_DESKTOP_RESOURCES.md. SIGUIENTE recorder/
   durable yautostartgestionadoconfiable sinactivacion/manual!=automaticcause;
   I07E/otrasrutas-descendencia/N03abierto, no nuevo formato/publicAPI/Windowsfeature.
   Revision125 termina INTERNAL PCIMANv1 atomico rawmanualreceipt+births/window/
   policy/historyonly/reprojection, IOlegacyshared sin formato publico cambiado.
   StrictLinux0600/currentownerBEFOREpayload; mountmodeignoredrefusal preserva
   oldtarget.4privados0600roundtrips/rawunbound/UNKNOWN/FDclean; Linux153+30/SDK0/
   MSVC10manual+2legacy. STARTUP_MANUAL_STORE.md. SIGUIENTE recorder/integracion
   autostartgestionadoconfiable sinactivacion/manual!=automaticcause, I07E/
   otrasrutas-descendencia/N03abierto. Sin publicAPI/Windowsfeatures/IDs nuevos.
   Revision126 termina INTERNAL bridgeEXISTENTErecorder manual-only/ownsource/
   finalize-canonical-savePCIMAN, noGIO-scan/legacytablesmix/rawunbound.4privados
   REALrecorder positive/failure/liveUNKNOWN/finish-save-reload/0600/FDclean,
   Linux154+30/MSVC20manual-legacy/SDK0. STARTUP_RECORDER_MANUAL.md. SIGUIENTE
   trustedmanagedautostartcontext sinactivacion yotrasrutas-reapers-descendencia,
   I07E/N03abierto/noAPI-format/Windowsfeature/ID nuevos.
   Revision127 AUDITORIA: causaOSauto no proviene de metadata/login/hostpropio;
   STARTUP_EARLY_DECISION.md. Decision de rol pendiente: mantenerobservador y
   aplazarcausaI07/seguirK03, oiniciadoropt-in solo desarrollo privado, sinactivar.
   No se modificaelorden/criterio/cierre antesrespuesta. No nuevos tests/codigo/
   porcentaje/IDs. Ultimaevidencia126 historica, I07E/N03abierto.
5. K03: completar adaptadores nativos de init alternativos OpenRC/runit/s6/SysV,
   uno por vez, con inventario/control y pruebas. Auditoría145: inventarios/vistas/
   selección Monitor implementados; K03 E aplazada por control OpenRC/SysV no
   autorizado y aceptación real no disponible. No más proyecciones por ese bloqueo;
   avanzar L05/Linux sin cerrar K03. LINUX_K03_AUDIT.md conserva matriz/evidencia.
   No contar archivos encontrados
   como servicios corriendo ni ejecutar utilidades externas para recolectar datos.
6. L05: parte Linux del helper/autorizacion polkit y sus pruebas de rechazo,
   alcance cerrado y consentimiento, sin instalar/activar componentes privilegiados
   automaticamente. El criterio compartido no se cierra por omitir su parte UAC.
7. B08/G07/N02: ejecutar validaciones pendientes cuando el entorno/hardware
   permita evidencia real de firmware, GPU y distribuciones. Siguen pendientes/
   bloqueadas donde corresponda; fixtures o datos Windows no prueban Linux fisico.
   Solicitar evidencia/entorno al usuario cuando sea el impedimento real.
8. A05/N01/N03: cerrar la parte Linux de contrato, empaquetado, CI y auditoria de
   campos/acciones. No declarar CI remota por compilar localmente ni cerrar un
   criterio compartido que aun requiera Windows. Documentar dependencias externas.

Este orden es de ejecucion, no una lista de nuevas subtareas ni trabajo paralelo.
Si F03 requiere una decision/permiso externo que impida continuar, informar el
bloqueo y pedir direccion; no consumir prompts repitiendo pruebas sin novedad.
Un limite real debe exponerse como tal, no esconderse ni sustituirse por un proxy.

## Alcance y conteo

Linux primero mantiene la restriccion original: ABI del kernel/firmware/driver
o bibliotecas ya presentes, sin exigir aplicaciones externas de telemetria ni
instalar paquetes salvo ultimo recurso autorizado. Disponibilidad exacta por
campo; no prometer que todos los kernels/firmwares/drivers publiquen lo mismo.

75 tareas activas,3X excluidas. C57/E11/P5/B2:57/75=76,0% cerrado,
18/75=24,0% abierto. El cambio de prioridad NO altera estados/denominador ni
otorga credito parcial. No se crea un porcentaje Linux estimado incompatible
con ese control. Las tareas Windows siguen dentro del alcance final, aplazadas.

Ultima implementacion/pruebas revision62: Linux115/115, Windows136/136 por
compilador. Revision63 no ejecuta suites nuevas ni reinterpreta esos resultados
como avance de Linux. ClickUp solo refleja este checkpoint, no es la autoridad.

Checkpoint64: ciclo de vida privado SELF de F03 terminado y probado en Linux,
sin implementacion Windows nueva. Linux116/116, nativo privado positivo y
consumidor instalado exit0; detalles en LINUX_NETWORK_SESSION.md. F03 sigue
en curso por netns/join/multiactor/cobertura/ventanas/integracion. Conteo sin
cambio, proxima unidad continuar F03. No se reabre otro frente.

Checkpoint65: control de namespace del socket terminado para sesion SELF con
cookie declarado y rechazo sticky de mezcla. Linux116/116 y nativo WSL positivo,
sin setns/unshare ni trabajo Windows; LINUX_NETWORK_NAMESPACE.md conserva limites.
F03 sigue E por buckets dinamicos/multiactor/join/cobertura/ventanas/integracion;
conteo sin cambio. Siguiente continuar F03 identidad/ambitos del productor.

Checkpoint66: captura SELF conectada al reductor/tasas por sampler interno con
dominios iniciales del kernel tipados y scope de1fila/cookie. Linux117/117 y
WSL nativo positivo de4tasas/UID, idle0/fork/reinicio; LINUX_NETWORK_WINDOW.md.
F03 sigue E por multiactor/buckets/join/cobertura/integracion publica. Sin trabajo
Windows ni cambio de conteo. Siguiente F03 identidad procfs para el productor.

Checkpoint67: asociacion directa SELF kernel/procfs terminada, metodo separado
con doble identidad BPF fresca y contexto local estable por extremos. Linux119/119
y WSL PID390/kernel4965 positivo, oraculo de tasas/fork/reinicio/sin fugaFD;
LINUX_NETWORK_SELF_JOIN.md conserva limitaciones. Sin Windows ni cambio de conteo.
F03 E por multiactor/buckets/actores externos/cobertura/integracion publica.
Siguiente F03 captura/identidad de actores externos, pruebas privadas y permisos
acotados; no ampliar autorizacion previa a captura global del sistema.

Checkpoint68: modo privado start_selected con token kernel/cookie declarado y
tipo separado de SELF; hijo cooperativo privado WSL positivo/padre excluido/
identidad incorrecta sticky/pidfd salida/limpieza. Linux119/119, sin Windows;
LINUX_NETWORK_SELECTED.md registra que no autentica actores arbitrarios. F03 E,
sin cambio de conteo. Siguiente descubrimiento/revalidacion confiable de actores,
manteniendo pruebas privadas y permiso acotado, no captura general del sistema.

Checkpoint69: identidad kernel desde pidfd sin token reportado por el objetivo,
sonda temporal filtrada al hilo de la consulta de permisos FD-1/EBADF. WSL privado
positivo PID554/kernel1640, captura y negativos/limpieza/mask; Linux120/120.
LINUX_NETWORK_PIDFD.md documenta limitaciones, efecto temporal en mascara caller
y ausencia de ptrace attach/senales/parada objetivo. F03 E, sin cambio de conteo
ni Windows. Siguiente unir/revalidar pidfd con proceso procfs seleccionado/contexto.

Checkpoint70: asociacion seleccionada pidfd/procfs/boot/contexto por extremos
terminada. Dos tokens kernel frescos, proyeccion Pid en tabla FD del hilo caller
y PID/start_ticks/status/ns revalidados, dominios separados. WSL hijo privado
autorizado PID2513/kernel6353 positivo/negativos/sin fugaFD; worker CLONE_FILES
no privilegiado prueba tabla distinta del lider. Linux122/122 sin skips,
14tests red20repeticiones, consumidor instalado exit0/build sin warnings.
LINUX_NETWORK_PIDFD_JOIN.md documenta ausencia de atomicidad/ABA/vida futura/
cookie externo automatico/cobertura global. F03 E, conteo sin cambio, sin Windows.
Siguiente conectar actor asociado a ventanas/tasas con revalidacion/reset de
baseline. No abrir otro frente ni ampliar autorizacion a captura global.

Checkpoint71: ventanas seleccionadas con joins frescos extremos y dup pidfd
propio terminadas. Reset baseline ante fallo/excepcion/cambio, cobertura1actor/
cookie separada SELF/global y productor separado actor, epoca compartida/tasas
estimadas/suma UID parcial. WSL privado autorizado PID402/kernel913 positivo
4tasas/UID/caller FD cerrado/idle0/fork/reinicio/salida/sin fugaFD. Linux123/123
sin skips,15tests red20repeticiones, consumidor instalado exit0; primer relink
warnings make por timestamps DrvFS preservados, build repetido sin warnings.
LINUX_NETWORK_SELECTED_WINDOW.md mantiene limites de extremos/ABA/vida futura/
cookie declarado/plazo duro/misma tabla FD. F03 E, conteo sin cambio, sin Windows.
Siguiente cobertura/buckets y actores multiples con limites claros antes de
integracion publica; pruebas propias privadas, no ampliar captura global.

Checkpoint72: coleccion multiactor fija1..16 terminada, dedup local/kernel,
observer/boot comun y rollback de TODOS los baselines ante fallo/excepcion/
cambio. Ventanas independientes, no foto simultanea/suma UID comun. Fixtures
2actores/intervalos2s-3s/fallos segunda fila/recuperacion; WSL2hijos privados
positivo4oraculos por actor/trafico cruzado-padre excluidos/fork/salida/limpieza.
Linux124/124 sin skips,16tests red20repeticiones, consumidor instalado exit0,
build final sin warnings (indentacion inicial corregida/log preservado).
LINUX_NETWORK_MULTI.md conserva limites. F03 E, conteo sin cambio, sin Windows.
Siguiente ventana temporal comun acotada y agregacion UID, antes de cobertura/
buckets/integracion publica; pruebas privadas, no ampliar captura global.

Checkpoint73: ventana nominal comun y sumas UID parciales terminadas. Joins
todos antes/despues, barrido<=1s/commit posterior, seleccion1..16/dedup TGID-start,
brackets conservados/tasas estimadas/availability-overflow; no snapshot atomico.
Fallo/excepcion/cambio/modo reinicia todos baselines. Fixtures mismoUID/2UID/
12fallos/overflow/limites/modos y WSL2hijos privados4oraculos actor-UID positivos;
regresion selected/independent nativa tambien exit0. Linux124/124 sin skips,
16tests red20repeticiones/consumidor instalado exit0/build sin warnings, mismos
CTests ampliados. LINUX_NETWORK_COMMON.md conserva limites. F03 E, conteo sin
cambio, sin Windows. Siguiente contrato de cobertura y proveedor publico opt-in
seleccionado, sin totales falsos; pruebas privadas, no ampliar captura global.

Checkpoint74: proveedor publico seleccionado terminado, LinuxProcessNetworkSampler
en encabezado instalado, Pimpl movable/start opt-in/sample/stop y reporte con
cobertura tipada/estimated/IDs locales-kernel separados/UID inicial parcial/
availability/brackets. Proyeccion pura/34negativos y WSL API PUBLICA2hijos positivos
4oraculos actor-UID/move/fork/FDs caller cerrados/limpieza. Linux125/125 sin skips,
17tests red20veces/consumidor instalado nuevo encabezado exit0/build sin warnings.
API ADITIVA/layouts-ABI existentes intactos, no autoMonitor/totales legacy ni
Windows nuevo/GUI/historial/dependencia. LINUX_PROCESS_NETWORK.md conserva limites.
F03 E, conteo sin cambio. Siguiente integracion opt-in de informe seleccionado
en flujo general Monitor/politica de cobertura-buckets, sin totales falsos;
pruebas privadas, no ampliar autorizacion a captura global.

Checkpoint75 (2026-10-22): informe seleccionado integrado Monitor/Sampler, flag
false/setup autorizado aparte/opt-out-stop-worker cierran/restart no reautoriza.
WSL2hijos4oraculos/ciclo worker/move/fork/rearmado/limpieza positivos; Linux126/126
sin skips/18tests red20veces/consumidor instalado final exit0/build verificado sin
warnings. Cambio layout0.x SystemSnapshot/SampleOptions: recompilar TODOS.
MSVC2/2 focalizadas pasa, consumidor general0xC0000409 tras checks nuevos sin
causa demostrada: riesgo N03 pendiente, no compatibilidad total ni frente Windows
nuevo. LINUX_PROCESS_NETWORK_MONITOR.md conserva detalles. F03 E, conteo sin
cambio, por seleccion dinamica/buckets namespace/cobertura general. Siguiente
eliminar alcance manual fijo sin totales falsos; pruebas privadas, no captura global.

Checkpoint76 (2026-10-22): update explicito proveedor publico/Monitor, AB->B->AB/
reorden conservando capturas coincidentes, retirando antes de crear nuevas,
fresh consent/epoch y reset TODOS baselines. Runtime failure cierra TODOS,
Monitor tambien lista autorizada invalida. Contadores retenidos conservan origen,
nuevos no cuentan trafico previo. WSL2hijos4oraculos/gap/fork/salida-recuperacion/
sin fugaFD; Linux126/126 sin skips,2tests20veces/consumidor instalado exit0/build
sin warnings. API aditiva sin layouts nuevos. LINUX_PROCESS_NETWORK_UPDATE.md.
F03 sigue E: update worker/politica-descubrimiento/buckets/cobertura general;
conteo sin cambio, H04/J04 abiertas. Siguiente update seguro del worker, no otro
frente ni captura global. Fallo consumidor MSVC75 sigue N03/no total validado.

Checkpoint77 (2026-10-22): Sampler update con worker activo/captura existente,
consentimiento nuevo, serializacion coleccion-commit-update-stop y prioridad para
update pendiente/wakeup protegido. Preserva historial/epochs/otras metricas,
update no rearma/fallo cierra captura no worker. WSL2hijos privados AB->B->AB con
worker/updater otro hilo/4oraculos actor-UID/gap/baseline/origen/fallo-restart/
limpieza positivos. Primer nativo fallo en retirada, corregida posible inanicion,
repeticion y final positivas/log preservado. Linux126/126 sin skips,3tests20veces,
consumidor instalado exit0/build sin warnings, MSVC3/3 focalizadas sin nuevos
warnings C++; fallo consumidor75 sigue N03/no total validado. API aditiva/sin
layouts nuevos. LINUX_PROCESS_NETWORK_WORKER.md. F03 E por politica-descubrimiento/
buckets/cobertura general, H04/J04 abiertas; conteo sin cambio. Siguiente buckets/
seleccion general acotados, sin captura global ni otro frente.

Checkpoint78 (2026-10-22): proveedor publico buckets declarados por namespace,
constructor opt-in/16filas proceso-cookie/varias por proceso/default estricto
intacto, exclusiones antes de mapa/calidad sticky/dedup-contexto-UID coherentes/
coverage nuevo/suma parcial sin duplicado. Update epoch-reset TODOS baselines/
retencion-retiro-readd/gap limpio. Fixtures2cookies positivos4lanes/negativos,
WSL1hijo2cookies declarados/SOLO1netns real/otro cookie no observado:4oraculos/
otro0/suma sin duplicado/gap/fork/FD limpios. NO2netns reales simultaneos; regresion
estricta2hijos positiva. Linux126/126 sin skips/5tests20veces/consumidor instalado
exit0/build sin warnings, MSVC5/5 focalizadas sin C++ warnings nuevos/D9025 assert
esperado. Fallo consumidor75 sigue N03/no total validado. Error protocolo fixture
overflow corregido/log preservado. API aditiva/enum ampliado/sin layouts nuevos.
LINUX_PROCESS_NETWORK_BUCKETS.md. F03 E por modo buckets Monitor/Sampler/politica-
descubrimiento/buckets dinamicos/cobertura general, H04/J04 abiertas; conteo sin
cambio. Siguiente integracion opt-in de buckets, solo pruebas privadas acotadas.

Checkpoint79 (2026-10-22): buckets declarados integrados Monitor/Sampler mediante
setup3args modo/consentimiento;2args siempre estricto tras stop, flag/restart no
autorizan/activo no cambia modo. Sampler detenido+flag/update conserva modo.
Fixtures y WSL Monitor/worker4oraculos bucket-UID/retiro-readd/gap/historial/update
concurrente/baselines/default estricto/limpieza positivos. SOLO1netns real/segunda
cookie NO observada, no2netns reales simultaneos ni descubrimiento. Linux126/126
sin skips,3tests20veces/consumidor instalado exit0/build sin warnings; MSVC3/3
focalizadas sin warnings C++ nuevos/D9025 assert esperado, consumidor75 sigue N03/
no total validado. API aditiva/sin layouts ni opciones nuevas. F03 E por politica-
descubrimiento/buckets dinamicos/cobertura general, H04/J04 abiertas; conteo sin
cambio. LINUX_PROCESS_NETWORK_BUCKET_MONITOR.md. Siguiente buckets dinamicos/
seleccion general, solo pruebas privadas acotadas, no captura global ni otro frente.

Checkpoint80 (2026-10-22): motor PRIVADO observado sin cookie entrada/token
kernel actor;ARRAY105/16slots/CMPXCHG64 sin eviccion/calidad sticky global/lectura
integral8reintentos/validador/vacio-detach-stop-reinicio. Fixtures16cookies/4lanes/
peer igual-distinto/capacidad/calidad/rollback y WSL1hijo/sin cookie productor/
oracle SO/TCP12288/12288 UDP768/768 peek24/2threads/otros excluidos/detach/restart
vacio luego TCP8192/8192 UDP512/512 peek16/FD limpios. SOLO1netns real;16cookies/
race distinto/capacidad fixtures, oracle inicializa cookie. Regresion declarada
Monitor positiva. Linux126/126 sin skips,2tests20veces/consumidor instalado exit0/
build sin warnings, MSVC2/2 focalizadas/sin C++ warnings nuevos/D9025 assert
esperado, consumidor75 sigue N03/no total. Macro assert fixture corregido/log
preservado. Sin API/layout publico nuevo. LINUX_NETWORK_DISCOVERY.md. F03 E por
join/contexto/ventanas-tasas/multiactor/proveedor publico/politica general;
H04/J04 abiertas; conteo sin cambio. Siguiente integrar al nivel seleccionado
con join/reset/ventanas, solo pruebas privadas acotadas, sin captura global.

Checkpoint81 (2026-10-22): ventanas INTERNAS seleccionadas observadas con pidfd
propio/join por extremos/boot-contexto/guard fork/bracket1s; sin cookie entrada,
vacio conocido/primer bucket sin tasa,4tasas/UID parcial, A->A+B. Error/contexto/
discard reset baselines sin ocultar regresion registro-contadores; stop/restart
mapa vacio/nuevo epoch/modos separados. Fixtures16cookies/solo exclusiones/scope
0-17-multiactor/8fallos inicio/21lectura/recuperacion. WSL1hijo/pidfd original
cerrado/TCP12288/12288 UDP768/768 peek24/2threads/4tasas-UID oracle/otros excluidos/
fork-padre intacto/discard/restart/exited2veces/FD limpios. SOLO1netns real/oracle
SO inicializa cookie/16cookies fixtures. Estricto nativo positivo. Linux126/126
sin skips,2tests20veces/consumidor instalado exit0/sin warnings; MSVC2/2 focalizadas
sin nuevos C++ warnings/D9025 esperado, consumidor75 sigue N03/no total.
Sin API/layout publico/GUI/persistencia/dependencia/CLI/captura global nuevos.
F03/H04/J04 abiertas, C57/E11/P5/B2 sin cambio. LINUX_NETWORK_DISCOVERED_WINDOWS.md.
Siguiente multiactor con bracket comun/join/reset grupal antes de API/Monitor/
Sampler observados, solo pruebas privadas acotadas; no abrir otro frente.

Checkpoint82 (2026-10-22): ventana comun INTERNA observada1..16actores/16cookies
cada uno/scope0..256filas, seleccion cookie0/modo fijo/joins TODOS por extremos/
bracket1s/guard fork/registro identidad aun vacio/primer cookie sin tasa/4tasas-UID
parcial. Fallo cualquiera reset TODOS/no parcial/commit registro final. Updates
retienen origen/retiro-readd vacio sin gap/epoch/reset grupal/runtime fallo cierra
todo. Fixtures32buckets/2UID/256filas reducer/limites/20fallos lectura/5inicio/
adicion fallida. WSL2hijos/pidfds originales cerrados/no cookies entrada/vacio-
tardio/4tasas-UID oracle/A TCP12288/12288 UDP768/768 peek24 y B doble/2threads por
actor/padre excluido/fork/retiro-readd-gap/exit rechaza grupo2veces/remover recupera/
FD limpios. SOLO1netns real compartido/oracle SO inicializa cookies/32-256fixtures/
no16actores reales. Regresiones comun estricta/individual observada positivas.
Linux126/126 sin skips/3tests20veces/consumidor instalado exit0/sin warnings;
MSVC3/3 focalizadas/sin nuevos C++ warnings/D9025 esperado, consumidor75 sigue N03/
no total. Sin API/layout publico/GUI/persistencia/dependencia/CLI/captura global
nuevos. F03/H04/J04 abiertas, conteo igual. LINUX_NETWORK_OBSERVED_MULTI.md.
Siguiente proveedor publico seleccionado observado con identidades/registros
vacios/calidad/brackets/limites explicitos, despues Monitor/Sampler/politica general.
Solo pruebas privadas acotadas, no otro frente ni captura global.

Checkpoint83 (2026-10-22): proveedor PUBLICO seleccionado observado, opt-in/
default estricto/1..16actores-cookie0/16cookies cada uno/256filas; coverage nuevo/
observed_processes identidades aun vacios/primer bucket sin tasa/seq actor
repetido no sumar/proyeccion integral sin parcial. Layout0.x snapshot/SystemSnapshot
nuevo: recompilar TODOS. Monitor/Sampler aun rechazado sin proveedor/siguiente.
Fixtures vacio-mixto/multicookie/23invalidos/256proyeccion/consumer tipos nuevos.
WSL PUBLICO2hijos/pidfds cerrados/move/vacio-tardio/4tasas-UID oracle/A TCP12288/12288
UDP768/768 peek24 y B doble/2threads por actor/padre excluido/fork/update/retiro-
readd-gap/epochs/exited2veces/remover recupera/stop-restart modo-vacio/FD limpios.
SOLO1netns real/oracle SO inicializa cookies,256fixtures. Estricto nativo positivo.
Linux126/126 sin skips/3tests20veces/consumidor instalado exit0/sin warnings;
MSVC3/3 focalizadas/sin nuevos C++ warnings/D9025 esperado, consumidor75 sigue N03/
no total Windows. Sin GUI/persistencia/dependencia/CLI/captura global nuevos.
F03/H04/J04 abiertas, conteo sin cambio. LINUX_PROCESS_NETWORK_OBSERVED.md.
Siguiente integracion opt-in Monitor/Sampler, despues politica de seleccion general;
solo pruebas privadas acotadas, no captura global ni otro frente.

Checkpoint84 (2026-10-22): observado integrado Monitor/Sampler setup3args opt-in,
2args siempre estricto/callback viejo rechazado. Activo-running no cambia modo/
Sampler detenido+flag; update vivo conserva modo/metadata/historial-epochs/
retained origin/reset TODOS/nuevo sin gap. Optout-stop cierran/flag-restart no
rearman/fallo update captura cerrada worker sigue. Fixtures ambos modos/calidad/
excepciones/callback viejo/metadata vacia/default/flag/running/consumer. WSL
Monitor PUBLICO2hijos/pidfds cerrados/move/vacio-tardio/4tasas-UID oracle/A
TCP12288/12288 UDP768/768 peek24 y B doble/2threads por actor/padre excluido/
fork/update-retiro-readd-gap/exited2veces/stop-restart/default/optout/FD limpios;
worker2actores/cookie0/vacios->buckets/4tasas-UID historial/AB->B->AB/updater
concurrente-historial/epochs-reset/retained origin/nuevo sin gap/fallo update
worker sigue/default/restart desarmado/FD limpios. SOLO1netns real/oracle SO
inicializa cookies/no general. Regresion Monitor declarado positiva. Linux126/126
sin skips/3tests20veces/consumidor instalado exit0/sin warnings; MSVC3/3
focalizadas/sin nuevos C++ warnings/D9025 esperado, consumidor75 sigue N03/no total.
Sin API/layout nuevo sobre83/GUI/persistencia/dependencia/CLI/captura global nuevos.
F03/H04/J04 abiertas/conteo igual. LINUX_PROCESS_NETWORK_OBSERVED_MONITOR.md.
Siguiente politica seleccion-cobertura general de procesos con identidad/retiros/
limites; solo pruebas privadas acotadas, pedir autoridad si se necesita ampliar
captura. No iniciar otro frente ni captura global.

Checkpoint85 (2026-10-22): owner PUBLICO readonly PID/inicio,0..16candidatos
declarados completos/consentimiento/shape-capacidad17 rechaza sin fuente ni
truncar/rotar. Adquiere-retiene-retira-reemplaza/reorden sin generation nueva/
TODOS por extremos/contexto/poll-fdinfo-stat/RAII/fork-move/vacio. Runtime fallo
limpia propios; NO captura/enumeracion/auto-update. Prestamos cookie0/downstream
duplica; clear/fallo owner NO detiene captura independiente (consumidor stop/update).
Fixtures16positivos/17rechazo/13fallos runtime-calidad-contextos-excepciones/
retencion-cambios/FD limpios; WSL readonly2hijos/move-fork/stale-start/exit-vacio y
autorizado owner->PUBLICO BPF2hijos/fd originales cerrados/clear owner captura
sigue/4tasas-UID oracle/A TCP12288/12288 UDP768/768 peek24 y B doble/2threads por
actor/gap-exit-restart/FD limpios. SOLO1netns real/oracle SO inicializa cookies/
16positivos fixtures/no general. Linux127/127 sin skips/2tests20veces/consumidor
instalado exit0/build final sin warnings/3warnings indentacion inicial corregidos-
log. MSVC2/2 focalizadas/sin nuevos C++ warnings/D9025 esperado, consumidor75
sigue N03/no total. Header/tipos/clase aditivos sin layout existente/GUI/
persistencia/dependencia/CLI/captura global nuevos. LINUX_NETWORK_SELECTION_OWNER.md.
F03/H04/J04 abiertas/conteo igual. Siguiente inventario con cobertura/admision
automatica y arquitectura que escale MAS DE16 sin ocultar perdidas/capacidad;
solo pruebas privadas acotadas, no captura general sin ampliar autoridad.

Checkpoint86 (2026-10-22): inventario PUBLICO readonly/procfs fijado-EOF/max100000/
contexto-boot-observer/bracket5s cooperativo, TODOS PID encontrados con Metric y
motivos verified-denied-disappeared-unavailable/UID observador/no0inventado, sin
parcial por fallo global/no censo host. Puente exige TODOS verificados/max16,
sin filtrar/truncar/rotar. Fixtures UID0-vacio/16-17/mixtos/3estados fallo/21fallos
fuente/15frames alterados. WSL readonly /proc real/2hijos IPC verificados/al owner
como subconjunto PRIVADO explicito/puente TODOS o ausencia/FD limpios; denegado-
salido positivos fixtures/no hidepid-exit race real. Sin BPF/root/trafico capturado.
Linux128/128 sin skips/2tests20veces/consumidor instalado exit0/build final sin
warnings/1warning inicial corregido-log/notas stat aclaradas. MSVC2/2 focalizadas/
sin nuevos C++ warnings/D9025 esperado, consumidor75 sigue N03/no total.
Header/tipos/funciones/test aditivos sin layout existente/GUI/persistencia/
dependencia/CLI/captura global nuevos. LINUX_NETWORK_INVENTORY.md.
F03/H04/J04 abiertas/conteo igual. Siguiente arquitectura de captura MAS DE16
con capacidad-calidad explicitas antes de admision general automatica; pruebas
de trafico solo actores privados autorizados, pedir autoridad si se amplia captura.

Checkpoint87 (2026-10-22): Revision87: F03 E->E, capacidad ESTRUCTURAL64/BTF por sesion y coste medido.
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

Checkpoint88 (2026-10-22): Revision88: F03 E->E, consulta pidfd de sesion con compuerta terminada.
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

## Checkpoint99: H04 P->E, LinuxHistoryStore terminado, 2026-10-24

Persistencia LinuxAppHistoryEntry/value-Support-nota/formato separado lista;
merge cumulativo sin sumar replays, periodos unicos ms, rechazo descenso/perdida
conocida, corrupcion/bounds/formatos ajenos. Save es reemplazo, no merge implicito.
Retencion por periodos enteros/reset unlink sin directorios, sin I/O automatico.
Linux134/134+30focales/consumidor instalado exit0, CPU REAL roundtrip con campos
de red/notificaciones aun ausentes. MSVC20focales fuera sandbox PASS; dentro
AccessDenied5 de reemplazo temporal en nuevo/antiguo, logs preservados.
Fixtures iniciales corregidas/assercciones activas/N03 no resuelto. Sin layouts
existentes/ABI/CPU/Windows nuevos ni captura eBPF. LINUX_HISTORY_STORE.md/logs
work/linux-history-store-*. H04 sigue abierta: atribucion/ventanas exe antes y
despues, conectar F03, positivo nativo y roundtrip de red REAL. No mezclar
periodos independientes por misma ruta ni recuperar gaps o sumar frames absolutos.
C58/E11/P4/B2,75activas/3X,77,3%/22,7%,sin credito parcial. Siguiente H04.
Espejo99 confirmado: comentario90140272938633; TXT manda.

## Checkpoint98: F03 aceptada, vista real y SELF, 2026-10-24

F03 E->C: criterio previo de proveedor opcional nativo/atribucion/ventanas/soporte
declarado satisfecho. SELF corrigio caso PID==caller que no emite hook externo:
identidad local completa+pidfd probado+2lecturas kernel nuevas, no bypass de
permisos externos. REAL /proc WSL30/30 strict/0excluded/tres snapshots pareados/
UDP delta97/tasas/FDs limpios PASS; permiso temporal confirmado/ensayos<10s,
sondas retiradas, no permanente. Linux133/133+30focales/MSVC10join/consumidor
instalado exit0, privado17 TCP-UDP/IPv4-IPv6/updates/gap/fork regresion positiva.
Fallos EINVAL/actor no vivo preservados; churn sigue falla-cerrado y exige
refresh autorizado; no total universal ni cierre de N02/N03/H04/J04.
LINUX_NETWORK_ACCEPTANCE.md; work/network-visible-*. C58/E10/P5/B2,
75activas/3X,77,3%/22,7%. SIGUIENTE H04, leer HISTORY.md antes de modificar
semantica/persistencia. Recheck build Linux exit0 sin avisos tras clock-skew generado.
Espejo98 confirmado: comentario90140272937903; TXT manda.

## Checkpoint97: capacidad1024/lotes y nativo65, 2026-10-24

Unidad de escala terminada:1024actores/16384buckets estructurales, RAW64/lote,
inicio/altas en lotes disjuntos/retencion sin reinicio/readmision sin gap.
Limite comun owner/inventario/politica/controlador/proyeccion/Monitor/worker;
>1024elegibles rechazaTODO. No soporte ilimitado/1Hz/cambio de RLIMIT ni captura
automatica. FDs N+6+8G, owner adicionalN; G hastaN por fragmentacion tras churn.
Recompilar TODOS por constexpr, no layouts nuevos. Fixtures65/129/1024/16384
buckets/fallos lotes2y16; nativo65 SOLOprivados PUBLICO+admision restringida,
censos87/152FDs/UDP/tasas/UID/unchangedrefresh/limpieza y regresion17 pasan.
Linux133/133+70focales, MSVC70focales/consumidor instalado exit0. Primer consumer
assert esperaba1024buckets viejo, actualizado16384/log preservado/checks activos.
Sin nuevos warnings C++/D9025 esperado/N03 sigue abierto. LINUX_NETWORK_SCALE.md,
work/network-scale-*. Siguiente F03: validar cobertura/vista real bajo permiso
apropiado. Autorizacion temporal WSL solicitada, NO recibida en esta revision,
no capturar procesos ajenos usando el permiso privado anterior. H04/J04 despues.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,sin credito parcial.
Espejo97 confirmado: comentario90140272937319; historicos remotos intactos/TXT manda.

## Checkpoint96: admision/procedencia Monitor/Sampler, 2026-10-24

Unidad terminada: refresh Monitor/start-refresh Sampler con consentimiento
CADAvez/defaultstrict/modos exclusivos. SystemSnapshot metadata ultimo refresh
pareada con red/epoch/exclusiones, no censo nuevo/hostglobal. Fallo cierra ambos,
worker continua, flag/restart no rearma. Refresh concurrente/history, retencion/
readmision sin gap, move y optout verificados. Layout SystemSnapshot0.x ampliado,
RECOMPILAR TODOS. Linux133/133+100focales/consumidor instalado exit0; MSVC40focales.
Nativo SOLO2privados+marcador sintetico/enum restringido por objeto/procfs-pidfd-BPF
publicos pasa, NO vista /proc completa autorizada ni validada. Primer assertion
MSVC preservado/causa no aislada, rebuild y repeats pasan sin relajar asserts;
N03 consumidor75 abierto. Avisos clock-skew WSL temporales registrados;
recompilacion de comprobacion exit0 sin avisos.
Docs LINUX_NETWORK_ADMISSION_MONITOR.md/logs work/network-admission-monitor-*.
Siguiente escala>64elegibles/cobertura/validacion real bajo autorizacion apropiada;
H04/J04 conservan criterios y siguen despues. C57/E11/P5/B2,75activas/3X,
76,0%/24,0%,sin cierre/ID/credito parcial.
Espejo96 confirmado: comentario90140272936245; historicos remotos intactos/TXT manda.

## Checkpoint95: politica parcial opt-in/procedencia, 2026-10-22

F03 E->E. Plan puro valida TODO inventario, candidatos readiness separado de
calidad all_encountered_verified y consentimiento. strict_all default CADArefresh,
verified_only explicito excluye SOLO filas noverificadas bienformadas con PID/
estado/motivo. Malformed/estimado/incompleto no se oculta. >64elegibles rechaza
TODO/no first64/truncamiento/rotacion. Change aporta conteos/politica/excluidos/
complete_encountered_set/epoch/tiempos; sample_with_admission une ULTIMO refresh
y red/mismoepoch, no nuevo censo/hostglobal. Raw sample notaPARTIAL/no filas/
tasas/UID inventados; todos excluidos visible>0/capturaoff/no visiblevacio-red0.
Mismo PID/start conserva epoch/tasas aunque cambien motivos/politica, procedencia
actualizada. Runtime owner/BPF/calidad/contextos falla cierraTODO/no retry filtrando.
LayoutAdmissionChange0.x ampliado, RECOMPILAR biblioteca/consumidores; no cambio
SystemSnapshot/snapshotred. Fixtures64/65/67con3excluidos/todos/20corrupciones/
controlador/fallos-excepciones pasan. Nativo SOLO2privados+marcador sin PIDajeno:
sobre visible3/verified2/excluded1/solo2actores, epoch-baseline26FDs/gap-origen/
strictdefault posterior cierraTODO/fork/restart/vacio/limpieza positivos. NO global.
Linux132/132 sin skips+100focales/consumidor instalado nuevoAPI exit0; MSVC40focales,
sin nuevos warnings C++/D9025 esperado/N03 no resuelto. Primer missinginitializer
corregido/log preservado. Docs LINUX_NETWORK_ADMISSION_POLICY.md/logs network-policy.
Siguiente integrar admision/procedencia Monitor/Sampler, escala>64elegibles y
validacion vista real bajo autorizacion apropiada; H04/J04 conservan criterios.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,sin cierre/ID/credito parcial.
Espejo95 confirmado: comentario90140272317289; lista leida/historicos intactos/TXT manda.

## Checkpoint94: admision estricta standalone, 2026-10-22

F03 E->E. Nueva LinuxVisibleProcessNetworkSampler/refresh_visible(bool)/sample/
stop/move, conecta inventario-owner-proveedor PUBLICO observado compartido.
Consiente CADArefresh; construccion no enumera/captura, sample no enumera/rearma,
sin polling/hilo. ALLverificados<=64 o rechazoTOTAL sin filtro/truncamiento/
rotacion ocultos; vacio detiene epoch0, no ausencia host. Refresh autorizado
fallido o sampleerror detieneTODO, no seleccion vieja/parcial. Contexto actual
boot/procfs/ns/caller antes/despues, inventario+commit<=5s cooperativo, noharddeadline.
Conjunto PID/start igual/reordenado conserva tasas/epoch sin update/sample
oculto; cambios/PIDreuse usan update compartido, origen retenido/gap excluido.
Owner extraNFDs,2N+6+8G persistentes. Fixtures64/65/unknown/stale/contextos/
PIDreuse/fallo-excepcion CADAoperacion y sample/fork pasan.
Nativo SOLO2hijos privados con enumerador restringido/procfs-pidfd-BPF nativos:
AB->unchangedAB->B->AB, tasas/epoch intactos/gap-origen/censos18->16->26FDs/
fork padre intacto/desconocido cierraTODO/restart fresco/vacio/limpieza pasan.
NO ejecutar refresh_visible(true) sobre sesion completa; permiso BPF previo
SOLOprivados, no ampliar captura. Vista completa no validada positivamente.
Final Linux131/131+80focales/consumidor nuevoheader-API instalado exit0;
MSVC nuevafixture20veces, sin nuevos warnings C++/D9025 esperado/N03 no resuelto.
Target sin regenerar CMake/skew0,75s WSL inicial preservados, build finalpositivo.
Docs LINUX_NETWORK_ADMISSION.md/logs work/network-admission-*.
Siguiente politica practica de vistas grandes/inaccesibles/cobertura, validacion
real autorizada e integracion admision Monitor/Sampler, H04/J04 despues.
C57/E11/P5/B2,75activas/3X,76,0%/24,0%,sin cierre/ID/credito parcial.
Espejo94 confirmado: comentario90140272312863; lista leida/historicos intactos/TXT manda.

## Checkpoint93: backend publico compartido observado, 2026-10-22

F03 E->E, rama PUBLICA observed_namespace_buckets -> compartido autenticado.
Default estricto y declarado conservan proveedor anterior. Stop ambas ramas,
move transfiere Pimpl/modo, misma proyeccion valida; sin firma/layout/API/
dependencia/config nuevos, ni captura al construir. Monitor/SystemSnapshot,
Sampler-worker y SelectionOwner conectados con las rutas publicas existentes.
Nativo PUBLICO17/censo N+14/tasas-UID/empty metadata/owner clear/padre excluido/
limpieza pasa. Standalone/Monitor/owner AB->B->AB censos16->15->24FDs propios/
gap excluido/origen retenido/epoch-tasas/move/fork/exit-recovery/stop-restart,
optout/flags no rearman positivos. Worker update concurrente/history/epoch/
ausencia/FD final pasa; estricto PUBLICO y declarado nativos pasan.
Final93 Linux130/130 sin skips+60focales/consumidor instalado exit0;
MSVC3/3+60focales pasan/sin nuevos warnings C++/D9025 esperado. Builds iniciales colisionaron
por error del asistente en mismo directorio/ranlib truncado-ausente; logs
preservados/reconstruccion serializada. No fallo de codigo ni arreglo de N03.
Docs LINUX_NETWORK_SHARED_PUBLIC.md/logs work/network-shared-public-*.
Siguiente politica/admision general de procesos visibles, no truncar>64 ni
mezclar identidad/unknown, luego H04/J04. Sigue sin captura global autorizada.
F03/H04/J04 abiertas,C57/E11/P5/B2,75activas/3X,76,0%/24,0%,sin credito parcial.
Espejo93 confirmado: comentario90140272305719; lista leida/historicos intactos/TXT manda.

## Checkpoint92: update compartido autenticado, 2026-10-22

F03 E->E, LinuxSharedSelectedNetworkSampler::update_selection terminado interno.
Consentimiento cada vez/forma1..64sinmutacion/joins TODOS antes-despues/kernel
unicos/boot-observer; runtime failure/exception cierra TODO/nuevos pidfds RAII.
LinuxSharedNetworkSet por lotes disjuntos/no slots reciclados/retira antes de
altas: retenidos sin reinicio/origen intacto, altas-readmisiones frescas sin gap.
Epoch/reset TODASlasbaselines aun reordenacion. Lotes vacios cierran,1<=G<=N<=64,
coste nativo N+6+8G FDs; no3sondas constantes tras churn ni corte atomico entre
lotes. Barrido total<=1s y coherencia porlote/calidad sticky preservados.
Nativo17/restart16/update16->15->16 con retirado aun enviando; oraculos TCP/UDP4/6,
cuatro tasas/suma UID/reorder/+8FDs exactos para lote nuevo/hijos-FDs limpios.
Fixtures80updates/64lotes-1024buckets/65rechazado/64->1/todasoperaciones fallos y
excepciones/quality/span pasan. Linux130/130+40focales/consumidor instalado exit0,
MSVC40focales pasan/sin nuevos warnings C++/D9025 esperado/N03 no resuelto.
Docs LINUX_NETWORK_SHARED_UPDATES.md/logs work/network-shared-update-*.
Siguiente activar/verificar backend PUBLICO observado/Monitor/Sampler/owner;
admision automatica pendiente. Proyeccion interna positiva NO es activacion.
F03/H04/J04 abiertas,C57/E11/P5/B2,75activas/3X,76,0%/24,0%,sin credito parcial.
Espejo92 confirmado: comentario90140272299452; lista leida/historicos intactos/TXT manda.

## Checkpoint91: retirada segura compartida, 2026-10-22

F03 E->E. LinuxSharedNetworkSession::retain solo subconjuntos exactos activos.
Retira HASH/no limpia ni recicla ARRAY/no reinicia sondas; una invocacion ya
admitida solo termina en registro retirado, no publicado. Writer/sequence global
incluye retirados; calidad GLOBAL sticky sigue mandando. Checkpoint retenido
por identidad preservado. Fallo/exception de borrado cierra todo; vacio cierra;
forma/consentimiento invalidos intactos; fork sin mutacion al mapa del padre.
Fixtures64->63/lleno/evento suspendido/regresion/fallos1y2 pasan. Nativo17->16->1,
todos siguen enviando, activos mantienen origen TCP/UDP/8FDs, retirados excluidos;
reinicio/hijos/FDs limpios. Linux130/130 sin skips, MSVC2/2 veinte veces cada una.
Docs LINUX_NETWORK_SHARED_RETENTION.md/logs work/network-shared-retain-*.
No equivale a update autenticado: faltan altas/readmision/epoch-reset tasas y
composicion sin reiniciar retenidos, luego backend publico y admision automatica.
F03/H04/J04 abiertas, C57/E11/P5/B2,75activas/3X,76,0%/24,0%;sin credito parcial.
Final Linux130/130+40focales/consumidor instalado exit0; MSVC40focales.
Espejo91 confirmado: comentario90140272294875; lista leida/historicos intactos/TXT manda.

Checkpoint89 (2026-10-22): Revision89: F03 E->E, productor COMPARTIDO INTERNO de seleccion fija terminado.
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

Checkpoint90 (2026-10-22): Revision90: F03 E->E, ventanas COMPARTIDAS autenticadas de seleccion FIJA terminadas.
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
