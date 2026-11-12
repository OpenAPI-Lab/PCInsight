# Informe seleccionado en Monitor y Sampler

Revision75 integra el [proveedor publico](LINUX_PROCESS_NETWORK.md) mediante
SystemSnapshot.linux_process_network (Metric independiente) y
SampleOptions.include_linux_process_network (false por defecto). No llena los
campos legacy de red del proceso, sesion o app con datos parciales.

**Cambio de layout0.x:** recompilar biblioteca y TODOS sus consumidores:
SystemSnapshot y SampleOptions cambiaron de tamano. Support/Metric conservan
tipo/layout/semantica, ahora definidos en metric.hpp para evitar dependencia
circular entre tipos generales y el reporte de red. No hay formato persistido
nuevo, GUI ni componente Windows de captura nuevo.

## Dos pasos explicitos

```cpp
#include <pcinsight/monitor.hpp>

void observar(pcinsight::Monitor& monitor,
              std::span<const pcinsight::LinuxProcessNetworkSelection> seleccion,
              bool consentimiento)
{
    const auto inicio = monitor.start_linux_process_network(consentimiento, seleccion);
    if (!pcinsight::metric_is_known(inicio)) return; // mostrar support/note

    pcinsight::SampleOptions opciones;
    opciones.include_linux_process_network = true;
    const auto muestra = monitor.sample(opciones);
    // mostrar muestra.linux_process_network y sus Metrics; primera sin tasas
    monitor.stop_linux_process_network();
}
```

La seleccion/autorizacion se entrega a start_linux_process_network, NO a las
opciones ni a sample. Flag true sin inicio explicito no carga BPF, devuelve
ausencia motivada. Un inicio denegado conserva permission_required en el informe,
sin fingir un vacio disponible. No nueva seleccion mientras captura activa:
stop primero, o usar desde revision76 la
[actualizacion explicita](LINUX_PROCESS_NETWORK_UPDATE.md) mediante
update_linux_process_network_selection. start false no inicia captura ni retira
una autorizacion activa.

Una muestra con flag false, incluida monitor.sample() con opciones por defecto,
CIERRA captura antes de recoger el backend general y entrega informe desactivado.
Volver a true NO reautoriza: necesita otro inicio explicito. stop, destruccion
y excepcion del backend general cierran referencias. Error de lectura ordinario
se refleja en Metric sin borrar otros grupos; excepcion del proveedor lo detiene.
No se reutiliza un reporte viejo cuando falla la muestra actual.

## Worker y graficas

```cpp
#include <pcinsight/sampler.hpp>
// opciones.include_linux_process_network = true ANTES de construir
// pcinsight::Sampler fondo(std::chrono::seconds(1), 0, opciones);
// fondo.start_linux_process_network(consentimiento, seleccion); // detenido
// fondo.start();  ... leer latest()/timed_history() ...  fondo.stop();
```

Sampler.start_linux_process_network exige worker detenido y flag habilitado en
sus opciones de construccion; rechaza reconfiguracion en marcha ANTES de tocar
descriptores. El setup usa el mismo Monitor privado. start del worker nunca
autoriza captura, y stop/fin del worker/excepcion/fracaso al crear hilo la cierran.
Para capturar en otro run, repetir setup autorizado con pidfds validos. Restart
sin setup sigue leyendo otros grupos pero deja el informe de red ausente.

Revision77 permite [actualizar la seleccion con el worker activo](LINUX_PROCESS_NETWORK_WORKER.md)
mediante update_linux_process_network_selection, con consentimiento nuevo,
serializacion y reset de baselines. No confundirlo con start_linux_process_network,
que sigue exigiendo worker detenido; no hay descubrimiento automatico.

latest tras stop sigue siendo la ultima muestra historica, NO estado vivo de
captura. start limpia historial/error como antes. No persistencia nueva de apps
ni de red. El intervalo solicitado no garantiza cadencia: el muestreo BPF/joins
es sincrono, stop puede esperar la operacion actual, sin plazo duro. No usar un
Sampler con jthread heredado despues de fork; esa restriccion de threads es
distinta del guard de identidad del Monitor/proveedor individual. Monitor solo
y su captura movida/forked se prueban sin worker activo heredado.

## Alcance y verificacion

Se heredan consentimiento,1..16 actores/cookies declarados, pidfds propios,
identidades locales/iniciales separadas, brackets nominales, soporte/BTF/permisos,
suma UID PARCIAL, ausencia distinta de0 y limites de [cobertura](LINUX_PROCESS_NETWORK.md).
No se inicia captura global ni descubrimiento automatico. No se eleva permisos
o instala paquetes/servicios/CLI. El consumidor puede enlazar desde IDE y dejar
su CMake para despues del GUI, sin cambiar este contrato.

Revision79 agrega [setup con modo de buckets](LINUX_PROCESS_NETWORK_BUCKET_MONITOR.md)
mediante tercer argumento a start_linux_process_network en Monitor/Sampler.
Llamadas de dos argumentos conservan estricto SIEMPRE; modo no se activa por flag
ni se hereda de una sesion anterior. Resto de este checkpoint75 es historico.

Adapter inyectable interno probado sin BPF: flag solo/consentimiento denegado/
inicio invalido-excepcion/lectura denegada-excepcion/recuperacion/opt-out/rearmado/
ausencia sin datos stale, sentinelas de campos legacy sin modificar y ciclo
Monitor/Sampler/move. Callback falso solo prueba integracion, no kernel real.

Manual NO CTest --authorized-private-monitor-window en WSL, dos hijos privados:
4oraculos actor/UID parcial, intervalo nominal2.303687773s, FD entrada cerrado,
move activo, padre excluido/idle0/fork-salida rechazados. Opt-out cierra FDs,
flag true solo no rearma, worker autorizado entrega reporte, rechaza setup
en marcha, stop cierra captura, restart queda sin ella, rearmado explicito tiene
otra epoch/primera muestra sin tasas, limpieza sin fugaFD. Watchdogs privados
60s/50s, no plazo duro BPF. work/network-monitor-native-linux.log.

Control oficial revision75 y logs work/network-monitor-* conservan regresion,
instalacion y consumidor. MSVC solo compatibilidad focalizada del cambio comun,
no funciones/captura Windows nuevas ni suite completa Windows actualizada.
F03 sigue abierto: seleccion dinamica/buckets por namespace y cobertura general;
una lista fija parcial no completa la solicitud general de red por app. H04/J04
no se cierran ni se integran persistencia o sesiones en este checkpoint.

Verificado Linux126/126 Release sin skips y18tests red repetidos20veces cada uno;
rebuild/suite repetidos y consumidor instalado final exit0. Logs verified-build,
verified-suite,final-consumer-linux. Warnings make DrvFS iniciales (milisegundos
futuros) preservados/rebuild limpio; shadow proto BTF detectado por MSVC corregido.
MSVC2/2 pruebas focalizadas actuales pasan, sin nuevos warnings C++ en rebuild
(D9025 inicial esperado para asserts activos). Consumidor general MSVC compila y
enlaza contra instalacion actual pero reproduce0xC0000409; marcadores confirman
checks nuevos pasados antes del fallo en recorrido posterior. NO demuestra causa
ni ausencia de regresion, se registra en N03 y se difiere por prioridad Linux.
Logs consumer-diagnostic/diagnostic2-msvc; no afirmar consumidor Windows exit0.
