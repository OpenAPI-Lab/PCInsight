# Buckets explicitos en Monitor y Sampler

Revision79 integra el [modo de buckets declarados](LINUX_PROCESS_NETWORK_BUCKETS.md)
en Monitor y Sampler mediante sobrecarga de setup con un TERCER argumento. No
agrega campos de opciones ni cambia layouts publicos. Es opt-in por sesion,
independiente de include_linux_process_network y del consentimiento.

```cpp
using Modo = pcinsight::LinuxProcessNetworkSelectionMode;
pcinsight::Monitor monitor;
auto inicio = monitor.start_linux_process_network(consentimiento, seleccion,
                                                 Modo::declared_namespace_buckets);
pcinsight::SampleOptions opciones;
opciones.include_linux_process_network = true;
if (pcinsight::metric_is_known(inicio)) {
    auto muestra = monitor.sample(opciones);
    // muestra.linux_process_network: filas de bucket, no totales por proceso.
}
monitor.stop_linux_process_network();

pcinsight::Sampler fondo(std::chrono::seconds(1), 0, opciones);
auto setup = fondo.start_linux_process_network(consentimiento, seleccion,
                                               Modo::declared_namespace_buckets);
if (pcinsight::metric_is_known(setup)) {
    fondo.start();
    // Puede actualizarse lista completa con consentimiento nuevo:
    // fondo.update_linux_process_network_selection(consentimiento, otra_lista);
    fondo.stop();
}
```

## Modo por sesion, sin autorizacion implicita

La llamada existente de DOS argumentos SIEMPRE usa strict_single_namespace,
incluso despues de detener una sesion de buckets. Para repetir buckets se tiene
que entregar de nuevo el tercer argumento y consentimiento. Esto difiere del
proveedor individual construido con modo fijo: no hereda silenciosamente una
configuracion anterior. Ambos simbolos de setup anteriores se conservan; API
aditiva, enlazar biblioteca actual para la sobrecarga nueva. Recompilar todos
los consumidores si vienen de layouts anteriores a revision75; no nuevo CMake
obligatorio en el proyecto del consumidor.

False consent no llama proveedor ni modifica captura activa. Enum desconocido
se rechaza antes de llamar proveedor. No reconfigura una captura activa mediante
start: detenerla primero. Un proveedor inyectado antiguo sin setup de modo puede
seguir atendiendo estricto, pero buckets se rechaza sin degradar silenciosamente
a estricto. Errores/excepciones al iniciar mantienen disponibilidad/limpieza
anteriores. Constructor/flag/sample/start del worker solos nunca autorizan BPF.

Monitor exige serializar llamadas. Sampler exige worker detenido/flag habilitado
para setup, serializa lifecycle->Monitor, espera worker terminado antes de setup
y rechaza setup en marcha ANTES de tocar descriptores. Update de lista mientras
corre conserva modo de la sesion, usa prioridad y bloqueos de revision77; no
permite cambiar modo. Para cambiarlo: stop/setup explicito/start. No unshare ni
manipular FDs propios, tabla FD compartida con worker; no Sampler heredado tras
fork ni move/destruccion concurrentes.

Desactivar flag en Monitor cierra captura; true solo no rearma. Stop/fin/error
del worker cierran; restart solo no rearma. Update autorizado fallido cierra
captura, no otros grupos del worker. Ultimo snapshot/historial puede seguir
siendo historico: no prueba captura activa ni de modo vigente. Esperar la nueva
selection_epoch y separar graficas entre epochs/modos; no borrar otras metricas.

## Alcance y evidencia

Se mantienen16filas totales proceso-cookie, cookies disjuntos por proceso,
identidades locales/kernel y UID coherentes, filtro de cookie del SOCKET con
fallos de calidad sticky, barrido comun nominal no atomico y tasas estimadas.
coverage=declared_tcp_udp_socket_namespace_buckets y processes son BUCKETS:
varias filas pueden repetir PID/start. UID suma buckets seleccionados parcialmente,
no procesos/sesiones ni trafico global. No llenar totales legacy ni afirmar que
un namespace declarado existe por observar contador0. Sin descubrimiento
automatico, setns/unshare, CLI, herramientas/dependencias nuevas ni elevacion auto.

Fixture del controller: modo correcto, flag sin setup, denegacion/enum invalido
sin callback, captura activa rechaza cambio, update conserva modo, opt-out,
rearmado explicito, retorno al estricto por llamada anterior, callback antiguo
sin buckets, inicio fallido/epoch0/estimated/excepcion y limpieza. Superficies
publicas prueban flag/worker detenido/habilitado/denegacion/movido/invalido; el
consumidor instalado enlaza ambas sobrecargas sin iniciar BPF.

Manuales NO CTest --authorized-private-monitor-buckets y
--authorized-private-worker-buckets: dos hijos privados autorizados, captura de
UNO con cookie real y otro deliberadamente no observado. Cuatro oraculos de
payload/tasas por bucket y suma UID sin duplicado, otras operaciones/padre/hijo
excluidos, retirar/reincorporar con nueva epoch/baseline/gap limpio, FD entrada
cerrado/limpieza sin fugaFD. Monitor: fork rechazado/opt-out cierra/flag no rearma/
default vuelve a estricto. Worker: updater concurrente con historial, cobertura
bucket/primera muestra sin tasas, rechaza cambio de modo en marcha, fallo de
update cierra captura pero worker sigue, stop/default estricto/restart sin rearmar.
SOLO UN namespace real, segundo cookie no observado: no prueba dos namespaces
reales simultaneos ni descubrimiento. Watchdogs parent60s/child50s, sin plazo
duro BPF. work/network-bucket-monitor-{native,worker-native}-linux.log.

F03 sigue abierta por politica/descubrimiento automatico, buckets dinamicos y
cobertura general. H04/J04 siguen pendientes. Sin persistencia/GUI/captura Windows
nueva; fallo del consumidor general MSVC revision75 sigue N03, no compatibilidad
Windows total ni certificacion universal de hardware/kernels/distros.

Resultado final: Linux126/126 Release sin skips,3tests focalizados20repeticiones
cada uno, consumidor instalado reconstruido/enlazado/ejecutado exit0 y build sin
warnings. MSVC biblioteca y3/3 focalizadas pasan/sin warnings C++ nuevos (D9025
esperado mantiene asserts); no consumidor/suite Windows completo nuevo. Logs
work/network-bucket-monitor-*; control oficial revision79:57/75=76,0% cerrado,
18/75=24,0% abierto; sin nuevos IDs/cierres/credito parcial.
