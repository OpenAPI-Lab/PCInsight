# Ventanas de trafico del actor seleccionado

Revision71: `LinuxSelectedNetworkSampler` conecta captura seleccionada,
[join pidfd/procfs](LINUX_NETWORK_PIDFD_JOIN.md) y reductor. Es **interno**, no
instalado/activado en Monitor. Sin API/ABI/historial/GUI/Windows nuevos. F03 E.

## Propiedad y transaccion

`start(authorized,pidfd,pid,start_ticks,socket_cookie)` exige opt-in, argumentos
validos y APIs completas. Retiene un duplicado CLOEXEC propio del pidfd: despues
de start el caller puede cerrar el descriptor de entrada. Obtiene un join,
descubre identidad kernel del productor con TEST_RUN local e inicia la captura
seleccionada. La epoca usa el mismo serial atomico sin wrap que SELF, dentro de
la misma copia enlazada; no UUID global entre DSOs. Error/excepcion revierte
captura y pidfd. stop/destructor liberan ambos y descartan referencias temporales.
Los contextos inyectados son prestados y deben vivir mas que el sampler.

Llamadas serializadas, misma tabla de FD entre llamadas (hilos con tabla
compartida admitidos). No unshare de tabla o reemplazo/cierre asincrono de FDs
internos. El guard getpid solo detecta fork, NO traduce TGID inicial. La copia
hija cierra sus referencias heredadas antes de hacer join o lectura de captura.

Cada muestra valida un join fresco antes y otro despues de los contadores:
PID/start esperado, boot del scope, observador correcto e identidad kernel fija.
Cada join realiza dos descubrimientos pidfd: cuatro por muestra, sin cache de
autenticidad. El contexto completo debe coincidir dentro de la muestra. Un
cambio local entre llamadas descarta la referencia temporal antes de muestrear;
boot/identidad kernel diferentes se rechazan, requieren reiniciar el sampler.

La lectura de contadores exige estado activo, tipo selected_actor, token/cookie
correctos, calidad coherente y bracket CLOCK_MONOTONIC positivo/ordenado/no
superpuesto/no mayor de1s. El bracket cubre la lectura de contadores, NO todo
el coste de joins ni un plazo duro de syscall. Solo tras las comprobaciones
posteriores se entrega una fila al reductor y se conserva contexto/timestamp.

Fallo/ausencia/permiso/cambio/excepcion elimina la referencia completa. Al
recuperarse, primera muestra valida sin tasa, segunda puede producirla. Una
ventana mayor de60s tampoco sirve para tasas. Este sampler no borra fallos
sticky/overflow de captura: resolver/reiniciar esa captura sigue siendo necesario.

## Semantica y cobertura

`PrivateSelectedNetworkWindow` conserva contexto local y `PrivateNetworkWindow`:
cuatro lanes TCP send/receive y UDP send/receive, calidad, bracket y contabilidad.
Tasas floor(delta_bytes*1e9/delta_ns), aritmetica entera comprobada. Timestamps
de final de lectura, tasas Support::estimated: no throughput instantaneo exacto.
No se garantiza frecuencia/latencia de muestreo ni rendimiento de produccion.

Cobertura tipada `selected_single_socket_namespace`, distinta de SELF y general:
exactamente una fila kernel valida, productor separado del actor objetivo.
enumeration_complete solo cubre ese bucket declarado, NO el sistema/proceso
completo. UID inicial de operacion separado de UID procfs del observador; suma
UID parcial, no conversion de IDs ni ticks/ns. No nuevos totales ProcessInfo o
UserSession ni integracion de historial de apps/usuarios.

Hereda limites del join: BTF/FEXIT/ptrace/privilegios compatibles, mascara caller
bloqueada temporalmente/restaurada (puede retrasar handlers), no cancelacion
asincrona ni plazo duro. Sin ptrace attach/senal/parada objetivo ni comandos de
captura externos/instalacion/configuracion permanente. Extremos no atomicos ante
ABA de credenciales/namespaces; no vida futura ni retencion de namespaces.

Cookie aun declarado, no discovery automatico; sin multiactor/buckets dinamicos,
cobertura total de rutas/asincrono/offload/wire bytes ni identidad comercial.
Siguiente F03: cobertura/buckets/multiactor antes de publicacion. H04/J04 abiertas.
Pruebas autorizadas propias/privadas, nunca captura general del sistema.
Revision72 agrega una [coleccion multiactor fija](LINUX_NETWORK_MULTI.md) con
intervalos independientes y rollback de todos los baselines ante un fallo;
no resuelve todavia la ventana comun/agregacion UID ni los cookies dinamicos.

## Evidencia

Fixtures: FD retenido0, dominios distintos productor/objetivo/local, cuatro
tasas y UID contra oraculo, idle0, cambio de contexto sin cruzar intervalo,
reinicio/epoca/fork. Ocho fallos de inicio/excepciones con rollback y15fallos de
muestra (join/soporte/contexto/kernel/boot/captura/calidad/reloj/excepciones),
referencia descartada y recuperacion sin tasa sobre hueco; intervalo mayor60s.
Reductor rechaza cero/dos filas, inicio0 y TGID fuera de rango firmado. Ningun
CTest carga BPF real.

Modo manual `--authorized-private-selected-window`, NO CTest: hijo privado sin
comunicar token kernel, start leido por padre, dup propio tras cerrar caller FD,
primera muestra sin tasa, cuatro lanes y UID contra oraculo por intervalo real,
padre excluido/idle0, fork rechazado sin afectar padre, reinicio epoca/referencia,
salida rechazada repetidamente y limpieza sin fugaFD. Conserva pruebas previas
de captura/join/detach/identidad sticky/mascara. Watchdogs harness45s/35s, no plazo
duro de BPF. Positivo WSL PID786/kernel2629 UID0, TCP4096/4096B,UDP256/256B,
8peeks, intervalo2.003597868s. `work/network-selected-window-native-linux.log`.

Linux123/123 Release sin skips,15tests de red repetidos20veces cada uno,
consumidor instalado reconstruido/ejecutado exit0, build verificado sin warnings.
El primer relink completo registro warnings de make por timestamps DrvFS
fracciones de segundo futuros; no warnings de compilador. Se preserva el log y se repite build/
suite sin cambiar el reloj ni relajar pruebas; ver logs verified-build/suite.
Logs `work/network-selected-window-{final-build,suite,repeat,install,consumer}-linux.log`.
Repeticion nativa con binario final tambien exit0: PID402/kernel913 UID0,
intervalo1.104113519s; `work/network-selected-window-final-native-linux.log`.
Positivos reales limitados a este WSL x86_64, no hardware/CI/distros universales.
