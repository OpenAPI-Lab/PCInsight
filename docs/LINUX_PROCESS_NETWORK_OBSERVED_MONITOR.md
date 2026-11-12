# Monitor/Sampler: cookies observados opt-in

Revision93: estas mismas rutas publicas usan ahora el
[backend compartido](LINUX_NETWORK_SHARED_PUBLIC.md), no fanout por actor.
Nativo positivo Monitor/Sampler-worker/owner con updates, gap/epoch/limpieza.
No cambio de API ni rearmado implicito; admision general sigue pendiente.

Continuacion revision85: [gestion de candidatos/pidfds](LINUX_NETWORK_SELECTION_OWNER.md)
para el consumidor; NO enumeracion/seleccion general ni rearmado automatico.

Revision84 conecta observed_namespace_buckets al ciclo de vida existente de
Monitor y Sampler. Se reutilizan las sobrecargas publicas de tres argumentos:
no nuevo metodo, layout, flag u otro formato de persistencia. El cambio de layout
de revision83 sigue requiriendo recompilar TODOS si se viene de headers anteriores.
F03 abierta: esto es seleccion explicita, NO inventario/captura general de apps.

## Uso

```cpp
#include <pcinsight/monitor.hpp>
#include <pcinsight/sampler.hpp>

pcinsight::Metric<std::uint64_t> preparar_monitor(
    pcinsight::Monitor& monitor,
    std::span<const pcinsight::LinuxProcessNetworkSelection> seleccion,
    bool consentimiento)
{
    //1..16 PID/start/pidfd verificados; cookie0 en TODAS las filas.
    return monitor.start_linux_process_network(consentimiento, seleccion,
        pcinsight::LinuxProcessNetworkSelectionMode::observed_namespace_buckets);
}

// Tras setup exitoso, en el worker del consumidor:
// pcinsight::SampleOptions opciones;
// opciones.include_linux_process_network = true;
// auto s = monitor.sample(opciones);
// Comprobar metric_is_known(s.linux_process_network) y cada Metric.
```

Para Sampler, construir con include_linux_process_network=true, hacer setup
autorizado de tres argumentos MIENTRAS ESTE DETENIDO y despues start() del worker.
include flag solo no autoriza BPF. No instalar CLI/dependencias, no elevacion
automatica ni obligacion de crear CMake del consumidor antes del GUI. No bloquear
el hilo GUI con setup/sample/parada: las APIs son sincronas, sin plazo duro BPF.

## Reglas de ciclo de vida

- Sobrecarga de DOS argumentos SIEMPRE estricta, tambien tras detener observado.
  No hereda el modo anterior. Pasar tres argumentos explicitamente para observado.
- Setup en captura activa rechaza cambio/reemplazo; en Sampler exige worker
  detenido y flag habilitado. Sin consentimiento no llama proveedor ni cambia
  una captura activa. Enum desconocido rechazado sin llamar proveedor.
- Adaptador inyectado antiguo sin start_mode solo permite estricto: nunca activar
  observado mediante callback viejo. Fallo/epoch0/estimated/excepcion de setup
  limpia proveedor, sin captura parcial ni rearmado silencioso.
- Monitor sample con flag=false cierra captura/pidfds y retira resultado; volver
  a true no rearma. Stop del worker tambien cierra captura. Reiniciar worker NO
  reautoriza, ni iniciar/actualizar captura desde stopped la reactiva.
- update_linux_process_network_selection requiere consentimiento NUEVO. Sampler
  permite hacerlo en vivo, serializado con Monitor/coleccion/parada, prioridad
  de update pendiente y wakeups protegidos existentes. Conserva modo observado,
  retained counters, epoch/reset conjunto; readd nuevo no replay de trafico del gap.
- Actualizacion fallida en el adaptador cierra captura, incluso si backend rechazo
  forma sin mutar. Worker sigue muestreando otras metricas y reporta red ausente;
  otro update no autoriza una sesion nueva. Rearmar requiere setup explicito.

## Resultado e historial en memoria

SystemSnapshot.linux_process_network conserva coverage observada y todas las
asociaciones observed_processes, incluso actores que aun tienen0buckets. Lista
processes contiene BUCKETS, no proceso con cero inventado. Primer bucket nuevo
sin tasa, segundo limpio puede dar tasa cero/positiva. Sumatoria UID inicial es
parcial, no total ni cuenta local. completed_updates es actor-wide/repetido por
bucket, no sumar. Ver [contrato publico](LINUX_PROCESS_NETWORK_OBSERVED.md).

Sampler guarda estas muestras en historial EN MEMORIA con epoch/brackets/metadata.
Muestras viejas no se reescriben al actualizar seleccion. No calcular deltas entre
epochs o desde una fila ausente; consultar Metric/nominal_interval_ns. Sin formato
de historial durable ni union app/usuario general en esta revision: H04/J04 abiertas.
Campos legacy ProcessInfo/UserSession/AppHistoryEntry no reciben totales inventados.

## Verificacion

linux-process-network-monitor prueba modos declarado/observado, consentimiento,
enum invalido, setup activo/default estricto tras stop, falta de callback start_mode,
cuatro fallos setup por modo, cinco fallos update tambien observado, lectura
denegada/recuperada/excepcion, opt-out/no rearmado y preservacion metadata vacia.
Checks de Sampler flag apagado/running setup rechazan antes de tocar descriptores.
Consumer instalado enlaza ambas sobrecargas observadas y denegacion sin capturar.

Manual --authorized-private-observed-monitor usa API Monitor/SystemSnapshot con
dos hijos privados propios, pidfds originales cerrados, move activo, metadata
vacia y llegada tardia sin tasa. A TCP12288/12288 UDP768/768 peek24 y B doble,
dos hilos concurrentes por actor: cuatro tasas y suma UID contra oracle entero,
padre excluido/fork rechazado/consentimiento update/retiro-readd-gap/epochs/reset/
exited rechaza grupo dos veces/remover recupera/stop-restart vacio. Default setup
estricto tras stop; opt-out cierra FDs y flag solo no rearma. Sin fugaFD.

Manual --authorized-private-observed-worker prueba setup observado/worker real,
dos actores seleccionados/cookie0, metadata inicial vacia y buckets posteriores.
Verifica cuatro deltas/tasas y UID en historial con intervalos nominales comunes;
primer bucket sin tasa/known-missing correctos. AB->B->AB con worker activo,
updater concurrente y lecturas historial, retained origin/new actor sin gap,
epoch/reset conjunto. Update denegado no cambia captura, update fallido cierra
captura pero worker sigue; update nuevo no rearma, default setup estricto tras
stop y restart desarmado, FDs limpios/ambos hijos terminan.

SOLO UN namespace real compartido, hijos inicializan SO_NETNS_COOKIE para oracle
independiente; no cookies entrada al productor, multiples netns reales ni prueba
global/16actores reales. Watchdogs parent60s/child50s, esperas de frames acotadas12s,
no plazo duro BPF. Sondas solo actores privados autorizados; sin configuracion
permanente, nuevo paquete, CLI o captura global.

Regresion nativa Monitor de buckets declarados positiva. Linux126/126 Release
sin skips,3tests20repeticiones cada uno, consumidor instalado recompilado-enlazado-
ejecutado exit0/build sin warnings. MSVC3/3 focalizadas de codigo compartido,
sin C++ warnings nuevos (D9025 esperado asserts activos). No suite/consumidor
Windows completos nuevos, fallo general revision75 sigue N03/no compatibilidad
total. No layout nuevo sobre revision83 ni GUI/persistencia/dependencias nuevos.

Logs work/network-observed-monitor-*. Proxima unidad: politica de seleccion y
cobertura general de procesos con identidad/retiros/limites explicitos, manteniendo
pruebas privadas acotadas y solicitando autoridad si se necesita ampliar captura.
No cerrar F03 por el exito de una seleccion manual; H04/J04 siguen pendientes.
